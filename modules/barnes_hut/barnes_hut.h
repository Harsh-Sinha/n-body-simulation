#pragma once

#include <vector>
#include <memory>

#include "octree.h"
#include "particle.h"
#include "data_store.h"

class BarnesHut
{
public:
    BarnesHut(std::vector<Particle*>& particles, double dt, 
              double simulationLength, std::string& simulationName, bool profile);
    
    ~BarnesHut() = default;

    void simulate();

private:
    BarnesHut() = default;

    void calculateCenterOfMass(std::vector<std::shared_ptr<Octree::Node>>& leafs);

    void calculateForce(std::vector<std::shared_ptr<Octree::Node>>& leafs, std::shared_ptr<Octree::Node>& root);

    void calculateForce(Particle*& particle, std::shared_ptr<Octree::Node>& node);

    bool isSufficientlyFar(Particle*& particle, const std::shared_ptr<Octree::Node>& node);

    void updateState(size_t iteration);

    std::vector<Particle*>& mParticles;
    double mDt;
    double mSimulationLength;
    std::string mSimulationName;
    bool mProfile;
    size_t mNumIterations;
    DataStore mDataStore;
};