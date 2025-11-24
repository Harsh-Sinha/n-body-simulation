#pragma once

#include <vector>
#include <array>
#include <cmath> 

#include "particle.h"

static constexpr size_t DEFAULT_MAX_POINTS_PER_NODE = 5;
// when node contains <= number of points switch to serial insert algorithm
static constexpr size_t PARALLEL_THRESHOLD_FOR_INSERT = 5000;

// valgrind will report possiblly lost memory for all these function calls
// because they are raw pointers... but look at assumption above constructor
class Octree
{
public:
    // assume that pointers are valid for as long as tree is used
    Octree(std::vector<Particle*>& points,
           bool supportMultithread = false,
           size_t parallelThresholdForInsert = PARALLEL_THRESHOLD_FOR_INSERT,
           size_t maxPointsPerNode = DEFAULT_MAX_POINTS_PER_NODE);
    ~Octree();

    struct BoundingBox
    {
        std::array<double, 3> center{0.0, 0.0, 0.0};
        double halfOfSideLength = 0.0; 
        
        bool isPointInBox(Particle*& point) const
        {
            bool inBox = true;
            auto& p = point->mPosition;
            
            inBox = inBox && std::abs(p[0] - center[0]) <= halfOfSideLength;
            inBox = inBox && std::abs(p[1] - center[1]) <= halfOfSideLength;
            inBox = inBox && std::abs(p[2] - center[2]) <= halfOfSideLength; 
            
            return inBox; 
        } 
    };

    struct Node
    { 
        BoundingBox boundingBox; 
        std::array<Node*, 8> octants;
        std::vector<Particle*> points;
        Node* parentNode;
        std::array<double, 3> com;
        double totalMass = 0;

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

    static inline size_t toOctantId(Particle*& point, const BoundingBox& box)
    {
        auto& p = point->mPosition;

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

    inline std::vector<Node*>& getLeafNodes()
    {
        return mLeafNodes;
    }

    inline Node*& getRootNode()
    {
        return mRoot;
    }

private: 
    Octree() = default;

    void freeNode(Node*& node);

    BoundingBox computeBoundingBox(std::vector<Particle*>& points);

    void insert(Node*& node, Particle*& point);

    void insertParallel(Node*& node);

    BoundingBox createChildBox(size_t index, const BoundingBox& parent);

    // assumes that a reader/writer lock is already held
    inline Node*& getCorrespondingOctant(Particle*& point, Node*& node)
    {
        size_t octandId = toOctantId(point, node->boundingBox);
        if (node->octants[octandId]) return node->octants[octandId];

        // create new leaf node if needed
        if (node->octants[octandId] == nullptr)
        {   
            node->octants[octandId] = new Node();
            node->octants[octandId]->boundingBox = createChildBox(octandId, node->boundingBox);
            node->octants[octandId]->parentNode = node;
        }
            
        return node->octants[octandId];
    }

    void generateLeafNodeList(Node*& node);

    Node* mRoot = new Node();
    std::vector<Node*> mLeafNodes;
    bool mSupportMultithread;
    size_t mMaxPointsPerNode;
    size_t mParallelThresholdForInsert;
    std::vector<Particle*> mRawParticles;
};
