#pragma once

#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>

namespace ParticleConfigParser
{
    struct Particle
    {
        std::array<double, 3> position;
        std::array<double, 3> velocity;
        std::array<double, 3> acceleration;
        double mass;
        size_t id;
    };

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
}