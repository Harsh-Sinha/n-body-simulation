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

    void calculateForce(std::vector<std::shared_ptr<Octree::Node>>& leafs, std::shared_ptr<Octree::Node>& root);

    void calculateForce(std::shared_ptr<Particle>& particle, std::shared_ptr<Octree::Node>& node);

    void calculateForce(std::shared_ptr<Particle>& particle, std::shared_ptr<Point3d> node);

    bool isSufficientlyFar(const std::shared_ptr<Particle>& particle, const std::shared_ptr<Octree::Node>& node);

    std::vector<std::shared_ptr<Point3d>>& mParticles;
    double mDt;
    double mSimulationLength;
    double mSoftening;
    size_t mNumIterations;
};