#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <filesystem>
#include <string>

#include "particle_config.hpp"

static std::filesystem::path base()
{
    return std::filesystem::canonical(std::filesystem::path(__FILE__)).parent_path();
}

TEST_CASE("Parses empty particle file")
{
    std::filesystem::path file = base() / "inputs" / "test_particle_config1.txt";
    auto particles = ParticleConfig::parse(file.string());

    REQUIRE(particles.size() == 0);
}

TEST_CASE("Parses five-particle file (checks all values)")
{
    std::filesystem::path file = base() / "inputs" / "test_particle_config0.txt";
    auto particles = ParticleConfig::parse(file.string());

    REQUIRE(particles.size() == 5);

    // particle 0
    {
        auto& p = particles[0];
        REQUIRE(p.position[0]     == Catch::Approx(0.196431));
        REQUIRE(p.position[1]     == Catch::Approx(4.03482));
        REQUIRE(p.position[2]     == Catch::Approx(4.99517));
        REQUIRE(p.velocity[0]     == Catch::Approx(2.18965));
        REQUIRE(p.velocity[1]     == Catch::Approx(2.64329));
        REQUIRE(p.velocity[2]     == Catch::Approx(2.76131));
        REQUIRE(p.acceleration[0] == Catch::Approx(0.0782128));
        REQUIRE(p.acceleration[1] == Catch::Approx(0.404698));
        REQUIRE(p.acceleration[2] == Catch::Approx(0.109318));
        REQUIRE(p.mass            == Catch::Approx(0.981739));
    }

    // particle 1
    {
        auto& p = particles[1];
        REQUIRE(p.position[0]     == Catch::Approx(-2.83488));
        REQUIRE(p.position[1]     == Catch::Approx(1.04505));
        REQUIRE(p.position[2]     == Catch::Approx(0.006225));
        REQUIRE(p.velocity[0]     == Catch::Approx(1.97666));
        REQUIRE(p.velocity[1]     == Catch::Approx(2.42329));
        REQUIRE(p.velocity[2]     == Catch::Approx(2.55052));
        REQUIRE(p.acceleration[0] == Catch::Approx(0.909799));
        REQUIRE(p.acceleration[1] == Catch::Approx(0.483506));
        REQUIRE(p.acceleration[2] == Catch::Approx(0.161014));
        REQUIRE(p.mass            == Catch::Approx(0.886232));
    }

    // particle 2
    {
        auto& p = particles[2];
        REQUIRE(p.position[0]     == Catch::Approx(-0.435844));
        REQUIRE(p.position[1]     == Catch::Approx(3.43062));
        REQUIRE(p.position[2]     == Catch::Approx(-0.613382));
        REQUIRE(p.velocity[0]     == Catch::Approx(1.87737));
        REQUIRE(p.velocity[1]     == Catch::Approx(1.5569));
        REQUIRE(p.velocity[2]     == Catch::Approx(1.36718));
        REQUIRE(p.acceleration[0] == Catch::Approx(0.437857));
        REQUIRE(p.acceleration[1] == Catch::Approx(0.983844));
        REQUIRE(p.acceleration[2] == Catch::Approx(0.60275));
        REQUIRE(p.mass            == Catch::Approx(0.375736));
    }

    // particle 3
    {
        auto& p = particles[3];
        REQUIRE(p.position[0]     == Catch::Approx(4.88886));
        REQUIRE(p.position[1]     == Catch::Approx(2.36267));
        REQUIRE(p.position[2]     == Catch::Approx(4.79081));
        REQUIRE(p.velocity[0]     == Catch::Approx(1.71583));
        REQUIRE(p.velocity[1]     == Catch::Approx(2.49618));
        REQUIRE(p.velocity[2]     == Catch::Approx(1.19669));
        REQUIRE(p.acceleration[0] == Catch::Approx(0.383051));
        REQUIRE(p.acceleration[1] == Catch::Approx(0.850002));
        REQUIRE(p.acceleration[2] == Catch::Approx(0.372707));
        REQUIRE(p.mass            == Catch::Approx(0.841716));
    }

    // particle 4
    {
        auto& p = particles[4];
        REQUIRE(p.position[0]     == Catch::Approx(-0.186322));
        REQUIRE(p.position[1]     == Catch::Approx(3.69757));
        REQUIRE(p.position[2]     == Catch::Approx(2.22083));
        REQUIRE(p.velocity[0]     == Catch::Approx(2.50604));
        REQUIRE(p.velocity[1]     == Catch::Approx(1.4802));
        REQUIRE(p.velocity[2]     == Catch::Approx(2.60814));
        REQUIRE(p.acceleration[0] == Catch::Approx(0.244115));
        REQUIRE(p.acceleration[1] == Catch::Approx(0.330374));
        REQUIRE(p.acceleration[2] == Catch::Approx(0.331437));
        REQUIRE(p.mass            == Catch::Approx(0.560983));
    }
}

