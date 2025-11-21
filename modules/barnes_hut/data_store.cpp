#include "data_store.h"

#include <fstream>

#include "Alembic/AbcGeom/All.h"
#include "Alembic/Abc/All.h"
#include "Alembic/AbcCoreOgawa/All.h"

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
    Alembic::Abc::OArchive archive(Alembic::AbcCoreOgawa::WriteArchive(), filename);

    Alembic::Abc::OObject topObj = archive.getTop();

    // create native point cloud object
    Alembic::AbcGeom::OPoints pointsObj(topObj, "particles");
    Alembic::AbcGeom::OPointsSchema &pointsSchema = pointsObj.getSchema();

    // normalize masses
    float minMass = *std::min_element(mMass.begin(), mMass.end());
    float maxMass = *std::max_element(mMass.begin(), mMass.end());
    float range   = maxMass - minMass;
    for (auto& mass : mMass)
    {
        // normalize to [0, 5]
        mass = ((mass - minMass) / range) * 10.0; 
    }

    // create mass information
    Alembic::Abc::FloatArraySample widthSample(mMass.data(), mMass.size());
    Alembic::AbcGeom::v12::OFloatGeomParam::Sample widths;
    widths.setVals(widthSample);

    // map particle ids
    std::vector<Alembic::Abc::uint64_t> ids(mMass.size());
    for (size_t i = 0; i < ids.size(); ++i)
    {
        ids[i] = static_cast<Alembic::Abc::uint64_t>(i);
    }

    // loop over every iteration and create a frame for it
    bool firstFrame = true;
    for (auto& iteration : mPositions)
    {
        // alembic requires 32 bit floating point NOT 64 bit
        std::vector<Alembic::AbcGeom::V3f> positions(iteration.size());
        for (size_t i = 0; i < iteration.size(); ++i)
        {
            positions[i] = Alembic::AbcGeom::V3f( static_cast<float>(iteration[i][0]),
                                                  static_cast<float>(iteration[i][1]),
                                                  static_cast<float>(iteration[i][2]) );
        }

        Alembic::AbcGeom::V3fArraySample positionsSample(positions.data(), positions.size());

        // construct frame
        Alembic::AbcGeom::OPointsSchema::Sample sample;
        sample.setPositions(positionsSample);
        sample.setWidths(widths);

        if (firstFrame)
        {
            sample.setIds(ids);
            firstFrame = false;
        }

        // commit frame to storage
        pointsSchema.set(sample);
    }
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