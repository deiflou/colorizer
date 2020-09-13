#include "window.h"
#include "scribble.hpp"
#include "preprocessing/threshold.hpp"
#include "preprocessing/skeleton.hpp"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>

void Window::setupUI()
{
    setMinimumSize(800, 600);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    QVBoxLayout *toolsLayout = new QVBoxLayout;

    m_widgetContainerImage = new QWidget;
    m_widgetContainerImage->installEventFilter(this);

    QVBoxLayout *layoutIO = new QVBoxLayout;
    QVBoxLayout *layoutIOContents = new QVBoxLayout;
    QPushButton *buttonOpenImage = new QPushButton("Open Image...");
    QPushButton *buttonSaveLabelingImage = new QPushButton("Save Colorization Image...");
    QPushButton *buttonSaveComposedImage = new QPushButton("Save Composed Image...");

    QVBoxLayout *layoutVisualization = new QVBoxLayout;
    QVBoxLayout *layoutVisualizationContents = new QVBoxLayout;
    QComboBox *comboBoxVisualization = new QComboBox;
    comboBoxVisualization->addItems(QStringList()
        << "Original Image"
        << "Skeleton"
        << "Space Partitioning"
        << "Space Partitioning (Scribbles)"
        << "Space Partitioning (Labels)"
        << "Space Partitioning (Neighbors)"
        << "Labeling"
        << "Composed Image"
    );
    comboBoxVisualization->setCurrentIndex(m_visualizationMode);

    QVBoxLayout *layoutBrush= new QVBoxLayout;
    QFormLayout *layoutBrushContents = new QFormLayout;
    QHBoxLayout *layoutBrushContentsSize = new QHBoxLayout;
    QSlider *sliderBrushSize = new QSlider(Qt::Horizontal);
    sliderBrushSize->setRange(3, 50);
    sliderBrushSize->setValue(m_brushSize);
    QLabel *labelBrushSize = new QLabel(QString::number(m_brushSize) + "px");
    m_widgetPalette = new QWidget;
    m_widgetPalette->setFixedSize(16 * kPaletteEntrySize, 8 * kPaletteEntrySize);
    m_widgetPalette->installEventFilter(this);

    QVBoxLayout *layoutOtherOptions= new QVBoxLayout;
    QFormLayout *layoutOtherOptionsContents = new QFormLayout;
    QCheckBox *checkBoxUseImplicitScribble = new QCheckBox;
    checkBoxUseImplicitScribble->setChecked(m_useImplicitScribble);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
        toolsLayout->setContentsMargins(10, 10, 10, 10);
        toolsLayout->setSpacing(20);

            layoutIO->setContentsMargins(0, 0, 0, 0);
            layoutIO->setSpacing(5);
            layoutIO->addWidget(new QLabel("I/O:"));
                layoutIOContents->setContentsMargins(10, 0, 0, 0);
                layoutIOContents->setSpacing(5);
                layoutIOContents->addWidget(buttonOpenImage);
                layoutIOContents->addWidget(buttonSaveLabelingImage);
                layoutIOContents->addWidget(buttonSaveComposedImage);
            layoutIO->addLayout(layoutIOContents);

            layoutVisualization->setContentsMargins(0, 0, 0, 0);
            layoutVisualization->setSpacing(5);
            layoutVisualization->addWidget(new QLabel("Visualization:"));
                layoutVisualizationContents->setContentsMargins(10, 0, 0, 0);
                layoutVisualizationContents->setSpacing(5);
                layoutVisualizationContents->addWidget(comboBoxVisualization);
            layoutVisualization->addLayout(layoutVisualizationContents);

            layoutBrush->setContentsMargins(0, 0, 0, 0);
            layoutBrush->setSpacing(5);
            layoutBrush->addWidget(new QLabel("Brush Options:"));
                layoutBrushContents->setContentsMargins(10, 0, 0, 0);
                layoutBrushContents->setSpacing(5);
                    layoutBrushContentsSize->setContentsMargins(0, 0, 0, 0);
                    layoutBrushContentsSize->setSpacing(5);
                    layoutBrushContentsSize->addWidget(sliderBrushSize);
                    layoutBrushContentsSize->addWidget(labelBrushSize);
                layoutBrushContents->addRow("Size:", layoutBrushContentsSize);
                layoutBrushContents->addRow("Color:", m_widgetPalette);
            layoutBrush->addLayout(layoutBrushContents);

            layoutOtherOptions->setContentsMargins(0, 0, 0, 0);
            layoutOtherOptions->setSpacing(5);
            layoutOtherOptions->addWidget(new QLabel("Other Options:"));
                layoutOtherOptionsContents->setContentsMargins(10, 0, 0, 0);
                layoutOtherOptionsContents->setSpacing(5);
                layoutOtherOptionsContents->addRow("Use Implicit Surrounding Background Scribble:", checkBoxUseImplicitScribble);
            layoutOtherOptions->addLayout(layoutOtherOptionsContents);
            
        toolsLayout->addLayout(layoutIO);
        toolsLayout->addLayout(layoutVisualization);
        toolsLayout->addLayout(layoutBrush);
        toolsLayout->addLayout(layoutOtherOptions);
        toolsLayout->addStretch();

    mainLayout->addLayout(toolsLayout);
    mainLayout->addWidget(m_widgetContainerImage, 1);

    setLayout(mainLayout);

    connect(buttonOpenImage, &QPushButton::clicked,
        [this]()
        {
            QString fileName;
            fileName = QFileDialog::getOpenFileName(this, "Open Image...", QString(), "Images (*.png *.xpm *.jpg)");

            if (fileName.isNull()) {
                return;
            }

            QImage i(fileName);
            if (i.isNull()) {
                return;
            }

            // m_originalImage = i.convertToFormat(QImage::Format_Grayscale8);
            // m_thresholdImage = Preprocessing::threshold(m_originalImage, 192);
            // m_skeletonImage = Preprocessing::skeletonChenHsu(m_thresholdImage);

            // QVector<ColorizerType::InputPoint> imagePoints;
            // for (int y = 0; y < m_skeletonImage.height(); ++y) {
            //     quint8 *currentPixel = static_cast<quint8*>(m_skeletonImage.scanLine(y));
            //     for (int x = 0; x < m_skeletonImage.width(); ++x, ++currentPixel) {
            //         if (*currentPixel == 0) {
            //             imagePoints.append(
            //                 ColorizerType::InputPoint{
            //                     QPoint(x, y),
            //                     ColorizerType::Intensity_Min
            //                 }
            //             );
            //         }
            //     }
            // }

            m_originalImage = i.convertToFormat(QImage::Format_Grayscale8);
            m_skeletonImage = QImage(i.width(), i.height(), QImage::Format_Grayscale8);

            QVector<ColorizerType::InputPoint> imagePoints;
            for (int y = 0; y < m_skeletonImage.height(); ++y) {
                const quint8 *currentPixel = m_originalImage.scanLine(y);
                quint8 *currentPixel2 = static_cast<quint8*>(m_skeletonImage.scanLine(y));
                for (int x = 0; x < m_skeletonImage.width(); ++x, ++currentPixel, ++currentPixel2) {
                    int pixelValue = *currentPixel;
                    pixelValue = pixelValue * pixelValue / 255;
                    if (pixelValue < 128) {
                        *currentPixel2 = static_cast<ColorizerType::IntensityType>(pixelValue);
                        imagePoints.append(
                            ColorizerType::InputPoint{
                                QPoint(x, y),
                                static_cast<ColorizerType::IntensityType>(pixelValue)
                            }
                        );
                    } else {
                        *currentPixel2 = 255;
                    }
                }
            }

            m_colorizer = ColorizerType(0, 0, m_skeletonImage.width(), m_skeletonImage.height(), kCellSize, imagePoints);
            m_colorizer.updateNeighbors();

            m_position = QPointF(0.0, 0.0);
            m_scale = 1.0;

            m_widgetPalette->update();
            m_widgetContainerImage->update();
        }
    );

    connect(comboBoxVisualization, qOverload<int>(&QComboBox::currentIndexChanged),
        [this](int index)
        {
            if (index == m_visualizationMode) {
                return;
            }

            m_visualizationMode = static_cast<VisualizationMode>(index);
            m_widgetContainerImage->update();
        }
    );

    connect(sliderBrushSize, &QSlider::valueChanged,
        [this, labelBrushSize](int value)
        {
            if (value == m_brushSize) {
                return;
            }

            labelBrushSize->setText(QString::number(value) + "px");
            m_brushSize = value;
        }
    );

    connect(checkBoxUseImplicitScribble, &QCheckBox::toggled,
        [this](bool toggled)
        {
            if (toggled == m_useImplicitScribble) {
                return;
            }
            m_useImplicitScribble = toggled;
            m_colorizer.colorize(m_useImplicitScribble);
            m_widgetContainerImage->update();
        }
    );
}
