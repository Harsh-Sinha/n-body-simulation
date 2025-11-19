#include "barnes_hut.h"

#include <cmath>
#include <cassert>
#include <chrono>

#include <omp.h>

namespace
{

class ScopedTimer
{
public:
    ScopedTimer(double& out)
        : mStart(std::chrono::high_resolution_clock::now())
        , mOut(out)
    {}

    void recordElapsedMs()
    {
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed_ms = end - mStart;

        mOut = elapsed_ms.count();
    }

    ~ScopedTimer() = default;

private:
    ScopedTimer() = default;

    std::chrono::high_resolution_clock::time_point mStart;
    double& mOut;
};


#define COMBINE(a, b) a##b
#define PROFILE(sectionId, functionCall) \
    double COMBINE(elapsed_, sectionId) = 0.0; \
    ScopedTimer COMBINE(timer_, sectionId)(COMBINE(elapsed_, sectionId)); \
    functionCall; \
    COMBINE(timer_, sectionId).recordElapsedMs(); \
    if (mProfile) \
    { \
        mDataStore.addProfileData(sectionId, COMBINE(elapsed_, sectionId)); \
    } 
}

BarnesHut::BarnesHut(std::vector<std::shared_ptr<Particle>>& particles, double dt, 
                     double simulationLength, std::string& simulationName, bool profile)
    : mParticles(particles)
    , mDt(dt)
    , mSimulationLength(simulationLength)
    , mSimulationName(simulationName)
    , mProfile(profile)
    , mNumIterations(simulationLength / dt)
    , mDataStore(particles.size(), dt, mNumIterations)
{
    #pragma omp parallel for schedule(dynamic)
    for (const auto& particle : particles)
    {
        mDataStore.addMass(particle->mId, particle->mMass);
        mDataStore.addPosition(0, particle->mId, particle->mPosition);
    }
}

void BarnesHut::simulate()
{
    for (size_t i = 0; i < mNumIterations; ++i)
    {
        PROFILE(0, Octree tree(mParticles, true, 5000, 1));

        // calculate center of mass
        PROFILE(1, calculateCenterOfMass(tree.getLeafNodes()));

        // apply forces
        PROFILE(2, calculateForce(tree.getLeafNodes(), tree.getRootNode()));

        // update pos/vel/acc
        PROFILE(3, updateState(tree.getLeafNodes(), i));
    }

    std::string filename = mSimulationName + ".bin";
    mDataStore.writeToBinaryFile(filename);

    if (mProfile)
    {
        filename = mSimulationName + ".txt";
        mDataStore.writeProfileData(filename);
    }
}

void BarnesHut::calculateCenterOfMass(std::vector<std::shared_ptr<Octree::Node>>& leafs)
{
    std::vector<std::shared_ptr<Octree::Node>> workingSet;
    workingSet.insert(workingSet.end(), leafs.begin(), leafs.end());

    auto numThreads = omp_get_max_threads();
    std::vector<std::vector<std::shared_ptr<Octree::Node>>> localNextSet(numThreads);

    while (!workingSet.empty())
    {
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < workingSet.size(); ++i)
        {
            auto tid = omp_get_thread_num();
            
            // calculate center of mass for current node
            double x = 0;
            double y = 0;
            double z = 0;
            double totalMass = 0;
            bool ready = true;
            for (const auto& particle : workingSet[i]->points)
            {
                if (!particle)
                {
                    ready = false;
                    break;
                }

                const std::array<double, 3>& pos = particle->mPosition;

                x += pos[0] * particle->mMass;
                y += pos[1] * particle->mMass;
                z += pos[2] * particle->mMass;
                totalMass += particle->mMass;
            }

            if (!ready)
            {
                // place it back in worker queue to be processed later
                localNextSet[tid].emplace_back(workingSet[i]);
                continue;
            }

            x = x / totalMass;
            y = y / totalMass;
            z = z / totalMass;

            // interior nodes have a single point representing the center of mass (do NOT clear actual particles)
            if (!workingSet[i]->isLeafNode())
            {
                workingSet[i]->points.clear();
                workingSet[i]->com[0] = x;
                workingSet[i]->com[1] = y;
                workingSet[i]->com[2] = z;
                workingSet[i]->totalMass = totalMass;
                //workingSet[i]->points.emplace_back(particle);
            }

            if (workingSet[i]->parentNode)
            {
                size_t myOctantId = Octree::toOctantId(workingSet[i]->points[0], workingSet[i]->parentNode->boundingBox);

                // have to place this node in the parent node's points vector
                // I have to find out which index this octant maps to to place
                int flattenedIndex = -1;
                for (size_t j = 0; j < 8; ++j)
                {
                    if (workingSet[i]->parentNode->octants[j])
                    {
                        ++flattenedIndex;
                        if (j == myOctantId)
                        {
                            break;
                        }
                    }
                }

                // these locations have been preallocated in the octree
                workingSet[i]->parentNode->points[flattenedIndex] = std::make_shared<Particle>(x, y, z, totalMass);
 
                // smallest flattened index is always responsible for emplacing into local sets (avoid duplicates)
                if (flattenedIndex == 0)
                {
                    localNextSet[tid].emplace_back(workingSet[i]->parentNode);
                }
            }
        }

        workingSet.clear();
        for (auto& nextSet : localNextSet)
        {
            workingSet.insert(workingSet.end(), nextSet.begin(), nextSet.end());
            nextSet.clear();
        }
    }
}

