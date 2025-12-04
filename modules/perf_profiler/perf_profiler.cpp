#include "perf_profiler.h"


#include <sys/syscall.h>
#include <sstream>
#include <fstream>




namespace
{
namespace PerfEvents
{
    static constexpr uint32_t L1D_TYPE = PERF_TYPE_HW_CACHE;
    static constexpr uint64_t L1D_READ_MISS =
        PERF_COUNT_HW_CACHE_L1D |
        (PERF_COUNT_HW_CACHE_OP_READ << 8) |
        (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);

    static constexpr uint32_t LLC_TYPE = PERF_TYPE_HARDWARE;
    static constexpr uint64_t LLC_MISS = PERF_COUNT_HW_CACHE_MISSES;

    static constexpr uint32_t CYCLES_TYPE = PERF_TYPE_HARDWARE;
    static constexpr uint64_t CYCLES = PERF_COUNT_HW_CPU_CYCLES;

    static constexpr uint32_t INSTR_TYPE = PERF_TYPE_HARDWARE;
    static constexpr uint64_t INSTR = PERF_COUNT_HW_INSTRUCTIONS;

    static constexpr uint32_t BRANCH_MISS_TYPE = PERF_TYPE_HARDWARE;
    static constexpr uint64_t BRANCH_MISS = PERF_COUNT_HW_BRANCH_MISSES;
}
}




PerfCounter::PerfCounter(uint32_t type, uint64_t config)
{
    memset(&mAttr, 0x0, sizeof(mAttr));
    mAttr.type = type;
    mAttr.size = sizeof(perf_event_attr);
    mAttr.config = config;
    mAttr.disabled = 1;
    mAttr.exclude_kernel = 1;
    mAttr.exclude_hv = 1;

    mFd = perf_event_open(&mAttr, 0, -1, -1, 0);
    if (mFd == -1)
    {
        throw std::runtime_error("perf_event_open failed: " + std::string(strerror(errno)));
    }
}

PerfCounter::~PerfCounter()
{
    if (mFd != -1)
    {
        close(mFd);
    }
}

int PerfCounter::perf_event_open(struct perf_event_attr* hw_event,
                                 pid_t pid, int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}




PerfSection::PerfSection(std::string& name, PerfProfiler& profilerInstance)
    : mName(name)
    , mProfilerInstance(profilerInstance)
{
    mCounters.emplace_back(PerfEvents::L1D_TYPE,         PerfEvents::L1D_READ_MISS);
    mCounters.emplace_back(PerfEvents::LLC_TYPE,         PerfEvents::LLC_MISS);
    mCounters.emplace_back(PerfEvents::CYCLES_TYPE,      PerfEvents::CYCLES);
    mCounters.emplace_back(PerfEvents::INSTR_TYPE,       PerfEvents::INSTR);
    mCounters.emplace_back(PerfEvents::BRANCH_MISS_TYPE, PerfEvents::BRANCH_MISS);
}

PerfSection::~PerfSection()
{
    for (auto& data : mData)
    {
        data = data / mNumIterations;
    }

    std::stringstream ss;

    ss << "Section: " << mName << "\n";
    ss << "L1 Misses:      " << mData[0] << "\n";
    ss << "LLC Misses:     " << mData[1] << "\n";
    ss << "Cycles:         " << mData[2] << "\n";
    ss << "Instructions:   " << mData[3] << "\n";
    ss << "Branch Misses:  " << mData[4] << "\n";

    auto str = ss.str();
    mProfilerInstance.addProfileData(str);
}




PerfProfiler::~PerfProfiler()
{
    std::string filename = mProfilerName + ".perf.txt";
    std::ofstream file(filename);

    if (!file.is_open())
    {
        throw std::runtime_error("unable to open file to write perf profile data");
    }

    file << mProfileData;

    file.close();
}

PerfProfiler& PerfProfiler::getInstance()
{
    static PerfProfiler profiler;
    return profiler;
}

std::unique_ptr<PerfSection> PerfProfiler::createSectionProfiler(std::string& name)
{
    return std::move(std::make_unique<PerfSection>(name, *this));
}