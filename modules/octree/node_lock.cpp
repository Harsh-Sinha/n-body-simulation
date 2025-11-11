#include "node_lock.h"

void NoOpLock::acquireReader() {};
void NoOpLock::elevateToWriter() {}
void NoOpLock::demoteToReader() {}
void NoOpLock::unlock() {}




thread_local std::unordered_map<SharedLock*, SharedLock::ThreadState> SharedLock::mThreadLockState{};

void SharedLock::acquireReader()
{
    getThreadState().reader.emplace(mMutex);
}

void SharedLock::elevateToWriter()
{
    auto& state = getThreadState();

    if (state.writer.has_value()) return;

    state.writer.emplace(state.reader.value());
}

void SharedLock::demoteToReader()
{
    auto& state = getThreadState();

    auto* mutex = state.writer->mutex();
    mutex->unlock_and_lock_upgrade();
    state.reader.emplace(*mutex, boost::adopt_lock_t{});
    state.writer.reset();
}

void SharedLock::unlock()
{
    auto& state = getThreadState();

    if (state.writer.has_value())
    {
        // destruct this first so that lock is now upgrade lock
        state.writer.reset();
        // next if case will fully release the lock
    }

    if (state.reader.has_value())
    {
        state.reader.reset();
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