#ifndef GRID_HPP
#define GRID_HPP

#include <QVector>
#include <QStack>
#include <QPoint>
#include <QRect>
#include <QDebug>

#include "quadtree.hpp"

namespace SpacePartitioning
{

template <typename DataTypeTP>
class Grid
{
public:
    using DataType = DataTypeTP;
    using CellType = QuadTreeNode<DataType>;

    Grid() = default;
    Grid(const Grid &) = default;
    Grid(Grid &&) = default;
    Grid& operator=(const Grid &) = default;
    Grid& operator=(Grid &&) = default;

    Grid(const QRect &rect, int cellSize)
    {
        int widthInCells = rect.width() / cellSize;
        if (widthInCells * cellSize != rect.width()) {
            ++widthInCells;
        }
        int heightInCells = rect.height() / cellSize;
        if (heightInCells * cellSize != rect.height()) {
            ++heightInCells;
        }

        m_rect = QRect(rect.x(), rect.y(), widthInCells * cellSize, heightInCells * cellSize);
        
        // Create top level cells
        m_cells = QVector<CellType*>(widthInCells * heightInCells);
        for (int y = 0; y < heightInCells; ++y) {
            for (int x = 0; x < widthInCells; ++x) {
                const int index = y * widthInCells + x;
                m_cells[index] = new CellType;
                m_cells[index]->setRect(x * cellSize + m_rect.x(),
                                        y * cellSize + m_rect.y(),
                                        cellSize,
                                        cellSize);
            }
        }
        
        m_widthInCells = widthInCells;
        m_heightInCells = heightInCells;
        m_cellSize = cellSize;
    }

    Grid(int x, int y, int width, int height, int cellSize)
        : Grid(QRect(x, y, width, height), cellSize)
    {}

    ~Grid()
    {
        for (CellType *cell : m_cells) {
            delete cell;
        }
    }

    Grid clone() const
    {
        Grid newGrid(m_rect, m_cellSize);

        // Subdivide (this method ensures that the cells siblings are computed)
        visitLeaves(
            [&newGrid](CellType * cell)
            {
                if (cell->size() == 1)
                {
                    newGrid.addPoint(cell->rect().topLeft());
                }
            }
        );

        // Now that the grids have the same structure, copy the data
        for (int i = 0; i < m_cells.size(); ++i) {
            QStack<CellType*> srcStack;
            QStack<CellType*> dstStack;
            srcStack.push(m_cells[i]);
            dstStack.push(newGrid.m_cells[i]);

            while (!srcStack.isEmpty()) {
                CellType *srcCell = srcStack.pop();
                CellType *dstCell = dstStack.pop();

                dstCell->setData(srcCell->data());

                if (srcCell->isSubdivided()) {
                    srcStack.push(srcCell->bottomRightChild());
                    srcStack.push(srcCell->bottomLeftChild());
                    srcStack.push(srcCell->topRightChild());
                    srcStack.push(srcCell->topLeftChild());
                    dstStack.push(dstCell->bottomRightChild());
                    dstStack.push(dstCell->bottomLeftChild());
                    dstStack.push(dstCell->topRightChild());
                    dstStack.push(dstCell->topLeftChild());
                }
            }
        }

        return newGrid;
    }

    bool isNull() const
    {
        return m_cells.isEmpty();
    }

    // Deletes the cells of the trees that intersect with the given rect
    // while keeping the top level ones
    void clear(const QRect &rect)
    {
        QRect cellsRect = rectToCells(rect);
        if (!cellsRect.isValid()) {
            return;
        }

        for (int y = cellsRect.top(); y <= cellsRect.bottom(); ++y) {
            for (int x = cellsRect.left(); x <= cellsRect.right(); ++x) {
                const int index = y * m_widthInCells + x;
                CellType *cell = m_cells[index];
                clearCell(cell);
            }
        }
    }

    // Deletes all the cells of the trees while keeping the top level ones
    void clear()
    {
        if (isNull()) {
            return;
        }
        for (CellType *cell : m_cells) {
            clearCell(cell);
        }
    }

    CellType* topLevelCellAt(const QPoint &point) const
    {
        if (isNull() || !m_rect.contains(point)) {
            return nullptr;
        }

        QPoint p = (point - m_rect.topLeft());
        int x = p.x() / m_cellSize;
        int y = p.y() / m_cellSize;
        int index = y * m_widthInCells + x;
        return m_cells[index];
    }

