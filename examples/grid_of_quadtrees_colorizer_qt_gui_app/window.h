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

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

#include <lazybrush/grid_of_quadtrees_colorizer/colorization_context.hpp>

class scribble;
class colorizer_scribble;

using colorization_context_type = lazybrush::grid_of_quadtrees_colorizer::colorization_context<colorizer_scribble>;
using grid_type = typename colorization_context_type::working_grid_type;
using cell_type = typename colorization_context_type::working_grid_cell_type;
using point_type = typename colorization_context_type::point_type;
using rect_type = typename colorization_context_type::rect_type;

constexpr int cell_size = 64;
constexpr int palette_entry_size = 12;

extern const unsigned char the_palette[128][3];

class window : public QWidget
{
    Q_OBJECT

public:
    window();
    ~window();

protected:
    bool
    eventFilter(QObject *o, QEvent *e) override;

private:
    enum painting_device_types
    {
        painting_device_type_none,
        painting_device_type_mouse,
        painting_device_type_pen
    };

    enum visualization_modes
    {
        visualization_mode_original_image,
        visualization_mode_preprocessed_image,
        visualization_mode_space_partitioning,
        visualization_mode_space_partitioning_scribbles,
        visualization_mode_space_partitioning_labels,
        visualization_mode_space_partitioning_neighbors,
        visualization_mode_labeling,
        visualization_mode_composed_image
    };

    QImage original_image_;
    QImage preprocessed_image_;
    QImage labeling_image_;
    colorization_context_type colorization_context_;
    QVector<scribble> scribbles_;
    QWidget * widget_container_image_;
    QWidget * widget_palette_;

    QPointF position_;
    qreal scale_;
    
    QPoint last_mouse_position_;
    bool is_dragging_;
    QPoint last_painting_position_;
    bool is_painting_;
    painting_device_types painting_device_;
    visualization_modes visualization_mode_;
    int brush_size_;
    cell_type * selected_cell_;
    int selected_color_index_;
    int selected_background_color_index_;
    bool use_implicit_scribble_;
    bool show_scribbles_;

    void
    setup_ui_();

    int
    pressure_to_radius_(qreal pressure) const;
    QPoint
    transform_pen_position_(const QPoint &penPosition) const;

    void
    colorize();

    QPoint
    point_type_to_QPoint(point_type const & point);
    QRect
    rect_type_to_QRect(rect_type const & rect);
    point_type
    QPoint_to_point_type(QPoint const & point);
    rect_type
    QRect_to_rect_type(QRect const & rect);
};

#endif