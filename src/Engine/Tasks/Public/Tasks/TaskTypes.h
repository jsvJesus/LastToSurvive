#pragma once

#include "Tasks/CancellationToken.h"

#include <cstdint>
#include <functional>

namespace engine::tasks
{
    enum class TaskPriority : std::uint8_t
    {
        Low = 0,
        Normal,
        High,
        Critical,

        Count
    };

    enum class TaskState : std::uint8_t
    {
        Invalid = 0,
        Queued,
        Running,
        Completed,
        Cancelled,
        Failed
    };

    using TaskCallback =
        std::function<void(
            const CancellationToken& cancellationToken)>;

    [[nodiscard]] const char* ToString(
        TaskPriority priority) noexcept;

    [[nodiscard]] const char* ToString(
        TaskState state) noexcept;
}