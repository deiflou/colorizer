#ifndef BKCOLORIZER_HPP
#define BKCOLORIZER_HPP

#include <grid.hpp>
#include "maxflow/graph.h"

#include <QPoint>
#include <QRect>
#include <QVector>
#include <QPair>
#include <QDebug>

namespace Colorizer
{

template <typename ScribbleTypeTP, typename LabelIdTypeTP = short>
class BKColorizer
{
public:
    using IndexType = int;
    using ScribbleIndexType = short;
    using LabelIdType = LabelIdTypeTP;
    using IntensityType = unsigned char;
    using MaxflowGraphType = Graph<int, int, int>;

    enum {
        Index_Undefined = -1,
        Index_ImplicitBackgroundCell = -2,

        ScribbleIndex_Undefined = -1,

        LabelId_Undefined = -1,
        LabelId_Background = -2,

        Intensity_Min = 0,
        Intensity_Max = 255
    };

    struct ReferenceGridCellData
    {
        IntensityType intensity{Intensity_Max};
    };

    struct WorkingGridCellData
    {
        IndexType index{Index_Undefined};
        ScribbleIndexType scribbleIndex{ScribbleIndex_Undefined};
        LabelIdType preferredLabelId{LabelId_Undefined};
        LabelIdType computedLabelId{LabelId_Undefined};
        IntensityType intensity{Intensity_Max};
    };

    struct InputPoint
    {
        QPoint position;
        IntensityType intensity;
    };

    using ScribbleType = ScribbleTypeTP;
    
    using ReferenceGridType = SpacePartitioning::Grid<ReferenceGridCellData>;
    using ReferenceGridCellType = typename ReferenceGridType::CellType;
    
    using WorkingGridType = SpacePartitioning::Grid<WorkingGridCellData>;
    using WorkingGridCellType = typename WorkingGridType::CellType;

    BKColorizer() = default;
    BKColorizer(const BKColorizer &) = default;
    BKColorizer(BKColorizer &&) = default;
    BKColorizer& operator=(const BKColorizer &) = default;
    BKColorizer& operator=(BKColorizer &&) = default;

    BKColorizer(const QRect &rect, int cellSize, const QVector<InputPoint> & points)
        : m_referenceGrid(rect, cellSize)
        , m_workingGrid(rect, cellSize)
    {
        for (const InputPoint & point : points) {
            ReferenceGridCellType *referenceCell = m_referenceGrid.addPoint(point.position);
            if (referenceCell) {
                referenceCell->data().intensity = point.intensity;
            }
            WorkingGridCellType *workingCell = m_workingGrid.addPoint(point.position);
            if (workingCell) {
                workingCell->data().intensity = point.intensity;
            }
        }

        m_implicitBackgroundCell = new WorkingGridCellType;
        m_implicitBackgroundCell->data().index = Index_ImplicitBackgroundCell;
        m_implicitBackgroundCell->data().preferredLabelId = LabelId_Background;
        m_implicitBackgroundCell->data().intensity = Intensity_Max;
    }

    BKColorizer(int x, int y, int width, int height, int cellSize, const QVector<InputPoint> & points)
        : BKColorizer(QRect(x, y, width, height), cellSize, points)
    {}

    bool isNull() const
    {
        return m_workingGrid.isNull();
    }

    const ReferenceGridType& referenceGrid() const
    {
        return m_referenceGrid;
    }

    const WorkingGridType& workingGrid() const
    {
        return m_workingGrid;
    }

    const QVector<ScribbleType>& scribbles() const
    {
        return m_scribbles;
    }

    const ScribbleType& scribble(int index) const
    {
        return m_scribbles[index];
    }

    void appendScribble(const ScribbleType & scribble)
    {
        if (isNull()) {
            return;
        }

        m_scribbles.append(scribble);
        clearAndAddScribblesToWorkingGrid(scribble.rect());
    }

    void insertScribble(int index, const ScribbleType &scribble)
    {
        if (isNull()) {
            return;
        }
        
        m_scribbles.insert(index, scribble);
        clearAndAddScribblesToWorkingGrid(scribble.rect());
    }

    void removeScribble(int index)
    {
        if (isNull()) {
            return;
        }
        
        const QRect scribbleRect = m_scribbles[index].rect();
        m_scribbles.removeAt(index);
        clearAndAddScribblesToWorkingGrid(scribbleRect);
    }

    void replaceScribble(int index, const ScribbleType &scribble)
    {
        if (isNull()) {
            return;
        }
        
        removeScribble(index);
        insertScribble(index, scribble);
    }

    LabelIdType backgroundLabelId() const
    {
        if (isNull()) {
            return LabelId_Undefined;
        }
        
        return m_implicitBackgroundCell->data().preferredLabelId;
    }

    void setBackgroundLabelId(LabelIdType newBackgroundLabelId)
    {
        if (isNull()) {
            return;
        }
        
        m_implicitBackgroundCell->data().preferredLabelId = newBackgroundLabelId;
    }

