#pragma once

#include <array>

class LeafBase
{
public:
    LeafBase() 
        : mPosition{0.0, 0.0, 0.0}
    {}
    
    LeafBase(double x, double y, double z)
        : mPosition{x, y, z}
    {}
    
    virtual ~LeafBase() = default;
        
    virtual const std::array<double, 3>& getPosition()
    {
        return mPosition;
    }

protected:
    std::array<double, 3> mPosition;

private:

};