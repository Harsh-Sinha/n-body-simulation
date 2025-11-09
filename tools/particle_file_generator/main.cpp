#include <iostream>
#include <array>
#include <string>
#include <fstream>
#include <random>

struct ParticleConfig
{
    std::array<std::array<double, 3>, 2> boundingBox;
    std::array<double, 2> massLimits;
    std::array<double, 2> velocityLimits;
    std::array<double, 2> accelerationLimits;
};

struct UserInput
{
    ParticleConfig particleConfig;
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

            out.particleConfig.boundingBox[0] = { d(1), d(2), d(3) };
            out.particleConfig.boundingBox[1] = { d(4), d(5), d(6) };
            i+=6;
            ++argsParsed;
        }
        else if (a=="-mass")
        {
            if (!need(2)) return false;
        
            out.particleConfig.massLimits = { d(1), d(2) };
            i+=2;
            ++argsParsed;
        }
        else if (a=="-vel")
        {
            if (!need(2)) return false;
        
            out.particleConfig.velocityLimits = { d(1), d(2) };
            i+=2;
            ++argsParsed;
        }
        else if (a=="-acc") 
        {
            if (!need(2)) return false;
         
            out.particleConfig.accelerationLimits = { d(1), d(2) };
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

double getRandomNumber(double lower, double upper)
{
    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_real_distribution<double> distribution(lower, upper);
    return distribution(rng);
}

void createParticle(std::ofstream& file, const ParticleConfig& config)
{
    file << "Position: (" 
         << getRandomNumber(config.boundingBox[0][0], config.boundingBox[1][0]) << ", " 
         << getRandomNumber(config.boundingBox[0][1], config.boundingBox[1][1]) << ", " 
         << getRandomNumber(config.boundingBox[0][2], config.boundingBox[1][2]) << ")\n";
    
    file << "Velocity: (" 
         << getRandomNumber(config.velocityLimits[0], config.velocityLimits[1]) << ", "
         << getRandomNumber(config.velocityLimits[0], config.velocityLimits[1]) << ", "
         << getRandomNumber(config.velocityLimits[0], config.velocityLimits[1]) << ")\n";

    file << "Acceleration: (" 
         << getRandomNumber(config.accelerationLimits[0], config.accelerationLimits[1]) << ", " 
         << getRandomNumber(config.accelerationLimits[0], config.accelerationLimits[1]) << ", "
         << getRandomNumber(config.accelerationLimits[0], config.accelerationLimits[1]) << ")\n";
    
    file << "Mass: " << getRandomNumber(config.massLimits[0], config.massLimits[1]) << "\n"; 
}

int main(int argc, char* argv[])
{
    UserInput input;
    bool success = parseArgs(argc, argv, input);

    if (success)
    {
        std::ofstream file(input.outFile, std::ios::out | std::ios::trunc);
        
        if (!file.is_open())
        {
            std::cout << "unable to open file: " << input.outFile << std::endl;
        }
        else
        {
            file << "Particle System with " << input.numParticles << " particles:\n";

            for (size_t i = 0; i < input.numParticles; ++i)
            {
                file << "Particle ID: " << i << std::endl;
                createParticle(file, input.particleConfig);
            }
        }

        file.close();
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