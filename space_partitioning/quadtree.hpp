#ifndef QUADTREE_HPP
#define QUADTREE_HPP

#include <QPoint>
#include <QRect>

namespace SpacePartitioning
{

template <typename DataTypeTP>
class QuadTreeNode
{
public:
    using DataType = DataTypeTP;

    QuadTreeNode() = default;
    QuadTreeNode(const QuadTreeNode &) = default;
    QuadTreeNode(QuadTreeNode &&) = default;

    QuadTreeNode(const DataType & data)
        : m_data(data)
    {}

    ~QuadTreeNode()
    {
        if (isSubdivided()) {
            for (int i = 0; i < 4; ++i) {
                delete m_children[i];
            }
        }
    }

    QuadTreeNode& operator=(const QuadTreeNode &) = default;
    QuadTreeNode& operator=(QuadTreeNode &&) = default;

    // Add a point to the tree, recursivelly
    QuadTreeNode* addPoint(QPoint point)
    {
        if (!rect().contains(point)) {
            return nullptr;
        }

        if (size() == 1) {
            return this;
        }

        if (!isSubdivided()) {
            // Subdivide
            for (int i = 0; i < 4; ++i) {
                m_children[i] = new QuadTreeNode;
                m_children[i]->setParent(this);
            }

            const QPoint centerPoint = center();
            const int childSize = size() >> 1;

            topLeftChild()->setRect(rect().x(), rect().y(), childSize, childSize);
            topRightChild()->setRect(centerPoint.x(), rect().y(), childSize, childSize);
            bottomLeftChild()->setRect(rect().x(), centerPoint.y(), childSize, childSize);
            bottomRightChild()->setRect(centerPoint.x(), centerPoint.y(), childSize, childSize);
        }

        QuadTreeNode *child = childAt(point);
        if (child) {
            return child->addPoint(point);
        }
        return nullptr;
    }

    QuadTreeNode* addPoint(int x, int y)
    {
        return addPoint(QPoint(x, y));
    }

    // The following functions are just for convenience
    // to improve readability in the algorithms

    QuadTreeNode* parent() const
    {
        return m_parent;
    }
    
    QuadTreeNode* topLeftChild() const
    {
        return m_children[0];
    }

    QuadTreeNode* topRightChild() const
    {
        return m_children[1];
    }

    QuadTreeNode* bottomRightChild() const
    {
        return m_children[2];
    }

    QuadTreeNode* bottomLeftChild() const
    {
        return m_children[3];
    }

    QuadTreeNode** children()
    {
        return m_children;
    }

    const QVector<QuadTreeNode*>& topLeafNeighbors() const
    {
        return m_topLeafNeighbors;
    }

    const QVector<QuadTreeNode*>& leftLeafNeighbors() const
    {
        return m_leftLeafNeighbors;
    }

    const QVector<QuadTreeNode*>& bottomLeafNeighbors() const
    {
        return m_bottomLeafNeighbors;
    }

    const QVector<QuadTreeNode*>& rightLeafNeighbors() const
    {
        return m_rightLeafNeighbors;
    }

    QVector<QuadTreeNode*> topMostLeaves() const
    {
        QVector<QuadTreeNode*> leaves;
        if (isSubdivided()) {
            leaves += topLeftChild()->topMostLeaves();
            leaves += topRightChild()->topMostLeaves();
        } else {
            leaves += const_cast<QuadTreeNode*>(this);
        }
        return leaves;
    }

    QVector<QuadTreeNode*> leftMostLeaves() const
    {
        QVector<QuadTreeNode*> leaves;
        if (isSubdivided()) {
            leaves += topLeftChild()->leftMostLeaves();
            leaves += bottomLeftChild()->leftMostLeaves();
        } else {
            leaves += const_cast<QuadTreeNode*>(this);
        }
        return leaves;
    }

    QVector<QuadTreeNode*> bottomMostLeaves() const
    {
        QVector<QuadTreeNode*> leaves;
        if (isSubdivided()) {
            leaves += bottomLeftChild()->bottomMostLeaves();
            leaves += bottomRightChild()->bottomMostLeaves();
        } else {
            leaves += const_cast<QuadTreeNode*>(this);
        }
        return leaves;
    }

    QVector<QuadTreeNode*> rightMostLeaves() const
    {
        QVector<QuadTreeNode*> leaves;
        if (isSubdivided()) {
            leaves += topRightChild()->rightMostLeaves();
            leaves += bottomRightChild()->rightMostLeaves();
        } else {
            leaves += const_cast<QuadTreeNode*>(this);
        }
        return leaves;
    }

