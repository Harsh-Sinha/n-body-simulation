#pragma once

#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <random>
#include <fstream>

namespace {
namespace ParticleConfig
{
    struct Particle
    {
        std::array<double, 3> position;
        std::array<double, 3> velocity;
        std::array<double, 3> acceleration;
        double mass;
        size_t id;
    };

    struct Limits
    {
        std::array<std::array<double, 3>, 2> boundingBox;
        std::array<double, 2> massLimits;
        std::array<double, 2> velocityLimits;
        std::array<double, 2> accelerationLimits;
    };

    std::ostream& operator<<(std::ostream& os, const Particle& p)
    {
        os << "Particle ID: " << p.id << std::endl;

        os << "Position: (" 
            << p.position[0] << ", " 
            << p.position[1] << ", " 
            << p.position[2] << ")\n";

        os << "Velocity: (" 
            << p.velocity[0] << ", "
            << p.velocity[1] << ", "
            << p.velocity[2] << ")\n";

        os << "Acceleration: (" 
            << p.acceleration[0] << ", " 
            << p.acceleration[1] << ", "
            << p.acceleration[2] << ")\n";

        os << "Mass: " << p.mass << "\n";

        return os;
    }

    // assumes that if the file exists then it is well formed
    static std::vector<Particle> parse(std::string fileName)
    {
        std::ifstream file(fileName);

        std::vector<Particle> particles;

        if (!file.is_open())
        {
            throw std::runtime_error("unable to open: " + fileName);
        }
        else
        {
            std::string temp;

            // skip first line since it is not needed
            std::getline(file, temp);

            while (file >> temp)
            {
                Particle particle;

                if (temp != "Particle")
                {
                    break; // invalid particle config
                }

                file >> temp;
                file >> particle.id;

                file >> temp;
                char c;
                file >> c; // '('
                file >> particle.position[0]; file >> c; // ','
                file >> particle.position[1]; file >> c; // ','
                file >> particle.position[2]; file >> c; // ')'

                file >> temp; 
                file >> c; // '('
                file >> particle.velocity[0]; file >> c; // ','
                file >> particle.velocity[1]; file >> c; // ','
                file >> particle.velocity[2]; file >> c; // ')'

                file >> temp;
                file >> c; // '('
                file >> particle.acceleration[0]; file >> c; // ','
                file >> particle.acceleration[1]; file >> c; // ','
                file >> particle.acceleration[2]; file >> c; // ')'

                file >> temp;
                file >> particle.mass;

                particles.push_back(particle);
            }
        }
        
        return particles;
    }

    static std::vector<Particle> generate(size_t numToGenerate, Limits& limits)
    {
        struct helper
        {
            static double getRandomNumber(double lower, double upper)
            {
                static std::mt19937_64 rng{std::random_device{}()};
                std::uniform_real_distribution<double> distribution(lower, upper);
                return distribution(rng);
            }
        };

        std::vector<Particle> particles;

        for (size_t i = 0; i < numToGenerate; ++i)
        {
            particles.emplace_back(Particle{
                                            { helper::getRandomNumber(limits.boundingBox[0][0], limits.boundingBox[1][0]),
                                              helper::getRandomNumber(limits.boundingBox[0][1], limits.boundingBox[1][1]),
                                              helper::getRandomNumber(limits.boundingBox[0][2], limits.boundingBox[1][2]) }, 
                                            { helper::getRandomNumber(limits.velocityLimits[0], limits.velocityLimits[1]),
                                              helper::getRandomNumber(limits.velocityLimits[0], limits.velocityLimits[1]),
                                              helper::getRandomNumber(limits.velocityLimits[0], limits.velocityLimits[1]) }, 
                                            { helper::getRandomNumber(limits.accelerationLimits[0], limits.accelerationLimits[1]),
                                              helper::getRandomNumber(limits.accelerationLimits[0], limits.accelerationLimits[1]),
                                              helper::getRandomNumber(limits.accelerationLimits[0], limits.accelerationLimits[1]) },
                                            helper::getRandomNumber(limits.massLimits[0], limits.massLimits[1]),
                                            i });
        }

        return particles;
    }

    static void generate(size_t numToGenerate, Limits& limits, std::string& filename)
    {
        std::ofstream file(filename, std::ios::out | std::ios::trunc);
        
        if (!file.is_open())
        {
            throw std::runtime_error("unable to open: " + filename + " to create particle config file");
        }
        else
        {
            file << "Particle System with " << numToGenerate << " particles:\n";

            auto particles = generate(numToGenerate, limits);
            for (const auto& particle : particles)
            {
                file << particle;
            }
        }

        file.close();
    }
} }