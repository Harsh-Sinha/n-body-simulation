#pragma once

#include <vector> 
#include <memory> 
#include <array> 
#include <cmath> 

#include "point3d.h" 

static constexpr size_t DEFAULT_MAX_POINTS_PER_NODE = 5;
// when node contains <= number of points switch to serial insert algorithm
static constexpr size_t PARALLEL_THRESHOLD_FOR_INSERT = 5000;

class Octree 
{ 
public:
    Octree(std::vector<std::shared_ptr<Point3d>>& points, 
           bool supportMultithread = false,
           size_t parallelThresholdForInsert = PARALLEL_THRESHOLD_FOR_INSERT,
           size_t maxPointsPerNode = DEFAULT_MAX_POINTS_PER_NODE);
    ~Octree() = default;

protected:
    struct BoundingBox
    {
        std::array<double, 3> center{0.0, 0.0, 0.0};
        double halfOfSideLength = 0.0; 
        
        bool isPointInBox(const std::shared_ptr<Point3d>& point) const
        {
            bool inBox = true;
            auto& p = point->getPosition();
            
            inBox = inBox && std::abs(p[0] - center[0]) <= halfOfSideLength;
            inBox = inBox && std::abs(p[1] - center[1]) <= halfOfSideLength;
            inBox = inBox && std::abs(p[2] - center[2]) <= halfOfSideLength; 
            
            return inBox; 
        } 
    };

    struct Node
    { 
        BoundingBox boundingBox; 
        std::array<std::shared_ptr<Node>, 8> octants;
        std::vector<std::shared_ptr<Point3d>> points;
        std::shared_ptr<Node> parentNode;
        
        bool isLeafNode() const
        {
            bool isLeaf = true;
            
            for (const auto& octant : octants) 
            {
                isLeaf = isLeaf && octant == nullptr;
            } 
            
            return isLeaf;
        }
    };

    std::shared_ptr<Node> mRoot = std::make_shared<Node>();
    std::vector<std::shared_ptr<Node>> mLeafNodes;

private: 
    Octree() = default;

    BoundingBox computeBoundingBox(std::vector<std::shared_ptr<Point3d>>& points);

    void insert(std::shared_ptr<Node>& node, std::shared_ptr<Point3d>& point);

    void insertParallel(std::shared_ptr<Node>& node);

    BoundingBox createChildBox(size_t index, const BoundingBox& parent);

    inline size_t toOctantId(const std::shared_ptr<Point3d>& point, const BoundingBox& box)
    {
        auto& p = point->getPosition();

        // decide if point is in upper or lower quadrant
        size_t id = (p[2] >= box.center[2]) ? 0 : 4;

        // use quadrant rules to place point
        // (+,+) - quandrant 0
        // (-,+) - quandrant 1
        // (-,-) - quandrant 2
        // (+,-) - quandrant 3
        if (p[0] >= box.center[0]) // (+,?)
        {
            id += (p[1] >= box.center[1]) ? 0 : 3;
        }
        else // (-,?)
        {
            id += (p[1] >= box.center[1]) ? 1 : 2;
        }
        
        return id;
    }

    // assumes that a reader/writer lock is already held
    inline std::shared_ptr<Node>& getCorrespondingOctant(const std::shared_ptr<Point3d>& point, std::shared_ptr<Node>& node)
    {
        size_t octandId = toOctantId(point, node->boundingBox);
        if (node->octants[octandId]) return node->octants[octandId];

        // create new leaf node if needed
        if (node->octants[octandId] == nullptr)
        {   
            node->octants[octandId] = std::make_shared<Node>();
            node->octants[octandId]->boundingBox = createChildBox(octandId, node->boundingBox);
            node->octants[octandId]->parentNode = node;
        }
            
        return node->octants[octandId];
    }

    void generateLeafNodeList(std::shared_ptr<Node>& node);

    bool mSupportMultithread;
    size_t mMaxPointsPerNode;
    size_t mParallelThresholdForInsert;
};