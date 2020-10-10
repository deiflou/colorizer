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

#ifndef LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_COLORIZATION_CONTEXT_HPP
#define LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_COLORIZATION_CONTEXT_HPP

#include <vector>

#include "types.hpp"
#include "grid.hpp"

namespace lazybrush
{
namespace grid_of_quadtrees_colorizer
{

template <typename scribble_type_tp>
class colorization_context
{
public:
    using index_type = int;
    using scribble_index_type = short;
    using label_type = short;
    using intensity_type = unsigned char;

    using scribble_type = scribble_type_tp;

    enum
    {
        index_undefined = -1,
        index_implicit_surrounding = -2,

        scribble_index_undefined = -1,

        label_undefined = -1,
        label_implicit_surrounding = -2,

        intensity_min = 0,
        intensity_max = 255
    };

    struct reference_grid_cell_data_type
    {
        intensity_type intensity{intensity_max};
    };

    struct working_grid_cell_data_type
    {
        index_type index{index_undefined};
        scribble_index_type scribble_index{scribble_index_undefined};
        label_type preferred_label{label_undefined};
        intensity_type intensity{intensity_max};
    };

    using reference_grid_type = grid<reference_grid_cell_data_type>;
    using reference_grid_cell_type = typename reference_grid_type::cell_type;
    
    using working_grid_type = grid<working_grid_cell_data_type>;
    using working_grid_cell_type = typename working_grid_type::cell_type;

    using point_type = typename working_grid_type::point_type;
    using rect_type = typename working_grid_type::rect_type;

    struct input_point
    {
        point_type position;
        intensity_type intensity;
    };

    colorization_context() = default;
    colorization_context(colorization_context const &) = default;
    colorization_context(colorization_context &&) = default;
    colorization_context &
    operator=(colorization_context const &) = default;
    colorization_context &
    operator=(colorization_context &&) = default;

    colorization_context(rect_type const & rect, int cell_size, std::vector<input_point> const & points)
        : reference_grid_(rect, cell_size)
        , working_grid_(rect, cell_size)
    {
        for (input_point const & point : points)
        {
            reference_grid_cell_type * reference_cell = reference_grid_.add_point(point.position);
            if (reference_cell)
            {
                reference_cell->data().intensity = point.intensity;
            }
            working_grid_cell_type * working_cell = working_grid_.add_point(point.position);
            if (working_cell)
            {
                working_cell->data().intensity = point.intensity;
            }
        }
    }

    colorization_context(int x, int y, int width, int height, int cell_size, std::vector<input_point> const & points)
        : colorization_context(rect_type(x, y, width, height), cell_size, points)
    {}

    bool
    is_null() const
    {
        return working_grid_.is_null();
    }

    reference_grid_type const &
    reference_grid() const
    {
        return reference_grid_;
    }

    working_grid_type const &
    working_grid() const
    {
        return working_grid_;
    }

    std::vector<scribble_type> const &
    scribbles() const
    {
        return scribbles_;
    }

    scribble_type const &
    scribble(int index) const
    {
        return scribbles_[index];
    }

    void
    append_scribble(scribble_type const & scribble)
    {
        if (is_null())
        {
            return;
        }

        scribbles_.push_back(scribble);
        clear_and_add_scribbles_to_working_grid(scribble.rect());
    }

    void
    insert_scribble(int index, scribble_type const & scribble)
    {
        if (is_null())
        {
            return;
        }
        
        scribbles_.insert(scribbles_.begin() + index, scribble);
        clear_and_add_scribbles_to_working_grid(scribble.rect());
    }

    void
    remove_scribble(int index)
    {
        if (is_null())
        {
            return;
        }
        
        rect_type const scribble_rect = scribbles_[index].rect();
        scribbles_.erase(scribbles_.begin() + index);
        clear_and_add_scribbles_to_working_grid(scribble_rect);
    }

    void
    replace_scribble(int index, scribble_type const & scribble)
    {
        remove_scribble(index);
        insert_cribble(index, scribble);
    }

    void
    update_neighbors()
    {
        if (is_null())
        {
            return;
        }
        
        // We only need to set the top and left neighbors for each cell since the
        // conections to the bottom/right cells are made by the bottom/right cells,
        // so true is passed
        working_grid_.update_neighbors(true);
    }

private:
    reference_grid_type reference_grid_;
    working_grid_type working_grid_;
    std::vector<scribble_type> scribbles_;

    void
    clear_working_grid(rect_type const & rect)
    {
        // Clear
        working_grid_.clear(rect);

        // Add the original points from the reference grid
        reference_grid_.visit_leaves(
            rect,
            [this](reference_grid_cell_type * cell) -> bool
            {
                if (cell->is_bottom_most_leaf()) {
                    working_grid_cell_type * new_cell = working_grid_.add_point(cell->center());
                    new_cell->data().intensity = cell->data().intensity;
                }
                return true;
            }
        );
    }

    void
    add_scribbles_to_working_grid(rect_type const & rect)
    {
        // Adjust the rect to grid cell boundaries
        rect_type const adjusted_rect = working_grid_.adjusted_rect(rect);

        // Add the scribbles' data to the working grid from the last one
        // to the first one
        for (int i = scribbles_.size() - 1; i >= 0; --i)
        {
            scribble_type const & scribble = scribbles_[i];

            // Continue if he scribble is outside of the interest rect
            rect_type const intersected_rect = adjusted_rect.intersected(scribble.rect());
            if (!intersected_rect.is_valid())
            {
                continue;
            }

            // Add the scribble data to the working grid
            
            // Add the contour points
            for (point_type const & point : scribble.contour_points())
            {
                // Continue if the point is outside the interest rect
                if (!adjusted_rect.contains(point))
                {
                    continue;
                }
                // Get the leaf cell at this point
                working_grid_cell_type * leaf_cell = working_grid_.leaf_cell_at(point);
                // Continue if the leaf cell's preferred scribble index is greater
                // than the current scribble index. A scribble with higher priority
                // was added in this position
                if (leaf_cell->data().scribble_index > i)
                {
                    continue;
                }
                // Add the point
                working_grid_.add_point(point);
            }

            // Mark the cells' preferred scribble with the scribble index
            working_grid_.visit_leaves(
                adjusted_rect,
                [&scribble, i](working_grid_cell_type * cell) -> bool
                {
                    // Return if the cell's preferred scribble index is greater
                    // than the current scribble index. A scribble with higher priority
                    // was added in this position
                    if (cell->data().scribble_index > i)
                    {
                        return true;
                    }
                    // Set the preferred scribble index if this cell is inside the scribble
                    if (scribble.contains_point(cell->center()))
                    {
                        cell->data().scribble_index = i;
                        cell->data().preferred_label = scribble.label();
                    }

                    return true;
                }
            );
        }
    }

    void clear_and_add_scribbles_to_working_grid(rect_type const & rect)
    {
        clear_working_grid(rect);
        add_scribbles_to_working_grid(rect);
    }
};

}
}

#endif
