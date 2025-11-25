#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <stdexcept>

class DataStore
{
public:
    DataStore(uint64_t n, double dt, uint64_t numIterations);
    ~DataStore() = default;

    inline void addMass(uint64_t id, double mass)
    {
        if (id >= mMass.size())
        {
            throw std::runtime_error("trying to insert mass for particle id out of range");
        }

        mMass[id] = static_cast<float>(mass);
    }

    inline std::vector<std::array<double, 3>>& getIterationStore(uint64_t iteration)
    {
        if (iteration >= mPositions.size())
        {
            throw std::runtime_error("trying to insert iteration out of range");
        }
        
        return mPositions[iteration];
    }

    inline void addPosition(uint64_t iteration, uint64_t id, std::array<double, 3>& position)
    {
        if (iteration >= mPositions.size())
        {
            throw std::runtime_error("trying to insert iteration out of range");
        }
        if (id > mPositions[iteration].size())
        {
            throw std::runtime_error("trying to insert position for iteration out of range");
        }

        mPositions[iteration][id] = position;
    }

    inline void addProfileData(uint64_t section, double time)
    {
        mProfileData[section] += time;
    }

    void writeToBinaryFile(std::string& filename);

    void writeProfileData(std::string& filename);

private:
    DataStore() = default;

    // store as float because alembic requires float
    std::vector<float> mMass;
    std::vector<std::vector<std::array<double, 3>>> mPositions;
    std::array<double, 7> mProfileData;
    uint64_t mNumIterations;
    uint64_t mN;
    double mDt;
    
};