#include "octree.h"

#include <stdexcept>
#include <limits> 

Octree::Octree(std::vector<std::shared_ptr<Point3d>>& points, bool supportMultithread, size_t maxPointsPerNode) 
    : mSupportMultithread(supportMultithread)
    , mMaxPointsPerNode(maxPointsPerNode)
{
    if (points.size() == 0)
    {
        throw std::runtime_error("trying to init octree with 0 points");
    }

    mRoot->boundingBox = computeBoundingBox(points);
    mRoot->lock = createNodeLock(mSupportMultithread);

    if (mSupportMultithread)
    {
        #pragma omp parallel for schedule(dynamic)
        for (auto& point : points)
        {
            mRoot->lock->acquireReader();
            insert(mRoot, point);
        }
    }
    else
    {
        for (auto& point : points)
        {
            mRoot->lock->acquireReader();
            insert(mRoot, point);
        }
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

// assumes node's reader has already been acquired
void Octree::insert(std::shared_ptr<Node>& node, std::shared_ptr<Point3d>& point)
{
    if (node->isLeafNode() && node->points.size() >= mMaxPointsPerNode)
    {
        // have to make this an interior node and push all points down the octree
        node->lock->elevateToWriter();
        for (size_t i = 0; i < node->points.size(); ++i)
        {
            std::shared_ptr<Node>& octant = getCorrespondingOctant(node->points[i], node);
            octant->lock->acquireReader();
            insert(octant, node->points[i]);
        }
        node->points.clear();
        node->lock->unlock();
        node->lock->acquireReader();
    }

    // assumes node still has a reader lock acquired at this point
    if (node->isLeafNode() && node->points.size() < mMaxPointsPerNode)
    {
        node->lock->elevateToWriter();
        node->points.emplace_back(point);
        node->lock->unlock();
    }
    else
    {
        // keep traversing down the octree to place
        std::shared_ptr<Node>& octant = getCorrespondingOctant(point, node);
        octant->lock->acquireReader();
        node->lock->unlock();
        insert(octant, point);
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