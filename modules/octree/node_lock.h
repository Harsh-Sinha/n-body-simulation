#pragma once

#include <shared_mutex>
#include <memory>

class NodeLock
{
public:
    NodeLock() = default;
    virtual ~NodeLock() = default;

    virtual void acquireReader() = 0;
    virtual void elevateToWriter() = 0;
    virtual void unlock() = 0;
};

class NoOpLock : public NodeLock
{
public:
    NoOpLock() = default;
    virtual ~NoOpLock() = default;

    virtual void acquireReader() override;
    virtual void elevateToWriter() override;
    virtual void unlock() override;
};

class SharedLock : public NodeLock
{
public:
    SharedLock() = default;
    virtual ~SharedLock() = default;

    virtual void acquireReader() override;
    virtual void elevateToWriter();
    virtual void unlock() override;

private:
    std::shared_mutex mMutex;
    bool mExclusiveLockAcquired = false;
};

std::unique_ptr<NodeLock> createNodeLock(bool supportMultithread);