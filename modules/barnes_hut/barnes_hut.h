#pragma once

#include <vector>
#include <memory>

#include "octree.h"
#include "point3d.h"

class BarnesHut
{
public:
    BarnesHut(std::vector<std::shared_ptr<Point3d>>& particles, double dt, 
              double simulationLength, double softening);
    
    ~BarnesHut() = default;

    void simulate();

private:
    BarnesHut() = default;

    void calculateCenterOfMass(std::vector<std::shared_ptr<Octree::Node>>& leafs);

    std::vector<std::shared_ptr<Point3d>>& mParticles;
    double mDt;
    double mSimulationLength;
    double mSoftening;
    size_t mNumIterations;
};