TEST_CASE("Try to parse non existant file")
{
    std::filesystem::path file = base() / "inputs" / "dummy.txt";
    REQUIRE_THROWS(ParticleConfig::parse(file.string()));
}

TEST_CASE("Particle generation returns particles within limits", "[particle][generate]")
{
    ParticleConfig::Limits limits;

    // bounding position limits
    limits.boundingBox = {{
        { -10.0, -5.0,  0.0 },   // min xyz
        {  10.0,  5.0,  1.0 }    // max xyz
    }};

    // mass, velocity, acceleration limits
    limits.massLimits          = { 1.0, 5.0 };
    limits.velocityLimits      = { -2.0, 2.0 };
    limits.accelerationLimits  = { -0.5, 0.5 };

    const size_t N = 1000;

    auto particles = ParticleConfig::generate(N, limits);

    REQUIRE(particles.size() == N);

    for (size_t i = 0; i < N; ++i)
    {
        const auto& p = particles[i];

        REQUIRE(p.id == i);

        for (int d = 0; d < 3; ++d)
        {
            REQUIRE(p.position[d] >= limits.boundingBox[0][d]);
            REQUIRE(p.position[d] <= limits.boundingBox[1][d]);
        }

        for (int d = 0; d < 3; ++d)
        {
            REQUIRE(p.velocity[d] >= limits.velocityLimits[0]);
            REQUIRE(p.velocity[d] <= limits.velocityLimits[1]);
        }

        for (int d = 0; d < 3; ++d)
        {
            REQUIRE(p.acceleration[d] >= limits.accelerationLimits[0]);
            REQUIRE(p.acceleration[d] <= limits.accelerationLimits[1]);
        }

        REQUIRE(p.mass >= limits.massLimits[0]);
        REQUIRE(p.mass <= limits.massLimits[1]);
    }
}


TEST_CASE("Particle::generate creates a valid output file", "[particle][generate][file]")
{
    ParticleConfig::Limits limits;

    limits.boundingBox = {{
        { -1.0, -1.0, -1.0 },
        {  1.0,  1.0,  1.0 }
    }};

    limits.massLimits          = { 0.1, 10.0 };
    limits.velocityLimits      = { -1.0, 1.0 };
    limits.accelerationLimits  = { -0.1, 0.1 };

    const size_t N = 10;

    std::string filename = "test_particle_output.txt";

    if (std::filesystem::exists(filename))
        std::filesystem::remove(filename);

    REQUIRE_NOTHROW( ParticleConfig::generate(N, limits, filename) );

    REQUIRE(std::filesystem::exists(filename));

    std::ifstream file(filename);
    REQUIRE(file.is_open());

    std::string line;
    REQUIRE(std::getline(file, line)); 

    REQUIRE(line.find(std::to_string(N)) != std::string::npos);

    bool foundParticleLine = false;
    while (std::getline(file, line))
    {
        if (line.find("Particle ID:") != std::string::npos)
        {
            foundParticleLine = true;
            break;
        }
    }

    REQUIRE(foundParticleLine);

    file.close();

    // Clean up after test
    std::filesystem::remove(filename);
    REQUIRE(!std::filesystem::exists(filename));
}