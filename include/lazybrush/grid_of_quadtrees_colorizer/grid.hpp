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

#ifndef LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_GRID_HPP
#define LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_GRID_HPP

#include <vector>
#include <stack>

#include "types.hpp"
#include "quadtree.hpp"

namespace lazybrush
{
namespace grid_of_quadtrees_colorizer
{

template <typename data_type_tp>
class grid
{
public:
    using data_type = data_type_tp;
    using cell_type = quadtree_node<data_type>;
    using point_type = typename cell_type::point_type;
    using rect_type = typename cell_type::rect_type;

    grid() = default;
    grid(grid const &) = default;
    grid(grid &&) = default;
    grid &
    operator=(grid const &) = default;
    grid &
    operator=(grid &&) = default;

    grid(rect_type const & rect, int cell_size)
    {
        int width_in_cells = rect.width() / cell_size;
        if (width_in_cells * cell_size != rect.width())
        {
            ++width_in_cells;
        }
        int height_in_cells = rect.height() / cell_size;
        if (height_in_cells * cell_size != rect.height())
        {
            ++height_in_cells;
        }

        rect_ = rect_type(rect.x(), rect.y(), width_in_cells * cell_size, height_in_cells * cell_size);
        
        // Create top level cells
        cells_ = std::vector<cell_type *>(width_in_cells * height_in_cells);
        for (int y = 0; y < height_in_cells; ++y)
        {
            for (int x = 0; x < width_in_cells; ++x)
            {
                int const index = y * width_in_cells + x;
                cells_[index] = new cell_type;
                cells_[index]->set_rect(x * cell_size + rect_.x(),
                                        y * cell_size + rect_.y(),
                                        cell_size,
                                        cell_size);
            }
        }
        
        width_in_cells_ = width_in_cells;
        height_in_cells_ = height_in_cells;
        cell_size_ = cell_size;
    }

    grid(int x, int y, int width, int height, int cell_ize)
        : grid(rect_type(x, y, width, height), cell_ize)
    {}

    ~grid()
    {
        for (cell_type * cell : cells_)
        {
            delete cell;
        }
    }

    grid
    clone() const
    {
        grid new_grid(rect_, cell_size_);

        // Subdivide (this method ensures that the cells siblings are computed)
        visit_leaves(
            [&new_grid](cell_type * cell)
            {
                if (cell->size() == 1)
                {
                    new_grid.add_point(cell->rect().top_left());
                }
            }
        );

        // Now that the grids have the same structure, copy the data
        for (int i = 0; i < cells_.size(); ++i)
        {
            std::stack<cell_type *> src_stack;
            std::stack<cell_type *> dst_stack;
            src_stack.push(cells_[i]);
            dst_stack.push(new_grid.cells_[i]);

            while (!src_stack.empty()) {
                cell_type *src_cell = src_stack.pop();
                cell_type *dst_cell = dst_stack.pop();

                dst_cell->set_data(src_cell->data());

                if (src_cell->is_subdivided()) {
                    src_stack.push(src_cell->bottom_right_child());
                    src_stack.push(src_cell->bottom_left_child());
                    src_stack.push(src_cell->top_right_child());
                    src_stack.push(src_cell->top_left_child());
                    dst_stack.push(dst_cell->bottom_right_child());
                    dst_stack.push(dst_cell->bottom_left_child());
                    dst_stack.push(dst_cell->top_right_child());
                    dst_stack.push(dst_cell->top_left_child());
                }
            }
        }

        return new_grid;
    }

    bool
    is_null() const
    {
        return cells_.empty();
    }

    // Deletes the cells of the trees that intersect with the given rect
    // while keeping the top level ones
    void
    clear(rect_type const & rect)
    {
        rect_type cells_rect = rect_to_cells(rect);
        if (!cells_rect.is_valid())
        {
            return;
        }

        for (int y = cells_rect.top(); y <= cells_rect.bottom(); ++y)
        {
            for (int x = cells_rect.left(); x <= cells_rect.right(); ++x)
            {
                const int index = y * width_in_cells_ + x;
                cell_type * cell = cells_[index];
                clear_cell(cell);
            }
        }
    }

