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

#ifndef LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_TYPES_HPP
#define LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_TYPES_HPP

#include <algorithm>

namespace lazybrush
{
namespace grid_of_quadtrees_colorizer
{

template <typename field_type_tp>
struct point
{
    using field_type = field_type_tp;

    point() = default;
    point(point const &) = default;
    point(point &&) = default;
    point& operator=(point const &) = default;
    point& operator=(point &&) = default;

    point(field_type x, field_type y) :
        x_(x),
        y_(y)
    {}

    field_type
    x() const
    {
        return x_;
    }

    field_type
    y() const
    {
        return y_;
    }

    void
    set_x(field_type new_x)
    {
        x_ = new_x;
    }

    void
    set_y(field_type new_y)
    {
        y_ = new_y;
    }

private:
    field_type x_{static_cast<field_type>(0)};
    field_type y_{static_cast<field_type>(0)};
};

template <typename field_type_tp>
struct rect
{
    using field_type = field_type_tp;
    using point_type = point<field_type>;

    rect(rect const &) = default;
    rect(rect &&) = default;
    rect& operator=(rect const &) = default;
    rect& operator=(rect &&) = default;

    rect() :
        top_left_(1, 1),
        bottom_right_(0, 0)
    {}

    rect(point_type const & top_left, point_type const & bottom_right) :
        top_left_(top_left),
        bottom_right_(bottom_right)
    {}

    rect(point_type const & top_left, field_type width, field_type height) :
        rect
        (
            top_left,
            point_type
            (
                top_left.x() + width - static_cast<field_type>(1),
                top_left.y() + height - static_cast<field_type>(1)
            )
        )
    {}

    rect(field_type x, field_type y, field_type width, field_type height) :
        rect(point_type(x, y), width, height)
    {}

    field_type
    x() const
    {
        return left();
    }

    field_type
    y() const
    {
        return top();
    }

    field_type
    left() const
    {
        return top_left_.x();
    }

    field_type
    right() const
    {
        return bottom_right_.x();
    }

    field_type
    top() const
    {
        return top_left_.y();
    }

    field_type
    bottom() const
    {
        return bottom_right_.y();
    }

    field_type
    width() const
    {
        return right() - left() + static_cast<field_type>(1);
    }

    field_type
    height() const
    {
        return bottom() - top() + static_cast<field_type>(1);
    }

    point_type const &
    top_left() const
    {
        return top_left_;
    }

    point_type const &
    bottom_right() const
    {
        return bottom_right_;
    }

    bool
    is_null() const
    {
        return width() == static_cast<field_type>(0) && height() == static_cast<field_type>(0);
    }

    bool
    is_empty() const
    {
        return left() > right() || top() > bottom();
    }

    bool
    is_valid() const
    {
        return !is_empty();
    }
    
    bool
    contains(point_type const & point) const
    {
        return
            point.x() >= left() && point.x() <= right() &&
            point.y() >= top() && point.y() <= bottom();
    }

    rect
    intersected(rect const & other) const
    {
        return
            rect
            (
                point_type
                (
                    std::max(left(), other.left()),
                    std::max(top(), other.top())
                ),
                point_type
                (
                    std::min(right(), other.right()),
                    std::min(bottom(), other.bottom())
                )
            );
    }

    rect
    translated(point_type const & point) const
    {
        return
            rect
            (
                point_type(left() + point.x(), top() + point.y()),
                point_type(right() + point.x(), bottom() + point.y())
            );
    }

    rect
    translated(field_type x, field_type y) const
    {
        return translated(point_type(x, y));
    }

    void
    set_left(field_type new_left)
    {
        top_left_.set_x(new_left);
    }

    void
    set_top(field_type new_top)
    {
        top_left_.set_y(new_top);
    }

    void
    set_right(field_type new_right)
    {
        bottom_right_.set_x(new_right);
    }

    void
    set_bottom(field_type new_bottom)
    {
        bottom_right_.set_y(new_bottom);
    }

    void
    translate(point_type const & point)
    {
        *this = translated(point);
    }

    void
    translate(field_type x, field_type y)
    {
        translate(point_type(x, y));
    }

private:
    point_type top_left_{};
    point_type bottom_right_{};
};

}
}

#endif
