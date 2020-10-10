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

#ifndef GRID_OF_QUADTREES_COLORIZER_QT_GUI_APP_SCRIBBLE_HPP
#define GRID_OF_QUADTREES_COLORIZER_QT_GUI_APP_SCRIBBLE_HPP

#include <lazybrush/grid_of_quadtrees_colorizer/types.hpp>
#include <lazybrush/grid_of_quadtrees_colorizer/colorization_context.hpp>

#include <vector>

#include <QImage>
#include <QVector>
#include <QPoint>
#include <QRect>
#include <QColor>

class colorizer_scribble;

// Quick and dirty scribble implementation
class scribble
{
public:
    scribble(scribble const &) = default;
    scribble(scribble &&) = default;
    scribble &
    operator=(scribble const &) = default;
    scribble &
    operator=(scribble &&) = default;
    scribble(QColor const & color);

    using point_type = typename lazybrush::grid_of_quadtrees_colorizer::colorization_context<colorizer_scribble>::point_type;
    using rect_type = typename lazybrush::grid_of_quadtrees_colorizer::colorization_context<colorizer_scribble>::rect_type;

    std::vector<point_type>
    contour_points() const;
    bool
    contains_point(QPoint const & point) const;
    QRect const &
    rect() const;

    void
    move_to(QPoint const & point, int radius = 0);
    void
    line_to(QPoint const & point, int radius);

    QImage const &
    image() const;

private:
    QImage image_;
    QPoint position_;
    int radius_;
    QRect image_rect_;
    QColor color_;
    mutable std::vector<point_type> cached_contour_points_;
    mutable bool cache_is_valid_{false};

    void
    resize_image_to_contain(QPoint const & point, int radius);
};

// Proxy scribble class to use with the colorizer
class colorizer_scribble
{
public:
    using colorization_context = lazybrush::grid_of_quadtrees_colorizer::colorization_context<colorizer_scribble>;
    using label_type = typename colorization_context::label_type;
    using point_type = typename colorization_context::point_type;
    using rect_type = typename colorization_context::rect_type;

    colorizer_scribble(colorizer_scribble const &) = default;
    colorizer_scribble(colorizer_scribble &&) = default;
    colorizer_scribble &
    operator=(colorizer_scribble const &) = default;
    colorizer_scribble &
    operator=(colorizer_scribble &&) = default;
    colorizer_scribble(scribble const & scribble, label_type label = colorization_context::label_undefined);

    std::vector<point_type>
    contour_points() const;
    bool
    contains_point(point_type const & point) const;
    rect_type
    rect() const;
    label_type
    label() const;

private:
    scribble scribble_;
    label_type label_;
};

#endif
