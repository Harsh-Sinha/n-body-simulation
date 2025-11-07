#include <iostream>
#include <array>
#include <vector>

#include "octree.h"

struct ExtendedPoint3D
{
    double x;
    double y;
    double z;
    double vx;
    double vy;
    double vz;
    double mass;
};
using ExtendedBox3D = std::array<ExtendedPoint3D, 2>;
using ExtendedRay3D = std::array<ExtendedPoint3D, 2>;
struct ExtendedPlane3D
{
    ExtendedPoint3D normal;
    double origoDistance; // distance from coordinate system center
        
};

struct ExtendedPoint3DAdaptorDef
{
    static double GetPointC(ExtendedPoint3D const& point, OrthoTree::dim_t i)
    {
        switch (i)
        {
        case 0: return point.x;
        case 1: return point.y;
        case 2: return point.z;
        default: assert(false); return point.x;
        }
    }

    static void SetPointC(ExtendedPoint3D point, OrthoTree::dim_t i, double val)
    {
        switch (i)
        {
        case 0: point.x = val; break;
        case 1: point.y = val; break;
        case 2: point.z = val; break;
        default: assert(false);
        }
    }

    static void SetBoxMinC(ExtendedBox3D& box, OrthoTree::dim_t i, double val) 
    { 
        SetPointC(box[0], i, val); 
    }
    static void SetBoxMaxC(ExtendedBox3D& box, OrthoTree::dim_t i, double val)
    {
        SetPointC(box[1], i, val);
    }
    static double GetBoxMinC(ExtendedBox3D const& box, OrthoTree::dim_t i)
    {
        return GetPointC(box[0], i);
    }
    static double GetBoxMaxC(ExtendedBox3D const& box, OrthoTree::dim_t i)
    {
        return GetPointC(box[1], i);
    }

    static ExtendedPoint3D const& GetRayDirection(ExtendedRay3D const& ray)
    {
        return ray[1];
    }
    static ExtendedPoint3D const& GetRayOrigin(ExtendedRay3D const& ray)
    {
        return ray[0];
    }

    static ExtendedPoint3D const& GetPlaneNormal(ExtendedPlane3D const& plane)
    {
        return plane.normal;
    }
    static double GetPlaneOrigoDistance(ExtendedPlane3D const& plane)
    {
        return plane.origoDistance;
    }
};

using ExtendedPoint3DAdaptor = OrthoTree::AdaptorGeneralBase<
    3,
    ExtendedPoint3D,
    ExtendedBox3D,
    ExtendedRay3D,
    ExtendedPlane3D,
    double,
    ExtendedPoint3DAdaptorDef>;

using OctreeExtendedPoint = OrthoTree::OrthoTreePoint<
    3,
    ExtendedPoint3D,
    ExtendedBox3D,
    ExtendedRay3D,
    ExtendedPlane3D,
    double,
    ExtendedPoint3DAdaptor>;

using OctreeExtendedBox = OrthoTree::OrthoTreeBoundingBox<
    3,
    ExtendedPoint3D,
    ExtendedBox3D,
    ExtendedRay3D,
    ExtendedPlane3D,
    double,
    true,
    ExtendedPoint3DAdaptor>;

int main(int argc, char* argv[])
{
    std::vector<ExtendedPoint3D> points = {{{1,1,1,0,0,0,0}, {0,0,0,0,0,0,0}, {2,2,2,0,0,0,0}}};
    auto octree = OctreeExtendedPoint(OrthoTree::PAR_EXEC, points, 20 /*max depth*/, std::nullopt, 1);

    auto dfs = octree.CollectAllEntitiesInDFS();

    std::cout << "DFS traversal: ";
    for (const auto& id : dfs)
    {
        std::cout << id << ", ";
    }
    std::cout << std::endl;


    return 0;
}