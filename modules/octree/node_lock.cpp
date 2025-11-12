#include "node_lock.h"

void NoOpLock::acquireReader() {};
void NoOpLock::elevateToWriter() {}
void NoOpLock::unlock() {}




void BasicLock::acquireReader() {mMutex.lock();}
void BasicLock::elevateToWriter() {}
void BasicLock::unlock() {mMutex.unlock();}




std::unique_ptr<NodeLock> createNodeLock(bool supportMultithread)
{
    std::unique_ptr<NodeLock> lock;

    if (supportMultithread)
    {
        lock = std::make_unique<BasicLock>();
    }
    else
    {
        lock = std::make_unique<NoOpLock>();
    }

    return std::move(lock);
}