#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "particle.h"
#include "particle_config.hpp"
#include "octree.h"
#include "barnes_hut.h"

struct UserInput
{
    std::string particleConfig;
    std::string simulationName;
    double t;
    double simulationLength;
    bool profile = false;
};

bool parseArgs(int argc, char** argv, UserInput &out)
{
    int argsParsed = 0;

    for (int i=1; i<argc; ++i)
    {
        std::string a = argv[i];

        auto need = [&](int k){ return (i+k) < argc; };
        auto d    = [&](int k){ return std::stod(argv[i+k]); };

        if (a == "-t")
        {
            if (!need(1)) return false;

            out.t = d(1);
            ++i;
            ++argsParsed;
        }
        else if  (a == "-l")
        {
            if (!need(1)) return false;

            out.simulationLength = d(1);
            ++i;
            ++argsParsed;
        }
        else if (a == "-p")
        {
            out.profile = true;
            ++argsParsed;
        }
        else if (a=="-in")
        {
            if (!need(1)) return false;
        
            out.particleConfig = argv[i+1];
            ++i;
            ++argsParsed;
        }
        else if (a=="-out")
        {
            if (!need(1)) return false;
        
            out.simulationName = argv[i+1];
            ++i;
            ++argsParsed;
        }
        else 
        {
            return false;
        }
    }

    return argsParsed >= 4;
}

int main(int argc, char* argv[])
{
    UserInput input;
    bool success = parseArgs(argc, argv, input);

    if (success)
    {
        auto inputParticles = ParticleConfig::parse(input.particleConfig);

        std::vector<std::shared_ptr<Particle>> particles;
        for (const auto& particle : inputParticles)
        {
            particles.emplace_back(std::make_shared<Particle>(particle));
        }

        BarnesHut bh(particles, input.t, input.simulationLength, input.simulationName, input.profile);
        bh.simulate();
    }
    else
    {
        std::cout << "Usage: ./b_hut -t A -l B -in particleConfig -out simulationName -p" << std::endl;
        std::cout << "A - time step (s)" << std::endl;
        std::cout << "B - length of simulation (s)" << std::endl;
        std::cout << "particleConfig - particle config file for the simulation" << std::endl;
        std::cout << "simulationName - name to be assigned to this simulation... no spaces and file extension" << std::endl;
        std::cout << "-p - optional flag that turns on profiling for barnes hut" << std::endl;
    }

    return 0;
}