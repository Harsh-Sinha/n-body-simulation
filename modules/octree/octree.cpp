#include "octree.h"

#include <stdexcept>
#include <limits> 
#include <omp.h>
#include <chrono>
#include <cstring>
#include <atomic>

namespace
{
class ScopedTimer
{
public:
    ScopedTimer(double& out)
        : mStart(std::chrono::steady_clock::now())
        , mOut(out)
    {}

    ~ScopedTimer()
    {
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double, std::milli> elapsed_ms = end - mStart;

        mOut = elapsed_ms.count();
    }

private:
    ScopedTimer() = default;

    std::chrono::steady_clock::time_point mStart;
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
                #pragma omp single nowait
                {
                    hybridParallelInsert(mRoot);
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

void Octree::insertParallel(Node*& node, bool benchmarkSingleIteration)
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

        if (benchmarkSingleIteration) return;

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

void Octree::partitionPointsInNode(Node*& node)
{
    if (node == nullptr) return;

    const size_t numPoints = node->points.size();

    if (numPoints == 0) return;
    if (numPoints <= mMaxPointsPerNode) return;

    if (numPoints <= mParallelThresholdForInsert)
    {
        std::vector<Particle*> temp;
        temp.swap(node->points);

        for (auto& point : temp)
        {
            insert(node, point);
        }
        return;
    }

    // count how many points go in each octant
    size_t a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0;
    #pragma omp parallel for reduction(+: a, b, c, d, e, f, g, h)
    for (size_t i = 0; i < numPoints; ++i)
    {
        size_t octantId = toOctantId(node->points[i], node->boundingBox);
        switch (octantId)
        {
            case 0: ++a; break;
            case 1: ++b; break;
            case 2: ++c; break;
            case 3: ++d; break;
            case 4: ++e; break;
            case 5: ++f; break;
            case 6: ++g; break;
            case 7: ++h; break;
        }
    }
    size_t elementsPerOctant[8] = { a, b, c, d, e, f, g, h };

    size_t offset[8];
    offset[0] = 0;
    for (size_t i = 1; i < 8; ++i)
    {
        offset[i] = offset[i - 1] + elementsPerOctant[i - 1];
    }

    std::vector<Particle*> temp(numPoints);

    std::atomic<size_t> writePosition[8] = { offset[0], offset[1], offset[2], offset[3],
                                             offset[4], offset[5], offset[6], offset[7] };

    #pragma omp parallel for
    for (size_t i = 0; i < numPoints; ++i)
    {
        auto*& point = node->points[i];
        size_t octantId = toOctantId(point, node->boundingBox);
        size_t index = writePosition[octantId].fetch_add(1);

        temp[index] = point;
    }

    #pragma omp parallel for
    for (size_t i = 0; i < 8; ++i)
    {
        if (elementsPerOctant[i] > 0)
        {
            Node* octant = new Node();
            node->octants[i] = octant;

            octant->boundingBox = createChildBox(i, node->boundingBox);
            octant->parentNode = node;

            size_t start = offset[i];
            size_t count = elementsPerOctant[i];
            
            octant->points.reserve(count);
            for (size_t j = 0; j < count; ++j)
            {
                octant->points.emplace_back(temp[start + j]);
            }
        }
        else
        {
            node->octants[i] = nullptr;
        }
    }

    node->points.clear();
}

void Octree::hybridParallelInsert(Node*& node)
{
    static constexpr size_t THRESHOLD_FOR_TASK_BASED = 50000;
    if (node == nullptr) return;

    const size_t numPoints = node->points.size();

    if (numPoints == 0) return;
    if (numPoints <= mMaxPointsPerNode) return;

    if (numPoints <= mParallelThresholdForInsert)
    {
        std::vector<Particle*> temp;
        temp.swap(node->points);

        for (auto& point : temp)
        {
            insert(node, point);
        }
        return;
    }
    else if (numPoints <= THRESHOLD_FOR_TASK_BASED)
    {
        insertParallel(node);
    }
    else
    {
        partitionPointsInNode(node);

        for (size_t i = 0; i < 8; ++i)
        {
            auto*& octant = node->octants[i];
            if (octant)
            {
                #pragma omp task default(none) firstprivate(octant)
                {
                    hybridParallelInsert(octant);
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

    std::vector<Node*> bfs;
    generateWorkForTreeTraversal(bfs);

    auto numThreads = omp_get_max_threads();
    std::vector<std::vector<Node*>> localLeafs(numThreads);

    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < bfs.size(); ++i)
    {
        auto tid = omp_get_thread_num();
        dfsLeafNodeSearch(bfs[i], localLeafs[tid]);
    }

    for (auto& local : localLeafs)
    {
        mLeafNodes.insert(mLeafNodes.end(), local.begin(), local.end());
    }
}

void Octree::generateWorkForTreeTraversal(std::vector<Node*>& bfs)
{
    const size_t targetBfsSize = 8 * omp_get_max_threads();

    bfs.emplace_back(mRoot);

    while (bfs.size() > 0)
    {
        if (bfs.size() >= targetBfsSize) break;

        std::vector<Node*> nextSet;
        for (auto*& node : bfs)
        {
            size_t numChildren = 0;
            for (const auto octantId : MORTON_ORDER)
            {
                if (node->octants[octantId])
                {
                    ++numChildren;
                    nextSet.emplace_back(node->octants[octantId]);
                }
            }

            // reserve number of points equal to num children
            // makes it easier for barnes hut
            node->points.reserve(numChildren);
            for (size_t i = 0; i < numChildren; ++i)
            {
                node->points.emplace_back(new Particle());
            }

            if (numChildren == 0)
            {
                mLeafNodes.emplace_back(node);
            }
        }

        if (nextSet.size() == 0) break;

        bfs.swap(nextSet);
    }
}

void Octree::dfsLeafNodeSearch(Node*& node, std::vector<Node*>& local)
{
    if (node == nullptr) return;

    size_t numChildren = 0;
    for (const auto octantId : MORTON_ORDER)
    {
        if (node->octants[octantId])
        {
            ++numChildren;
            dfsLeafNodeSearch(node->octants[octantId], local);
        }
    }

    if (numChildren == 0)
    {
        local.emplace_back(node);
    }
    else
    {
        // reserve number of points equal to num children
        // makes it easier for barnes hut
        node->points.reserve(numChildren);
        for (size_t i = 0; i < numChildren; ++i)
        {
            node->points.emplace_back(new Particle());
        }
    }
}