    // Deletes all the cells of the trees while keeping the top level ones
    void
    clear()
    {
        if (is_null())
        {
            return;
        }
        for (cell_type * cell : cells_)
        {
            clear_cell(cell);
        }
    }

    cell_type *
    top_level_cell_at(point_type const & point) const
    {
        if (is_null() || !rect_.contains(point))
        {
            return nullptr;
        }

        point_type p(point.x() - rect_.left(), point.y() - rect_.top());
        int x = p.x() / cell_size_;
        int y = p.y() / cell_size_;
        int index = y * width_in_cells_ + x;
        return cells_[index];
    }

    cell_type *
    leaf_cell_at(point_type const & point) const
    {
        cell_type * cell = top_level_cell_at(point);
        if (cell)
        {
            return cell->leaf_at(point);
        }
        return nullptr;
    }

    cell_type *
    add_point(point_type const & point)
    {
        cell_type * cell = top_level_cell_at(point);
        if (cell)
        {
            return cell->add_point(point);
        }
        return nullptr;
    }

    cell_type *
    add_point(int x, int y)
    {
        return add_point(point_type(x, y));
    }

    // traverse the cells in preorder
    template <typename visitor_type_tp>
    void
    visit(visitor_type_tp visitor) const
    {
        if (is_null())
        {
            return;
        }

        for (cell_type * top_level_cell : cells_)
        {
            std::stack<cell_type *> stack;
            stack.push(top_level_cell);

            while (!stack.empty())
            {
                cell_type * cell = stack.top();
                stack.pop();
                if (!visitor(cell))
                {
                    return;
                };
                if (cell->is_subdivided())
                {
                    stack.push(cell->bottom_right_child());
                    stack.push(cell->bottom_left_child());
                    stack.push(cell->top_right_child());
                    stack.push(cell->top_left_child());
                }
            }
        }
    }

    template <typename visitor_type_tp>
    void
    visit_leaves(visitor_type_tp visitor) const
    {
        visit(
            [visitor](cell_type * cell)
            {
                if (cell->is_leaf())
                {
                    return(visitor(cell));
                }
                return true;
            }
        );
    }

    template <typename visitor_type_tp>
    void
    visit(rect_type const & rect, visitor_type_tp visitor) const
    {
        if (is_null())
        {
            return;
        }
        
        rect_type cells_rect = rect_to_cells(rect);
        if (!cells_rect.is_valid())
        {
            return;
        }

        for (int y = cells_rect.top(); y <= cells_rect.bottom(); ++y)
        {
            for (int x = cells_rect.left(); x <= cells_rect.right(); ++x)
            {
                int const index = y * width_in_cells_ + x;

                std::stack<cell_type *> stack;
                stack.push(cells_[index]);

                while (!stack.empty())
                {
                    cell_type * cell = stack.top();
                    stack.pop();
                    if (!visitor(cell))
                    {
                        return;
                    };
                    if (cell->is_subdivided())
                    {
                        stack.push(cell->bottom_right_child());
                        stack.push(cell->bottom_left_child());
                        stack.push(cell->top_right_child());
                        stack.push(cell->top_left_child());
                    }
                }
            }
        }
    }

    template <typename visitor_type_tp>
    void
    visit_leaves(rect_type const & rect, visitor_type_tp visitor) const
    {
        visit(
            rect,
            [visitor](cell_type * cell)
            {
                if (cell->is_leaf())
                {
                    return visitor(cell);
                }
                return true;
            }
        );
    }

