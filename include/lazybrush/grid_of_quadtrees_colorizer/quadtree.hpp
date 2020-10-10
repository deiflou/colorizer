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

#ifndef LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_QUADTREE_HPP
#define LAZYBRUSH_GRID_OF_QUADTREES_COLORIZER_QUADTREE_HPP

#include <vector>

#include "types.hpp"

namespace lazybrush
{
namespace grid_of_quadtrees_colorizer
{

template <typename data_type_tp>
class quadtree_node
{
public:
    using data_type = data_type_tp;
    using point_type = point<int>;
    using rect_type = lazybrush::grid_of_quadtrees_colorizer::rect<int>;

    quadtree_node() = default;
    quadtree_node(quadtree_node const &) = default;
    quadtree_node(quadtree_node &&) = default;

    quadtree_node(data_type const & data)
        : data_(data)
    {}

    ~quadtree_node()
    {
        if (is_subdivided())
        {
            for (int i = 0; i < 4; ++i)
            {
                delete children_[i];
            }
        }
    }

    quadtree_node &
    operator=(quadtree_node const &) = default;
    quadtree_node &
    operator=(quadtree_node &&) = default;

    // Add a point to the tree, recursivelly
    quadtree_node *
    add_point(point_type const & point)
    {
        if (!rect().contains(point))
        {
            return nullptr;
        }

        if (size() == 1)
        {
            return this;
        }

        if (!is_subdivided())
        {
            // Subdivide
            for (int i = 0; i < 4; ++i)
            {
                children_[i] = new quadtree_node;
                children_[i]->set_parent(this);
            }

            point_type const center_point = center();
            int const child_size = size() / 2;

            top_left_child()->set_rect(rect().x(), rect().y(), child_size, child_size);
            top_right_child()->set_rect(center_point.x(), rect().y(), child_size, child_size);
            bottom_left_child()->set_rect(rect().x(), center_point.y(), child_size, child_size);
            bottom_right_child()->set_rect(center_point.x(), center_point.y(), child_size, child_size);
        }

        quadtree_node * child = child_at(point);
        if (child)
        {
            return child->add_point(point);
        }

        return nullptr;
    }

    quadtree_node *
    add_point(int x, int y)
    {
        return add_point(point_type(x, y));
    }

    // The following functions are just for convenience
    // to improve readability in the algorithms

    quadtree_node *
    parent() const
    {
        return parent_;
    }
    
    quadtree_node *
    top_left_child() const
    {
        return children_[0];
    }

    quadtree_node *
    top_right_child() const
    {
        return children_[1];
    }

    quadtree_node *
    bottom_right_child() const
    {
        return children_[2];
    }

    quadtree_node *
    bottom_left_child() const
    {
        return children_[3];
    }

    quadtree_node **
    children()
    {
        return children_;
    }

    std::vector<quadtree_node *> const &
    top_leaf_neighbors() const
    {
        return top_leaf_neighbors_;
    }

    std::vector<quadtree_node *> const &
    left_leaf_neighbors() const
    {
        return left_leaf_neighbors_;
    }

    std::vector<quadtree_node *> const &
    bottom_leaf_neighbors() const
    {
        return bottom_leaf_neighbors_;
    }

    std::vector<quadtree_node *> const &
    right_leaf_neighbors() const
    {
        return right_leaf_neighbors_;
    }

    std::vector<quadtree_node *>
    top_most_leaves() const
    {
        std::vector<quadtree_node *> leaves;
        if (is_subdivided())
        {
            for (quadtree_node * node : top_left_child()->top_most_leaves())
            {
                leaves.push_back(node);
            }
            for (quadtree_node * node : top_right_child()->top_most_leaves())
            {
                leaves.push_back(node);
            }
        }
        else
        {
            leaves.push_back(const_cast<quadtree_node *>(this));
        }
        return leaves;
    }

    std::vector<quadtree_node *>
    left_most_leaves() const
    {
        std::vector<quadtree_node*> leaves;
        if (is_subdivided())
        {
            for (quadtree_node * node : top_left_child()->left_most_leaves())
            {
                leaves.push_back(node);
            }
            for (quadtree_node * node : bottom_left_child()->left_most_leaves())
            {
                leaves.push_back(node);
            }
        }
        else
        {
            leaves.push_back(const_cast<quadtree_node *>(this));
        }
        return leaves;
    }

    std::vector<quadtree_node *>
    bottom_most_leaves() const
    {
        std::vector<quadtree_node*> leaves;
        if (is_subdivided())
        {
            for (quadtree_node * node : bottom_left_child()->bottom_most_leaves())
            {
                leaves.push_back(node);
            }
            for (quadtree_node * node : bottom_right_child()->bottom_most_leaves())
            {
                leaves.push_back(node);
            }
        }
        else
        {
            leaves.push_back(const_cast<quadtree_node *>(this));
        }
        return leaves;
    }

