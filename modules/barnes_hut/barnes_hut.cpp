#include "barnes_hut.h"

#include <cmath>
#include <cassert>

#include <omp.h>

BarnesHut::BarnesHut(std::vector<std::shared_ptr<Point3d>>& particles, double dt, 
                     double simulationLength, double softening)
    : mParticles(particles)
    , mDt(dt)
    , mSimulationLength(simulationLength)
    , mSoftening(softening)
    , mNumIterations(simulationLength / dt)
    , mDataStore(particles.size(), dt, mNumIterations)
{
    #pragma omp parallel for schedule(dynamic)
    for (const auto& point : particles)
    {
        auto particle = std::dynamic_pointer_cast<Particle>(point);

        assert(particle);

        mDataStore.addMass(particle->mId, particle->mMass);
    }
}

void BarnesHut::simulate()
{
    for (size_t i = 0; i < mNumIterations; ++i)
    {
        Octree tree(mParticles, true);

        // calculate center of mass
        calculateCenterOfMass(tree.getLeafNodes());

        // apply forces
        calculateForce(tree.getLeafNodes(), tree.getRootNode());

        // update pos/vel/acc
        updateState(tree.getLeafNodes(), i);
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
            for (const auto& point : workingSet[i]->points)
            {
                auto particle = std::dynamic_pointer_cast<Particle>(point);

                assert(particle);

                auto& pos = particle->getPosition();

                x += pos[0] * particle->mMass;
                y += pos[1] * particle->mMass;
                z += pos[2] * particle->mMass;
                totalMass += particle->mMass;
            }

            x = x / totalMass;
            y = y / totalMass;
            z = z / totalMass;

            // interior nodes have a single point representing the center of mass (do NOT clear actual particles)
            if (!workingSet[i]->isLeafNode())
            {
                workingSet[i]->points.clear();
                workingSet[i]->points.emplace_back(std::make_shared<Particle>(x, y, z, totalMass));
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
                workingSet[i]->parentNode->points[flattenedIndex] = workingSet[i]->points[0];
 
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
            auto particle = std::dynamic_pointer_cast<Particle>(leafs[i]->points[j]);

            assert(particle);

            calculateForce(particle, root);
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
                auto temp = std::dynamic_pointer_cast<Particle>(point);

                assert(temp);

                particle->applyForce(temp);
            }
        }
        else
        {
            // estimate all particles within this octant using computed center of mass
            auto temp = std::dynamic_pointer_cast<Particle>(node->points[0]);

            assert(temp);

            particle->applyForce(temp);
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
                auto temp = std::dynamic_pointer_cast<Particle>(point);

                assert(temp);
                
                if (temp->mId != particle->mId)
                {
                    particle->applyForce(temp);
                }
            }
        }
    }
}

bool BarnesHut::isSufficientlyFar(const std::shared_ptr<Particle>& particle, const std::shared_ptr<Octree::Node>& node)
{
    double s = node->boundingBox.halfOfSideLength * 2.0;

    auto& posA = particle->getPosition();
    auto& posB = node->points[0]->getPosition();
    double d = std::sqrt(std::pow(posA[0] - posB[0], 2) + std::pow(posA[1] - posB[1], 2) + std::pow(posA[2] - posB[2], 2));

    double quotient = s / d;

    static constexpr double THETA = 0.5;

    return quotient < THETA;
}

void BarnesHut::updateState(std::vector<std::shared_ptr<Octree::Node>>& leafs, size_t iteration)
{
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < leafs.size(); ++i)
    {
        for (auto& point : leafs[i]->points)
        {
            auto particle = std::dynamic_pointer_cast<Particle>(point);

            assert(particle);

            // update position based on last iteration of velocity and acceleration
            auto& pos = particle->getPosition();
            pos[0] += particle->mVelocity[0] * mDt + 0.5 * particle->mAcceleration[0] * mDt * mDt;
            pos[1] += particle->mVelocity[1] * mDt + 0.5 * particle->mAcceleration[1] * mDt * mDt;
            pos[2] += particle->mVelocity[2] * mDt + 0.5 * particle->mAcceleration[2] * mDt * mDt;

            std::array<double, 3> prevAcceleration = particle->mAcceleration;

            particle->mAcceleration[0] = particle->mAppliedForce[0] / particle->mMass;
            particle->mAcceleration[1] = particle->mAppliedForce[1] / particle->mMass;
            particle->mAcceleration[2] = particle->mAppliedForce[2] / particle->mMass;

            particle->mVelocity[0] += 0.5 * (prevAcceleration[0] + particle->mAcceleration[0]) * mDt;
            particle->mVelocity[1] += 0.5 * (prevAcceleration[1] + particle->mAcceleration[1]) * mDt;
            particle->mVelocity[2] += 0.5 * (prevAcceleration[2] + particle->mAcceleration[2]) * mDt;

            particle->mAppliedForce.fill(0.0);

            mDataStore.addPosition(iteration, particle->mId, pos);
        }
    }
}