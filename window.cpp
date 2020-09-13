#include "window.h"
#include "scribble.hpp"

#include <cmath>

#include <QEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QElapsedTimer>
#include <QTabletEvent>

Window::Window()
    : m_position(0.0, 0.0)
    , m_scale(1.0)
    , m_lastMousePosition(0, 0)
    , m_isDragging(false)
    , m_lastPaintingPosition(0, 0)
    , m_isPainting(false)
    , m_paintingDevice(PaintingDevice_None)
    , m_visualizationMode(VisualizationMode::VisualizationMode_ComposedImage)
    , m_brushSize(20)
    , m_selectedCell(nullptr)
    , m_selectedColorIndex(0)
    , m_selectedBackgroundColorIndex(-1)
    , m_useImplicitScribble(false)
    , m_showScribbles(true)
{
    setupUI();
}

Window::~Window()
{
}

bool Window::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_widgetContainerImage) {
        if (e->type() == QEvent::Paint) {
            QPainter painter(m_widgetContainerImage);
            painter.fillRect(m_widgetContainerImage->rect(), Qt::darkGray);

            QTransform tfr;
            tfr.translate(m_widgetContainerImage->width() / 2.0, m_widgetContainerImage->height() / 2.0);
            tfr.translate(m_position.x(), m_position.y());
            tfr.scale(m_scale, m_scale);

            painter.setTransform(tfr);

            QPointF imagePosition(-(m_skeletonImage.width() / 2.0), -(m_skeletonImage.height() / 2.0));

            painter.save();
            if (m_visualizationMode == VisualizationMode_OriginalImage) {
                painter.drawImage(imagePosition, m_originalImage);
                
            } else if (m_visualizationMode == VisualizationMode_Skeleton) {
                painter.drawImage(imagePosition, m_skeletonImage);

            } else if (m_visualizationMode == VisualizationMode_SpacePartitioning) {
                const GridType &workingGrid = m_colorizer.workingGrid();
                workingGrid.visitLeafs(
                    [&painter, &imagePosition](CellType* cell) -> bool
                    {
                        int c = static_cast<int>(std::log2(cell->size())) * 300 / static_cast<int>(std::log2(kCellSize));
                        if (cell->data().intensity == ColorizerType::Intensity_Min) {
                            painter.fillRect(QRectF(cell->rect()).translated(imagePosition), qRgb(0, 0, 0));
                        } else {
                            painter.fillRect(QRectF(cell->rect()).translated(imagePosition), QColor::fromHsv(c, 255, 255));
                        }
                        return true;
                    }
                );

            } else if (m_visualizationMode == VisualizationMode_SpacePartitioningScribbles) {
                const GridType &workingGrid = m_colorizer.workingGrid();

                workingGrid.visitLeafs(
                    [&painter, &imagePosition, this](CellType* cell) -> bool
                    {
                        int h, s, v;
                        if (cell->data().scribbleIndex == ColorizerType::ScribbleIndex_Undefined) {
                            h = 0;
                            s = 0;
                        } else {
                            h = cell->data().scribbleIndex * 255 / m_scribbles.size();
                            s = 255;
                        }
                        v = static_cast<int>(std::log2(cell->size())) * 127 / static_cast<int>(std::log2(kCellSize)) + 128;
                        painter.fillRect(QRectF(cell->rect()).translated(imagePosition), QColor::fromHsv(h, s, v));
                        return true;
                    }
                );

            } else if (m_visualizationMode == VisualizationMode_SpacePartitioningLabels) {
                const GridType &workingGrid = m_colorizer.workingGrid();

                workingGrid.visitLeafs(
                    [&painter, &imagePosition, this](CellType* cell) -> bool
                    {
                        QBrush b;
                        if (cell->data().preferredLabelId == ColorizerType::LabelId_Undefined) {
                            int v = static_cast<int>(std::log2(cell->size())) * 127 / static_cast<int>(std::log2(kCellSize)) + 128;
                            b = QBrush(QColor::fromHsv(0, 0, v));
                        } else {
                            const int index = cell->data().preferredLabelId;
                            if (index == m_selectedBackgroundColorIndex) {
                                b = QBrush(QColor(the_palette[index][0],
                                                    the_palette[index][1],
                                                    the_palette[index][2]),
                                                    Qt::FDiagPattern);
                            } else {
                                if (index >= 0 && index < 128) {
                                    b = QBrush(QColor(the_palette[index][0],
                                                      the_palette[index][1],
                                                      the_palette[index][2]));
                                }
                            }
                        }
                        painter.fillRect(QRectF(cell->rect()).translated(imagePosition), b);
                        return true;
                    }
                );

            } else if (m_visualizationMode == VisualizationMode_SpacePartitioningNeighbors) {
                const GridType &workingGrid = m_colorizer.workingGrid();

                workingGrid.visitLeafs(
                    [&painter, &imagePosition, this](CellType* cell) -> bool
                    {
                        int v = static_cast<int>(std::log2(cell->size())) * 127 / static_cast<int>(std::log2(kCellSize)) + 128;
                        if (cell == m_selectedCell) {
                            painter.fillRect(QRectF(cell->rect()).translated(imagePosition), QColor::fromHsv(0, 255, v));

                            QPen pen(QColor(0, 0, 255));
                            pen.setCosmetic(true);
                            painter.setPen(pen);
                            for (const CellType *cell2 : cell->topLeafNeighbors()) {
                                painter.drawLine(cell->center() + imagePosition, cell2->center() + imagePosition);
                            }
                            for (const CellType *cell2 : cell->leftLeafNeighbors()) {
                                painter.drawLine(cell->center() + imagePosition, cell2->center() + imagePosition);
                            }
                            for (const CellType *cell2 : cell->bottomLeafNeighbors()) {
                                painter.drawLine(cell->center() + imagePosition, cell2->center() + imagePosition);
                            }
                            for (const CellType *cell2 : cell->rightLeafNeighbors()) {
                                painter.drawLine(cell->center() + imagePosition, cell2->center() + imagePosition);
                            }
                        } else {
                            painter.fillRect(QRectF(cell->rect()).translated(imagePosition), QColor::fromHsv(0, 0, v));
                        }
                        return true;
                    }
                );

            } else if (m_visualizationMode == VisualizationMode_Labeling) {
                const GridType &workingGrid = m_colorizer.workingGrid();

                int alpha = m_showScribbles ? 64 : 255;

                workingGrid.visitLeafs(
                    [&painter, &imagePosition, alpha, this](CellType* cell) -> bool
                    {
                        if (cell->data().computedLabelId != ColorizerType::LabelId_Undefined &&
                            cell->data().computedLabelId != ColorizerType::LabelId_ImplicitSurrounding) {
                            const int index = cell->data().computedLabelId;
                            if (index != m_selectedBackgroundColorIndex && index >= 0 && index < 128 && index) {
                                QColor c = QColor(the_palette[index][0],
                                                  the_palette[index][1],
                                                  the_palette[index][2],
                                                  alpha);
                                painter.fillRect(QRectF(cell->rect()).translated(imagePosition), c);
                            }
                        }
                        return true;
                    }
                );

                if (m_showScribbles) {
                    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                    for (const Scribble & scribble : m_scribbles) {
                        painter.drawImage(QRectF(scribble.rect()).translated(imagePosition), scribble.image());
                    }
                }

            } else if (m_visualizationMode == VisualizationMode_ComposedImage) {
                const GridType &workingGrid = m_colorizer.workingGrid();

                int alpha = m_showScribbles ? 64 : 255;
                
                workingGrid.visitLeafs(
                    [&painter, &imagePosition, alpha, this](CellType* cell) -> bool
                    {
                        if (cell->data().computedLabelId != ColorizerType::LabelId_Undefined &&
                            cell->data().computedLabelId != ColorizerType::LabelId_ImplicitSurrounding) {
                            const int index = cell->data().computedLabelId;
                            if (index != m_selectedBackgroundColorIndex && index >= 0 && index < 128) {
                                QColor c = QColor(the_palette[index][0],
                                                  the_palette[index][1],
                                                  the_palette[index][2],
                                                  alpha);
                                painter.fillRect(QRectF(cell->rect()).translated(imagePosition), c);
                            }
                        }
                        return true;
                    }
                );

                painter.setCompositionMode(QPainter::CompositionMode_Multiply);
                painter.drawImage(imagePosition, m_originalImage);

                if (m_showScribbles) {
                    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                    for (const Scribble & scribble : m_scribbles) {
                        painter.drawImage(QRectF(scribble.rect()).translated(imagePosition), scribble.image());
                    }
                }
            }
            painter.restore();

            if (m_isPainting) {
                const Scribble &currentScribble = m_scribbles.back();
                painter.drawImage(-(m_skeletonImage.width() / 2.0) + currentScribble.rect().x(),
                                  -(m_skeletonImage.height() / 2.0) + currentScribble.rect().y(),
                                  currentScribble.image());
            }

            return true;
            
        } else if (e->type() == QEvent::Wheel && !m_isDragging && !m_isPainting) {
            QWheelEvent * we = dynamic_cast<QWheelEvent*>(e);
            const QPointF widgetCenter(m_widgetContainerImage->width() / 2, m_widgetContainerImage->height() / 2);
            const QPointF mousePosition = we->position() - widgetCenter;
            const QPointF mouseDelta = QPointF(m_position - mousePosition) / m_scale;

            if (we->angleDelta().y() > 0) {
                m_scale *= 2.0;
            } else {
                m_scale /= 2.0;
            }
            if (m_scale > 8.0) {
                m_scale = 8.0;
            } else if (m_scale < 0.125) {
                m_scale = 0.125;
            }
            m_position = mousePosition + mouseDelta * m_scale;
            m_widgetContainerImage->update();

            return true;

        } else if (e->type() == QEvent::MouseButtonPress && !m_isDragging && !m_isPainting) {
            QMouseEvent * me = dynamic_cast<QMouseEvent*>(e);
            if (me->button() == Qt::MidButton) {
                m_lastMousePosition = me->pos();
                m_isDragging = true;
            } else if (me->button() == Qt::LeftButton) {
                const QPoint penPosition = transformPenPosition(me->pos());

                m_scribbles.push_back(Scribble(QColor(
                                                    the_palette[m_selectedColorIndex][0],
                                                    the_palette[m_selectedColorIndex][1],
                                                    the_palette[m_selectedColorIndex][2]
                                               )));
                m_scribbles.back().moveTo(penPosition);
                m_scribbles.back().lineTo(penPosition, pressureToRadius(1.0));

                m_lastPaintingPosition = me->pos();
                m_isPainting = true;
                m_paintingDevice = PaintingDevice_Mouse;

                m_widgetContainerImage->update();
            } else if (me->button() == Qt::RightButton) {
                if (m_visualizationMode == VisualizationMode_SpacePartitioningNeighbors) {
                    m_selectedCell = m_colorizer.workingGrid().leafCellAt(transformPenPosition(me->pos()));
                    update();
                }
            }

            return true;

        } else if (e->type() == QEvent::MouseMove) {
            QMouseEvent * me = dynamic_cast<QMouseEvent*>(e);

            if (m_isDragging) {
                QPoint offset = me->pos() - m_lastMousePosition;
                if (me->buttons() & Qt::MidButton) {
                    m_position += QPointF(offset.x(), offset.y());
                }
                m_lastMousePosition = me->pos();
                m_widgetContainerImage->update();
            } else if (m_isPainting && m_paintingDevice == PaintingDevice_Mouse) {
                const QPoint paintingPosition = transformPenPosition(me->pos());

                m_scribbles.back().lineTo(paintingPosition, pressureToRadius(1.0));
                m_lastPaintingPosition = me->pos();

                m_widgetContainerImage->update();
            }

            return true;

        } else if (e->type() == QEvent::MouseButtonRelease) {
            m_isDragging = false;
            if (m_isPainting && m_paintingDevice == PaintingDevice_Mouse) {
                m_colorizer.appendScribble(BKColorizerScribble(m_scribbles.back(), m_selectedColorIndex));

                m_isPainting = false;
                m_paintingDevice = PaintingDevice_None;

                m_selectedCell = nullptr;

                m_colorizer.colorize(m_useImplicitScribble);

                m_widgetContainerImage->update();
            }

            return true;

        } else if (e->type() == QEvent::TabletPress && !m_isDragging && !m_isPainting) {
            QTabletEvent * te = dynamic_cast<QTabletEvent*>(e);

            if (te->buttons() & Qt::LeftButton) {
                const QPoint penPosition = transformPenPosition(te->pos());

                m_scribbles.push_back(Scribble(QColor(
                                                    the_palette[m_selectedColorIndex][0],
                                                    the_palette[m_selectedColorIndex][1],
                                                    the_palette[m_selectedColorIndex][2]
                                               )));
                m_scribbles.back().moveTo(penPosition);
                m_scribbles.back().lineTo(penPosition, pressureToRadius(te->pressure()));

                m_lastPaintingPosition = te->pos();
                m_isPainting = true;
                m_paintingDevice = PaintingDevice_Pen;

                m_widgetContainerImage->update();
            }

            return true;

        } else if (e->type() == QEvent::TabletMove && m_isPainting && m_paintingDevice == PaintingDevice_Pen) {
            QTabletEvent * te = dynamic_cast<QTabletEvent*>(e);
            
            const QPoint penPosition = transformPenPosition(te->pos());

            m_scribbles.back().lineTo(penPosition, pressureToRadius(te->pressure()));
            m_lastPaintingPosition = te->pos();

            m_widgetContainerImage->update();

            return true;

        } else if (e->type() == QEvent::TabletRelease && m_isPainting && m_paintingDevice == PaintingDevice_Pen) {
            m_colorizer.appendScribble(BKColorizerScribble(m_scribbles.back(), m_selectedColorIndex));

            m_isPainting = false;
            m_paintingDevice = PaintingDevice_None;

            m_selectedCell = nullptr;

            m_colorizer.colorize(m_useImplicitScribble);

            m_widgetContainerImage->update();

            return true;
        }
    }

    if (o == m_widgetPalette) {
        if (e->type() == QEvent::Paint) {
            QPainter painter(m_widgetPalette);
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 16; ++x) {
                    const int index = y * 16 + x;
                    painter.fillRect(x * kPaletteEntrySize,
                                     y * kPaletteEntrySize,
                                     kPaletteEntrySize,
                                     kPaletteEntrySize,
                                     qRgb(the_palette[index][0],
                                          the_palette[index][1],
                                          the_palette[index][2]));

                    const int xx = x * kPaletteEntrySize;
                    const int yy = y * kPaletteEntrySize;
                    const int halfPaleteEntrySize = kPaletteEntrySize / 2;

                    if (index == m_selectedBackgroundColorIndex) {
                        const QRect symbolRect(xx + 2, yy + 2, halfPaleteEntrySize - 2, halfPaleteEntrySize - 2);
                        painter.fillRect(symbolRect.translated(1, 1), Qt::black);
                        painter.fillRect(symbolRect, Qt::green);
                    }

                    if (index == m_selectedColorIndex) {
                        painter.setBrush(Qt::NoBrush);
                        painter.setPen(Qt::white);
                        painter.drawRect(xx + 1, yy + 1, kPaletteEntrySize - 3, kPaletteEntrySize - 3);
                        painter.setPen(Qt::black);
                        painter.drawRect(xx, yy, kPaletteEntrySize - 1, kPaletteEntrySize - 1);
                    }
                }
            }

            return true;

        } else if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent * me = dynamic_cast<QMouseEvent*>(e);

            const int x = me->pos().x() / kPaletteEntrySize;
            const int y = me->pos().y() / kPaletteEntrySize;
            const int index = y * 16 + x;

            if (index >= 0 && index < 128) {
                if (me->button() == Qt::LeftButton) {
                    m_selectedColorIndex = index;
                } else if (me->button() == Qt::RightButton) {
                    if (index == m_selectedBackgroundColorIndex) {
                        m_selectedBackgroundColorIndex = -1;
                    } else {
                        m_selectedBackgroundColorIndex = index;
                    }
                    m_widgetContainerImage->update();
                }
                m_widgetPalette->update();
            }

            return true;
        }
    }

    return false;
}

int Window::pressureToRadius(qreal pressure) const
{
    const qreal maxRadius = m_brushSize / 2.0;
    return static_cast<int>(std::round(std::pow(pressure, 4) * maxRadius + 0.5));
}

QPoint Window::transformPenPosition(const QPoint &penPosition) const
{
    const QPointF widgetCenter(m_widgetContainerImage->width() / 2, m_widgetContainerImage->height() / 2);
    const QPointF imagePosition(-(m_originalImage.width() / 2.0), -(m_originalImage.height() / 2.0));
    const QPointF transformedPenPositionF = QPointF(penPosition - (m_position + widgetCenter)) / m_scale - imagePosition;
    return QPoint(static_cast<int>(transformedPenPositionF.x()), static_cast<int>(transformedPenPositionF.y()));
}
