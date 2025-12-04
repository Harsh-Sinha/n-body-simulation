#pragma once

#include <memory>
#include <string>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <errno.h>
#include <array>

class PerfCounter
{
public:
    PerfCounter(uint32_t type, uint64_t config);

    ~PerfCounter();

    inline void start()
    {
        ioctl(mFd, PERF_EVENT_IOC_RESET, 0);
        ioctl(mFd, PERF_EVENT_IOC_ENABLE, 0);
    }

    inline void stop()
    {
        ioctl(mFd, PERF_EVENT_IOC_DISABLE, 0);
    }

    inline long long read() const
    {
        long long value = 0;
        ::read(mFd, &value, sizeof(value));
        return value;
    }

private:
    perf_event_attr mAttr{};
    int mFd = -1;

    static int perf_event_open(struct perf_event_attr* hw_event,
                               pid_t pid, int cpu, int group_fd, unsigned long flags);
};

// forward declaration
class PerfProfiler;

class PerfSection
{
public:
    PerfSection(std::string& name, PerfProfiler& profilerInstance);
    ~PerfSection();

    inline void start()
    {
        for (auto& counter : mCounters)
        {
            counter.start();
        }
    }

    inline void stop()
    {
        for (auto& counter : mCounters)
        {
            counter.stop();
        }
        update();
    }

private:
    PerfSection() = default;

    inline void update()
    {
        for (size_t i = 0; i < mCounters.size(); ++i)
        {
            mData[i] += mCounters[i].read();
        }
        ++mNumIterations;
    }

    std::string mName;
    std::vector<PerfCounter> mCounters;
    PerfProfiler& mProfilerInstance;
    std::array<long long, 5> mData = {0, 0, 0, 0, 0};
    size_t mNumIterations = 0;
};

class PerfProfiler
{
public:
    ~PerfProfiler();

    static PerfProfiler& getInstance();

    inline void setProfilerName(std::string& profilerName)
    {
        mProfilerName = profilerName;
    }

    std::unique_ptr<PerfSection> createSectionProfiler(std::string name);

    inline void addProfileData(std::string& profileData)
    {
        mProfileData += profileData;
    }

private:
    PerfProfiler() = default;

    std::string mProfileData = "";
    std::string mProfilerName = "";
};