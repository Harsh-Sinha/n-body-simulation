// tests/test_octree.cpp

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

// expose internals for testing
#define private public
#define protected public
#include "octree.h"
#undef private
#undef protected

#include <memory>
#include <vector>
#include <cmath>


static std::shared_ptr<Point3d> makePoint(double x, double y, double z)
{
    return std::make_shared<Point3d>(x, y, z);
}

static int computeMaxDepth(const std::shared_ptr<Octree::Node>& node)
{
    if (!node)
    {
        return 0;
    }
    
    int maxChild = 0;
    
    for (const auto& octant : node->octants)
    {
        if (octant)
        {
            maxChild = std::max(maxChild, computeMaxDepth(octant));
        }
    }

    return 1 + maxChild;
}

static std::size_t countPointsInTree(const std::shared_ptr<Octree::Node>& node)
{
    if (!node)
    {
        return 0;
    }

    std::size_t count = node->points.size();
    
    for (const auto& octant : node->octants)
    {
        count += countPointsInTree(octant);
    }
    
    return count;
}

// check that child's bbox is spatially inside parent's bbox
static void assertChildInsideParent(const Octree::BoundingBox& parent,
                                    const Octree::BoundingBox& child)
{
    // child center must be within parent's box
    auto dummy = std::make_shared<Point3d>(child.center[0], child.center[1], child.center[2]);
    REQUIRE(parent.isPointInBox(dummy));

    // child must not be bigger than parent
    REQUIRE(child.halfOfSideLength <= parent.halfOfSideLength);
}

// traverse and check per-node invariants
static void validateNodeRecursive(const std::shared_ptr<Octree::Node>& node,
                                  std::size_t maxPointsPerNode,
                                  const std::shared_ptr<Octree::Node>& expectedParent = nullptr)
{
    REQUIRE(node != nullptr);

    if (expectedParent == nullptr)
    {
        // root
        REQUIRE(node->parentNode == nullptr);
    } 
    else 
    {
        REQUIRE(node->parentNode == expectedParent);
    }

    REQUIRE(node->boundingBox.halfOfSideLength > 0.0);

    for (const auto& p : node->points)
    {
        REQUIRE(node->boundingBox.isPointInBox(p));
    }

    bool hasChildren = false;
    for (const auto& child : node->octants)
    {
        if (child)
        {
            hasChildren = true;
            assertChildInsideParent(node->boundingBox, child->boundingBox);
            validateNodeRecursive(child, maxPointsPerNode, node);
        }
    }

    if (hasChildren)
    {
        REQUIRE(node->points.empty());
    }
    else
    {
        REQUIRE(node->points.size() >= 1);
        REQUIRE(node->points.size() <= maxPointsPerNode);
    }
}

TEST_CASE("Bounding box is computed correctly for simple cube")
{
    std::vector<std::shared_ptr<Point3d>> pts;
    pts.push_back(makePoint(0.0, 0.0, 0.0));
    pts.push_back(makePoint(1.0, 1.0, 1.0));

    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    REQUIRE(root != nullptr);

    auto box = root->boundingBox;

    REQUIRE(box.center[0] == Approx(0.5));
    REQUIRE(box.center[1] == Approx(0.5));
    REQUIRE(box.center[2] == Approx(0.5));

    REQUIRE(box.halfOfSideLength == Approx(0.5005).epsilon(1e-6));
}

TEST_CASE("Node reports leaf status correctly")
{
    std::vector<std::shared_ptr<Point3d>> pts{ makePoint(0,0,0) };
    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    REQUIRE(root->isLeafNode());

    root->octants[0] = std::make_shared<Octree::Node>();
    REQUIRE_FALSE(root->isLeafNode());
}

TEST_CASE("BoundingBox::isPointInBox respects padding")
{
    std::vector<std::shared_ptr<Point3d>> pts{
        makePoint(-1, -1, -1),
        makePoint( 1,  1,  1)
    };
    Octree tree(pts, false, 5);
    auto box = tree.mRoot->boundingBox;

    auto onEdge = makePoint(1, 1, 1);
    REQUIRE(box.isPointInBox(onEdge));
}

TESTCASE("toOctantId assigns all 8 octants correctly")
{
    std::vector<std::shared_ptr<Point3d>> pts{
        makePoint(-1, -1, -1),
        makePoint( 1,  1,  1)
    };
    Octree tree(pts, false, 5);
    auto root = tree.mRoot;
    auto box = root->boundingBox;

    // (+,+,+) -> 0
    REQUIRE(tree.toOctantId(makePoint( 1,  1,  1), box) == 0);
    // (-,+,+) -> 1
    REQUIRE(tree.toOctantId(makePoint(-1,  1,  1), box) == 1);
    // (-,-,+) -> 2
    REQUIRE(tree.toOctantId(makePoint(-1, -1,  1), box) == 2);
    // (+,-,+) -> 3
    REQUIRE(tree.toOctantId(makePoint( 1, -1,  1), box) == 3);
    // (+,+,-) -> 4
    REQUIRE(tree.toOctantId(makePoint( 1,  1, -1), box) == 4);
    // (-,+,-) -> 5
    REQUIRE(tree.toOctantId(makePoint(-1,  1, -1), box) == 5);
    // (-,-,-) -> 6
    REQUIRE(tree.toOctantId(makePoint(-1, -1, -1), box) == 6);
    // (+,-,-) -> 7
    REQUIRE(tree.toOctantId(makePoint( 1, -1, -1), box) == 7);
}

