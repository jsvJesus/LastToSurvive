#pragma once

#include "TaskState.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace engine::tasks::detail
{
    class JobQueue final
    {
    public:
        JobQueue() = default;

        JobQueue(const JobQueue&) = delete;
        JobQueue& operator=(const JobQueue&) = delete;

        void Push(
            QueuedTask task);

        [[nodiscard]] bool TryPop(
            QueuedTask& task);

        [[nodiscard]] bool RemoveById(
            std::uint64_t taskId,
            QueuedTask& removedTask);

        [[nodiscard]] std::vector<QueuedTask>
            TakeAll();

        [[nodiscard]] std::size_t
            GetSize() const noexcept;

        [[nodiscard]] bool IsEmpty() const noexcept;

    private:
        static constexpr std::size_t PriorityCount =
            static_cast<std::size_t>(
                TaskPriority::Count);

        [[nodiscard]] static constexpr std::size_t
            ToIndex(TaskPriority priority) noexcept
        {
            return static_cast<std::size_t>(
                priority);
        }

        std::array<
            std::deque<QueuedTask>,
            PriorityCount>
                queues_;

        std::size_t taskCount_ = 0;
    };
}