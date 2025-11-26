#include "octree.h"

#include <stdexcept>
#include <limits> 
#include <omp.h>
#include <chrono>

namespace
{
class ScopedTimer
{
public:
    ScopedTimer(double& out)
        : mStart(std::chrono::high_resolution_clock::now())
        , mOut(out)
    {}

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed_ms = end - mStart;

        mOut = elapsed_ms.count();
    }

private:
    ScopedTimer() = default;

    std::chrono::high_resolution_clock::time_point mStart;
    double& mOut;
};
}

Octree::Octree(std::vector<Particle*>& points, bool supportMultithread, 
               size_t parallelThresholdForInsert, size_t maxPointsPerNode) 
    : mSupportMultithread(supportMultithread)
    , mMaxPointsPerNode(maxPointsPerNode)
    , mParallelThresholdForInsert(parallelThresholdForInsert)
{
    if (points.size() == 0)
    {
        throw std::runtime_error("trying to init octree with 0 points");
    }
    
    {
        ScopedTimer timer(mProfileData[0]);
        mRoot->boundingBox = computeBoundingBox(points);
    }

    {
        ScopedTimer timer(mProfileData[1]);
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
    }

    {
        ScopedTimer timer(mProfileData[2]);
        generateLeafNodeList(mRoot);
    }
}

Octree::~Octree()
{
    freeNode(mRoot);
}

void Octree::freeNode(Node*& node)
{
    if (node == nullptr) return;

    bool isLeaf = true;
    for (auto*& octant : node->octants)
    {
        if (octant)
        {
            isLeaf = false;
            freeNode(octant);
            delete octant;
        }
    }

    if (!isLeaf)
    {
        for (auto*& pt : node->points)
        {
            delete pt;
        }
    }
}

Octree::BoundingBox Octree::computeBoundingBox(std::vector<Particle*>& points)
{ 
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double minZ = std::numeric_limits<double>::infinity();

    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    double maxZ = -std::numeric_limits<double>::infinity();

    #pragma omp parallel for reduction(min:minX, minY, minZ) reduction(max:maxX, maxY, maxZ)
    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto* pos = points[i]->mPosition.data();

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

void Octree::insert(Node*& node, Particle*& point)
{
    if (node->isLeafNode() && node->points.size() >= mMaxPointsPerNode)
    {
        // have to make this an interior node and push all points down the octree
        for (size_t i = 0; i < node->points.size(); ++i)
        {
            Node*& octant = getCorrespondingOctant(node->points[i], node);
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
        Node*& octant = getCorrespondingOctant(point, node);
        insert(octant, point);
    }
}

void Octree::insertParallel(Node*& node)
{
    if (node == nullptr) return;
    if (node->points.size() <= mMaxPointsPerNode) return;

    if (node->points.size() < mParallelThresholdForInsert)
    {
        std::vector<Particle*> temp;
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
                node->octants[octantId] = new Node();
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
            Node*& child = node->octants[octantId];
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

void Octree::generateLeafNodeList(Node*& node)
{
    if (node == nullptr) return;

    if (node->isLeafNode())
    {
        mLeafNodes.emplace_back(node);
    }
    else
    {
        // morton order traversal based on my octant ordering
        static constexpr std::array<size_t, 8> MORTON_ORDER = {6, 7, 5, 4, 2, 3, 1, 0};
        
        size_t numChildren = 0;
        for (const auto octantId : MORTON_ORDER)
        {
            if (node->octants[octantId])
            {
                ++numChildren;
                generateLeafNodeList(node->octants[octantId]);
            }
        }

        // reserve number of points equal to num children
        // makes it easier for barnes hut
        node->points.reserve(numChildren);
        for (size_t i = 0; i < numChildren; ++i)
        {
            node->points.emplace_back(new Particle());
        }
    }
}
