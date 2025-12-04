#include "perf_profiler.h"


#include <sys/syscall.h>
#include <sstream>
#include <fstream>




PerfCounter::PerfCounter(uint32_t type, uint64_t config)
{
    memset(&mAttr, 0x0, sizeof(mAttr));
    mAttr.type = type;
    mAttr.size = sizeof(perf_event_attr);
    mAttr.config = config;
    mAttr.disabled = 1;
    mAttr.exclude_kernel = 1;
    mAttr.exclude_hv = 1;
    mAttr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;

    mFd = perf_event_open(&mAttr, getpid(), -1, -1, 0);

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
    mCounters.emplace_back(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES);
    mCounters.emplace_back(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    mCounters.emplace_back(PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    mCounters.emplace_back(PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    mCounters.emplace_back(PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES);
}

PerfSection::~PerfSection()
{
    for (auto& data : mData)
    {
        data = data / mNumIterations;
    }

    std::stringstream ss;

    ss << "Section: " << mName << "\n";
    ss << "cache-references:            " << mData[0] << "\n";
    ss << "cache-misses:                " << mData[1] << "\n";
    ss << "cycles:                      " << mData[2] << "\n";
    ss << "instructions:                " << mData[3] << "\n";
    ss << "branch-misses:               " << mData[4] << "\n";
    ss << "cache-miss %:                " << static_cast<double>(mData[1]) / static_cast<double>(mData[0]) << "\n";
    ss << "cache-misses / instructions: " << static_cast<double>(mData[1]) / static_cast<double>(mData[3]) << "\n";
    ss << "IPC:                         " << static_cast<double>(mData[3]) / static_cast<double>(mData[2]) << "\n";
 
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

std::unique_ptr<PerfSection> PerfProfiler::createSectionProfiler(std::string name)
{
    return std::move(std::make_unique<PerfSection>(name, *this));
}