    CellType* leafCellAt(const QPoint &point) const
    {
        CellType *cell = topLevelCellAt(point);
        if (cell) {
            return cell->leafAt(point);
        }
        return nullptr;
    }

    CellType* addPoint(QPoint point)
    {
        CellType *cell = topLevelCellAt(point);
        if (cell) {
            return cell->addPoint(point);
        }
        return nullptr;
    }

    CellType* addPoint(int x, int y)
    {
        return addPoint(QPoint(x, y));
    }

    // traverse the cells in preorder
    template <typename VisitorTypeTP>
    void visit(VisitorTypeTP visitor) const
    {
        if (isNull()) {
            return;
        }

        for (int i = 0; i < m_cells.size(); ++i) {
            QStack<CellType*> stack;
            stack.push(m_cells[i]);

            while (!stack.isEmpty()) {
                CellType *cell = stack.pop();
                if (!visitor(cell)) {
                    return;
                };
                if (cell->isSubdivided()) {
                    stack.push(cell->bottomRightChild());
                    stack.push(cell->bottomLeftChild());
                    stack.push(cell->topRightChild());
                    stack.push(cell->topLeftChild());
                }
            }
        }
    }

    template <typename VisitorTypeTP>
    void visitLeaves(VisitorTypeTP visitor) const
    {
        visit(
            [visitor](CellType *cell) {
                if (cell->isLeaf()) {
                    return(visitor(cell));
                }
                return true;
            }
        );
    }

    template <typename VisitorTypeTP>
    void visit(const QRect &rect, VisitorTypeTP visitor) const
    {
        if (isNull()) {
            return;
        }
        
        QRect cellsRect = rectToCells(rect);
        if (!cellsRect.isValid()) {
            return;
        }

        for (int y = cellsRect.top(); y <= cellsRect.bottom(); ++y) {
            for (int x = cellsRect.left(); x <= cellsRect.right(); ++x) {
                const int index = y * m_widthInCells + x;
                QStack<CellType*> stack;
                stack.push(m_cells[index]);

                while (!stack.isEmpty()) {
                    CellType *cell = stack.pop();
                    if (!visitor(cell)) {
                        return;
                    };
                    if (cell->isSubdivided()) {
                        stack.push(cell->bottomRightChild());
                        stack.push(cell->bottomLeftChild());
                        stack.push(cell->topRightChild());
                        stack.push(cell->topLeftChild());
                    }
                }
            }
        }
    }

    template <typename VisitorTypeTP>
    void visitLeaves(const QRect &rect, VisitorTypeTP visitor) const
    {
        visit(
            rect,
            [visitor](CellType *cell) {
                if (cell->isLeaf()) {
                    return visitor(cell);
                }
                return true;
            }
        );
    }

