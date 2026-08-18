#include "Tasks/TaskTypes.h"

namespace engine::tasks
{
    const char* ToString(
        const TaskPriority priority) noexcept
    {
        switch (priority)
        {
        case TaskPriority::Low:
            return "low";

        case TaskPriority::Normal:
            return "normal";

        case TaskPriority::High:
            return "high";

        case TaskPriority::Critical:
            return "critical";

        default:
            return "unknown";
        }
    }

    const char* ToString(
        const TaskState state) noexcept
    {
        switch (state)
        {
        case TaskState::Invalid:
            return "invalid";

        case TaskState::Queued:
            return "queued";

        case TaskState::Running:
            return "running";

        case TaskState::Completed:
            return "completed";

        case TaskState::Cancelled:
            return "cancelled";

        case TaskState::Failed:
            return "failed";

        default:
            return "unknown";
        }
    }
}