    void updateNeighbors()
    {
        if (isNull()) {
            return;
        }
        
        // We only need to set the top and left neighbors for each cell since the
        // conections to the bottom/right cells are made by the bottom/right cells
        m_workingGrid.updateNeighbors(true);
    }

    void colorize(bool useImplicitBackgroundScribbleForSuroundingArea = false)
    {
        if (isNull()) {
            return;
        }
        
        // If there are no scribbles and the implicit background scribble
        // is used then set the label os all cells to the background label
        if (m_scribbles.isEmpty()) {
            if (useImplicitBackgroundScribbleForSuroundingArea) {
                labelAllCellsAsBackground();
            }
            return;
        }

        // Clear the previously computed labeling
        clearComputedLabeling();

        // The neighbors for each cell must be updated because the topology of
        // the grid might be changed for example by adding a new scribble.
        updateNeighbors();

        QVector<LabelIdType> processedLabelIds;
        // Soft scribbles have weight = 5% of K
        // Hard scribbles have weight = 100% of K
        const int K = m_workingGrid.cellSize() * 2 * (m_workingGrid.rect().width() + m_workingGrid.rect().height());
        const int softScribbleWeight = 5 * K / 100;
        const int hardScribbleWeight = K;

        for (const ScribbleType &scribble : m_scribbles) {
            const LabelIdType currentLabelId = scribble.labelId();

            // Continue if the label id is undefined or it was already processed
            if (currentLabelId == LabelId_Undefined ||
                processedLabelIds.contains(currentLabelId)) {
                continue;
            }
            processedLabelIds.append(currentLabelId);

            // Cell indices must be recomputed from iteration to iteration 
            // because the new maxflow graph only contains the previously
            // non labeled cells
            QPair<int, int> count = indexNonLabeledCells();

            // Create the maxflow graph
            MaxflowGraphType maxflowGraph(count.first, 2 * count.second);

            // Add nodes
            maxflowGraph.add_node(count.first);
            // Add extra node to account for the surrounding area
            if (useImplicitBackgroundScribbleForSuroundingArea) {
                maxflowGraph.add_node();
            }

            // Add edges and set capacities
            visitNonLabeledLeafs(
                [&maxflowGraph, currentLabelId, K, softScribbleWeight]
                (WorkingGridCellType *cell)
                {
                    const IndexType cellIndex = cell->data().index;
                    const LabelIdType cellPreferredLabelId = cell->data().preferredLabelId;
                    const IntensityType cellIntensity = cell->data().intensity;

                    // Connect current node to the source or sink node (data term)
                    if (cellPreferredLabelId != LabelId_Undefined) {
                        if (cellPreferredLabelId == currentLabelId) {
                            // This node must be connected to the source node
                            maxflowGraph.add_tweights(cellIndex, softScribbleWeight, 0);
                        } else {
                            // This node must be connected to the sink node
                            maxflowGraph.add_tweights(cellIndex, 0, softScribbleWeight);
                        }
                    }

                    // connect the node to its top left neighbors and
                    // set capacities (smoothnes term)
                    const int cellEdgeWeight = 1 + K * cellIntensity / Intensity_Max;

                    for (const WorkingGridCellType* const neighborCell : cell->topLeafNeighbors()) {
                        // Continue if the neighbor was already labeled
                        const LabelIdType neighborCellComputedLabelId = neighborCell->data().computedLabelId;
                        if (neighborCellComputedLabelId != LabelId_Undefined) {
                            continue;
                        }
                        const IndexType neighborCellIndex = neighborCell->data().index;
                        const IntensityType neighborCellIntensity = neighborCell->data().intensity;
                        const int neighborCellEdgeWeight = 1 + K * neighborCellIntensity / Intensity_Max;
                        const int nPixelsInEdge = qMin(cell->size(), neighborCell->size());
                        maxflowGraph.add_edge(
                            cellIndex,
                            neighborCellIndex,
                            cellEdgeWeight * nPixelsInEdge,
                            neighborCellEdgeWeight * nPixelsInEdge
                        );
                    }

                    for (const WorkingGridCellType* const neighborCell : cell->leftLeafNeighbors()) {
                        // Continue if the neighbor was already labeled
                        const LabelIdType neighborCellComputedLabelId = neighborCell->data().computedLabelId;
                        if (neighborCellComputedLabelId != LabelId_Undefined) {
                            continue;
                        }
                        const IndexType neighborCellIndex = neighborCell->data().index;
                        const IntensityType neighborCellIntensity = neighborCell->data().intensity;
                        const int neighborCellEdgeWeight = 1 + K * neighborCellIntensity / Intensity_Max;
                        const int nPixelsInEdge = qMin(cell->size(), neighborCell->size());
                        maxflowGraph.add_edge(
                            cellIndex,
                            neighborCellIndex,
                            cellEdgeWeight * nPixelsInEdge,
                            neighborCellEdgeWeight * nPixelsInEdge
                        );
                    }

                    return true;
                }
            );

            // Connect the surrounding background node to the sink and to the border cells
            if (useImplicitBackgroundScribbleForSuroundingArea) {
                maxflowGraph.add_tweights(count.first, 0, hardScribbleWeight);
            }

            // Compute maxflow
            maxflowGraph.maxflow();

            // Set the labels
            visitNonLabeledLeafs(
                [&maxflowGraph, currentLabelId](WorkingGridCellType *cell)
                {
                    if (maxflowGraph.what_segment(cell->data().index) == MaxflowGraphType::SOURCE) {
                        cell->data().computedLabelId = currentLabelId;
                    }
                    return true;
                }
            );
        }
    }

private:
    ReferenceGridType m_referenceGrid;
    WorkingGridType m_workingGrid;
    WorkingGridCellType *m_implicitBackgroundCell;
    QVector<ScribbleType> m_scribbles;

