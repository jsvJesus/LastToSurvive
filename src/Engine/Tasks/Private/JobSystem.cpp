#include "Tasks/JobSystem.h"

#include "Tasks/JobQueue.h"
#include "Tasks/TaskState.h"

#include <Platform/SystemInfo.h>
#include <Platform/Thread.h>

#include <algorithm>
#include <exception>
#include <utility>

namespace engine::tasks
{
    namespace
    {
        void FinishSharedState(
            const std::shared_ptr<
                detail::TaskSharedState>& state,
            const TaskState finalState) noexcept
        {
            if (state == nullptr)
            {
                return;
            }

            state->state.store(
                finalState,
                std::memory_order_release);

            (void)state->completionEvent.Signal();
        }

        void CompleteFence(
            TaskFence* const fence) noexcept
        {
            if (fence != nullptr)
            {
                (void)fence->Complete();
            }
        }
    }

    JobSystem::JobSystem()
        : queue_(
              std::make_unique<
                  detail::JobQueue>()),
          workAvailable_(
              0,
              MaximumWorkSignals)
    {
    }

    JobSystem::~JobSystem() noexcept
    {
        Shutdown();
    }

    bool JobSystem::Initialize(
        const JobSystemConfig& config)
    {
        if (initialized_.load(
                std::memory_order_acquire))
        {
            return false;
        }

        const std::uint32_t resolvedWorkerCount =
            ResolveWorkerCount(
                config.workerCount);

        if (resolvedWorkerCount == 0)
        {
            return false;
        }

        workAvailable_ =
            engine::platform::Semaphore(
                0,
                MaximumWorkSignals);

        if (!workAvailable_.IsValid())
        {
            return false;
        }

        {
            engine::platform::MutexLockGuard lock(
                queueMutex_);

            if (queue_ == nullptr)
            {
                queue_ =
                    std::make_unique<
                        detail::JobQueue>();
            }

            /*
             * После корректного Shutdown очередь
             * всегда должна быть пустой.
             */
            if (!queue_->IsEmpty())
            {
                return false;
            }
        }

        workerNamePrefix_ =
            config.workerNamePrefix.empty()
                ? L"LTS.Worker"
                : config.workerNamePrefix;

        nextTaskId_.store(
            1,
            std::memory_order_release);

        submittedTaskCount_.store(
            0,
            std::memory_order_release);

        completedTaskCount_.store(
            0,
            std::memory_order_release);

        cancelledTaskCount_.store(
            0,
            std::memory_order_release);

        failedTaskCount_.store(
            0,
            std::memory_order_release);

        runningTaskCount_.store(
            0,
            std::memory_order_release);

        workerCount_.store(
            resolvedWorkerCount,
            std::memory_order_release);

        stopping_.store(
            false,
            std::memory_order_release);

        acceptingTasks_.store(
            false,
            std::memory_order_release);

        initialized_.store(
            false,
            std::memory_order_release);

        workers_.clear();
        workers_.reserve(
            resolvedWorkerCount);

        try
        {
            for (std::uint32_t workerIndex = 0;
                 workerIndex < resolvedWorkerCount;
                 ++workerIndex)
            {
                workers_.emplace_back(
                    [this, workerIndex]()
                    {
                        WorkerMain(workerIndex);
                    });
            }
        }
        catch (...)
        {
            stopping_.store(
                true,
                std::memory_order_release);

            if (!workers_.empty())
            {
                (void)workAvailable_.Release(
                    static_cast<std::uint32_t>(
                        workers_.size()));
            }

            for (std::thread& worker :
                 workers_)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }

            workers_.clear();

            workerCount_.store(
                0,
                std::memory_order_release);

            stopping_.store(
                false,
                std::memory_order_release);

            return false;
        }

        initialized_.store(
            true,
            std::memory_order_release);

        acceptingTasks_.store(
            true,
            std::memory_order_release);

