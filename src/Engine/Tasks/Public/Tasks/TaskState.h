#pragma once

#include "Tasks/TaskTypes.h"

#include <Platform/Synchronization.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>

namespace engine::tasks
{
    class TaskFence;

    namespace detail
    {
        struct TaskSharedState final
        {
            explicit TaskSharedState(
                const std::uint64_t taskId) noexcept
                : id(taskId),
                  completionEvent(
                      engine::platform::
                          EventResetMode::Manual,
                      false)
            {
            }

            std::uint64_t id = 0;

            std::atomic<TaskState> state{
                TaskState::Queued
            };

            engine::platform::Event completionEvent;
        };

        struct QueuedTask final
        {
            std::uint64_t id = 0;

            TaskPriority priority =
                TaskPriority::Normal;

            TaskCallback callback;

            CancellationToken cancellationToken;

            std::shared_ptr<TaskSharedState>
                sharedState;

            /*
             * Non-owning.
             *
             * Fence должен жить до завершения
             * либо отмены опубликованной задачи.
             */
            TaskFence* fence = nullptr;
        };
    }
}