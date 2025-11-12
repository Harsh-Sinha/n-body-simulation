#pragma once

#include <memory>
#include <optional>
#include <unordered_map>
#include <mutex>

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
    virtual ~NoOpLock() override = default;

    virtual void acquireReader() override;
    virtual void elevateToWriter() override;
    virtual void unlock() override;
};

class BasicLock : public NodeLock
{
public:
    BasicLock() = default;
    virtual ~BasicLock() override = default;

    virtual void acquireReader() override;
    virtual void elevateToWriter() override;
    virtual void unlock() override;
private:
    std::mutex mMutex;
};

std::unique_ptr<NodeLock> createNodeLock(bool supportMultithread);