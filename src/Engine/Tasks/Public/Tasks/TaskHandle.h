#pragma once

#include "Tasks/TaskTypes.h"

#include <Platform/Synchronization.h>

#include <cstdint>
#include <memory>

namespace engine::tasks
{
    namespace detail
    {
        struct TaskSharedState;
    }

    class JobSystem;

    class TaskHandle final
    {
    public:
        static constexpr std::uint32_t InfiniteWait =
            engine::platform::Event::InfiniteWait;

        TaskHandle() noexcept = default;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] std::uint64_t GetId() const noexcept;

        [[nodiscard]] TaskState GetState() const noexcept;

        [[nodiscard]] bool IsFinished() const noexcept;

        [[nodiscard]] engine::platform::WaitResult Wait(
            std::uint32_t timeoutMilliseconds =
                InfiniteWait) const noexcept;

    private:
        explicit TaskHandle(
            std::shared_ptr<
                detail::TaskSharedState> state) noexcept;

        std::shared_ptr<
            detail::TaskSharedState> state_;

        friend class JobSystem;
    };
}