void BarnesHut::calculateForce(std::vector<std::shared_ptr<Octree::Node>>& leafs, std::shared_ptr<Octree::Node>& root)
{
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < leafs.size(); ++i)
    {
        for (size_t j = 0; j < leafs[i]->points.size(); ++j)
        {
            calculateForce(leafs[i]->points[j], root);
        }
    }
}

void BarnesHut::calculateForce(std::shared_ptr<Particle>& particle, std::shared_ptr<Octree::Node>& node)
{
    if (!node->boundingBox.isPointInBox(particle) && isSufficientlyFar(particle, node))
    {
        if (node->isLeafNode())
        {
            // use all particles in this node to apply forces on particle
            for (auto& point : node->points)
            {
                particle->applyForce(point);
            }
        }
        else
        {
            // estimate all particles within this octant using computed center of mass
            particle->applyForce(node->com, node->totalMass);
        }
    }
    else
    {
        // we can be in this case for 2 reasons:
        // 1. particle is contained inside the node of the bounding box which
        // means we cannot make use of the total mass and center of mass estimate 
        // 2. node is not sufficiently far away so we cannot use the
        // center of mass and total mass of all children node of this
        // root encompasses all nodes so we cannot use it in the calculation
        bool isLeaf = true;
        for (auto& octant : node->octants)
        {
            if (octant)
            {
                isLeaf = false;
                calculateForce(particle, octant);
            }
        }

        if (isLeaf)
        {
            // calculate forces with all other particles in the leaf node

            // current particle will also be in this list
            for (auto& point : node->points)
            {
                if (point->mId != particle->mId)
                {
                    particle->applyForce(point);
                }
            }
        }
    }
}

bool BarnesHut::isSufficientlyFar(const std::shared_ptr<Particle>& particle, const std::shared_ptr<Octree::Node>& node)
{
    double s = node->boundingBox.halfOfSideLength * 2.0;

    auto& posA = particle->mPosition;
    auto& posB = node->com;
    double d = std::sqrt(std::pow(posA[0] - posB[0], 2) + std::pow(posA[1] - posB[1], 2) + std::pow(posA[2] - posB[2], 2));

    double quotient = s / d;

    static constexpr double THETA = 0.5;

    return quotient < THETA;
}

void BarnesHut::updateState(std::vector<std::shared_ptr<Octree::Node>>& leafs, size_t iteration)
{
    // +1 because index 0 is the initial state of the simulation in the data store
    auto& iterationStore = mDataStore.getIterationStore(iteration + 1);

    // precompute multiplication factors
    const double halfDt = 0.5 * mDt;
    const double halfDtSquared = halfDt * mDt;

    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < leafs.size(); ++i)
    {
        for (auto& particle : leafs[i]->points)
        {
            // perform leapfrog integration

            // x_{i+1} = x_i + v_i*dt + 0.5*a_i*dt^2
            particle->mPosition[0] += particle->mVelocity[0] * mDt + halfDtSquared * particle->mAcceleration[0];
            particle->mPosition[1] += particle->mVelocity[1] * mDt + halfDtSquared * particle->mAcceleration[1];
            particle->mPosition[2] += particle->mVelocity[2] * mDt + halfDtSquared * particle->mAcceleration[2];

            // a_{i+1} = F / m
            double inverseMass = 1.0 / particle->mMass;
            double axUpdated = particle->mAppliedForce[0] * inverseMass;
            double ayUpdated = particle->mAppliedForce[1] * inverseMass;
            double azUpdated = particle->mAppliedForce[2] * inverseMass;

            // v_{i+1} = v_i + 0.5*(a_i + a_{i+1})*dt
            particle->mVelocity[0] += halfDt * (particle->mAcceleration[0] + axUpdated);
            particle->mVelocity[1] += halfDt * (particle->mAcceleration[1] + ayUpdated);
            particle->mVelocity[2] += halfDt * (particle->mAcceleration[2] + azUpdated);

            particle->mAcceleration[0] = axUpdated;
            particle->mAcceleration[1] = ayUpdated;
            particle->mAcceleration[2] = azUpdated;

            iterationStore[particle->mId] = particle->mPosition;
        }
    }
}
