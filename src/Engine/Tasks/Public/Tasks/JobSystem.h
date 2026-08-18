#pragma once

#include "Tasks/TaskFence.h"
#include "Tasks/TaskHandle.h"
#include "Tasks/TaskTypes.h"

#include <Platform/Synchronization.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace engine::tasks
{
    namespace detail
    {
        class JobQueue;
        struct QueuedTask;
    }

    struct JobSystemConfig final
    {
        /*
         * 0 = автоматически:
         * logical processors - 1.
         */
        std::uint32_t workerCount = 0;

        std::wstring workerNamePrefix =
            L"LTS.Worker";
    };

    struct JobSystemStats final
    {
        std::uint64_t submittedTaskCount = 0;
        std::uint64_t completedTaskCount = 0;
        std::uint64_t cancelledTaskCount = 0;
        std::uint64_t failedTaskCount = 0;

        std::uint32_t runningTaskCount = 0;

        std::size_t pendingTaskCount = 0;
    };

    class JobSystem final
    {
    public:
        JobSystem();

        ~JobSystem() noexcept;

        JobSystem(const JobSystem&) = delete;
        JobSystem& operator=(const JobSystem&) = delete;

        JobSystem(JobSystem&&) = delete;
        JobSystem& operator=(JobSystem&&) = delete;

        [[nodiscard]] bool Initialize(
            const JobSystemConfig& config = {});

        void Shutdown() noexcept;

        [[nodiscard]] bool
            IsInitialized() const noexcept;

        [[nodiscard]] bool
            IsAcceptingTasks() const noexcept;

        [[nodiscard]] std::uint32_t
            GetWorkerCount() const noexcept;

        [[nodiscard]] TaskHandle Submit(
            TaskCallback callback,
            TaskPriority priority =
                TaskPriority::Normal,
            CancellationToken cancellationToken = {},
            TaskFence* fence = nullptr);

        [[nodiscard]] std::size_t
            GetPendingTaskCount() const noexcept;

        [[nodiscard]] JobSystemStats
            GetStats() const noexcept;

    private:
        static constexpr std::uint32_t
            MaximumWorkerCount = 32;

        static constexpr std::uint32_t
            MaximumWorkSignals = 0x7FFFFFFFu;

        void WorkerMain(
            std::uint32_t workerIndex) noexcept;

        void ExecuteTask(
            detail::QueuedTask task) noexcept;

        void CancelTask(
            detail::QueuedTask& task) noexcept;

        [[nodiscard]] static std::uint32_t
            ResolveWorkerCount(
                std::uint32_t requestedWorkerCount) noexcept;

        std::unique_ptr<detail::JobQueue>
            queue_;

        mutable engine::platform::Mutex
            queueMutex_;

        engine::platform::Semaphore
            workAvailable_;

        std::vector<std::thread>
            workers_;

        std::wstring workerNamePrefix_;

        std::atomic<bool> initialized_{
            false
        };

        std::atomic<bool> acceptingTasks_{
            false
        };

        std::atomic<bool> stopping_{
            false
        };

        std::atomic<std::uint32_t> workerCount_{
            0
        };

        std::atomic<std::uint64_t> nextTaskId_{
            1
        };

        std::atomic<std::uint64_t>
            submittedTaskCount_{0};

        std::atomic<std::uint64_t>
            completedTaskCount_{0};

        std::atomic<std::uint64_t>
            cancelledTaskCount_{0};

        std::atomic<std::uint64_t>
            failedTaskCount_{0};

        std::atomic<std::uint32_t>
            runningTaskCount_{0};
    };
}