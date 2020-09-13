#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

#include <BKColorizer.hpp>

class Scribble;
class BKColorizerScribble;

using ColorizerType = Colorizer::BKColorizer<BKColorizerScribble>;
using GridType = typename ColorizerType::WorkingGridType;
using CellType = typename ColorizerType::WorkingGridCellType;

constexpr int kCellSize = 64;
constexpr int kPaletteEntrySize = 12;

extern const unsigned char the_palette[128][3];

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();
    ~Window();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    enum PaintingDevice {
        PaintingDevice_None,
        PaintingDevice_Mouse,
        PaintingDevice_Pen
    };

    enum VisualizationMode {
        VisualizationMode_OriginalImage,
        VisualizationMode_Skeleton,
        VisualizationMode_SpacePartitioning,
        VisualizationMode_SpacePartitioningScribbles,
        VisualizationMode_SpacePartitioningLabels,
        VisualizationMode_SpacePartitioningNeighbors,
        VisualizationMode_Labeling,
        VisualizationMode_ComposedImage
    };

    QImage m_originalImage;
    QImage m_thresholdImage;
    QImage m_skeletonImage;
    ColorizerType m_colorizer;
    QVector<Scribble> m_scribbles;
    QWidget *m_widgetContainerImage;
    QWidget *m_widgetPalette;

    QPointF m_position;
    qreal m_scale;
    
    QPoint m_lastMousePosition;
    bool m_isDragging;
    QPoint m_lastPaintingPosition;
    bool m_isPainting;
    PaintingDevice m_paintingDevice;
    VisualizationMode m_visualizationMode;
    int m_brushSize;
    CellType *m_selectedCell;
    int m_selectedColorIndex;
    int m_selectedBackgroundColorIndex;
    bool m_useImplicitScribble;
    bool m_showScribbles;

    void setupUI();

    int pressureToRadius(qreal pressure) const;
    QPoint transformPenPosition(const QPoint &penPosition) const;
};

#endif