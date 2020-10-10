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

#include "scribble.hpp"

#include <cmath>
#include <QPainter>

scribble::scribble(QColor const & color) :
    color_(color)
{}

std::vector<scribble::point_type>
scribble::contour_points() const
{
    if (!cache_is_valid_)
    {
        cached_contour_points_.clear();

        if (!image_.isNull())
        {
            for (int y = 1; y < image_.height() - 1; ++y)
            {
                const quint8 * pixel = image_.scanLine(y);
                for (int x = 0; x < image_.width() - 1; ++x, pixel += 4)
                {
                    if (pixel[3] == 0)
                    {
                        continue;
                    }

                    if ((pixel - image_.bytesPerLine() - 4)[3] == 0 ||
                        (pixel - image_.bytesPerLine() - 0)[3] == 0 ||
                        (pixel - image_.bytesPerLine() + 4)[3] == 0 ||
                        (pixel - 4)[3] == 0 ||
                        (pixel + 4)[3] == 0 ||
                        (pixel + image_.bytesPerLine() - 4)[3] == 0 ||
                        (pixel + image_.bytesPerLine() - 0)[3] == 0 ||
                        (pixel + image_.bytesPerLine() + 4)[3] == 0)
                    {
                        cached_contour_points_.push_back(point_type(x + image_rect_.x(), y + image_rect_.y()));
                    }
                } 
            }
        }

        cache_is_valid_ = true;
    }
    return cached_contour_points_;
}

bool
scribble::contains_point(QPoint const & point) const
{
    if (image_.isNull() || !image_rect_.contains(point)) {
        return false;
    }

    const int x = point.x() - image_rect_.x();
    const int y = point.y() - image_rect_.y();
    
    return (image_.constBits() + y * image_.bytesPerLine() + x * 4)[3] == 255;
}

void
scribble::move_to(QPoint const & point, int radius)
{
    position_ = point;
    radius_ = radius;
}

void
scribble::line_to(QPoint const & point, int radius)
{                       
    resize_image_to_contain(point, radius);

    constexpr double spacing = 1.0;
    const double dx = point.x() - position_.x();
    const double dy = point.y() - position_.y();
    const double dradius = radius - radius_;
    const double dist = std::sqrt(dx * dx + dy * dy);
    const double inc = spacing / dist;
    const double incx = dx * inc;
    const double incy = dy * inc;
    const double incradius = dradius * inc;

    QPainter painter(&image_);
    double x = position_.x();
    double y = position_.y();
    double r = radius_;
    double t = 0.0;

    painter.setPen(Qt::NoPen);
    painter.setBrush(color_);
    
    while (t < 1.0)
    {
        int ri = static_cast<int>(std::round(r));
        painter.drawEllipse
        (
            static_cast<int>(std::round(x)) - image_rect_.x() - ri,
            static_cast<int>(std::round(y)) - image_rect_.y() - ri,
            ri * 2 + 1,
            ri * 2 + 1
        );

        t += inc;
        x += incx;
        y += incy;
        r += incradius;
    }

    painter.drawEllipse
    (
        point.x() - image_rect_.x() - radius,
        point.y() - image_rect_.y() - radius,
        radius * 2 + 1,
        radius * 2 + 1
    );

    move_to(point, radius);

    cache_is_valid_ = false;
}

const QRect &
scribble::rect() const
{
    return image_rect_;
}

const QImage &
scribble::image() const
{
    return image_;
}

void
scribble::resize_image_to_contain(QPoint const & point, int radius)
{
    QRect start_point_rect(position_.x() - radius_,
                           position_.y() - radius_,
                           radius_ * 2 + 1,
                           radius_ * 2 + 1);
    QRect end_point_rect(point.x() - radius,
                         point.y() - radius,
                         radius * 2 + 1,
                         radius * 2 + 1);
    QRect rect = start_point_rect.united(end_point_rect).adjusted(-1, -1, 1, 1).united(image_rect_);

    QImage img(rect.width(), rect.height(), QImage::Format_ARGB32);
    img.fill(0);
    QPainter painter(&img);
    painter.drawImage(image_rect_.topLeft() - rect.topLeft(), image_);

    image_ = img;
    image_rect_ = rect;
}

colorizer_scribble::colorizer_scribble(scribble const & scribble,
                                       colorizer_scribble::label_type label) :
    scribble_(scribble),
    label_(label)
{}

std::vector<colorizer_scribble::point_type>
colorizer_scribble::contour_points() const
{
    return scribble_.contour_points();
}

bool
colorizer_scribble::contains_point(colorizer_scribble::point_type const & point) const
{
    return scribble_.contains_point(QPoint(point.x(), point.y()));
}

colorizer_scribble::rect_type
colorizer_scribble::rect() const
{
    return rect_type(scribble_.rect().x(), scribble_.rect().y(), scribble_.rect().width(), scribble_.rect().height());
}

colorizer_scribble::label_type
colorizer_scribble::label() const
{
    return label_;
}
