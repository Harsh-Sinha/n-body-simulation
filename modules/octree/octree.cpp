#include "octree.h"

#include <stdexcept>
#include <limits> 
#include <omp.h>

Octree::Octree(std::vector<std::shared_ptr<Point3d>>& points, bool supportMultithread, 
               size_t parallelThresholdForInsert, size_t maxPointsPerNode) 
    : mSupportMultithread(supportMultithread)
    , mMaxPointsPerNode(maxPointsPerNode)
    , mParallelThresholdForInsert(parallelThresholdForInsert)
{
    if (points.size() == 0)
    {
        throw std::runtime_error("trying to init octree with 0 points");
    }

    mRoot->boundingBox = computeBoundingBox(points);

    if (mSupportMultithread)
    {
        mRoot->points.insert(mRoot->points.end(), points.begin(), points.end());
        #pragma omp parallel
        {
            #pragma omp single
            {
                insertParallel(mRoot);
            }
        }
    }
    else
    {
        for (auto& point : points)
        {
            insert(mRoot, point);
        }
    }

    generateLeafNodeList(mRoot);
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
    if (node->isLeafNode() && node->points.size() >= mMaxPointsPerNode)
    {
        // have to make this an interior node and push all points down the octree
        for (size_t i = 0; i < node->points.size(); ++i)
        {
            std::shared_ptr<Node>& octant = getCorrespondingOctant(node->points[i], node);
            insert(octant, node->points[i]);
        }
        node->points.clear();
    }

    if (node->isLeafNode() && node->points.size() < mMaxPointsPerNode)
    {
        node->points.emplace_back(point);
    }
    else
    {
        // keep traversing down the octree to place
        std::shared_ptr<Node>& octant = getCorrespondingOctant(point, node);
        insert(octant, point);
    }
}

void Octree::insertParallel(std::shared_ptr<Node>& node)
{
    if (node == nullptr) return;
    if (node->points.size() <= mMaxPointsPerNode) return;

    if (node->points.size() < mParallelThresholdForInsert)
    {
        std::vector<std::shared_ptr<Point3d>> temp;
        temp.swap(node->points);

        for (auto& point : temp)
        {
            insert(node, point);
        }
    }
    else
    {
        std::array<size_t, 8> elementsPerOctant;
        elementsPerOctant.fill(0);
        for (size_t i = 0; i < node->points.size(); ++i)
        {
            size_t octantId = toOctantId(node->points[i], node->boundingBox);
            ++elementsPerOctant[octantId];
        }

        for (size_t octantId = 0; octantId < 8; ++octantId)
        {
            if (elementsPerOctant[octantId] > 0)
            {
                node->octants[octantId] = std::make_shared<Node>();
                node->octants[octantId]->boundingBox = createChildBox(octantId, node->boundingBox);
                node->octants[octantId]->parentNode = node;

                node->octants[octantId]->points.resize(elementsPerOctant[octantId]);
            }
        }

        std::array<size_t, 8> index;
        index.fill(0);
        for (size_t i = 0; i < node->points.size(); ++i)
        {
            size_t octantId = toOctantId(node->points[i], node->boundingBox);
            std::shared_ptr<Node>& child = node->octants[octantId];
            child->points[index[octantId]] = node->points[i];
            ++index[octantId];
        }

        node->points.clear();

        for (size_t octantId = 0; octantId < 8; ++octantId)
        {
            if (node->octants[octantId] &&
                node->octants[octantId]->points.size() > mMaxPointsPerNode)
            {
                #pragma omp task firstprivate(octantId) shared(node)
                {
                    insertParallel(node->octants[octantId]);
                }
            }
        }

        #pragma omp taskwait
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

void Octree::generateLeafNodeList(std::shared_ptr<Node>& node)
{
    if (node == nullptr) return;

    if (node->isLeafNode())
    {
        mLeafNodes.emplace_back(node);
    }
    else
    {
        for (auto& octant : node->octants)
        {
            if (octant)
            {
                generateLeafNodeList(octant);
            }
        }
    }
}