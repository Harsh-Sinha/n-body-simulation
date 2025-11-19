#pragma once

#include <array>
#include <cmath>

#include "particle_config.hpp"

struct Particle
{
    Particle() = default;
    Particle(const ParticleConfig::Particle& particle)
        : mPosition(particle.position)
        , mVelocity(particle.velocity)
        , mAcceleration(particle.acceleration)
        , mAppliedForce( {0.0, 0.0, 0.0} )
        , mMass(particle.mass)
        , mId(particle.id)
    {}
    Particle(double x, double y, double z, double mass)
        : mPosition( {x, y, z} )
        , mVelocity( {0.0, 0.0, 0.0} )
        , mAcceleration( {0.0, 0.0, 0.0} )
        , mAppliedForce( {0.0, 0.0, 0.0} )
        , mMass(mass)
        , mId(0)
    {}
        
    virtual ~Particle() = default;

    void applyForce(std::shared_ptr<Particle>& particle)
    {
        applyForce(particle->mPosition, particle->mMass);
    }

    void applyForce(std::array<double, 3>& com, double& mass)
    {
        static constexpr double G = -6.6743 * (10^(-11)); // meters^3 / (kilograms * seconds^2)
        static constexpr double epsilon = 1e-8;

        auto& posA = mPosition;
        auto& posB = com;

        double dx = posB[0] - posA[0];
        double dy = posB[1] - posA[1];
        double dz = posB[2] - posA[2];

        // epsilon used to avoid d=0.0
        double d = std::sqrt(dx*dx + dy*dy + dz*dz) + epsilon;

        double force = (G *((mMass * mass) / (d*d)));

        mAppliedForce[0] += dx * force;
        mAppliedForce[1] += dy * force;
        mAppliedForce[2] += dz * force;
    }
    
    std::array<double, 3> mPosition;        // meters
    std::array<double, 3> mVelocity;        // meters / seconds
    std::array<double, 3> mAcceleration;    // meters / seconds^2
    std::array<double, 3> mAppliedForce;    // meters / seconds^2
    double mMass;                           // kilograms
    size_t mId;
};