TEST_CASE("getCorrespondingOctant lazily creates child and sets parent")
{
    std::vector<std::shared_ptr<Point3d>> pts{ makePoint(0,0,0), makePoint(1,1,1) };
    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    auto p = makePoint(1,1,1);

    auto &childRef = tree.getCorrespondingOctant(p, root);
    REQUIRE(childRef != nullptr);
    REQUIRE(childRef->parentNode == root);

    // calling again for same point should give same node, not a new one
    auto &childRef2 = tree.getCorrespondingOctant(p, root);
    REQUIRE(childRef2 == childRef);
}

TEST_CASE("createChildBox produces correct centers for all 8 children")
{
    std::vector<std::shared_ptr<Point3d>> pts{
        makePoint(-1, -1, -1),
        makePoint( 1,  1,  1)
    };
    Octree tree(pts, false, 5);
    auto parent = tree.mRoot->boundingBox;

    for (size_t i = 0; i < 8; ++i) {
        auto child = tree.createChildBox(i, parent);

        REQUIRE(child.halfOfSideLength == Approx(parent.halfOfSideLength / 2.0));
    }
}

TEST_CASE("Insert splits node when maxPointsPerNode is small")
{
    std::vector<std::shared_ptr<Point3d>> pts{
        makePoint( 1,  1,  1),
        makePoint(-1,  1,  1),
        makePoint(-1, -1,  1),
        makePoint( 1, -1,  1),
        makePoint( 1,  1, -1),
        makePoint(-1,  1, -1),
        makePoint(-1, -1, -1),
        makePoint( 1, -1, -1),
    };

    Octree tree(pts, false, 1);

    auto root = tree.mRoot;
    REQUIRE_FALSE(root->isLeafNode());
    REQUIRE(root->points.empty());

    size_t nonNullCount = 0;
    for (const auto &oct : root->octants) {
        if (oct != nullptr) {
            ++nonNullCount;
            REQUIRE(oct->points.size() == 1);
            REQUIRE(oct->isLeafNode());
        }
    }
    REQUIRE(nonNullCount == 8);
}

TEST_CASE("splitBoundingBox explicitly creates 8 children")
{
    std::vector<std::shared_ptr<Point3d>> pts{ makePoint(0,0,0) };
    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    tree.splitBoundingBox(root);

    for (const auto &oct : root->octants) {
        REQUIRE(oct != nullptr);
        REQUIRE(oct->boundingBox.halfOfSideLength > 0.0);
    }
}

TEST_CASE("Octree should handle empty point sets")
{
    std::vector<std::shared_ptr<Point3d>> empty;

    REQUIRE_THROWS(Octree(empty));
}

TEST_CASE("Large octree (≈500 pts) forms a valid spatial subdivision")
{
    // build a 3D grid of points in [-1, 1]^3
    // 8 * 8 * 8 = 512; we can take 500 of them
    std::vector<std::shared_ptr<Point3d>> pts;
    pts.reserve(500);

    int added = 0;
    for (int ix = 0; ix < 8 && added < 500; ++ix)
    {
        for (int iy = 0; iy < 8 && added < 500; ++iy)
        {
            for (int iz = 0; iz < 8 && added < 500; ++iz)
            {
                // map 0..7 to [-1,1]
                double x = -1.0 + (2.0 * ix) / 7.0;
                double y = -1.0 + (2.0 * iy) / 7.0;
                double z = -1.0 + (2.0 * iz) / 7.0;
                pts.push_back(makePoint(x, y, z));
                ++added;
            }
        }
    }

    const std::size_t capacity = 4;
    Octree tree(pts, false, capacity);

    auto root = tree.mRoot;
    REQUIRE(root != nullptr);

    validateNodeRecursive(root, capacity);

    auto totalInTree = countPointsInTree(root);
    REQUIRE(totalInTree == pts.size());

    int depth = computeMaxDepth(root);
    REQUIRE(depth > 0);
    REQUIRE(depth < 20);

    for (const auto &p : pts)
    {
        REQUIRE(root->boundingBox.isPointInBox(p));
    }
}

TEST_CASE("Octree handles highly clustered points plus distant outliers") {
    std::vector<std::shared_ptr<Point3d>> pts;
    pts.reserve(500);

    // big cluster of points near origin =450 points in a tiny cube around (0,0,0)
    for (int i = 0; i < 450; ++i)
    {
        double x = (i % 10) * 0.0005;          // 0 .. 0.0045
        double y = ((i / 10) % 10) * 0.0005;
        double z = (i / 100) * 0.0005;
        pts.push_back(makePoint(x, y, z));
    }

    // the rest are far away points to stretch the bounding box
    pts.push_back(makePoint(10.0, 10.0, 10.0));
    pts.push_back(makePoint(-10.0, 10.0, 10.0));
    pts.push_back(makePoint(10.0, -10.0, 10.0));
    pts.push_back(makePoint(10.0, 10.0, -10.0));
    pts.push_back(makePoint(-10.0, -10.0, -10.0));
    pts.push_back(makePoint(8.0, -9.0, 7.5));
    pts.push_back(makePoint(-7.0, 6.5, -9.5));
    while (pts.size() < 500) 
    {
        pts.push_back(makePoint(0.001 * (pts.size() % 5),
                                0.001 * ((pts.size() / 5) % 5),
                                0.001 * ((pts.size() / 25) % 5)));
    }

    const std::size_t capacity = 4;
    Octree tree(pts, false, capacity);
    auto root = tree.mRoot;
    REQUIRE(root != nullptr);

    validateNodeRecursive(root, capacity);

    auto total = countPointsInTree(root);
    REQUIRE(total == pts.size());

    for (const auto& p : pts)
    {
        REQUIRE(root->boundingBox.isPointInBox(p));
    }

    int depth = computeMaxDepth(root);
    REQUIRE(depth >= 3);
    REQUIRE(depth < 25);
}