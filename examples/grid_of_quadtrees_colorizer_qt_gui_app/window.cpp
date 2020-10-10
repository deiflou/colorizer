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

#include <lazybrush/grid_of_quadtrees_colorizer/colorizer.hpp>

#include <cmath>
#include <vector>
#include <utility>

#include <QEvent>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QElapsedTimer>
#include <QTabletEvent>

window::window()
    : position_(0.0, 0.0)
    , scale_(1.0)
    , last_mouse_position_(0, 0)
    , is_dragging_(false)
    , last_painting_position_(0, 0)
    , is_painting_(false)
    , painting_device_(painting_device_type_none)
    , visualization_mode_(visualization_modes::visualization_mode_composed_image)
    , brush_size_(20)
    , selected_cell_(nullptr)
    , selected_color_index_(0)
    , selected_background_color_index_(-1)
    , use_implicit_scribble_(false)
    , show_scribbles_(true)
{
    setup_ui_();
}

window::~window()
{
}

bool
window::eventFilter(QObject *o, QEvent *e)
{
    if (o == widget_container_image_)
    {
        if (e->type() == QEvent::Paint)
        {
            QPainter painter(widget_container_image_);
            painter.fillRect(widget_container_image_->rect(), Qt::darkGray);

            QTransform tfr;
            tfr.translate(widget_container_image_->width() / 2.0, widget_container_image_->height() / 2.0);
            tfr.translate(position_.x(), position_.y());
            tfr.scale(scale_, scale_);

            painter.setTransform(tfr);

            QPointF image_position(-(preprocessed_image_.width() / 2.0), -(preprocessed_image_.height() / 2.0));

            painter.save();
            if (visualization_mode_ == visualization_mode_original_image)
            {
                painter.drawImage(image_position, original_image_);
                
            }
            else if (visualization_mode_ == visualization_mode_preprocessed_image)
            {
                painter.drawImage(image_position, preprocessed_image_);

            }
            else if (visualization_mode_ == visualization_mode_space_partitioning)
            {
                grid_type const & working_grid = colorization_context_.working_grid();
                working_grid.visit_leaves
                (
                    [&painter, &image_position, this](cell_type* cell) -> bool
                    {
                        int c = static_cast<int>(std::log2(cell->size())) * 300 / static_cast<int>(std::log2(cell_size));
                        if (cell->data().intensity == colorization_context_type::intensity_min)
                        {
                            painter.fillRect(QRectF(rect_type_to_QRect(cell->rect())).translated(image_position), qRgb(0, 0, 0));
                        }
                        else
                        {
                            painter.fillRect(QRectF(rect_type_to_QRect(cell->rect())).translated(image_position), QColor::fromHsv(c, 255, 255));
                        }
                        return true;
                    }
                );

            }
            else if (visualization_mode_ == visualization_mode_space_partitioning_scribbles)
            {
                grid_type const & working_grid = colorization_context_.working_grid();

                working_grid.visit_leaves
                (
                    [&painter, &image_position, this](cell_type* cell) -> bool
                    {
                        int h, s, v;
                        if (cell->data().scribble_index == colorization_context_type::scribble_index_undefined)
                        {
                            h = 0;
                            s = 0;
                        }
                        else
                        {
                            h = cell->data().scribble_index * 255 / scribbles_.size();
                            s = 255;
                        }
                        v = static_cast<int>(std::log2(cell->size())) * 127 / static_cast<int>(std::log2(cell_size)) + 128;
                        painter.fillRect(QRectF(rect_type_to_QRect(cell->rect())).translated(image_position), QColor::fromHsv(h, s, v));
                        return true;
                    }
                );

            }
            else if (visualization_mode_ == visualization_mode_space_partitioning_labels)
            {
                grid_type const & working_grid = colorization_context_.working_grid();

                working_grid.visit_leaves
                (
                    [&painter, &image_position, this](cell_type* cell) -> bool
                    {
                        QBrush b;
                        if (cell->data().preferred_label == colorization_context_type::label_undefined)
                        {
                            int v = static_cast<int>(std::log2(cell->size())) * 127 / static_cast<int>(std::log2(cell_size)) + 128;
                            b = QBrush(QColor::fromHsv(0, 0, v));
                        }
                        else
                        {
                            int const index = cell->data().preferred_label;
                            if (index == selected_background_color_index_)
                            {
                                b = QBrush(QColor(the_palette[index][0], the_palette[index][1], the_palette[index][2]), Qt::FDiagPattern);
                            }
                            else
                            {
                                if (index >= 0 && index < 128)
                                {
                                    b = QBrush(QColor(the_palette[index][0], the_palette[index][1], the_palette[index][2]));
                                }
                            }
                        }
                        painter.fillRect(QRectF(rect_type_to_QRect(cell->rect())).translated(image_position), b);
                        return true;
                    }
                );

            }
            else if (visualization_mode_ == visualization_mode_space_partitioning_neighbors)
            {
                grid_type const & working_grid = colorization_context_.working_grid();

                working_grid.visit_leaves
                (
                    [&painter, &image_position, this](cell_type* cell) -> bool
                    {
                        int v = static_cast<int>(std::log2(cell->size())) * 127 / static_cast<int>(std::log2(cell_size)) + 128;
                        if (cell == selected_cell_)
                        {
                            painter.fillRect(QRectF(rect_type_to_QRect(cell->rect())).translated(image_position), QColor::fromHsv(0, 255, v));

                            QPen pen(QColor(0, 0, 255));
                            pen.setCosmetic(true);
                            painter.setPen(pen);
                            for (cell_type const * cell2 : cell->top_leaf_neighbors())
                            {
                                painter.drawLine(point_type_to_QPoint(cell->center()) + image_position, point_type_to_QPoint(cell2->center()) + image_position);
                            }
                            for (cell_type const * cell2 : cell->left_leaf_neighbors())
                            {
                                painter.drawLine(point_type_to_QPoint(cell->center()) + image_position, point_type_to_QPoint(cell2->center()) + image_position);
                            }
                            for (cell_type const * cell2 : cell->bottom_leaf_neighbors())
                            {
                                painter.drawLine(point_type_to_QPoint(cell->center()) + image_position, point_type_to_QPoint(cell2->center()) + image_position);
                            }
                            for (cell_type const * cell2 : cell->right_leaf_neighbors())
                            {
                                painter.drawLine(point_type_to_QPoint(cell->center()) + image_position, point_type_to_QPoint(cell2->center()) + image_position);
                            }
                        }
                        else
                        {
                            painter.fillRect(QRectF(rect_type_to_QRect(cell->rect())).translated(image_position), QColor::fromHsv(0, 0, v));
                        }
                        return true;
                    }
                );

            }
            else if (visualization_mode_ == visualization_mode_labeling)
            {
                grid_type const & working_grid = colorization_context_.working_grid();

                qreal alpha = show_scribbles_ ? .25 : 1.;

                painter.setOpacity(alpha);
                painter.drawImage(image_position, labeling_image_);
                painter.setOpacity(255);

                if (show_scribbles_)
                {
                    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                    for (scribble const & scribble : scribbles_)
                    {
                        painter.drawImage(QRectF(scribble.rect()).translated(image_position), scribble.image());
                    }
                }

            }
            else if (visualization_mode_ == visualization_mode_composed_image)
            {
                grid_type const & working_grid = colorization_context_.working_grid();

                qreal alpha = show_scribbles_ ? .25 : 1.;
                
                painter.setOpacity(alpha);
                painter.drawImage(image_position, labeling_image_);
                painter.setOpacity(255);
                painter.setCompositionMode(QPainter::CompositionMode_Multiply);
                painter.drawImage(image_position, original_image_);

                if (show_scribbles_)
                {
                    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                    for (scribble const & scribble : scribbles_)
                    {
                        painter.drawImage(QRectF(scribble.rect()).translated(image_position), scribble.image());
                    }
                }
            }
            painter.restore();

            if (is_painting_)
            {
                scribble const & current_scribble = scribbles_.back();
                painter.drawImage
                (
                    -(preprocessed_image_.width() / 2.0) + current_scribble.rect().x(),
                    -(preprocessed_image_.height() / 2.0) + current_scribble.rect().y(),
                    current_scribble.image()
                );
            }

            return true;
            
        }
        else if (e->type() == QEvent::Wheel && !is_dragging_ && !is_painting_)
        {
            QWheelEvent * we = dynamic_cast<QWheelEvent*>(e);
            QPointF const widget_center(widget_container_image_->width() / 2, widget_container_image_->height() / 2);
            QPointF const mouse_position = we->position() - widget_center;
            QPointF const mouse_delta = QPointF(position_ - mouse_position) / scale_;

            if (we->angleDelta().y() > 0)
            {
                scale_ *= 2.0;
            }
            else
            {
                scale_ /= 2.0;
            }
            if (scale_ > 8.0)
            {
                scale_ = 8.0;
            }
            else if (scale_ < 0.125)
            {
                scale_ = 0.125;
            }
            position_ = mouse_position + mouse_delta * scale_;
            widget_container_image_->update();

            return true;

        }
        else if(e->type() == QEvent::MouseButtonPress && !is_dragging_ && !is_painting_)
        {
            QMouseEvent * me = dynamic_cast<QMouseEvent*>(e);
            if (me->button() == Qt::MidButton)
            {
                last_mouse_position_ = me->pos();
                is_dragging_ = true;
            }
            else if (me->button() == Qt::LeftButton)
            {
                const QPoint pen_position = transform_pen_position_(me->pos());

                scribbles_.push_back
                (
                    scribble
                    (
                        QColor
                        (
                            the_palette[selected_color_index_][0],
                            the_palette[selected_color_index_][1],
                            the_palette[selected_color_index_][2]
                        )
                    )
                );
                scribbles_.back().move_to(pen_position);
                scribbles_.back().line_to(pen_position, pressure_to_radius_(1.0));

                last_painting_position_ = me->pos();
                is_painting_ = true;
                painting_device_ = painting_device_type_mouse;

                widget_container_image_->update();
            }
            else if (me->button() == Qt::RightButton)
            {
                if (visualization_mode_ == visualization_mode_space_partitioning_neighbors)
                {
                    selected_cell_ = colorization_context_.working_grid().leaf_cell_at(QPoint_to_point_type(transform_pen_position_(me->pos())));
                    update();
                }
            }

            return true;

        }
        else if (e->type() == QEvent::MouseMove)
        {
            QMouseEvent * me = dynamic_cast<QMouseEvent*>(e);

            if (is_dragging_)
            {
                QPoint offset = me->pos() - last_mouse_position_;
                if (me->buttons() & Qt::MidButton)
                {
                    position_ += QPointF(offset.x(), offset.y());
                }
                last_mouse_position_ = me->pos();
                widget_container_image_->update();
            }
            else if (is_painting_ && painting_device_ == painting_device_type_mouse)
            {
                QPoint const painting_position = transform_pen_position_(me->pos());

                scribbles_.back().line_to(painting_position, pressure_to_radius_(1.0));
                last_painting_position_ = me->pos();

                widget_container_image_->update();
            }

            return true;

        }
        else if (e->type() == QEvent::MouseButtonRelease)
        {
            is_dragging_ = false;
            if (is_painting_ && painting_device_ == painting_device_type_mouse)
            {
                colorization_context_.append_scribble(colorizer_scribble(scribbles_.back(), selected_color_index_));

                is_painting_ = false;
                painting_device_ = painting_device_type_none;

                selected_cell_ = nullptr;

                colorize();

                widget_container_image_->update();
            }

            return true;

        }
        else if (e->type() == QEvent::TabletPress && !is_dragging_ && !is_painting_)
        {
            QTabletEvent * te = dynamic_cast<QTabletEvent*>(e);

            if (te->buttons() & Qt::LeftButton)
            {
                QPoint const pen_position = transform_pen_position_(te->pos());

                scribbles_.push_back
                (
                    scribble
                    (
                        QColor
                        (
                            the_palette[selected_color_index_][0],
                            the_palette[selected_color_index_][1],
                            the_palette[selected_color_index_][2]
                        )
                    )
                );
                scribbles_.back().move_to(pen_position);
                scribbles_.back().line_to(pen_position, pressure_to_radius_(te->pressure()));

                last_painting_position_ = te->pos();
                is_painting_ = true;
                painting_device_ = painting_device_type_pen;

                widget_container_image_->update();
            }

            return true;

        }
        else if (e->type() == QEvent::TabletMove && is_painting_ && painting_device_ == painting_device_type_pen)
        {
            QTabletEvent * te = dynamic_cast<QTabletEvent*>(e);
            
            QPoint const pen_position = transform_pen_position_(te->pos());

            scribbles_.back().line_to(pen_position, pressure_to_radius_(te->pressure()));
            last_painting_position_ = te->pos();

            widget_container_image_->update();

            return true;

        }
        else if (e->type() == QEvent::TabletRelease && is_painting_ && painting_device_ == painting_device_type_pen)
        {
            colorization_context_.append_scribble(colorizer_scribble(scribbles_.back(), selected_color_index_));

            is_painting_ = false;
            painting_device_ = painting_device_type_none;

            selected_cell_ = nullptr;

            colorize();

            widget_container_image_->update();

            return true;
        }
    }

    if (o == widget_palette_)
    {
        if (e->type() == QEvent::Paint)
        {
            QPainter painter(widget_palette_);
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 16; ++x) {
                    int const index = y * 16 + x;
                    painter.fillRect
                    (
                        x * palette_entry_size,
                        y * palette_entry_size,
                        palette_entry_size,
                        palette_entry_size,
                        qRgb
                        (
                            the_palette[index][0],
                            the_palette[index][1],
                            the_palette[index][2]
                        )
                    );

                    int const xx = x * palette_entry_size;
                    int const yy = y * palette_entry_size;
                    int const half_palette_entry_size = palette_entry_size / 2;

                    if (index == selected_background_color_index_)
                    {
                        QRect const symbol_rect(xx + 2, yy + 2, half_palette_entry_size - 2, half_palette_entry_size - 2);
                        painter.fillRect(symbol_rect.translated(1, 1), Qt::black);
                        painter.fillRect(symbol_rect, Qt::green);
                    }

                    if (index == selected_color_index_)
                    {
                        painter.setBrush(Qt::NoBrush);
                        painter.setPen(Qt::white);
                        painter.drawRect(xx + 1, yy + 1, palette_entry_size - 3, palette_entry_size - 3);
                        painter.setPen(Qt::black);
                        painter.drawRect(xx, yy, palette_entry_size - 1, palette_entry_size - 1);
                    }
                }
            }

            return true;

        } else if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent * me = dynamic_cast<QMouseEvent*>(e);

            int const x = me->pos().x() / palette_entry_size;
            int const y = me->pos().y() / palette_entry_size;
            int const index = y * 16 + x;

            if (index >= 0 && index < 128)
            {
                if (me->button() == Qt::LeftButton)
                {
                    selected_color_index_ = index;
                }
                else if (me->button() == Qt::RightButton)
                {
                    if (index == selected_background_color_index_)
                    {
                        selected_background_color_index_ = -1;
                    }
                    else
                    {
                        selected_background_color_index_ = index;
                    }
                    widget_container_image_->update();
                }
                widget_palette_->update();
            }

            return true;
        }
    }

    return false;
}

