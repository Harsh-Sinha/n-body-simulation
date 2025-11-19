#include <vector>
#include <memory>

#include "octree.h"

int main(int argc, char* argv[])
{
    std::vector<std::shared_ptr<Particle>> points = {std::make_shared<Particle>(0.0,0.0,0.0,0.0), std::make_shared<Particle>(1.0,1.0,1.0,1.0), std::make_shared<Particle>(2.0,2.0,2.0,2.0)};

    Octree tree(points, false);

    return 0;
}