        return true;
    }

    void JobSystem::Shutdown() noexcept
    {
        if (!initialized_.exchange(
                false,
                std::memory_order_acq_rel))
        {
            return;
        }

        acceptingTasks_.store(
            false,
            std::memory_order_release);

        stopping_.store(
            true,
            std::memory_order_release);

        std::vector<detail::QueuedTask>
            cancelledTasks;

        {
            engine::platform::MutexLockGuard lock(
                queueMutex_);

            if (queue_ != nullptr)
            {
                cancelledTasks =
                    queue_->TakeAll();
            }
        }

        for (detail::QueuedTask& task :
             cancelledTasks)
        {
            CancelTask(task);
        }

        const std::uint32_t workerCount =
            static_cast<std::uint32_t>(
                workers_.size());

        if (workerCount != 0)
        {
            (void)workAvailable_.Release(
                workerCount);
        }

        for (std::thread& worker :
             workers_)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        workers_.clear();

        workerCount_.store(
            0,
            std::memory_order_release);

        runningTaskCount_.store(
            0,
            std::memory_order_release);

        stopping_.store(
            false,
            std::memory_order_release);
    }

    bool JobSystem::IsInitialized() const noexcept
    {
        return initialized_.load(
            std::memory_order_acquire);
    }

    bool JobSystem::
        IsAcceptingTasks() const noexcept
    {
        return
            initialized_.load(
                std::memory_order_acquire) &&
            acceptingTasks_.load(
                std::memory_order_acquire) &&
            !stopping_.load(
                std::memory_order_acquire);
    }

    std::uint32_t
        JobSystem::GetWorkerCount() const noexcept
    {
        return workerCount_.load(
            std::memory_order_acquire);
    }

    TaskHandle JobSystem::Submit(
        TaskCallback callback,
        const TaskPriority priority,
        CancellationToken cancellationToken,
        TaskFence* const fence)
    {
        if (!callback ||
            !IsAcceptingTasks())
        {
            return {};
        }

        const std::uint64_t taskId =
            nextTaskId_.fetch_add(
                1,
                std::memory_order_relaxed);

        auto sharedState =
            std::make_shared<
                detail::TaskSharedState>(
                    taskId);

        if (!sharedState->completionEvent.IsValid())
        {
            return {};
        }

        TaskHandle handle(sharedState);

        if (cancellationToken.
                IsCancellationRequested())
        {
            submittedTaskCount_.fetch_add(
                1,
                std::memory_order_relaxed);

            cancelledTaskCount_.fetch_add(
                1,
                std::memory_order_relaxed);

            FinishSharedState(
                sharedState,
                TaskState::Cancelled);

            return handle;
        }

        bool fenceAdded = false;

        if (fence != nullptr)
        {
            fenceAdded =
                fence->Add();

            if (!fenceAdded)
            {
                submittedTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);

                failedTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);

                FinishSharedState(
                    sharedState,
                    TaskState::Failed);

                return handle;
            }
        }

        detail::QueuedTask task;
        task.id = taskId;
        task.priority = priority;
        task.callback =
            std::move(callback);
        task.cancellationToken =
            std::move(cancellationToken);
        task.sharedState = sharedState;
        task.fence = fence;

        bool published = false;

        {
            engine::platform::MutexLockGuard lock(
                queueMutex_);

            if (!IsAcceptingTasks())
            {
                if (fenceAdded)
                {
                    CompleteFence(fence);
                }

                submittedTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);

                cancelledTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);

                FinishSharedState(
                    sharedState,
                    TaskState::Cancelled);

                return handle;
            }

            queue_->Push(
                std::move(task));

            if (workAvailable_.Release())
            {
                published = true;
            }
            else
            {
                detail::QueuedTask removedTask;

                (void)queue_->RemoveById(
                    taskId,
                    removedTask);
            }
        }

        submittedTaskCount_.fetch_add(
            1,
            std::memory_order_relaxed);

        if (!published)
        {
            if (fenceAdded)
            {
                CompleteFence(fence);
            }

            failedTaskCount_.fetch_add(
                1,
                std::memory_order_relaxed);

            FinishSharedState(
                sharedState,
                TaskState::Failed);
        }

        return handle;
    }

    std::size_t
        JobSystem::GetPendingTaskCount() const noexcept
    {
        engine::platform::MutexLockGuard lock(
            queueMutex_);

        if (queue_ == nullptr)
        {
            return 0;
        }

        return queue_->GetSize();
    }

    JobSystemStats JobSystem::GetStats() const noexcept
    {
        JobSystemStats stats;

        stats.submittedTaskCount =
            submittedTaskCount_.load(
                std::memory_order_acquire);

        stats.completedTaskCount =
            completedTaskCount_.load(
                std::memory_order_acquire);

        stats.cancelledTaskCount =
            cancelledTaskCount_.load(
                std::memory_order_acquire);

        stats.failedTaskCount =
            failedTaskCount_.load(
                std::memory_order_acquire);

        stats.runningTaskCount =
            runningTaskCount_.load(
                std::memory_order_acquire);

        stats.pendingTaskCount =
            GetPendingTaskCount();

        return stats;
    }

    void JobSystem::WorkerMain(
        const std::uint32_t workerIndex) noexcept
    {
        const std::wstring workerName =
            workerNamePrefix_ +
            L"." +
            std::to_wstring(
                workerIndex);

        (void)engine::platform::
            SetCurrentThreadName(
                workerName.c_str());

        for (;;)
        {
            const engine::platform::WaitResult
                waitResult =
                    workAvailable_.Wait();

            if (waitResult ==
                engine::platform::WaitResult::Failed)
            {
                if (stopping_.load(
                        std::memory_order_acquire))
                {
                    break;
                }

                engine::platform::
                    YieldCurrentThread();

                continue;
            }

            if (stopping_.load(
                    std::memory_order_acquire))
            {
                break;
            }

            detail::QueuedTask task;
            bool taskFound = false;

            {
                engine::platform::MutexLockGuard lock(
                    queueMutex_);

                if (stopping_.load(
                        std::memory_order_acquire))
                {
                    break;
                }

                if (queue_ != nullptr)
                {
                    taskFound =
                        queue_->TryPop(task);
                }
            }

            if (!taskFound)
            {
                continue;
            }

            ExecuteTask(
                std::move(task));
        }
    }

    void JobSystem::ExecuteTask(
        detail::QueuedTask task) noexcept
    {
        if (task.sharedState == nullptr)
        {
            CompleteFence(task.fence);
            return;
        }

        if (task.cancellationToken.
                IsCancellationRequested())
        {
            cancelledTaskCount_.fetch_add(
                1,
                std::memory_order_relaxed);

            FinishSharedState(
                task.sharedState,
                TaskState::Cancelled);

            CompleteFence(task.fence);
            return;
        }

        task.sharedState->state.store(
            TaskState::Running,
            std::memory_order_release);

        runningTaskCount_.fetch_add(
            1,
            std::memory_order_relaxed);

        TaskState finalState =
            TaskState::Completed;

        try
        {
            task.callback(
                task.cancellationToken);

            if (task.cancellationToken.
                    IsCancellationRequested())
            {
                finalState =
                    TaskState::Cancelled;
            }
        }
        catch (...)
        {
            finalState =
                TaskState::Failed;
        }

        runningTaskCount_.fetch_sub(
            1,
            std::memory_order_relaxed);

        switch (finalState)
        {
            case TaskState::Completed:
                completedTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);
                break;

            case TaskState::Cancelled:
                cancelledTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);
                break;

            case TaskState::Failed:
                failedTaskCount_.fetch_add(
                    1,
                    std::memory_order_relaxed);
                break;

            default:
                break;
        }

        FinishSharedState(
            task.sharedState,
            finalState);

        CompleteFence(task.fence);
    }

    void JobSystem::CancelTask(
        detail::QueuedTask& task) noexcept
    {
        cancelledTaskCount_.fetch_add(
            1,
            std::memory_order_relaxed);

        FinishSharedState(
            task.sharedState,
            TaskState::Cancelled);

        CompleteFence(task.fence);
    }

    std::uint32_t JobSystem::ResolveWorkerCount(
        const std::uint32_t requestedWorkerCount) noexcept
    {
        if (requestedWorkerCount != 0)
        {
            return std::min(
                requestedWorkerCount,
                MaximumWorkerCount);
        }

        const engine::platform::SystemInfo
            systemInfo =
                engine::platform::
                    QuerySystemInfo();

        std::uint32_t workerCount = 1;

        if (systemInfo.logicalProcessorCount > 1)
        {
            workerCount =
                systemInfo.logicalProcessorCount - 1;
        }

        return std::min(
            workerCount,
            MaximumWorkerCount);
    }
}