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

#ifndef LAZYBRUSH_HPP
#define LAZYBRUSH_HPP

#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>

#include <maxflow/graph.h>

namespace lazybrush
{

template <typename node_type_tp>
class node_traits;

template
<
    typename node_random_access_iterator_tp,
    typename label_input_iterator_tp,
    typename label_output_iterator_tp
>
void
label
(
    node_random_access_iterator_tp nodes_begin,
    node_random_access_iterator_tp nodes_end,
    label_input_iterator_tp preferred_labels_begin,
    label_input_iterator_tp preferred_labels_end,
    label_output_iterator_tp computed_labels_begin,
    int k,
    bool use_implicit_label_for_surounding_area = false
)
{
    using node_type = typename node_random_access_iterator_tp::value_type;
    using node_traits = node_traits<node_type>;
    using label_type = typename label_input_iterator_tp::value_type;
    using index_type = int;
    using intensity_type = decltype(node_traits::intensity(*nodes_begin));
    using node_difference_type = typename node_random_access_iterator_tp::difference_type;

    node_difference_type number_of_unlabeled_nodes = nodes_end - nodes_begin;

    // Lazybrush constants
    int const soft_scribble_weight = 5 * k / 100;
    int const hard_scribble_weight = k;
    int const implicit_surrounding_cell_edge_weight = 1 + k;

    struct additional_node_info_type
    {
        int maxflow_index;
        int weight_of_edge_to_source_sink;
        int weight_of_edge_to_neighbor_node;
        label_type computed_label;
    };

    // This vector contains additional information about the nodes
    std::vector<additional_node_info_type> additional_node_info(number_of_unlabeled_nodes);

    // The following vector contains indices to the nodes.
    // The indices of the unlabeled nodes are kept in the beginning
    // of the vector. This way we only iterate over the unlabeled nodes
    // on each iteration. This optimization saves some milliseconds
    std::vector<node_difference_type> nodes_indices(number_of_unlabeled_nodes);

    // Precompute some info from the nodes
    {
        for (auto it = nodes_begin; it != nodes_end; ++it)
        {
            node_type const & node = *it;
            node_difference_type d = std::distance(nodes_begin, it);

            // Set the additional info
            additional_node_info[d].weight_of_edge_to_source_sink = soft_scribble_weight * node_traits::area(node);
            additional_node_info[d].weight_of_edge_to_neighbor_node = 1 + k * node_traits::intensity(node) / node_traits::intensity_max;
            additional_node_info[d].computed_label = node_traits::label_undefined;

            // Set computed labels to undefined
            // add the index to the nodes indices vector
            nodes_indices[d] = d;
        }
    }

    // Go through the user labels and compute the final labeling
    std::vector<label_type> processed_labels;

    for (auto it = preferred_labels_begin; it != preferred_labels_end; ++it)
    {
        label_type const & current_label = *it;

        // Continue if the label is undefined or if it was already processed
        if
        (
            current_label == node_traits::label_undefined ||
            std::find
            (
                processed_labels.begin(),
                processed_labels.end(),
                current_label
            ) != processed_labels.end()
        )
        {
            continue;
        }

        // node maxflow indices must be recomputed from iteration to
        // iteration because the new maxflow graph only contains the
        // previously non labeled nodes
        int total_neighbor_count = 0;
        for (int i = 0; i < number_of_unlabeled_nodes; ++i)
        {
            additional_node_info_type & node_info = additional_node_info[nodes_indices[i]];
            node_info.maxflow_index = i;
            node_type const & node = *(nodes_begin + nodes_indices[i]);
            total_neighbor_count += node_traits::connections(node).size();
        }

        // Create the maxflow graph
        using maxflow_graph_type = Graph<int, int, int>;
        maxflow_graph_type maxflow_graph(number_of_unlabeled_nodes, 2 * total_neighbor_count);

        // Add maxflow nodes
        maxflow_graph.add_node(number_of_unlabeled_nodes);

        // Add extra node to account for the surrounding area
        if (use_implicit_label_for_surounding_area)
        {
            maxflow_graph.add_node();
            // Connect the implicit surrounding node to the sink.
            // Make the connection from the implicit surrounding cell to the
            // sink super strong to reflect that the surroundings extend
            // infinitelly
            maxflow_graph.add_tweights(number_of_unlabeled_nodes, 0, std::numeric_limits<int>::max());
        }

        // Add edges and set capacities
        for (int i = 0; i < number_of_unlabeled_nodes; ++i)
        {
            node_type const & node = *(nodes_begin + nodes_indices[i]);
            additional_node_info_type const & node_info = additional_node_info[nodes_indices[i]];

            // Connect current node to the source or sink node (data term)
            // if the preferred label is defined and was not already processed
            // The weight is scaled by the number of pixels that would
            // fit in the cell
            if
            (
                node_traits::preferred_label(node) != node_traits::label_undefined &&
                std::find
                (
                    processed_labels.begin(),
                    processed_labels.end(),
                    node_traits::preferred_label(node)
                ) == processed_labels.end()
            )
            {
                if (node_traits::preferred_label(node) == current_label)
                {
                    // This node must be connected to the source node
                    maxflow_graph.add_tweights(node_info.maxflow_index, node_info.weight_of_edge_to_source_sink, 0);
                }
                else
                {
                    // This node must be connected to the sink node
                    maxflow_graph.add_tweights(node_info.maxflow_index, 0, node_info.weight_of_edge_to_source_sink);
                }
            }

            // connect the node to its neighbors and
            // set capacities (smoothnes term)
            for (auto const & connection : node_traits::connections(node))
            {
                additional_node_info_type const & neighbor_node_info = additional_node_info[connection.first];

                // Continue if the neighbor was already labeled
                if (neighbor_node_info.computed_label != node_traits::label_undefined)
                {
                    continue;
                }
                // The edge weight is scaled by the length of the border
                // between the nodes to reflect that we are
                // cutting though various pixels
                maxflow_graph.add_edge
                (
                    node_info.maxflow_index,
                    neighbor_node_info.maxflow_index,
                    node_info.weight_of_edge_to_neighbor_node * connection.second,
                    neighbor_node_info.weight_of_edge_to_neighbor_node * connection.second
                );
            }

            // Connect the implicit surrounding node to the border nodes
            // and set capacities (smoothnes term)
            if (node_traits::is_border_node(node) && use_implicit_label_for_surounding_area)
            {
                // The edge weight is scaled by the length of the border
                // between the cells to reflect that we are
                // cutting though various pixels
                maxflow_graph.add_edge
                (
                    node_info.maxflow_index,
                    number_of_unlabeled_nodes,
                    node_info.weight_of_edge_to_neighbor_node * node_traits::surounding_border_size(node),
                    implicit_surrounding_cell_edge_weight * node_traits::surounding_border_size(node)
                );
            }
        }

        // Compute maxflow
        maxflow_graph.maxflow();

        // Set the labels
        {
            int i = 0;
            while (i < number_of_unlabeled_nodes)
            {
                additional_node_info_type & node_info = additional_node_info[nodes_indices[i]];
                if (maxflow_graph.what_segment(node_info.maxflow_index) == maxflow_graph_type::SOURCE)
                {
                    node_info.computed_label = current_label;
                    // The following lines have the effect of putting the
                    // labeled index on the back and "resizing" the
                    // indices vector.
                    // "i" is not incremented here because the last unlabeled
                    // index is put in this position
                    std::swap(nodes_indices[i], nodes_indices[number_of_unlabeled_nodes - 1]);
                    --number_of_unlabeled_nodes;
                }
                else
                {
                    ++i;
                }
            }
        }

        // Add the current label to the list of processed labels
        processed_labels.push_back(current_label);
    }

    // Copy the computed labeling
    // If there is still any unlabeled cells, then label them
    // as implicit surrounding if the option was set
    for (additional_node_info_type const & node_info : additional_node_info)
    {
        if (use_implicit_label_for_surounding_area &&
            node_info.computed_label == node_traits::label_undefined)
            {
            *computed_labels_begin = node_traits::label_implicit_surrounding;
        }
        else
        {
            *computed_labels_begin = node_info.computed_label;
        }
        ++computed_labels_begin;
    }
}

}

#endif