int
window::pressure_to_radius_(qreal pressure) const
{
    qreal const max_radius = brush_size_ / 2.0;
    return static_cast<int>(std::round(std::pow(pressure, 4) * max_radius + 0.5));
}

QPoint
window::transform_pen_position_(const QPoint &pen_position) const
{
    QPointF const widget_center(widget_container_image_->width() / 2, widget_container_image_->height() / 2);
    QPointF const image_position(-(original_image_.width() / 2.0), -(original_image_.height() / 2.0));
    QPointF const transformed_pen_positionF = QPointF(pen_position - (position_ + widget_center)) / scale_ - image_position;
    return QPoint(static_cast<int>(transformed_pen_positionF.x()), static_cast<int>(transformed_pen_positionF.y()));
}

void
window::colorize()
{
    using rect_type = typename colorization_context_type::rect_type;
    using label_type = typename colorization_context_type::label_type;
    std::vector<std::pair<rect_type, label_type>> labeling =
        lazybrush::grid_of_quadtrees_colorizer::colorize(colorization_context_, use_implicit_scribble_);

    labeling_image_.fill(0);
    QPainter painter(&labeling_image_);

    grid_type const & working_grid = colorization_context_.working_grid();
    for (auto const & node : labeling)
    {
        if
        (
            node.second != colorization_context_type::label_undefined &&
            node.second != colorization_context_type::label_implicit_surrounding
        )
        {
            int const index = node.second;
            if (index != selected_background_color_index_ && index >= 0 && index < 128)
            {
                QColor c =
                    QColor
                    (
                        the_palette[index][0],
                        the_palette[index][1],
                        the_palette[index][2]
                    );
                painter.fillRect(rect_type_to_QRect(node.first), c);
            }
        }
    }
}

QPoint
window::point_type_to_QPoint(point_type const & point)
{
    return QPoint(point.x(), point.y());
}

QRect
window::rect_type_to_QRect(rect_type const & rect)
{
    return QRect(rect.x(), rect.y(), rect.width(), rect.height());
}

point_type
window::QPoint_to_point_type(QPoint const & point)
{
    return point_type(point.x(), point.y());
}

rect_type
window::QRect_to_rect_type(QRect const & rect)
{
    return rect_type(rect.x(), rect.y(), rect.width(), rect.height());
}
