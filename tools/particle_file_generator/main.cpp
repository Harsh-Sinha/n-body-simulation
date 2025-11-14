#include <iostream>

#include "particle_config.hpp"

struct UserInput
{
    ParticleConfig::Limits limits;
    size_t numParticles;
    std::string outFile;
};

bool parseArgs(int argc, char** argv, UserInput &out)
{
    int argsParsed = 0;

    for (int i=1; i<argc; ++i)
    {
        std::string a = argv[i];

        auto need = [&](int k){ return (i+k) < argc; };
        auto d    = [&](int k){ return std::atof(argv[i+k]); };

        if (a=="-box")
        {
            if (!need(6)) return false;

            out.limits.boundingBox[0] = { d(1), d(2), d(3) };
            out.limits.boundingBox[1] = { d(4), d(5), d(6) };
            i+=6;
            ++argsParsed;
        }
        else if (a=="-mass")
        {
            if (!need(2)) return false;
        
            out.limits.massLimits = { d(1), d(2) };
            i+=2;
            ++argsParsed;
        }
        else if (a=="-vel")
        {
            if (!need(2)) return false;
        
            out.limits.velocityLimits = { d(1), d(2) };
            i+=2;
            ++argsParsed;
        }
        else if (a=="-acc") 
        {
            if (!need(2)) return false;
         
            out.limits.accelerationLimits = { d(1), d(2) };
            i+=2;
            ++argsParsed;
        }
        else if (a=="-n")
        {
            if (!need(1)) return false;
        
            out.numParticles = std::strtoull(argv[i+1], nullptr, 10);
            i+=1;
            ++argsParsed;
        }
        else if (a=="-f")
        {
            if (!need(1)) return false;
        
            out.outFile = argv[i+1];
            i+=1;
            ++argsParsed;
        }
        else 
        {
            return false;
        }
    }

    return argsParsed == 6;
}

int main(int argc, char* argv[])
{
    UserInput input;
    bool success = parseArgs(argc, argv, input);

    if (success)
    {
        try
        {
            ParticleConfig::generate(input.numParticles, input.limits, input.outFile);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    else
    {
        std::cout << "Usage: ./particle_config_generator -box A B C D E F -mass H I -vel J K -acc L M -n N -f file_name" << std::endl;
        std::cout << "A,B,C - lower limits of bounding box" << std::endl;
        std::cout << "D,E,F - upper limits of bounding box" << std::endl;
        std::cout << "H,I - mass limits for particles" << std::endl;
        std::cout << "J,K - velocity limits for particles" << std::endl;
        std::cout << "L,M - acceleration limits for particles" << std::endl;
        std::cout << "file_name - output file name" << std::endl;
    }

    return 0;
}