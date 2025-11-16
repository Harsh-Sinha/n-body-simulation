#include "data_store.h"

#include <fstream>

namespace
{
    template <class T>
    void write(std::ostream& os, T& t)
    {
        os.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }
}

DataStore::DataStore(uint64_t n, double dt, uint64_t numIterations)
    : mMass(n)
    , mPositions(numIterations, std::vector<std::array<double, 3>>(n))
    , mN(n)
    , mDt(dt)
{}

void DataStore::writeToBinaryFile(std::string& filename)
{
    std::ofstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("unable to open binary file to store simulation data");
    }

    write(file, mN);
    write(file, mDt);

    for (const auto& mass : mMass)
    {
        write(file, mass);
    }

    for (const auto& iteration : mPositions)
    {
        for (const auto& position : iteration)
        {
            write(file, position[0]);
            write(file, position[1]);
            write(file, position[2]);
        }
    }


    file.close();
}