    void clearWorkingGrid(const QRect &rect)
    {
        // Clear
        m_workingGrid.clear(rect);

        // Add the original points from the reference grid
        m_referenceGrid.visitLeafs(
            rect,
            [this](ReferenceGridCellType *cell) -> bool
            {
                if (cell->isBottomMostLeaf()) {
                    WorkingGridCellType *newCell = m_workingGrid.addPoint(cell->center());
                    newCell->data().intensity = cell->data().intensity;
                }
                return true;
            }
        );
    }

    void addScribblesToWorkingGrid(const QRect &rect)
    {
        // Adjust the rect to grid cell boundaries
        QRect adjustedRect = m_workingGrid.adjustedRect(rect);

        // Add the scribbles' data to the working grid from the last one
        // to the first one
        for (int i = m_scribbles.size() - 1; i >= 0; --i) {
            const ScribbleType &scribble = m_scribbles[i];

            // Continue if he scribble is outside of the interest rect
            QRect intersectedRect = adjustedRect.intersected(scribble.rect());
            if (!intersectedRect.isValid()) {
                continue;
            }

            // Add the scribble data to the working grid
            
            // Add the contour points
            for (const QPoint &point : scribble.contourPoints()) {
                // Continue if the point is outside the interest rect
                if (!adjustedRect.contains(point)) {
                    continue;
                }
                // Get the leaf cell at this point
                WorkingGridCellType *leafCell = m_workingGrid.leafCellAt(point);
                // Continue if the leaf cell's preferred scribble index is greater
                // than the current scribble index. A scribble with higher priority
                // was added in this position
                if (leafCell->data().scribbleIndex > i) {
                    continue;
                }
                // Add the point
                m_workingGrid.addPoint(point);
            }

            // Mark the cells' preferred scribble with the scribble index
            m_workingGrid.visitLeafs(
                adjustedRect,
                [&scribble, i](WorkingGridCellType *cell) -> bool
                {
                    // Return if the cell's preferred scribble index is greater
                    // than the current scribble index. A scribble with higher priority
                    // was added in this position
                    if (cell->data().scribbleIndex > i) {
                        return true;
                    }
                    // Set the preferred scribble index if this cell is inside the scribble
                    if (scribble.containsPoint(cell->center())) {
                        cell->data().scribbleIndex = i;
                        cell->data().preferredLabelId = scribble.labelId();
                    }

                    return true;
                }
            );
        }
    }

    void clearAndAddScribblesToWorkingGrid(const QRect &rect)
    {
        clearWorkingGrid(rect);
        addScribblesToWorkingGrid(rect);
    }

    void clearComputedLabeling()
    {
        m_workingGrid.visitLeafs(
            [](WorkingGridCellType *cell) -> bool
            {
                cell->data().computedLabelId = LabelId_Undefined;
                return true;
            }
        );
    }

    void labelAllCellsAsBackground()
    {
        m_workingGrid.visitLeafs(
            [this](WorkingGridCellType *cell) -> bool
            {
                cell->data().computedLabelId = backgroundLabelId();
                return true;
            }
        );
    }

    template <typename VisitorTypeTP>
    void visitNonLabeledLeafs(VisitorTypeTP visitor) const
    {
        m_workingGrid.visitLeafs(
            [visitor](WorkingGridCellType *cell) -> bool
            {
                if (cell->data().computedLabelId == LabelId_Undefined) {
                    return visitor(cell);
                }
                return true;
            }
        );
    }

    QPair<int, int> indexNonLabeledCells()
    {
        int cellCount = 0;
        int neighborCount = 0;
        visitNonLabeledLeafs(
            [&cellCount, &neighborCount](WorkingGridCellType *cell) -> bool
            {
                cell->data().index = cellCount++;
                neighborCount += cell->topLeafNeighbors().size();
                neighborCount += cell->leftLeafNeighbors().size();
                // neighborCount += cell->bottomLeafNeighbors().size();
                // neighborCount += cell->rightLeafNeighbors().size();
                return true;
            }
        );
        return QPair<int, int>(cellCount, neighborCount);
    }
};

}

#endif
