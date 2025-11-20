// tests/test_octree.cpp

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

// expose internals for testing
#define private public
#define protected public
#include "octree.h"
#undef private
#undef protected

#include <memory>
#include <vector>
#include <cmath>
#include <filesystem>

#include "particle_config.hpp"

static std::filesystem::path base()
{
    return std::filesystem::canonical(std::filesystem::path(__FILE__)).parent_path();
}

static Particle* makePoint(double x, double y, double z)
{
    return new Particle(x, y, z, 0.0);
}

static void validateLeafNodesList(const Octree& tree, const size_t expectedPoints)
{
    size_t numPoints = 0;
    for (const auto& leaf : tree.mLeafNodes)
    {
        numPoints += leaf->points.size();
    }

    REQUIRE(numPoints == expectedPoints);
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

    std::size_t count = 0;

    if (node->isLeafNode())
    {
        count += node->points.size();
    }
    
    for (const auto& octant : node->octants)
    {
        count += countPointsInTree(octant);
    }
    
    return count;
}

static void assertChildInsideParent(const Octree::BoundingBox& parent,
                                    const Octree::BoundingBox& child)
{
    auto childCenter = std::make_shared<Particle>(child.center[0], child.center[1], child.center[2], 0.0);
    auto* p = childCenter.get();
    REQUIRE(parent.isPointInBox(p));

    REQUIRE(child.halfOfSideLength == Catch::Approx(0.5 * parent.halfOfSideLength));
}

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

    if (node->isLeafNode())
    {
        for (auto* p : node->points)
        {
            REQUIRE(node->boundingBox.isPointInBox(p));
        }
    }

    size_t numChildren = 0;
    for (const auto& child : node->octants)
    {
        if (child)
        {
            ++numChildren;
            assertChildInsideParent(node->boundingBox, child->boundingBox);
            validateNodeRecursive(child, maxPointsPerNode, node);
        }
    }

    if (numChildren > 0)
    {
        REQUIRE(node->points.size() == numChildren);
    }
    else
    {
        REQUIRE(node->points.size() >= 1);
        REQUIRE(node->points.size() <= maxPointsPerNode);
    }
}

TEST_CASE("Bounding box is computed correctly for simple cube")
{
    std::vector<Particle*> pts;
    pts.push_back(makePoint(0.0, 0.0, 0.0));
    pts.push_back(makePoint(1.0, 1.0, 1.0));

    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    REQUIRE(root != nullptr);

    auto box = root->boundingBox;

    REQUIRE(box.center[0] == Catch::Approx(0.5));
    REQUIRE(box.center[1] == Catch::Approx(0.5));
    REQUIRE(box.center[2] == Catch::Approx(0.5));

    REQUIRE(box.halfOfSideLength == Catch::Approx(0.5005).epsilon(1e-6));

    for (auto* p : pts)
    {
        delete p;
    }
}

TEST_CASE("Node reports leaf status correctly")
{
    std::vector<Particle*> pts{ makePoint(0,0,0) };
    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    REQUIRE(root->isLeafNode());

    root->octants[0] = std::make_shared<Octree::Node>();
    REQUIRE_FALSE(root->isLeafNode());

    delete pts[0];
}

TEST_CASE("BoundingBox::isPointInBox respects padding")
{
    std::vector<Particle*> pts{
        makePoint(-1, -1, -1),
        makePoint( 1,  1,  1)
    };
    Octree tree(pts, false, 5);
    auto box = tree.mRoot->boundingBox;

    auto onEdge = makePoint(1, 1, 1);
    REQUIRE(box.isPointInBox(onEdge));

    delete pts[0];
    delete pts[1];
    delete onEdge;
}

