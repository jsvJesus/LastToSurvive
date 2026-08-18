#pragma once

#include <Platform/Synchronization.h>

#include <cstdint>

namespace engine::tasks
{
    class TaskFence final
    {
    public:
        static constexpr std::uint32_t InfiniteWait =
            engine::platform::Event::InfiniteWait;

        TaskFence() noexcept;

        TaskFence(const TaskFence&) = delete;
        TaskFence& operator=(const TaskFence&) = delete;

        TaskFence(TaskFence&&) = delete;
        TaskFence& operator=(TaskFence&&) = delete;

        [[nodiscard]] bool IsValid() const noexcept;

        /*
         * Увеличивает число незавершённых задач.
         * Add должен вызываться до публикации задачи.
         */
        [[nodiscard]] bool Add(
            std::uint32_t count = 1) noexcept;

        /*
         * Уменьшает число незавершённых задач.
         * При достижении нуля fence сигнализируется.
         */
        [[nodiscard]] bool Complete(
            std::uint32_t count = 1) noexcept;

        [[nodiscard]] bool IsComplete() const noexcept;

        [[nodiscard]] std::uint32_t
            GetPendingCount() const noexcept;

        [[nodiscard]] engine::platform::WaitResult Wait(
            std::uint32_t timeoutMilliseconds =
                InfiniteWait) const noexcept;

    private:
        mutable engine::platform::Mutex mutex_;

        engine::platform::Event completedEvent_;

        std::uint32_t pendingCount_ = 0;
    };
}