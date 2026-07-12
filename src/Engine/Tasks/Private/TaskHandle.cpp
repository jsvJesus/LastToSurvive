#include "Tasks/TaskHandle.h"
#include "Tasks/TaskState.h"

#include <utility>

namespace engine::tasks
{
    TaskHandle::TaskHandle(
        std::shared_ptr<
            detail::TaskSharedState> state) noexcept
        : state_(std::move(state))
    {
    }

    bool TaskHandle::IsValid() const noexcept
    {
        return state_ != nullptr &&
            state_->completionEvent.IsValid();
    }

    TaskHandle::operator bool() const noexcept
    {
        return IsValid();
    }

    std::uint64_t TaskHandle::GetId() const noexcept
    {
        if (state_ == nullptr)
        {
            return 0;
        }

        return state_->id;
    }

    TaskState TaskHandle::GetState() const noexcept
    {
        if (state_ == nullptr)
        {
            return TaskState::Invalid;
        }

        return state_->state.load(
            std::memory_order_acquire);
    }

    bool TaskHandle::IsFinished() const noexcept
    {
        switch (GetState())
        {
        case TaskState::Completed:
        case TaskState::Cancelled:
        case TaskState::Failed:
            return true;

        default:
            return false;
        }
    }

    engine::platform::WaitResult TaskHandle::Wait(
        const std::uint32_t timeoutMilliseconds) const noexcept
    {
        if (!IsValid())
        {
            return engine::platform::WaitResult::Failed;
        }

        return state_->completionEvent.Wait(
            timeoutMilliseconds);
    }
}