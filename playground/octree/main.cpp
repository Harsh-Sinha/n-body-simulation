#include <vector>
#include <memory>

#include "octree.h"

int main(int argc, char* argv[])
{
    std::vector<std::shared_ptr<LeafBase>> points = {std::make_shared<LeafBase>(0.0,0.0,0.0), std::make_shared<LeafBase>(1.0,1.0,1.0), std::make_shared<LeafBase>(2.0,2.0,2.0)};

    Octree tree(points, false);

    return 0;
}