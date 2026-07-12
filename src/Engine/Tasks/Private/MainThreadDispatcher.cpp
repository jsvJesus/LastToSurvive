#include "Tasks/MainThreadDispatcher.h"

#include <Platform/Thread.h>

#include <algorithm>
#include <utility>

namespace engine::tasks
{
    MainThreadDispatcher::
        ~MainThreadDispatcher() noexcept
    {
        Shutdown();
    }

    bool MainThreadDispatcher::Initialize() noexcept
    {
        const std::uint32_t currentThreadId =
            engine::platform::GetCurrentThreadId();

        if (currentThreadId == 0)
        {
            return false;
        }

        std::deque<Callback> staleCallbacks;

        {
            engine::platform::MutexLockGuard lock(
                mutex_);

            if (initialized_)
            {
                return
                    ownerThreadId_ ==
                    currentThreadId;
            }

            staleCallbacks.swap(
                pendingCallbacks_);

            ownerThreadId_ = currentThreadId;

            initialized_ = true;
            acceptingCallbacks_ = true;
        }

        /*
         * Старые callback уничтожаются вне mutex.
         */
        staleCallbacks.clear();

        return true;
    }

    void MainThreadDispatcher::Shutdown() noexcept
    {
        std::deque<Callback> abandonedCallbacks;

        {
            engine::platform::MutexLockGuard lock(
                mutex_);

            acceptingCallbacks_ = false;
            initialized_ = false;
            ownerThreadId_ = 0;

            abandonedCallbacks.swap(
                pendingCallbacks_);
        }

        /*
         * Callback уничтожаются вне mutex,
         * чтобы destructor захваченного объекта
         * не вызвал deadlock.
         */
        abandonedCallbacks.clear();
    }

    bool MainThreadDispatcher::
        IsInitialized() const noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        return initialized_;
    }

    bool MainThreadDispatcher::
        IsOwnerThread() const noexcept
    {
        const std::uint32_t currentThreadId =
            engine::platform::GetCurrentThreadId();

        engine::platform::MutexLockGuard lock(
            mutex_);

        return initialized_ &&
            ownerThreadId_ != 0 &&
            ownerThreadId_ == currentThreadId;
    }

    bool MainThreadDispatcher::Post(
        Callback callback)
    {
        if (!callback)
        {
            return false;
        }

        engine::platform::MutexLockGuard lock(
            mutex_);

        if (!initialized_ ||
            !acceptingCallbacks_)
        {
            return false;
        }

        pendingCallbacks_.push_back(
            std::move(callback));

        return true;
    }

    std::size_t MainThreadDispatcher::Dispatch(
        const std::size_t maximumCount)
    {
        if (maximumCount == 0)
        {
            return 0;
        }

        const std::uint32_t currentThreadId =
            engine::platform::GetCurrentThreadId();

        std::deque<Callback> callbacksToExecute;

        {
            engine::platform::MutexLockGuard lock(
                mutex_);

            if (!initialized_ ||
                ownerThreadId_ == 0 ||
                ownerThreadId_ != currentThreadId)
            {
                return 0;
            }

            const std::size_t dispatchCount =
                std::min(
                    maximumCount,
                    pendingCallbacks_.size());

            for (std::size_t index = 0;
                 index < dispatchCount;
                 ++index)
            {
                callbacksToExecute.push_back(
                    std::move(
                        pendingCallbacks_.front()));

                pendingCallbacks_.pop_front();
            }
        }

        std::size_t executedCount = 0;

        while (!callbacksToExecute.empty())
        {
            Callback callback =
                std::move(
                    callbacksToExecute.front());

            callbacksToExecute.pop_front();

            if (callback)
            {
                callback();
                ++executedCount;
            }
        }

        return executedCount;
    }

    std::size_t MainThreadDispatcher::
        GetPendingCount() const noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        return pendingCallbacks_.size();
    }
}