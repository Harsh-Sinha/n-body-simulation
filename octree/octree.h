#pragma once

#include <vector> 
#include <memory> 
#include <array> 
#include <cmath> 

#include "leaf_base.h" 

static constexpr size_t DEFAULT_MAX_POINTS_PER_NODE = 5;

class Octree 
{ 
public:
    Octree(std::vector<std::shared_ptr<LeafBase>>& leafs, bool supportMultithread = false, size_t maxPointsPerNode = DEFAULT_MAX_POINTS_PER_NODE);
    ~Octree() = default;

private: 
    Octree() = default;
    
    struct BoundingBox
    {
        std::array<double, 3> center{0.0, 0.0, 0.0}; 
        double halfOfSideLength = 0.0; 
        
        bool pointInBox(const std::array<double, 3>& p) const
        {
            bool inBox = true; 
            
            inBox = inBox && std::abs(p[0] - center[0]) <= halfOfSideLength;
            inBox = inBox && std::abs(p[1] - center[1]) <= halfOfSideLength;
            inBox = inBox && std::abs(p[2] - center[2]) <= halfOfSideLength; 
            
            return inBox; 
        } 
    };
    
    struct Node
    { 
        BoundingBox boundingBox; 
        std::array<std::unique_ptr<Node>, 8> children;
        std::shared_ptr<LeafBase> leaf;
        
        bool isLeafNode() const
        {
            bool isLeaf = true;
            
            for (const auto& child : children) 
            {
                isLeaf = isLeaf && child == nullptr;
            } 
            
            return isLeaf;
        } 
        
        bool hasPoint() const
        {
            return leaf != nullptr;
        }
    };

    BoundingBox computeBoundingBox(std::vector<std::shared_ptr<LeafBase>>& leafs);
    
    void insert(std::unique_ptr<Node>& node, std::shared_ptr<LeafBase>& leaf);
    
    void splitBoundingBox(std::unique_ptr<Node>& node);
    
    BoundingBox createChildBox(int index, const BoundingBox& parent); 
    
    inline int toChildIndex(const std::array<double, 3>& p, const BoundingBox& box) 
    {
        int ix = (p[0] >= box.center[0]) ? 1 : 0;
        int iy = (p[1] >= box.center[1]) ? 1 : 0;
        int iz = (p[2] >= box.center[2]) ? 1 : 0;
        
        return (ix) | (iy << 1) | (iz << 2);
    
    } 
    
    std::unique_ptr<Node> mRoot = std::make_unique<Node>(); 
    bool mSupportMultithread;
    size_t mMaxPointsPerNode;
};