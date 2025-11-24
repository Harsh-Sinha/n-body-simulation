#pragma once

#include <vector>

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

    void calculateCenterOfMass(std::vector<Octree::Node*>& leafs);

    void calculateForce(std::vector<Octree::Node*>& leafs, Octree::Node*& root);

    void calculateForce(Particle*& particle, Octree::Node*& node);

    bool isSufficientlyFar(Particle*& particle, Octree::Node*& node);

    void updateState(size_t iteration);

    std::vector<Particle*>& mParticles;
    double mDt;
    double mSimulationLength;
    std::string mSimulationName;
    bool mProfile;
    size_t mNumIterations;
    DataStore mDataStore;
};