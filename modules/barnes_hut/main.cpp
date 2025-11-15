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
    std::string filename;
    double t;
    double simulationLength;
    double softening;
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
        else if (a == "-s")
        {
            if (!need(1)) return false;

            out.softening = d(1);
            ++i;
            ++argsParsed;
        }
        else if (a=="-f")
        {
            if (!need(1)) return false;
        
            out.filename = argv[i+1];
            ++i;
            ++argsParsed;
        }
        else 
        {
            return false;
        }
    }

    return argsParsed == 4;
}

int main(int argc, char* argv[])
{
    UserInput input;
    bool success = parseArgs(argc, argv, input);

    if (success)
    {
        auto inputParticles = ParticleConfig::parse(input.filename);

        std::vector<std::shared_ptr<Point3d>> particles;
        for (const auto& particle : inputParticles)
        {
            particles.emplace_back(std::make_shared<Particle>(particle));
        }

        BarnesHut bh(particles, input.t, input.simulationLength, input.softening);
        bh.simulate();
    }
    else
    {
        std::cout << "Usage: ./b_hut -t A -s B -f file_name" << std::endl;
        std::cout << "A - time step (s)" << std::endl;
        std::cout << "B - softening factor" << std::endl;
        std::cout << "file_name - particle config file to use" << std::endl;
    }

    return 0;
}