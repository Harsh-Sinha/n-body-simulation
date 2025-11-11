#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

class NodeLock
{
public:
    NodeLock() = default;
    virtual ~NodeLock() = default;

    virtual void acquireReader() = 0;
    virtual void elevateToWriter() = 0;
    virtual void demoteToReader() = 0;
    virtual void unlock() = 0;
};

class NoOpLock : public NodeLock
{
public:
    NoOpLock() = default;
    virtual ~NoOpLock() override = default;

    virtual void acquireReader() override;
    virtual void elevateToWriter() override;
    virtual void demoteToReader() override;
    virtual void unlock() override;
};

class SharedLock : public NodeLock
{
public:
    SharedLock() = default;
    virtual ~SharedLock() override = default;

    virtual void acquireReader() override;
    virtual void elevateToWriter() override;
    virtual void demoteToReader() override;
    virtual void unlock() override;

private:
    struct ThreadState
    {
        std::optional<boost::upgrade_lock<boost::shared_mutex>> reader;
        std::optional<boost::unique_lock<boost::shared_mutex>> writer;
    };

    inline ThreadState& getThreadState()
    {
        return mThreadLockState[this];
    }

    static thread_local std::unordered_map<SharedLock*, ThreadState> mThreadLockState;

    boost::shared_mutex mMutex;
};

std::unique_ptr<NodeLock> createNodeLock(bool supportMultithread);