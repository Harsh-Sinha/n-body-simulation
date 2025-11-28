#include <iostream>
#include <memory>
#include <chrono>
#include <omp.h>
#include <iomanip>
#include <algorithm>

#include "octree.h"
#include "particle_config.hpp"

std::vector<Particle*> createParticles(size_t numParticles)
{
    ParticleConfig::Limits limits;
    limits.boundingBox          = {{ {-500.0, -500.0, -500.0},
                                     {500.0, 500.0, 500.0} }};
    limits.velocityLimits       = { 10.0, 20.0 };
    limits.accelerationLimits   = { 1.0, 10.0 };
    limits.massLimits           = { 40.0, 70.0 };

    auto generated = ParticleConfig::generate(numParticles, limits);

    std::vector<Particle*> particles;
    for (const auto& particle : generated)
    {
        particles.emplace_back(new Particle(particle));
    }

    return particles;
}

void deleteParticles(std::vector<Particle*>& particles)
{
    for (auto*& particle : particles)
    {
        delete particle;
    }
}

Octree::Node* createNode(std::vector<Particle*>& particles)
{
    Octree::Node* node = new Octree::Node();

    node->parentNode = nullptr;
    node->boundingBox = Octree::computeBoundingBox(particles);

    node->points.insert(node->points.end(), particles.begin(), particles.end());

    return node;
}

void deleteNode(Octree::Node* node)
{
    // if (node == nullptr) return;

    // for (auto*& octant : node->octants)
    // {
    //     if (octant)
    //     {
    //         deleteNode(octant);
    //         delete octant;
    //     }
    // }

    // delete node;
}

template <class F>
double benchmark(F&& function)
{
    auto start = std::chrono::steady_clock::now();
    function();
    auto end = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> elapsed = end - start;

    return elapsed.count();
}

struct Result
{
    size_t numParticles;
    double serialMs;
    double insertParallelMs;
    double partitionNodeMs;
};

int main()
{
    int maxThreads = omp_get_max_threads();
    std::cout << "benchmarking with " << maxThreads << "\n";

    static constexpr size_t MAX_POINTS_PER_NODE = 1;
    static constexpr size_t THRESHOLD_FOR_SERIAL = 2;

    {
        std::vector<Particle*> dummyPts;
        dummyPts.push_back(new Particle(0.0, 0.0, 0.0, 1.0));

        Octree dummyTree(dummyPts,
                         false,
                         THRESHOLD_FOR_SERIAL,
                         MAX_POINTS_PER_NODE);


        Octree& tree = dummyTree;

        const std::vector<std::size_t> testSizes = {
            1000,
            2000,
            5000,
            10000,
            20000,
            50000,
            100000,
            200000,
            500000,
            1000000
        };

        const int repetitions = 5;

        std::cout << std::setw(8)  << "Num particles"
                  << std::setw(14) << "serial(ms)"
                  << std::setw(16) << "insertPar(ms)"
                  << std::setw(16) << "partition(ms)"
                  << std::setw(10) << "best"
                  << "\n";

        std::cout << std::string(8+14+16+16+10, '-') << "\n";

        for (size_t size : testSizes)
        {
            auto particles = createParticles(size);

            double serialSum = 0.0;
            double insertParallelSum = 0.0;
            double partitionNodeSum = 0.0;

            for (int rep = 0; rep < repetitions; ++rep)
            {
                {
                    Octree::Node* node = createNode(particles);

                    double t = benchmark([&]() {
                        std::vector<Particle*> temp;
                        temp.swap(node->points);
                        for (auto* p : temp)
                        {
                            tree.insert(node, p);
                        }
                    });

                    serialSum += t;
                    deleteNode(node);
                }

                {
                    Octree::Node* node = createNode(particles);

                    double t = benchmark([&]() {
                        #pragma omp parallel
                        {
                            #pragma omp single
                            {
                                tree.insertParallel(node);
                            }
                        }
                    });

                    insertParallelSum += t;
                    deleteNode(node);
                }

                {
                    Octree::Node* node = createNode(particles);

                    double t = benchmark([&]() {
                        tree.partitionPointsInNode(node);
                    });

                    partitionNodeSum += t;
                    deleteNode(node);
                }
            }

            double serialAvg = serialSum / repetitions;
            double insertParallelAvg = insertParallelSum / repetitions;
            double partitionNodeAvg   = partitionNodeSum / repetitions;

            double minVal = std::min({serialAvg, insertParallelAvg, partitionNodeAvg});
            const char* best =
                (minVal == serialAvg) ? "serial" :
                (minVal == insertParallelAvg) ? "insertPar" :
                                        "partition";

            std::cout << std::setw(8)  << size
                      << std::setw(14) << std::fixed << std::setprecision(3) << serialAvg
                      << std::setw(16) << std::fixed << std::setprecision(3) << insertParallelAvg
                      << std::setw(16) << std::fixed << std::setprecision(3) << partitionNodeAvg
                      << std::setw(10) << best
                      << "\n";

            deleteParticles(particles);
        }
    }

    return 0;
}