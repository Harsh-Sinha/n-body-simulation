#include "octree.h"

#include <limits> 

Octree::Octree(std::vector<std::shared_ptr<Point3d>>& points, bool supportMultithread, size_t maxPointsPerNode) 
    : mSupportMultithread(supportMultithread)
    , mMaxPointsPerNode(maxPointsPerNode)
{
    mRoot->boundingBox = computeBoundingBox(points);

    for (auto& point : points)
    {
        insert(mRoot, point);
    }
}

Octree::BoundingBox Octree::computeBoundingBox(std::vector<std::shared_ptr<Point3d>>& points)
{ 
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double minZ = std::numeric_limits<double>::infinity();

    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    double maxZ = -std::numeric_limits<double>::infinity();

    for (const auto& point : points)
    {
        auto& pos = point->getPosition();

        minX = std::min(minX, pos[0]);
        minY = std::min(minY, pos[1]);
        minZ = std::min(minZ, pos[2]);

        maxX = std::max(maxX, pos[0]);
        maxY = std::max(maxY, pos[1]);
        maxZ = std::max(maxZ, pos[2]);
    }

    double sideLength = std::max(maxX - minX, std::max(maxY - minY, maxZ - minZ));

    BoundingBox box; box.halfOfSideLength = sideLength / 2.0;
    box.center[0] = box.halfOfSideLength + minX;
    box.center[1] = box.halfOfSideLength + minY;
    box.center[2] = box.halfOfSideLength + minZ;
    // add padding to ensure no points on box boundary
    box.halfOfSideLength += std::max(1e-9, 0.001 * 0.5 * sideLength);

    return box;
}

void Octree::insert(std::shared_ptr<Node>& node, std::shared_ptr<Point3d>& point)
{
    if (node->isLeafNode())
    {
        if (node->hasPoint())
        {
            auto oldLeaf = node->leaf;
    
            splitBoundingBox(node);
    
            node->leaf = nullptr;
    
            insert(node->children[toChildIndex(oldLeaf->getPosition(), node->boundingBox)], oldLeaf);
            insert(node->children[toChildIndex(leaf->getPosition(), node->boundingBox)], leaf);
        }
        else
        {
            node->leaf = leaf;
        }
    }
    else
    {
        int childIndex = toChildIndex(leaf->getPosition(), node->boundingBox);
    
        if (node->children[childIndex] == nullptr)
        {
            node->children[childIndex] = std::make_unique<Node>();
            node->children[childIndex]->boundingBox = createChildBox(childIndex, node->boundingBox);
        }
    
        insert(node->children[childIndex], leaf);
    }
}

void Octree::splitBoundingBox(std::shared_ptr<Node>& node)
{
    for (int i = 0; i < node->octants.size(); ++i)
    {
        node->octants[i] = std::make_shared<Node>();
        node->octants[i]->boundingBox = createChildBox(i, node->boundingBox);
    }
}

Octree::BoundingBox Octree::createChildBox(size_t index, const BoundingBox& parent)
{
    BoundingBox child;

    child.halfOfSideLength = parent.halfOfSideLength / 2.0;
    child.center = parent.center;

    child.center[0] += (index == 0 || index == 3 || index == 4 || index == 7) ? child.halfOfSideLength : -child.halfOfSideLength;
    child.center[1] += (index == 0 || index == 1 || index == 4 || index == 5) ? child.halfOfSideLength : -child.halfOfSideLength;
    child.center[2] += (index < 4) ? child.halfOfSideLength : -child.halfOfSideLength;
    
    return child;
}