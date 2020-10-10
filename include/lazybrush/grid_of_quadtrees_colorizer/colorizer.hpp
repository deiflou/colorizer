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

#ifndef LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_COLORIZER_HPP
#define LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_COLORIZER_HPP

#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>
#include <unordered_map>

#include "types.hpp"
#include "colorization_context.hpp"
#include "../lazybrush.hpp"

namespace lazybrush
{

namespace grid_of_quadtrees_colorizer
{
namespace detail
{

template <typename context_type_tp>
struct leaf_type
{
    using context_type = context_type_tp;

    typename context_type::label_type preferred_label{context_type::label_undefined};
    typename context_type::intensity_type intensity{context_type::intensity_max};
    int area{0};
    bool is_border_leaf{false};
    int surounding_border_size{0};
    // The pair have the index of the neighbor in the vector
    // in the first field, and the border size between the cells
    // in the second field
    std::vector<std::pair<int, int>> connections;
};

}
}

template <typename context_type_tp>
struct node_traits<grid_of_quadtrees_colorizer::detail::leaf_type<context_type_tp>>
{
    using context_type = context_type_tp;
    using leaf_type = grid_of_quadtrees_colorizer::detail::leaf_type<context_type>;
    using label_type = typename context_type::label_type;
    using index_type = typename context_type::index_type;
    using intensity_type = typename context_type::intensity_type;

    static constexpr label_type label_undefined = context_type::label_undefined;
    static constexpr label_type label_implicit_surrounding = context_type::label_implicit_surrounding;
    static constexpr index_type index_undefined = context_type::index_undefined;
    static constexpr intensity_type intensity_max = context_type::intensity_max;

    static intensity_type
    intensity(leaf_type const & node)
    {
        return node.intensity;
    }

    static int
    area(leaf_type const & node)
    {
        return node.area;
    }

    static std::vector<std::pair<int, int>> const &
    connections(leaf_type const & node)
    {
        return node.connections;
    }

    static label_type
    preferred_label(leaf_type const & node)
    {
        return node.preferred_label;
    }

    static bool
    is_border_node(leaf_type const & node)
    {
        return node.is_border_leaf;
    }

    static int
    surounding_border_size(leaf_type const & node)
    {
        return node.surounding_border_size;
    }
};

namespace grid_of_quadtrees_colorizer
{

template <typename scribble_type_tp>
using colorization_return_element_type =
    std::pair
    <
        typename colorization_context<scribble_type_tp>::rect_type,
        typename colorization_context<scribble_type_tp>::label_type
    >;

template <typename scribble_type_tp>
using colorization_return_type =
    std::vector<colorization_return_element_type<scribble_type_tp>>;

template <typename scribble_type_tp>
colorization_return_type<scribble_type_tp>
colorize
(
    colorization_context<scribble_type_tp> & context,
    bool use_implicit_label_for_surounding_area = false
)
{
    using scribble_type = scribble_type_tp;
    using return_element_type = colorization_return_element_type<scribble_type_tp>;
    using return_type = colorization_return_type<scribble_type_tp>;
    using context_type = colorization_context<scribble_type_tp>;

    if (context.is_null())
    {
        return return_type();
    }
    
    // If there are no scribbles and an implicit surrounding scribble
    // must be used, then return one big rectangle with the surrounding area label
    if (context.scribbles().empty())
    {
        return_type return_value;
        if (use_implicit_label_for_surounding_area)
        {
            return_value.push_back
            (
                return_element_type
                (
                    context.working_grid().rect(),
                    context_type::label_implicit_surrounding
                )
            );
        }
        return return_value;
    }

    // Create the preferred label vector, removing duplicates
    std::vector<typename context_type::label_type> preferred_labels(context.scribbles().size());
    std::transform
    (
        context.scribbles().begin(),
        context.scribbles().end(),
        preferred_labels.begin(),
        [](scribble_type const & scribble)
        {
            return scribble.label();
        }
    );
    typename std::vector<typename context_type::label_type>::iterator it =
        std::unique(preferred_labels.begin(), preferred_labels.end());
    preferred_labels.resize(std::distance(preferred_labels.begin(), it));

    // If there is only one scribble and an implicit surrounding scribble
    // must not be used, then set the label of all cells to
    // the label of the unique scribble
    if (preferred_labels.size() == 1 && !use_implicit_label_for_surounding_area)
    {
        return_type return_value;
        return_value.push_back
        (
            return_element_type
            (
                context.working_grid().rect(),
                preferred_labels.back()
            )
        );
        return return_value;
    }

    // The neighbors for each cell must be updated because the topology of
    // the grid might be changed for example by adding a new scribble.
    context.update_neighbors();

    // Make a flat representation of the leaf cells
    using cell_type = typename context_type::working_grid_cell_type;
    using leaf_type = detail::leaf_type<context_type>;
    std::vector<leaf_type> leaves;

    // Copy info from the tree leaves and assign indices
    {
        std::unordered_map<typename context_type::working_grid_cell_type*, int> indices;
        context.working_grid().visit_leaves(
            [&leaves, &indices](cell_type * cell) -> bool
            {
                leaf_type leaf;
                leaf.preferred_label = cell->data().preferred_label;
                leaf.intensity = cell->data().intensity;
                leaf.area = cell->size() * cell->size();
                leaf.surounding_border_size = cell->size();
                leaves.push_back(leaf);
                indices.insert({cell, indices.size()});
                return true;
            }
        );

        // Now that the cells have an index we can properly
        // set the border and neighbor info
        context.working_grid().visit_border_leaves(
            [&leaves, &indices](cell_type * cell) -> bool
            {
                leaves[indices[cell]].is_border_leaf = true;
                return true;
            }
        );

        context.working_grid().visit_leaves(
            [&leaves, &indices](cell_type * cell) -> bool
            {
                leaf_type & leaf = leaves[indices[cell]];
                for (cell_type * neighbor_cell : cell->top_leaf_neighbors()) {
                    leaf.connections.push_back
                    (
                        std::pair<int, int>
                        (
                            indices[neighbor_cell],
                            std::min(cell->size(), neighbor_cell->size())
                        )
                    );
                }
                for (cell_type * neighbor_cell : cell->left_leaf_neighbors()) {
                    leaf.connections.push_back
                    (
                        std::pair<int, int>
                        (
                            indices[neighbor_cell],
                            std::min(cell->size(), neighbor_cell->size())
                        )
                    );
                }
                return true;
            }
        );
    }

    // Compute labeling
    std::vector<typename context_type::label_type> computed_labels(leaves.size(), context_type::label_undefined);
    int const k = 2 * (context.working_grid().rect().width() + context.working_grid().rect().height());

    label
    (
        leaves.begin(),
        leaves.end(),
        preferred_labels.begin(),
        preferred_labels.end(),
        computed_labels.begin(),
        k,
        use_implicit_label_for_surounding_area
    );

    // construct the vector with associated
    return_type colorization(leaves.size());

    {
        int i = 0;
        context.working_grid().visit_leaves(
            [&computed_labels, &colorization, &i](cell_type * cell) -> bool
            {
                colorization[i] = std::pair(cell->rect(), computed_labels[i]);
                ++i;
                return true;
            }
        );
    }

    return colorization;
}

}
}

#endif
