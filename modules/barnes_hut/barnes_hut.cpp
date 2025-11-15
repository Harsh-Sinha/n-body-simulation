#include "barnes_hut.h"
#include "particle.h"

#include <cassert>

#include <omp.h>

BarnesHut::BarnesHut(std::vector<std::shared_ptr<Point3d>>& particles, double dt, 
                     double simulationLength, double softening)
    : mParticles(particles)
    , mDt(dt)
    , mSimulationLength(simulationLength)
    , mSoftening(softening)
    , mNumIterations(simulationLength / dt)
{}

void BarnesHut::simulate()
{
    for (size_t i = 0; i < mNumIterations; ++i)
    {
        Octree tree(mParticles, true);

        // calculate center of mass
        calculateCenterOfMass(tree.getLeafNodes());

        // apply forces

        // update pos/vel/acc
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
                // decide if I insert or another octant is inserting parent
                size_t myOctantId = Octree::toOctantId(workingSet[i]->points[0], workingSet[i]->parentNode->boundingBox);

                // place parent node in next working set if you have the lowest octant id
                bool place = false;
                for (size_t i = 0; i < 8; ++i)
                {
                    if (workingSet[i]->parentNode->octants[i])
                    {
                        place = i == myOctantId;
                        break;
                    }
                }

                if (place)
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