    std::vector<quadtree_node *>
    right_most_leaves() const
    {
        std::vector<quadtree_node*> leaves;
        if (is_subdivided())
        {
            for (quadtree_node * node : top_right_child()->right_most_leaves())
            {
                leaves.push_back(node);
            }
            for (quadtree_node * node : bottom_right_child()->right_most_leaves())
            {
                leaves.push_back(node);
            }
        }
        else
        {
            leaves.push_back(const_cast<quadtree_node *>(this));
        }
        return leaves;
    }

    data_type const &
    data() const
    {
        return data_;
    }

    data_type &
    data()
    {
        return data_;
    }

    rect_type const &
    rect() const
    {
        return rect_;
    }

    int
    size() const
    {
        return rect().width();
    }

    point_type
    center() const
    {
        int const size_over_two = size() / 2;
        return
            point_type
            (
                rect().left() + size_over_two,
                rect().top() + size_over_two
            );
    }

    void
    set_parent(quadtree_node * new_parent)
    {
        parent_ = new_parent;
    }
    
    void
    set_top_left_child(quadtree_node * new_child)
    {
        children_[0] = new_child;
    }

    void
    set_top_right_child(quadtree_node * new_child)
    {
        children_[1] = new_child;
    }

    void
    set_bottom_right_child(quadtree_node * new_child)
    {
        children_[2] = new_child;
    }

    void
    set_bottom_left_child(quadtree_node * new_child)
    {
        children_[3] = new_child;
    }

    void
    set_top_leaf_neighbors(std::vector<quadtree_node *> const & new_top_leaf_neighbors)
    {
        top_leaf_neighbors_ = new_top_leaf_neighbors;
    }

    void
    set_left_leaf_neighbors(std::vector<quadtree_node*> const & new_left_leaf_neighbors)
    {
        left_leaf_neighbors_ = new_left_leaf_neighbors;
    }

    void
    set_bottom_leaf_neighbors(std::vector<quadtree_node*> const & new_bottom_leaf_neighbors)
    {
        bottom_leaf_neighbors_ = new_bottom_leaf_neighbors;
    }

    void
    set_right_leaf_neighbors(std::vector<quadtree_node*> const & new_right_leaf_neighbors)
    {
        right_leaf_neighbors_ = new_right_leaf_neighbors;
    }

    void
    set_data(data_type const & new_data)
    {
        data_ = new_data;
    }

    void
    set_rect(rect_type const & new_rect)
    {
        rect_ = new_rect;
    }

    void
    set_rect(int x, int y, int width, int height)
    {
        set_rect(rect_type(x, y, width, height));
    }

    bool
    has_parent() const
    {
        return parent() != nullptr;
    }
    
    bool
    has_top_left_child() const
    {
        return top_left_child() != nullptr;
    }

    bool
    has_top_right_child() const
    {
        return top_right_child() != nullptr;
    }

    bool
    has_bottom_right_child() const
    {
        return bottom_right_child() != nullptr;
    }

    bool
    has_bottom_left_child() const
    {
        return bottom_left_child() != nullptr;
    }

    bool
    is_subdivided() const
    {
        return has_top_left_child();
    }

    bool
    is_root() const
    {
        return !has_parent();
    }

    bool
    is_leaf() const
    {
        return !is_subdivided();
    }

    bool
    is_bottom_most_leaf() const
    {
        return size() == 1;
    }

    quadtree_node *
    child_at(point_type const & point)
    {
        if (!rect().contains(point))
        {
            return nullptr;
        }

        if (is_leaf())
        {
            return nullptr;
        }

        point_type const center_point = center();

        if (point.x() < center_point.x())
        {
            if (point.y() < center_point.y())
            {
                return top_left_child();
            }
            else
            {
                return bottom_left_child();
            }
        }
        else
        {
            if (point.y() < center_point.y())
            {
                return top_right_child();
            }
            else
            {
                return bottom_right_child();
            }
        }
    }

    quadtree_node *
    child_at(int x, int y)
    {
        return child_at(point_type(x, y));
    }

    quadtree_node *
    leaf_at(point_type const & point)
    {
        if (!rect().contains(point))
        {
            return nullptr;
        }

        if (is_leaf())
        {
            return this;
        }

        point_type const center_point = center();

        if (point.x() < center_point.x()) {
            if (point.y() < center_point.y()) {
                return top_left_child()->leaf_at(point);
            } else {
                return bottom_left_child()->leaf_at(point);
            }
        } else {
            if (point.y() < center_point.y()) {
                return top_right_child()->leaf_at(point);
            } else {
                return bottom_right_child()->leaf_at(point);
            }
        }
    }

    quadtree_node *
    leaf_at(int x, int y)
    {
        return leaf_at(point_type(x, y));
    }

private:
    quadtree_node * parent_{nullptr};
    quadtree_node * children_[4]{nullptr};

    std::vector<quadtree_node *> top_leaf_neighbors_;
    std::vector<quadtree_node *> left_leaf_neighbors_;
    std::vector<quadtree_node *> bottom_leaf_neighbors_;
    std::vector<quadtree_node *> right_leaf_neighbors_;

    rect_type rect_;

    data_type data_;
};

}
}

#endif