TEST_CASE("toOctantId assigns all 8 octants correctly")
{
    std::vector<Particle*> pts{
        makePoint(-1, -1, -1),
        makePoint( 1,  1,  1)
    };
    Octree tree(pts, false, 5);
    auto root = tree.mRoot;
    auto box = root->boundingBox;

    // (+,+,+) -> 0
    auto* temp = makePoint( 1,  1,  1);
    REQUIRE(tree.toOctantId(temp, box) == 0);
    delete temp;
    // (-,+,+) -> 1
    temp = makePoint(-1,  1,  1);
    REQUIRE(tree.toOctantId(temp, box) == 1);
    delete temp;
    // (-,-,+) -> 2
    temp = makePoint(-1,  -1,  1);
    REQUIRE(tree.toOctantId(temp, box) == 2);
    delete temp;
    // (+,-,+) -> 3
    temp = makePoint(1,  -1,  1);
    REQUIRE(tree.toOctantId(temp, box) == 3);
    delete temp;
    // (+,+,-) -> 4
    temp = makePoint(1, 1, -1);
    REQUIRE(tree.toOctantId(temp, box) == 4);
    delete temp;
    // (-,+,-) -> 5
    temp = makePoint(-1, 1, -1);
    REQUIRE(tree.toOctantId(temp, box) == 5);
    delete temp;
    // (-,-,-) -> 6
    temp = makePoint(-1, -1, -1);
    REQUIRE(tree.toOctantId(temp, box) == 6);
    delete temp;
    // (+,-,-) -> 7
    temp = makePoint(1,  -1,  -1);
    REQUIRE(tree.toOctantId(temp, box) == 7);
    delete temp;

    delete pts[0];
    delete pts[1];
}

TEST_CASE("getCorrespondingOctant lazily creates child and sets parent")
{
    std::vector<Particle*> pts{ makePoint(0,0,0), makePoint(1,1,1) };
    Octree tree(pts, false, 5);

    auto root = tree.mRoot;
    auto p = makePoint(1,1,1);

    auto& childRef = tree.getCorrespondingOctant(p, root);
    REQUIRE(childRef != nullptr);
    REQUIRE(childRef->parentNode == root);

    // calling again for same point should give same node, not a new one
    auto& childRef2 = tree.getCorrespondingOctant(p, root);
    REQUIRE(childRef2 == childRef);

    delete pts[0];
    delete pts[1];
    delete p;
}

TEST_CASE("Insert splits node when maxPointsPerNode is small")
{
    std::vector<Particle*> pts{
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

    validateNodeRecursive(root, 1);

    size_t nonNullCount = 0;
    for (const auto& oct : root->octants)
    {
        if (oct != nullptr)
        {
            ++nonNullCount;
            REQUIRE(oct->points.size() == 1);
            REQUIRE(oct->isLeafNode());
        }
    }
    REQUIRE(nonNullCount == 8);

    for (auto* p : pts)
    {
        delete p;
    }
}

TEST_CASE("Octree should handle empty point sets")
{
    std::vector<Particle*> empty;

    REQUIRE_THROWS(Octree(empty));
}

TEST_CASE("Large octree (≈500 pts) forms a valid spatial subdivision")
{
    // build a 3D grid of points in [-1, 1]^3
    // 8 * 8 * 8 = 512; we can take 500 of them
    std::vector<Particle*> pts;
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

    validateLeafNodesList(tree, totalInTree);

    int depth = computeMaxDepth(root);
    REQUIRE(depth > 0);
    REQUIRE(depth < 20);

    for (auto* p : pts)
    {
        REQUIRE(root->boundingBox.isPointInBox(p));
    }

    for (auto* p : pts)
    {
        delete p;
    }
}

TEST_CASE("Octree handles highly clustered points plus distant outliers")
{
    std::vector<Particle*> pts;
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

    validateLeafNodesList(tree, total);

    for (auto* p : pts)
    {
        REQUIRE(root->boundingBox.isPointInBox(p));
    }

    int depth = computeMaxDepth(root);
    REQUIRE(depth >= 3);
    REQUIRE(depth < 25);

    for (auto* p : pts)
    {
        delete p;
    }
}

TEST_CASE("Parallel Octree generation with large input size")
{
    std::filesystem::path file = base() / "inputs" / "test_particle_config_parallel_tree.txt";
    auto particles = ParticleConfig::parse(file.string());

    std::vector<Particle*> pts;
    for (const auto& particle : particles)
    {
        pts.emplace_back(new Particle(particle.position[0], particle.position[1], particle.position[2], 0.0));
    }

    Octree tree(pts, true);
    auto root = tree.mRoot;
    REQUIRE(root != nullptr);

    validateNodeRecursive(root, tree.mMaxPointsPerNode);

    auto total = countPointsInTree(root);
    REQUIRE(total == pts.size());

    validateLeafNodesList(tree, total);

    for (auto* p : pts)
    {
        REQUIRE(root->boundingBox.isPointInBox(p));
    }

    for (auto* p : pts)
    {
        delete p;
    }
}