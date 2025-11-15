#pragma once

#include <array>
#include <cmath>

#include "point3d.h"
#include "particle_config.hpp"

struct Particle : public Point3d
{
    Particle() = default;
    Particle(const ParticleConfig::Particle& particle)
        : Point3d(particle.position[0], particle.position[1], particle.position[2])
        , mVelocity(particle.velocity)
        , mAcceleration(particle.acceleration)
        , mAppliedForce(0.0)
        , mMass(particle.mass)
        , mId(particle.id)
    {}
    Particle(double x, double y, double z, double mass)
        : Point3d(x, y, z)
        , mVelocity( {0.0, 0.0, 0.0} )
        , mAcceleration( {0.0, 0.0, 0.0} )
        , mAppliedForce(0.0)
        , mMass(mass)
        , mId(0)
    {}
        
    virtual ~Particle() = default;

    void setPosition(const std::array<double, 3>& updatedPos)
    {
        setPosition(updatedPos[0], updatedPos[1], updatedPos[1]);
    }

    void setPosition(double x, double y, double z)
    {
        mPosition[0] = x;
        mPosition[1] = y;
        mPosition[2] = z;
    }

    void applyForce(std::shared_ptr<Particle>& particle)
    {
        static constexpr double G = 1.0;

        auto& posA = getPosition();
        auto& posB = particle->getPosition();
        double d = std::sqrt(std::pow(posA[0] - posB[0], 2) + std::pow(posA[1] - posB[1], 2) + std::pow(posA[2] - posB[2], 2));

        mAppliedForce += (G *((mMass * particle->mMass) / (d*d)));
    }
    
    std::array<double, 3> mVelocity;
    std::array<double, 3> mAcceleration;
    double mAppliedForce;
    double mMass;
    size_t mId;
};
