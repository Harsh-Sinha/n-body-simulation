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
    , mPositions(numIterations+1, std::vector<std::array<double, 3>>(n))
    , mNumIterations(numIterations)
    , mN(n)
    , mDt(dt)
{
    mProfileData.fill(0.0);
}

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

void DataStore::writeProfileData(std::string& filename)
{
    double sum = 0.0;
    for (auto& data : mProfileData)
    {
        data /= static_cast<double>(mNumIterations);
        sum += data;
    }

    std::ofstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("unable to open  file to store simulation profile");
    }

    file << "all times in milliseconds\n";
    file << "octree creation: " << mProfileData[0] << "\n";
    file << "center of mass calculation: " << mProfileData[1] << "\n";
    file << "applying forces calculation: " << mProfileData[2] << "\n";
    file << "update pos/vel/acc: " << mProfileData[3] << "\n";
    file << "overall: " << sum << "\n";

    file.close();
}