    // Visit all border leaf cells in clockwise direction
    // starting from the top left one
    template <typename visitor_type_tp>
    void
    visit_border_leaves(visitor_type_tp visitor) const
    {
        int index;

        // Top cells
        for (int x = 0; x < width_in_cells_; ++x) {
            cell_type * top_level_cell = cells_[x];
            for (cell_type * cell : top_level_cell->top_most_leaves())
            {
                if (!visitor(cell))
                {
                    return;
                }
            }
        }

        // Right cells (we must avoid processing again the top right corner cell)
        index = width_in_cells_ - 1;
        {
            cell_type * top_level_cell = cells_[index];
            std::vector<cell_type *> border_leaf_cells = top_level_cell->right_most_leaves();
            for (int i = 1; i < border_leaf_cells.size(); ++i)
            {
                cell_type * cell = border_leaf_cells[i];
                if (!visitor(cell))
                {
                    return;
                }
            }
            index += width_in_cells_;
        }
        for (int y = 1; y < height_in_cells_; ++y, index += width_in_cells_)
        {
            cell_type * top_level_cell = cells_[index];
            for (cell_type * cell : top_level_cell->right_most_leaves())
            {
                if (!visitor(cell)) {
                    return;
                }
            }
        }

        // Bottom cells (we must avoid processing again the bottom right corner cell)
        index = (height_in_cells_ - 1) * width_in_cells_ + width_in_cells_ - 1;
        {
            cell_type * top_level_cell = cells_[index];
            std::vector<cell_type *> border_leaf_cells = top_level_cell->bottom_most_leaves();
            for (int i = border_leaf_cells.size() - 2; i >= 0; --i) {
                cell_type * cell = border_leaf_cells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
            --index;
        }
        for (int x = width_in_cells_ - 2; x >= 0; --x, --index) {
            cell_type * top_level_cell = cells_[index];
            std::vector<cell_type *> border_leaf_cells = top_level_cell->bottom_most_leaves();
            for (int i = border_leaf_cells.size() - 1; i >= 0; --i) {
                cell_type * cell = border_leaf_cells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
        }

        // Left cells (we must avoid processing again the bottom left corner cell)
        index = (height_in_cells_ - 1) * width_in_cells_;
        {
            cell_type * top_level_cell = cells_[index];
            std::vector<cell_type *> border_leaf_cells = top_level_cell->left_most_leaves();
            for (int i = border_leaf_cells.size() - 2; i >= 0; --i) {
                cell_type * cell = border_leaf_cells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
            index -= width_in_cells_;
        }
        for (int y = height_in_cells_ - 2; y >= 0; --y, index -= width_in_cells_) {
            cell_type * top_level_cell = cells_[index];
            std::vector<cell_type *> border_leaf_cells = top_level_cell->left_most_leaves();
            for (int i = border_leaf_cells.size() - 1; i >= 0; --i) {
                cell_type * cell = border_leaf_cells[i];
                if (!visitor(cell)) {
                    return;
                }
            }
        }
    }

    int
    cell_size() const
    {
        return cell_size_;
    }
    
    rect_type const &
    rect() const
    {
        return rect_;
    }
    
    rect_type
    adjusted_rect(rect_type const & rect) const
    {
        if (is_null()) {
            return rect_type();
        }

        rect_type adjusted_rect = rect_.intersected(rect);
        if (!adjusted_rect.is_valid()) {
            return rect_type();
        }
        adjusted_rect.translate(-rect_.left(), -rect_.top());
        adjusted_rect.set_left(adjusted_rect.left() / cell_size_ * cell_size_);
        adjusted_rect.set_top(adjusted_rect.top() / cell_size_ * cell_size_);
        adjusted_rect.set_right(adjusted_rect.right() / cell_size_ * cell_size_ + cell_size_ - 1);
        adjusted_rect.set_bottom(adjusted_rect.bottom() / cell_size_ * cell_size_ + cell_size_ - 1);
        return adjusted_rect.translated(rect_.top_left());
    }

    void
    update_neighbors(bool find_top_left_neighbors_only = false)
    {
        if (is_null())
        {
            return;
        }

        for (int y = 0; y < height_in_cells_; ++y)
        {
            for (int x = 0; x < width_in_cells_; ++x)
            {
                std::stack<cell_type *> stack;
                const int index = y * width_in_cells_ + x;
                stack.push(cells_[index]);

                while (!stack.empty())
                {
                    cell_type * cell = stack.top();
                    stack.pop();
                    if (cell->is_leaf())
                    {
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

                        cell_type * side_cell;
                        bool is_same_level;
                        
                        is_same_level = find_top_cell(cell, x, y, &side_cell);
                        if (side_cell)
                        {
                            if (is_same_level)
                            {
                                cell->set_top_leaf_neighbors(side_cell->bottom_most_leaves());
                            }
                            else
                            {
                                cell->set_top_leaf_neighbors(std::vector<cell_type *>(1, side_cell));
                            }
                        }

                        is_same_level = find_left_cell(cell, x, y, &side_cell);
                        if (side_cell)
                        {
                            if (is_same_level)
                            {
                                cell->set_left_leaf_neighbors(side_cell->right_most_leaves());
                            }
                            else
                            {
                                cell->set_left_leaf_neighbors(std::vector<cell_type *>(1, side_cell));
                            }
                        }

                        if (!find_top_left_neighbors_only) {
                            is_same_level = find_bottom_cell(cell, x, y, &side_cell);
                            if (side_cell) {
                                if (is_same_level) {
                                    cell->set_bottom_leaf_neighbors(side_cell->top_most_leaves());
                                }
                                else
                                {
                                    cell->set_bottom_leaf_neighbors(std::vector<cell_type *>(1, side_cell));
                                }
                            }

                            is_same_level = find_right_cell(cell, x, y, &side_cell);
                            if (side_cell) {
                                if (is_same_level) {
                                    cell->set_right_leaf_neighbors(side_cell->left_most_leaves());
                                }
                                else
                                {
                                    cell->set_right_leaf_neighbors(std::vector<cell_type *>(1, side_cell));
                                }
                            }
                        }

                    }
                    else
                    {
                        stack.push(cell->bottom_right_child());
                        stack.push(cell->bottom_left_child());
                        stack.push(cell->top_right_child());
                        stack.push(cell->top_left_child());
                    }
                }
            }
        }
    }

private:
    std::vector<cell_type *> cells_;
    int width_in_cells_, height_in_cells_, cell_size_;
    rect_type rect_;

    rect_type rect_to_cells(rect_type const & rect) const
    {
        if (is_null())
        {
            return rect_type();
        }

        rect_type adjusted_rect = rect_.intersected(rect);
        if (!adjusted_rect.is_valid())
        {
            return rect_type();
        }
        adjusted_rect.translate(-rect_.left(), -rect_.top());
        adjusted_rect.set_left(adjusted_rect.left() / cell_size_);
        adjusted_rect.set_top(adjusted_rect.top() / cell_size_);
        adjusted_rect.set_right(adjusted_rect.right() / cell_size_);
        adjusted_rect.set_bottom(adjusted_rect.bottom() / cell_size_);
        return adjusted_rect;
    }

    void clear_cell(cell_type * cell)
    {
        delete cell->top_left_child();
        delete cell->top_right_child();
        delete cell->bottom_left_child();
        delete cell->bottom_right_child();
        cell->set_top_left_child(nullptr);
        cell->set_top_right_child(nullptr);
        cell->set_bottom_left_child(nullptr);
        cell->set_bottom_right_child(nullptr);
        cell->set_data(data_type());
    }

    // "topCell" is set to the cell at the top of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no top cell ("cell" is a top border cell).
    // true is returned if the top cell is at the same level.
    // false is returned if the top cell is in a level above or null.
    bool find_top_cell(cell_type * cell, int cell_x, int cell_y, cell_type ** top_cell) const
    {
        cell_type * parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell or it hs no sibling
        if (!parent)
        {
            if (cell_y == 0) {
                // "cell" is a top level border cell
                *top_cell = nullptr;
                return false;
            }
            *top_cell = cells_[(cell_y - 1) * width_in_cells_ + cell_x];
            return true;
        }

        // Trivial cases. "cell" is a child in the bottom of the parent cell
        // so the top cell to "cell" is a child in the top of the same parent
        {
            if (cell == parent->bottom_left_child()) {
                *top_cell = parent->top_left_child();
                return true;
            }

            if (cell == parent->bottom_right_child()) {
                *top_cell = parent->top_right_child();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the top of the parent cell
        // so the top cell to "cell" doesn't have the same parent.
        // Recursion is used to find the top cell which can be in the same level,
        // in a level above (if the top cell is not subdivided enough),
        // or null (if this cell is in the top border of the grid)
        {
            if (cell == parent->top_left_child())
            {
                cell_type * parent_top_cell;
                bool parent_top_cell_is_in_same_level_as_parent =
                    find_top_cell(parent, cell_x, cell_y, &parent_top_cell);
                if (!parent_top_cell)
                {
                    // Border cell case
                    *top_cell = nullptr;
                    return false;
                }
                else
                {
                    if (parent_top_cell_is_in_same_level_as_parent)
                    {
                        if (!parent_top_cell->is_subdivided())
                        {
                            // Level above case
                            *top_cell = parent_top_cell;
                            return false;
                        }
                        else
                        {
                            // Same level case
                            *top_cell = parent_top_cell->bottom_left_child();
                            return true;
                        }
                    }
                    else
                    {
                        // Level above case
                        // The top cell is already above the parent cell
                        *top_cell = parent_top_cell;
                        return false;
                    }
                }
            }

            /* if (cell == parent->top_right_child()) */
            {
                cell_type * parent_top_cell;
                bool parent_top_cell_is_in_same_level_as_parent =
                    find_top_cell(parent, cell_x, cell_y, &parent_top_cell);
                if (!parent_top_cell)
                {
                    // Border cell case
                    *top_cell = nullptr;
                    return false;
                }
                else
                {
                    if (parent_top_cell_is_in_same_level_as_parent)
                    {
                        if (!parent_top_cell->is_subdivided())
                        {
                            // Level above case
                            *top_cell = parent_top_cell;
                            return false;
                        }
                        else
                        {
                            // Same level case
                            *top_cell = parent_top_cell->bottom_right_child();
                            return true;
                        }
                    }
                    else
                    {
                        // Level above case
                        // The top cell is already above the parent cell
                        *top_cell = parent_top_cell;
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
    bool find_left_cell(cell_type * cell, int cell_x, int cell_y, cell_type ** left_cell) const
    {
        cell_type *parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell
        if (!parent)
        {
            if (cell_x == 0)
            {
                *left_cell = nullptr;
                return false;
            }
            *left_cell = cells_[cell_y * width_in_cells_ + cell_x - 1];
            return true;
        }

        // Trivial cases. "cell" is a child in the right of the parent cell
        // so the left cell to "cell" is a child in the left of the same parent
        {
            if (cell == parent->top_right_child())
            {
                *left_cell = parent->top_left_child();
                return true;
            }

            if (cell == parent->bottom_right_child())
            {
                *left_cell = parent->bottom_left_child();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the left of the parent cell
        // so the left cell to "cell" doesn't have the same parent.
        // Recursion is used to find the left cell which can be in the same level,
        // in a level above (if the left cell is not subdivided enough),
        // or null (if this cell is in the left border of the grid)
        {
            if (cell == parent->top_left_child())
            {
                cell_type * parent_left_cell;
                bool parent_left_cell_is_in_same_level_as_parent =
                    find_left_cell(parent, cell_x, cell_y, &parent_left_cell);
                if (!parent_left_cell)
                {
                    // Border cell case
                    *left_cell = nullptr;
                    return false;
                } else {
                    if (parent_left_cell_is_in_same_level_as_parent)
                    {
                        if (!parent_left_cell->is_subdivided())
                        {
                            // Level above case
                            *left_cell = parent_left_cell;
                            return false;
                        }
                        else
                        {
                            // Same level case
                            *left_cell = parent_left_cell->top_right_child();
                            return true;
                        }
                    }
                    else
                    {
                        // Level above case
                        // The left cell is already above the parent cell
                        *left_cell = parent_left_cell;
                        return false;
                    }
                }
            }

            /* if (currentCell == parent->bottom_left_child()) */
            {
                cell_type * parent_left_cell;
                bool parent_left_cell_is_in_same_level_as_parent =
                    find_left_cell(parent, cell_x, cell_y, &parent_left_cell);
                if (!parent_left_cell)
                {
                    // Border cell case
                    *left_cell = nullptr;
                    return false;
                }
                else
                {
                    if (parent_left_cell_is_in_same_level_as_parent)
                    {
                        if (!parent_left_cell->is_subdivided())
                        {
                            // Level above case
                            *left_cell = parent_left_cell;
                            return false;
                        }
                        else
                        {
                            // Same level case
                            *left_cell = parent_left_cell->bottom_right_child();
                            return true;
                        }
                    }
                    else
                    {
                        // Level above case
                        // The left cell is already above the parent cell
                        *left_cell = parent_left_cell;
                        return false;
                    }
                }
            }
        }
    }

    // "bottom_cell" is set to the cell at the bottom of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no bottom cell ("cell" is a bottom border cell).
    // true is returned if the bottom cell is at the same level.
    // false is returned if the bottom cell is in a level above or null.
    bool find_bottom_cell(cell_type * cell, int cell_x, int cell_y, cell_type ** bottom_cell) const
    {
        cell_type * parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell
        if (!parent)
        {
            if (cell_y == height_in_cells_ - 1)
            {
                // "cell" is a bottom level border cell
                *bottom_cell = nullptr;
                return false;
            }
            *bottom_cell = cells_[(cell_y + 1) * width_in_cells_ + cell_x];
            return true;
        }

        // Trivial cases. "cell" is a child in the top of the parent cell
        // so the bottom cell to "cell" is a child in the bottom of the same parent
        {
            if (cell == parent->top_left_child())
            {
                *bottom_cell = parent->bottom_left_child();
                return true;
            }

            if (cell == parent->top_right_child())
            {
                *bottom_cell = parent->bottom_right_child();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the bottom of the parent cell
        // so the bottom cell to "cell" doesn't have the same parent.
        // Recursion is used to find the bottom cell which can be in the same level,
        // in a level above (if the bottom cell is not subdivided enough),
        // or null (if this cell is in the bottom border of the grid)
        {
            if (cell == parent->bottom_left_child())
            {
                cell_type * parent_bottom_cell;
                bool parent_bottom_cell_is_in_same_level_as_parent =
                    find_bottom_cell(parent, cell_x, cell_y, &parent_bottom_cell);
                if (!parent_bottom_cell)
                {
                    // Border cell case
                    *bottom_cell = nullptr;
                    return false;
                }
                else
                {
                    if (parent_bottom_cell_is_in_same_level_as_parent)
                    {
                        if (!parent_bottom_cell->is_subdivided())
                        {
                            // Level above case
                            *bottom_cell = parent_bottom_cell;
                            return false;
                        }
                        else
                        {
                            // Same level case
                            *bottom_cell = parent_bottom_cell->top_left_child();
                            return true;
                        }
                    }
                    else
                    {
                        // Level above case
                        // The bottom cell is already above the parent cell
                        *bottom_cell = parent_bottom_cell;
                        return false;
                    }
                }
            }

            /* if (cell == parent->bottom_right_child()) */
            {
                cell_type * parent_bottom_cell;
                bool parent_bottom_cell_is_in_same_level_as_parent =
                    find_bottom_cell(parent, cell_x, cell_y, &parent_bottom_cell);
                if (!parent_bottom_cell)
                {
                    // Border cell case
                    *bottom_cell = nullptr;
                    return false;
                }
                else
                {
                    if (parent_bottom_cell_is_in_same_level_as_parent)
                    {
                        if (!parent_bottom_cell->is_subdivided())
                        {
                            // Level above case
                            *bottom_cell = parent_bottom_cell;
                            return false;
                        }
                        else
                        {
                            // Same level case
                            *bottom_cell = parent_bottom_cell->top_right_child();
                            return true;
                        }
                    }
                    else
                    {
                        // Level above case
                        // The bottom cell is already above the parent cell
                        *bottom_cell = parent_bottom_cell;
                        return false;
                    }
                }
            }
        }
    }

    // "right_cell" is set to the cell at the right of "cell" that is
    // at the same level or above in the tree, or to nullptr if
    // there is no right cell ("cell" is a right border cell).
    // true is returned if the right cell is at the same level.
    // false is returned if the right cell is in a level above or null.
    bool find_right_cell(cell_type * cell, int cell_x, int cell_y, cell_type ** right_cell)
    {
        cell_type * parent = cell->parent();

        // If the cell doesn't have a parent it is a top level cell,
        // so the sibling is another top level cell
        if (!parent)
        {
            if (cell_x == width_in_cells_ - 1) {
                *right_cell = nullptr;
                return false;
            }
            *right_cell = cells_[cell_y * width_in_cells_ + cell_x + 1];
            return true;
        }

        // Trivial cases. "cell" is a child in the left of the parent cell
        // so the right cell to "cell" is a child in the right of the same parent
        {
            if (cell == parent->top_left_child()) {
                *right_cell = parent->top_right_child();
                return true;
            }

            if (cell == parent->bottom_left_child()) {
                *right_cell = parent->bottom_right_child();
                return true;
            }
        }

        // Non trivial cases. "cell" is a child in the right of the parent cell
        // so the right cell to "cell" doesn't have the same parent.
        // Recursion is used to find the right cell which can be in the same level,
        // in a level above (if the right cell is not subdivided enough),
        // or null (if this cell is in the right border of the grid)
        {
            if (cell == parent->top_right_child()) {
                cell_type * parent_right_cell;
                bool parent_right_cell_is_in_same_level_as_parent =
                    find_right_cell(parent, cell_x, cell_y, &parent_right_cell);
                if (!parent_right_cell) {
                    // Border cell case
                    *right_cell = nullptr;
                    return false;
                } else {
                    if (parent_right_cell_is_in_same_level_as_parent) {
                        if (!parent_right_cell->is_subdivided()) {
                            // Level above case
                            *right_cell = parent_right_cell;
                            return false;
                        } else {
                            // Same level case
                            *right_cell = parent_right_cell->top_left_child();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The right cell is already above the parent cell
                        *right_cell = parent_right_cell;
                        return false;
                    }
                }
            }

            /* if (cell == parent->bottom_right_child()) */
            {
                cell_type * parent_right_cell;
                bool parent_right_cell_is_in_same_level_as_parent =
                    find_right_cell(parent, cell_x, cell_y, &parent_right_cell);
                if (!parent_right_cell) {
                    // Border cell case
                    *right_cell = nullptr;
                    return false;
                } else {
                    if (parent_right_cell_is_in_same_level_as_parent) {
                        if (!parent_right_cell->is_subdivided()) {
                            // Level above case
                            *right_cell = parent_right_cell;
                            return false;
                        } else {
                            // Same level case
                            *right_cell = parent_right_cell->bottom_left_child();
                            return true;
                        }
                    } else {
                        // Level above case
                        // The right cell is already above the parent cell
                        *right_cell = parent_right_cell;
                        return false;
                    }
                }
            }
        }
    }
};

}
}

#endif