    const DataType& data() const
    {
        return m_data;
    }

    DataType& data()
    {
        return m_data;
    }

    const QRect& rect() const
    {
        return m_rect;
    }

    int size() const
    {
        return rect().width();
    }

    QPoint center() const
    {
        const int cellCenter = size() >> 1;
        return rect().topLeft() + QPoint(cellCenter, cellCenter);
    }

    void setParent(QuadTreeNode *newParent)
    {
        m_parent = newParent;
    }
    
    void setTopLeftChild(QuadTreeNode *newChild)
    {
        m_children[0] = newChild;
    }

    void setTopRightChild(QuadTreeNode *newChild)
    {
        m_children[1] = newChild;
    }

    void setBottomRightChild(QuadTreeNode *newChild)
    {
        m_children[2] = newChild;
    }

    void setBottomLeftChild(QuadTreeNode *newChild)
    {
        m_children[3] = newChild;
    }

    void setTopLeafNeighbors(const QVector<QuadTreeNode*> &newTopLeafNeighbors)
    {
        m_topLeafNeighbors = newTopLeafNeighbors;
    }

    void setLeftLeafNeighbors(const QVector<QuadTreeNode*> &newLeftLeafNeighbors)
    {
        m_leftLeafNeighbors = newLeftLeafNeighbors;
    }

    void setBottomLeafNeighbors(const QVector<QuadTreeNode*> &newBottomLeafNeighbors)
    {
        m_bottomLeafNeighbors = newBottomLeafNeighbors;
    }

    void setRightLeafNeighbors(const QVector<QuadTreeNode*> &newRightLeafNeighbors)
    {
        m_rightLeafNeighbors = newRightLeafNeighbors;
    }

    void setData(DataType newData)
    {
        m_data = newData;
    }

    void setRect(const QRect &newRect)
    {
        m_rect = newRect;
    }

    void setRect(int x, int y, int width, int height)
    {
        setRect(QRect(x, y, width, height));
    }

    bool hasParent() const
    {
        return parent() != nullptr;
    }
    
    bool hasTopLeftChild() const
    {
        return topLeftChild() != nullptr;
    }

    bool hasTopRightChild() const
    {
        return topRightChild() != nullptr;
    }

    bool hasBottomRightChild() const
    {
        return bottomRightChild() != nullptr;
    }

    bool hasBottomLeftChild() const
    {
        return bottomLeftChild() != nullptr;
    }

    bool isSubdivided() const
    {
        return hasTopLeftChild();
    }

    bool isRoot() const
    {
        return m_parent == nullptr;
    }

    bool isLeaf() const
    {
        return !isSubdivided();
    }

    bool isBottomMostLeaf() const
    {
        return size() == 1;
    }

    QuadTreeNode* childAt(QPoint point)
    {
        if (!rect().contains(point)) {
            return nullptr;
        }

        if (isLeaf()) {
            return nullptr;
        }

        const QPoint centerPoint = center();

        if (point.x() < centerPoint.x()) {
            if (point.y() < centerPoint.y()) {
                return topLeftChild();
            } else {
                return bottomLeftChild();
            }
        } else {
            if (point.y() < centerPoint.y()) {
                return topRightChild();
            } else {
                return bottomRightChild();
            }
        }
    }

    QuadTreeNode* childAt(int x, int y)
    {
        return childAt(QPoint(x, y));
    }

    QuadTreeNode* leafAt(const QPoint &point)
    {
        if (!rect().contains(point)) {
            return nullptr;
        }

        if (isLeaf()) {
            return this;
        }

        const QPoint centerPoint = center();

        if (point.x() < centerPoint.x()) {
            if (point.y() < centerPoint.y()) {
                return topLeftChild()->leafAt(point);
            } else {
                return bottomLeftChild()->leafAt(point);
            }
        } else {
            if (point.y() < centerPoint.y()) {
                return topRightChild()->leafAt(point);
            } else {
                return bottomRightChild()->leafAt(point);
            }
        }
    }

    QuadTreeNode* leafAt(int x, int y)
    {
        return leafAt(QPoint(x, y));
    }

private:
    QuadTreeNode *m_parent{nullptr};
    QuadTreeNode *m_children[4]{nullptr};

    QVector<QuadTreeNode*> m_topLeafNeighbors;
    QVector<QuadTreeNode*> m_leftLeafNeighbors;
    QVector<QuadTreeNode*> m_bottomLeafNeighbors;
    QVector<QuadTreeNode*> m_rightLeafNeighbors;

    QRect m_rect;

    DataType m_data;
};

}

#endif
