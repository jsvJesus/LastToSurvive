#include "Tasks/TaskFence.h"

#include <limits>

namespace engine::tasks
{
    TaskFence::TaskFence() noexcept
        : completedEvent_(
              engine::platform::EventResetMode::Manual,
              true)
    {
    }

    bool TaskFence::IsValid() const noexcept
    {
        return completedEvent_.IsValid();
    }

    bool TaskFence::Add(
        const std::uint32_t count) noexcept
    {
        if (count == 0 ||
            !completedEvent_.IsValid())
        {
            return false;
        }

        engine::platform::MutexLockGuard lock(
            mutex_);

        const std::uint32_t maximumCount =
            std::numeric_limits<
                std::uint32_t>::max();

        if (pendingCount_ >
            maximumCount - count)
        {
            return false;
        }

        if (pendingCount_ == 0)
        {
            if (!completedEvent_.Reset())
            {
                return false;
            }
        }

        pendingCount_ += count;

        return true;
    }

    bool TaskFence::Complete(
        const std::uint32_t count) noexcept
    {
        if (count == 0 ||
            !completedEvent_.IsValid())
        {
            return false;
        }

        engine::platform::MutexLockGuard lock(
            mutex_);

        if (count > pendingCount_)
        {
            return false;
        }

        pendingCount_ -= count;

        if (pendingCount_ == 0)
        {
            return completedEvent_.Signal();
        }

        return true;
    }

    bool TaskFence::IsComplete() const noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        return pendingCount_ == 0;
    }

    std::uint32_t
        TaskFence::GetPendingCount() const noexcept
    {
        engine::platform::MutexLockGuard lock(
            mutex_);

        return pendingCount_;
    }

    engine::platform::WaitResult TaskFence::Wait(
        const std::uint32_t timeoutMilliseconds) const noexcept
    {
        return completedEvent_.Wait(
            timeoutMilliseconds);
    }
}