    // Visit all border leaf cells in clockwise direction
    // starting from the top left one
    template <typename VisitorTypeTP>
    void visitBorderLeaves(VisitorTypeTP visitor) const
    {
        int index;

        // Top cells
        for (int x = 0; x < m_widthInCells; ++x) {
            CellType *topLevelCell = m_cells[x];
            for (CellType *cell : topLevelCell->topMostLeaves()) {
                if (!visitor(cell)) {
                    return;
                }
            }
        }

        // Right cells (we must avoid processing again the top right corner cell)
        index = m_widthInCells - 1;
        {
            CellType *topLevelCell = m_cells[index];
            QVector<CellType*> borderLeafCells = topLevelCell->rightMostLeaves();
            for (int i = 1; i < borderLeafCells.size(); ++i) {
                CellType *cell = borderLeafCells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
            index += m_widthInCells;
        }
        for (int y = 1; y < m_heightInCells; ++y, index += m_widthInCells) {
            CellType *topLevelCell = m_cells[index];
            for (CellType *cell : topLevelCell->rightMostLeaves()) {
                if (!visitor(cell)) {
                    return;
                }
            }
        }

        // Bottom cells (we must avoid processing again the bottom right corner cell)
        index = (m_heightInCells - 1) * m_widthInCells + m_widthInCells - 1;
        {
            CellType *topLevelCell = m_cells[index];
            QVector<CellType*> borderLeafCells = topLevelCell->bottomMostLeaves();
            for (int i = borderLeafCells.size() - 2; i >= 0; --i) {
                CellType *cell = borderLeafCells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
            --index;
        }
        for (int x = m_widthInCells - 2; x >= 0; --x, --index) {
            CellType *topLevelCell = m_cells[index];
            QVector<CellType*> borderLeafCells = topLevelCell->bottomMostLeaves();
            for (int i = borderLeafCells.size() - 1; i >= 0; --i) {
                CellType *cell = borderLeafCells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
        }

        // Left cells (we must avoid processing again the bottom left corner cell)
        index = (m_heightInCells - 1) * m_widthInCells;
        {
            CellType *topLevelCell = m_cells[index];
            QVector<CellType*> borderLeafCells = topLevelCell->leftMostLeaves();
            for (int i = borderLeafCells.size() - 2; i >= 0; --i) {
                CellType *cell = borderLeafCells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
            index -= m_widthInCells;;
        }
        for (int y = m_heightInCells - 2; y >= 0; --y, index -= m_widthInCells) {
            CellType *topLevelCell = m_cells[index];
            QVector<CellType*> borderLeafCells = topLevelCell->leftMostLeaves();
            for (int i = borderLeafCells.size() - 1; i >= 0; --i) {
                CellType *cell = borderLeafCells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
        }
    }

    int cellSize() const
    {
        return m_cellSize;
    }
    
    const QRect& rect() const
    {
        return m_rect;
    }
    
    QRect adjustedRect(const QRect &rect) const
    {
        if (isNull()) {
            return QRect();
        }

        QRect adjustedRect = m_rect.intersected(rect);
        if (!adjustedRect.isValid()) {
            return QRect();
        }
        adjustedRect.translate(-m_rect.topLeft());
        adjustedRect.setLeft(adjustedRect.left() / m_cellSize * m_cellSize);
        adjustedRect.setTop(adjustedRect.top() / m_cellSize * m_cellSize);
        adjustedRect.setRight(adjustedRect.right() / m_cellSize * m_cellSize + m_cellSize - 1);
        adjustedRect.setBottom(adjustedRect.bottom() / m_cellSize * m_cellSize + m_cellSize - 1);
        return adjustedRect.translated(m_rect.topLeft());
    }

    void updateNeighbors(bool findTopLeftNeighborsOnly = false)
    {
        if (isNull()) {
            return;
        }

        for (int y = 0; y < m_heightInCells; ++y) {
            for (int x = 0; x < m_widthInCells; ++x) {
                QStack<CellType*> stack;
                const int index = y * m_widthInCells + x;
                stack.push(m_cells[index]);

                while (!stack.isEmpty()) {
                    CellType *cell = stack.pop();
                    if (cell->isLeaf()) {
                        // Set the neighbor cells in the current cell.
                        // First: find the cells at each side of the current cell.
                        // Second:
                        //     * If the side cell is in the same level, get
                        //       the closest leaf cells to the current cell
                        //       and set them as the neighbors.
                        //     * If the side cell is in a level above the current
                        //       cell, then use it as the neighbor (since
                        //       it is already a leaf).
                        //     * If the side cell is null, then don't set any
                        //       neighbors at that side

                        CellType *sideCell;
                        bool isSameLevel;
                        
                        isSameLevel = findTopCell(cell, x, y, &sideCell);
                        if (sideCell) {
                            if (isSameLevel) {
                                cell->setTopLeafNeighbors(sideCell->bottomMostLeaves());
                            } else {
                                cell->setTopLeafNeighbors(QVector<CellType*>() << sideCell);
                            }
                        }

                        isSameLevel = findLeftCell(cell, x, y, &sideCell);
                        if (sideCell) {
                            if (isSameLevel) {
                                cell->setLeftLeafNeighbors(sideCell->rightMostLeaves());
                            } else {
                                cell->setLeftLeafNeighbors(QVector<CellType*>() << sideCell);
                            }
                        }
                        if (!findTopLeftNeighborsOnly) {
                            isSameLevel = findBottomCell(cell, x, y, &sideCell);
                            if (sideCell) {
                                if (isSameLevel) {
                                    cell->setBottomLeafNeighbors(sideCell->topMostLeaves());
                                } else {
                                    cell->setBottomLeafNeighbors(QVector<CellType*>() << sideCell);
                                }
                            }

                            isSameLevel = findRightCell(cell, x, y, &sideCell);
                            if (sideCell) {
                                if (isSameLevel) {
                                    cell->setRightLeafNeighbors(sideCell->leftMostLeaves());
                                } else {
                                    cell->setRightLeafNeighbors(QVector<CellType*>() << sideCell);
                                }
                            }
                        }

                    } else {
                        stack.push(cell->bottomRightChild());
                        stack.push(cell->bottomLeftChild());
                        stack.push(cell->topRightChild());
                        stack.push(cell->topLeftChild());
                    }
                }
            }
        }
    }

private:
    QVector<CellType*> m_cells;
    int m_widthInCells, m_heightInCells, m_cellSize;
    QRect m_rect;

    QRect rectToCells(const QRect &rect) const
    {
        if (isNull()) {
            return QRect();
        }

        QRect adjustedRect = m_rect.intersected(rect);
        if (!adjustedRect.isValid()) {
            return QRect();
        }
        adjustedRect.translate(-m_rect.topLeft());
        adjustedRect.setLeft(adjustedRect.left() / m_cellSize);
        adjustedRect.setTop(adjustedRect.top() / m_cellSize);
        adjustedRect.setRight(adjustedRect.right() / m_cellSize);
        adjustedRect.setBottom(adjustedRect.bottom() / m_cellSize);
        return adjustedRect;
    }

    void clearCell(CellType *cell)
    {
        delete cell->topLeftChild();
        delete cell->topRightChild();
        delete cell->bottomLeftChild();
        delete cell->bottomRightChild();
        cell->setTopLeftChild(nullptr);
        cell->setTopRightChild(nullptr);
        cell->setBottomLeftChild(nullptr);
        cell->setBottomRightChild(nullptr);
        cell->setData(DataType());
    }

    // "topCell" is set to the cell at the top of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no top cell ("cell" is a top border cell).
    // true is returned if the top cell is at the same level.
    // false is returned if the top cell is in a level above or null.
    bool findTopCell(CellType *cell, int cellX, int cellY, CellType **topCell) const
    {
        CellType *parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell or it hs no sibling
        if (!parent) {
            if (cellY == 0) {
                // "cell" is a top level border cell
                *topCell = nullptr;
                return false;
            }
            *topCell = m_cells[(cellY - 1) * m_widthInCells + cellX];
            return true;
        }

        // Trivial cases. "cell" is a child in the bottom of the parent cell
        // so the top cell to "cell" is a child in the top of the same parent
        {
            if (cell == parent->bottomLeftChild()) {
                *topCell = parent->topLeftChild();
                return true;
            }

            if (cell == parent->bottomRightChild()) {
                *topCell = parent->topRightChild();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the top of the parent cell
        // so the top cell to "cell" doesn't have the same parent.
        // Recursion is used to find the top cell which can be in the same level,
        // in a level above (if the top cell is not subdivided enough),
        // or null (if this cell is in the top border of the grid)
        {
            if (cell == parent->topLeftChild()) {
                CellType *parentTopCell;
                bool parentTopCellIsInSameLevelAsParent = findTopCell(parent, cellX, cellY, &parentTopCell);
                if (!parentTopCell) {
                    // Border cell case
                    *topCell = nullptr;
                    return false;
                } else {
                    if (parentTopCellIsInSameLevelAsParent) {
                        if (!parentTopCell->isSubdivided()) {
                            // Level above case
                            *topCell = parentTopCell;
                            return false;
                        } else {
                            // Same level case
                            *topCell = parentTopCell->bottomLeftChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The top cell is already above the parent cell
                        *topCell = parentTopCell;
                        return false;
                    }
                }
            }

            /* if (cell == parent->topRightChild()) */
            {
                CellType *parentTopCell;
                bool parentTopCellIsInSameLevelAsParent = findTopCell(parent, cellX, cellY, &parentTopCell);
                if (!parentTopCell) {
                    // Border cell case
                    *topCell = nullptr;
                    return false;
                } else {
                    if (parentTopCellIsInSameLevelAsParent) {
                        if (!parentTopCell->isSubdivided()) {
                            // Level above case
                            *topCell = parentTopCell;
                            return false;
                        } else {
                            // Same level case
                            *topCell = parentTopCell->bottomRightChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The top cell is already above the parent cell
                        *topCell = parentTopCell;
                        return false;
                    }
                }
            }
        }
    }

    // "leftCell" is set to the cell at the left of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no left cell ("cell" is a left border cell).
    // true is returned if the left cell is at the same level.
    // false is returned if the left cell is in a level above or null.
    bool findLeftCell(CellType *cell, int cellX, int cellY, CellType **leftCell) const
    {
        CellType *parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell
        if (!parent) {
            if (cellX == 0) {
                *leftCell = nullptr;
                return false;
            }
            *leftCell = m_cells[cellY * m_widthInCells + cellX - 1];
            return true;
        }

        // Trivial cases. "cell" is a child in the right of the parent cell
        // so the left cell to "cell" is a child in the left of the same parent
        {
            if (cell == parent->topRightChild()) {
                *leftCell = parent->topLeftChild();
                return true;
            }

            if (cell == parent->bottomRightChild()) {
                *leftCell = parent->bottomLeftChild();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the left of the parent cell
        // so the left cell to "cell" doesn't have the same parent.
        // Recursion is used to find the left cell which can be in the same level,
        // in a level above (if the left cell is not subdivided enough),
        // or null (if this cell is in the left border of the grid)
        {
            if (cell == parent->topLeftChild()) {
                CellType *parentLeftCell;
                bool parentLeftCellIsInSameLevelAsParent = findLeftCell(parent, cellX, cellY, &parentLeftCell);
                if (!parentLeftCell) {
                    // Border cell case
                    *leftCell = nullptr;
                    return false;
                } else {
                    if (parentLeftCellIsInSameLevelAsParent) {
                        if (!parentLeftCell->isSubdivided()) {
                            // Level above case
                            *leftCell = parentLeftCell;
                            return false;
                        } else {
                            // Same level case
                            *leftCell = parentLeftCell->topRightChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The left cell is already above the parent cell
                        *leftCell = parentLeftCell;
                        return false;
                    }
                }
            }

            /* if (currentCell == parent->bottomLeftChild()) */
            {
                CellType *parentLeftCell;
                bool parentLeftCellIsInSameLevelAsParent = findLeftCell(parent, cellX, cellY, &parentLeftCell);
                if (!parentLeftCell) {
                    // Border cell case
                    *leftCell = nullptr;
                    return false;
                } else {
                    if (parentLeftCellIsInSameLevelAsParent) {
                        if (!parentLeftCell->isSubdivided()) {
                            // Level above case
                            *leftCell = parentLeftCell;
                            return false;
                        } else {
                            // Same level case
                            *leftCell = parentLeftCell->bottomRightChild();
                            return true;
                    }
                    } else {
                        // Level above case
                        // The left cell is already above the parent cell
                        *leftCell = parentLeftCell;
                        return false;
                    }
                }
            }
        }
    }

    // "bottomCell" is set to the cell at the bottom of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no bottom cell ("cell" is a bottom border cell).
    // true is returned if the bottom cell is at the same level.
    // false is returned if the bottom cell is in a level above or null.
    bool findBottomCell(CellType *cell, int cellX, int cellY, CellType **bottomCell) const
    {
        CellType *parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell
        if (!parent) {
            if (cellY == m_heightInCells - 1) {
                // "cell" is a bottom level border cell
                *bottomCell = nullptr;
                return false;
            }
            *bottomCell = m_cells[(cellY + 1) * m_widthInCells + cellX];
            return true;
        }

        // Trivial cases. "cell" is a child in the top of the parent cell
        // so the bottom cell to "cell" is a child in the bottom of the same parent
        {
            if (cell == parent->topLeftChild()) {
                *bottomCell = parent->bottomLeftChild();
                return true;
            }

            if (cell == parent->topRightChild()) {
                *bottomCell = parent->bottomRightChild();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the bottom of the parent cell
        // so the bottom cell to "cell" doesn't have the same parent.
        // Recursion is used to find the bottom cell which can be in the same level,
        // in a level above (if the bottom cell is not subdivided enough),
        // or null (if this cell is in the bottom border of the grid)
        {
            if (cell == parent->bottomLeftChild()) {
                CellType *parentBottomCell;
                bool parentBottomCellIsInSameLevelAsParent = findBottomCell(parent, cellX, cellY, &parentBottomCell);
                if (!parentBottomCell) {
                    // Border cell case
                    *bottomCell = nullptr;
                    return false;
                } else {
                    if (parentBottomCellIsInSameLevelAsParent) {
                        if (!parentBottomCell->isSubdivided()) {
                            // Level above case
                            *bottomCell = parentBottomCell;
                            return false;
                        } else {
                            // Same level case
                            *bottomCell = parentBottomCell->topLeftChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The bottom cell is already above the parent cell
                        *bottomCell = parentBottomCell;
                        return false;
                    }
                }
            }

            /* if (cell == parent->bottomRightChild()) */
            {
                CellType *parentBottomCell;
                bool parentBottomCellIsInSameLevelAsParent = findTopCell(parent, cellX, cellY, &parentBottomCell);
                if (!parentBottomCell) {
                    // Border cell case
                    *bottomCell = nullptr;
                    return false;
                } else {
                    if (parentBottomCellIsInSameLevelAsParent) {
                        if (!parentBottomCell->isSubdivided()) {
                            // Level above case
                            *bottomCell = parentBottomCell;
                            return false;
                        } else {
                            // Same level case
                            *bottomCell = parentBottomCell->topRightChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The bottom cell is already above the parent cell
                        *bottomCell = parentBottomCell;
                        return false;
                    }
                }
            }
        }
    }

    // "rightCell" is set to the cell at the right of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no right cell ("cell" is a right border cell).
    // true is returned if the right cell is at the same level.
    // false is returned if the right cell is in a level above or null.
    bool findRightCell(CellType *cell, int cellX, int cellY, CellType **rightCell)
    {
        CellType *parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell
        if (!parent) {
            if (cellX == m_widthInCells - 1) {
                *rightCell = nullptr;
                return false;
            }
            *rightCell = m_cells[cellY * m_widthInCells + cellX + 1];
            return true;
        }

        // Trivial cases. "cell" is a child in the left of the parent cell
        // so the right cell to "cell" is a child in the right of the same parent
        {
            if (cell == parent->topLeftChild()) {
                *rightCell = parent->topRightChild();
                return true;
            }

            if (cell == parent->bottomLeftChild()) {
                *rightCell = parent->bottomRightChild();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the right of the parent cell
        // so the right cell to "cell" doesn't have the same parent.
        // Recursion is used to find the right cell which can be in the same level,
        // in a level above (if the right cell is not subdivided enough),
        // or null (if this cell is in the right border of the grid)
        {
            if (cell == parent->topRightChild()) {
                CellType *parentRightCell;
                bool parentRightCellIsInSameLevelAsParent = findRightCell(parent, cellX, cellY, &parentRightCell);
                if (!parentRightCell) {
                    // Border cell case
                    *rightCell = nullptr;
                    return false;
                } else {
                    if (parentRightCellIsInSameLevelAsParent) {
                        if (!parentRightCell->isSubdivided()) {
                            // Level above case
                            *rightCell = parentRightCell;
                            return false;
                        } else {
                            // Same level case
                            *rightCell = parentRightCell->topLeftChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The right cell is already above the parent cell
                        *rightCell = parentRightCell;
                        return false;
                    }
                }
            }

            /* if (cell == parent->bottomRightChild()) */
            {
                CellType *parentRightCell;
                bool parentRightCellIsInSameLevelAsParent = findRightCell(parent, cellX, cellY, &parentRightCell);
                if (!parentRightCell) {
                    // Border cell case
                    *rightCell = nullptr;
                    return false;
                } else {
                    if (parentRightCellIsInSameLevelAsParent) {
                        if (!parentRightCell->isSubdivided()) {
                            // Level above case
                            *rightCell = parentRightCell;
                            return false;
                        } else {
                            // Same level case
                            *rightCell = parentRightCell->bottomLeftChild();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The right cell is already above the parent cell
                        *rightCell = parentRightCell;
                        return false;
                    }
                }
            }
        }
    }
};

}

#endif
