// Copyright (C) 2020 deiflou
// 
// This file is part of colorizer.
// 
// colorizer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// colorizer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with colorizer.  If not, see <http://www.gnu.org/licenses/>.

#include "window.h"
#include "scribble.hpp"
#include "../preprocessing/threshold.hpp"
#include "../preprocessing/skeleton.hpp"

#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>

void
window::setup_ui_()
{
    setMinimumSize(800, 600);

    QHBoxLayout * main_layout = new QHBoxLayout;
    QVBoxLayout * tools_layout = new QVBoxLayout;

    widget_container_image_ = new QWidget;
    widget_container_image_->installEventFilter(this);

    QVBoxLayout * layout_io = new QVBoxLayout;
    QVBoxLayout * layout_io_contents = new QVBoxLayout;
    QPushButton * button_open_image = new QPushButton("Open Image...");
    QPushButton * button_save_labeling_image = new QPushButton("Save Colorization Image...");
    QPushButton * button_save_composed_image = new QPushButton("Save Composed Image...");

    QVBoxLayout * layout_visualization = new QVBoxLayout;
    QVBoxLayout * layout_visualization_contents = new QVBoxLayout;
    QComboBox * combo_box_visualization = new QComboBox;
    combo_box_visualization->addItems
    (
        QStringList()
            << "Original Image"
            << "Preprocessed Image"
            << "Space Partitioning"
            << "Space Partitioning (Scribbles)"
            << "Space Partitioning (Labels)"
            << "Space Partitioning (Neighbors)"
            << "Labeling"
            << "Composed Image"
    );
    combo_box_visualization->setCurrentIndex(visualization_mode_);

    QVBoxLayout * layout_brush= new QVBoxLayout;
    QFormLayout * layout_brush_contents = new QFormLayout;
    QHBoxLayout * layout_brush_contents_size = new QHBoxLayout;
    QSlider * slider_brush_size = new QSlider(Qt::Horizontal);
    slider_brush_size->setRange(3, 50);
    slider_brush_size->setValue(brush_size_);
    QLabel * label_brush_size = new QLabel(QString::number(brush_size_) + "px");
    widget_palette_ = new QWidget;
    widget_palette_->setFixedSize(16 * palette_entry_size, 8 * palette_entry_size);
    widget_palette_->installEventFilter(this);

    QVBoxLayout * layout_other_options= new QVBoxLayout;
    QFormLayout * layout_other_options_contents = new QFormLayout;
    QCheckBox * check_box_use_implicit_scribble = new QCheckBox;
    check_box_use_implicit_scribble->setChecked(use_implicit_scribble_);
    QCheckBox * check_box_show_scribbles = new QCheckBox;
    check_box_show_scribbles->setChecked(show_scribbles_);

    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);
        tools_layout->setContentsMargins(10, 10, 10, 10);
        tools_layout->setSpacing(20);

            layout_io->setContentsMargins(0, 0, 0, 0);
            layout_io->setSpacing(5);
            layout_io->addWidget(new QLabel("I/O:"));
                layout_io_contents->setContentsMargins(10, 0, 0, 0);
                layout_io_contents->setSpacing(5);
                layout_io_contents->addWidget(button_open_image);
                layout_io_contents->addWidget(button_save_labeling_image);
                layout_io_contents->addWidget(button_save_composed_image);
            layout_io->addLayout(layout_io_contents);

            layout_visualization->setContentsMargins(0, 0, 0, 0);
            layout_visualization->setSpacing(5);
            layout_visualization->addWidget(new QLabel("Visualization:"));
                layout_visualization_contents->setContentsMargins(10, 0, 0, 0);
                layout_visualization_contents->setSpacing(5);
                layout_visualization_contents->addWidget(combo_box_visualization);
            layout_visualization->addLayout(layout_visualization_contents);

            layout_brush->setContentsMargins(0, 0, 0, 0);
            layout_brush->setSpacing(5);
            layout_brush->addWidget(new QLabel("Brush Options:"));
                layout_brush_contents->setContentsMargins(10, 0, 0, 0);
                layout_brush_contents->setSpacing(5);
                    layout_brush_contents_size->setContentsMargins(0, 0, 0, 0);
                    layout_brush_contents_size->setSpacing(5);
                    layout_brush_contents_size->addWidget(slider_brush_size);
                    layout_brush_contents_size->addWidget(label_brush_size);
                layout_brush_contents->addRow("Size:", layout_brush_contents_size);
                layout_brush_contents->addRow("Color:", widget_palette_);
            layout_brush->addLayout(layout_brush_contents);

            layout_other_options->setContentsMargins(0, 0, 0, 0);
            layout_other_options->setSpacing(5);
            layout_other_options->addWidget(new QLabel("Other Options:"));
                layout_other_options_contents->setContentsMargins(10, 0, 0, 0);
                layout_other_options_contents->setSpacing(5);
                layout_other_options_contents->addRow("Use Implicit Surrounding Background Scribble:", check_box_use_implicit_scribble);
                layout_other_options_contents->addRow("Show Scribbles:", check_box_show_scribbles);
            layout_other_options->addLayout(layout_other_options_contents);
            
        tools_layout->addLayout(layout_io);
        tools_layout->addLayout(layout_visualization);
        tools_layout->addLayout(layout_brush);
        tools_layout->addLayout(layout_other_options);
        tools_layout->addStretch();

    main_layout->addLayout(tools_layout);
    main_layout->addWidget(widget_container_image_, 1);

    setLayout(main_layout);

    connect
    (
        button_open_image,
        &QPushButton::clicked,
        [this]()
        {
            QString file_name;
            file_name = QFileDialog::getOpenFileName(this, "Open Image...", QString(), "Images (*.png *.xpm *.jpg)");

            if (file_name.isNull())
            {
                return;
            }

            QImage i(file_name);
            if (i.isNull())
            {
                return;
            }

            original_image_ = i.convertToFormat(QImage::Format_Grayscale8);
            preprocessed_image_ =
                preprocessing::skeleton_chen_hsu(preprocessing::threshold(original_image_, 192));

            std::vector<colorization_context_type::input_point> image_points;
            for (int y = 0; y < preprocessed_image_.height(); ++y)
            {
                quint8 * current_pixel = static_cast<quint8*>(preprocessed_image_.scanLine(y));
                for (int x = 0; x < preprocessed_image_.width(); ++x, ++current_pixel)
                {
                    if (*current_pixel == 0)
                    {
                        image_points.push_back
                        (
                            colorization_context_type::input_point
                            {
                                point_type(x, y),
                                colorization_context_type::intensity_min
                            }
                        );
                    }
                }
            }

            // original_image_ = i.convertToFormat(QImage::Format_Grayscale8);
            // preprocessed_image_ = QImage(i.width(), i.height(), QImage::Format_Grayscale8);

            // QVector<ColorizerType::InputPoint> image_points;
            // for (int y = 0; y < preprocessed_image_.height(); ++y) {
            //     const quint8 *current_pixel = original_image_.scanLine(y);
            //     quint8 *currentPixel2 = static_cast<quint8*>(preprocessed_image_.scanLine(y));
            //     for (int x = 0; x < preprocessed_image_.width(); ++x, ++current_pixel, ++currentPixel2) {
            //         int pixelValue = *current_pixel;
            //         pixelValue = pixelValue * pixelValue / 255;
            //         if (pixelValue < 128) {
            //             *currentPixel2 = static_cast<ColorizerType::IntensityType>(pixelValue);
            //             image_points.append(
            //                 ColorizerType::InputPoint{
            //                     QPoint(x, y),
            //                     static_cast<ColorizerType::IntensityType>(pixelValue)
            //                 }
            //             );
            //         } else {
            //             *currentPixel2 = 255;
            //         }
            //     }
            // }

            colorization_context_ =
                colorization_context_type
                (
                    0,
                    0,
                    preprocessed_image_.width(),
                    preprocessed_image_.height(),
                    cell_size,
                    image_points
                );
            colorization_context_.update_neighbors();

            scribbles_.clear();

            labeling_image_ = QImage(original_image_.width(), original_image_.height(), QImage::Format_ARGB32);
            labeling_image_.fill(0);

            position_ = QPointF(0.0, 0.0);
            scale_ = 1.0;

            widget_palette_->update();
            widget_container_image_->update();
        }
    );

    connect
    (
        button_save_labeling_image,
        &QPushButton::clicked,
        [this]()
        {
            if (labeling_image_.isNull())
            {
                return;
            }

            QString file_name;
            file_name = QFileDialog::getSaveFileName(this, "Open Image...", QString(), "Images (*.png *.xpm *.jpg)");

            if (file_name.isNull())
            {
                return;
            }

            labeling_image_.save(file_name);
        }
    );

    connect
    (
        combo_box_visualization,
        qOverload<int>(&QComboBox::currentIndexChanged),
        [this](int index)
        {
            if (index == visualization_mode_)
            {
                return;
            }

            visualization_mode_ = static_cast<visualization_modes>(index);
            widget_container_image_->update();
        }
    );

    connect
    (
        slider_brush_size,
        &QSlider::valueChanged,
        [this, label_brush_size](int value)
        {
            if (value == brush_size_)
            {
                return;
            }

            label_brush_size->setText(QString::number(value) + "px");
            brush_size_ = value;
        }
    );

    connect
    (
        check_box_use_implicit_scribble,
        &QCheckBox::toggled,
        [this](bool toggled)
        {
            if (toggled == use_implicit_scribble_)
            {
                return;
            }
            use_implicit_scribble_ = toggled;
            colorize();
            widget_container_image_->update();
        }
    );

    connect
    (
        check_box_show_scribbles,
        &QCheckBox::toggled,
        [this](bool toggled)
        {
            if (toggled == show_scribbles_)
            {
                return;
            }
            show_scribbles_ = toggled;
            widget_container_image_->update();
        }
    );
}
