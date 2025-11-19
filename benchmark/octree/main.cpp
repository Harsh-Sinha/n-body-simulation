#include <chrono>
#include <iostream>
#include <string>

#include "octree.h"
#include "particle_config.hpp"

class ScopedTimer
{
public:
    ScopedTimer(double& out)
        : mStart(std::chrono::high_resolution_clock::now())
        , mOut(out)
    {}

    ~ScopedTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = end - mStart;

        mOut = diff.count();
    }

private:
    std::chrono::high_resolution_clock::time_point mStart;
    double& mOut;
};

double benchmark(std::vector<std::shared_ptr<Particle>>& points, bool parallel, size_t threshold, size_t iterations)
{
    double cumulative = 0.0;
    for (int i = 0; i < iterations; ++i)
    {
        double time_ms = 0.0;
        {
            ScopedTimer timer(time_ms);
            Octree tree(points, parallel, threshold);
        }
        cumulative += time_ms;
    }
    return cumulative / static_cast<double>(iterations);
}

int main(int argc, char* argv[])
{
    ParticleConfig::Limits limits;
    limits.boundingBox          = {{ {-500.0, -500.0, -500.0},
                                     {500.0, 500.0, 500.0} }};
    limits.velocityLimits       = { 10.0, 20.0 };
    limits.accelerationLimits   = { 1.0, 10.0 };
    limits.massLimits           = { 40.0, 70.0 };
                
    auto generated = ParticleConfig::generate(1000000, limits);
    
    std::vector<std::shared_ptr<Particle>> particles;
    for (const auto& particle : generated)
    {
        particles.emplace_back(std::make_shared<Particle>(particle));
    }

    // find when serial is more efficient than parallel
    std::vector<size_t> sizes = { 100, 1000, 2500, 5000, 10000, 50000, 100000 };

    std::array<std::vector<double>, 2> timing;

    for (const auto& size : sizes)
    {
        std::vector<std::shared_ptr<Particle>> temp;
        temp.insert(temp.end(), particles.begin(), particles.begin() + size);

        std::cout << size << ", ";
        timing[0].emplace_back(benchmark(temp, false, 1, 10));
        timing[1].emplace_back(benchmark(temp, true, 1, 10));
    }
    std::cout << std::endl;

    for (const auto& arr : timing)
    {
        for (const auto& time : arr)
        {
            std::cout << time << ", ";
        }
        std::cout << std::endl;
    }

    size_t optimal_threshold = 0;
    for (size_t i = 0; i < sizes.size(); ++i)
    {
        if (timing[0][i] > timing[1][i])
        {
            optimal_threshold = sizes[i];
            break;
        }
    }

    std::cout << "optimal threshold: " << optimal_threshold << std::endl;

    double serial = 0;
    double parallel = 0;


    serial = benchmark(particles, false, 5000, 10);
    parallel = benchmark(particles, true, 5000, 10);
    
    std::cout << "init octree with " << particles.size() << " points" << std::endl;
    std::cout << "serial (ms)=" << serial << std::endl;
    std::cout << "parallel (ms)=" << parallel << std::endl; 

    return 0;
}