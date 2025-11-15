#pragma once

#include <array>

#include "point3d.h"
#include "particle_config.hpp"

struct Particle : public Point3d
{
    Particle() = default;
    Particle(const ParticleConfig::Particle& particle)
        : Point3d(particle.position[0], particle.position[1], particle.position[2])
        , mVelocity(particle.velocity)
        , mAcceleration(particle.acceleration)
        , mMass(particle.mass)
        , mId(particle.id)
    {}
    Particle(double x, double y, double z, double mass)
        : Point3d(x, y, z)
        , mVelocity( {0.0, 0.0, 0.0} )
        , mAcceleration( {0.0, 0.0, 0.0} )
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
    
    std::array<double, 3> mVelocity;
    std::array<double, 3> mAcceleration;
    double mMass;
    size_t mId;
};
