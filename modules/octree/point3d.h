#pragma once

#include <array>

class Point3d
{
public:
    Point3d()
        : mPosition{0.0, 0.0, 0.0}
    {}

    Point3d(double x, double y, double z)
        : mPosition{x, y, z}
    {}

    virtual ~Point3d() = default;

    virtual std::array<double, 3>& getPosition()
    {
        return mPosition;
    }

protected:
    std::array<double, 3> mPosition;

private:

};