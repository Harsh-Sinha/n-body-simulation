#include "node_lock.h"

void NoOpLock::acquireReader() {};
void NoOpLock::elevateToWriter() {}
void NoOpLock::unlock() {}

void SharedLock::acquireReader()
{
    mMutex.lock_shared();
}

void SharedLock::elevateToWriter()
{
    mMutex.unlock_shared();
    mMutex.lock();
    mExclusiveLockAcquired = true;
}

void SharedLock::unlock()
{
    if (mExclusiveLockAcquired)
    {
        mMutex.unlock();
        mExclusiveLockAcquired = false;
    }
    else
    {
        mMutex.unlock_shared();
    }
}

std::unique_ptr<NodeLock> createNodeLock(bool supportMultithread)
{
    std::unique_ptr<NodeLock> lock;

    if (supportMultithread)
    {
        lock = std::make_unique<SharedLock>();
    }
    else
    {
        lock = std::make_unique<NoOpLock>();
    }

    return std::move(lock);
}