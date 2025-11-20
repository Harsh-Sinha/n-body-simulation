#include <vector>
#include <memory>

#include "octree.h"

int main(int argc, char* argv[])
{
    std::vector<Particle*> points = {new Particle(0.0,0.0,0.0,0.0), new Particle(1.0,1.0,1.0,1.0), new Particle(2.0,2.0,2.0,2.0)};

    Octree tree(points, false);

    delete points[0];
    delete points[1];
    delete points[2];

    return 0;
}