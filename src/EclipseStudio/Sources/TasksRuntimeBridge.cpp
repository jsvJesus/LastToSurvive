#include "r3dPCH.h"
#include "r3d.h"

#include "TasksRuntimeBridge.h"

#include "r3dBackgroundTaskDispatcher.h"
#include "r3dDeviceQueue.h"

#include <Platform/Thread.h>

#include <Tasks/JobSystem.h>
#include <Tasks/MainThreadDispatcher.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

using FrameStartCallback =
    void (*)();

/*
 * Эти символы определены в Eternity.
 * Объявления должны оставаться в глобальном namespace.
 */
extern FrameStartCallback
    r3dFrameStartCallback;

extern CRITICAL_SECTION
    g_ResourceCritSection;

namespace
{
    constexpr std::size_t
        MaximumCallbacksPerFrame = 1024;

    constexpr std::size_t
        MaximumCallbacksDuringWait = 64;

    std::unique_ptr<engine::tasks::JobSystem>
        g_jobSystem;

    std::unique_ptr<engine::tasks::JobSystem>
        g_legacyJobSystem;

    std::unique_ptr<
        engine::tasks::MainThreadDispatcher>
            g_mainThreadDispatcher;

    std::atomic<bool>
        g_initialized{false};

    std::atomic<bool>
        g_shuttingDown{false};

    std::atomic<bool>
        g_acceptingLegacyTasks{false};

    std::atomic<bool>
        g_threadMismatchLogged{false};

    std::atomic<bool>
        g_firstLegacyTaskLogged{false};

    std::atomic<std::uint64_t>
        g_outstandingLegacyTaskCount{0};

    std::atomic<std::uint64_t>
        g_submittedLegacyTaskCount{0};

    std::atomic<std::uint64_t>
        g_finishedLegacyTaskCount{0};

    std::atomic<std::uint64_t>
        g_peakOutstandingLegacyTaskCount{0};

    std::uint32_t
        g_ownerThreadId = 0;

    std::uint64_t
        g_frameCount = 0;

    std::uint64_t
        g_dispatchedCallbackCount = 0;

    bool g_firstFrameLogged = false;

    FrameStartCallback
        g_previousFrameStartCallback = nullptr;

    [[nodiscard]] bool IsOwnerThread() noexcept
    {
        return
            g_ownerThreadId != 0 &&
            engine::platform::GetCurrentThreadId() ==
                g_ownerThreadId;
    }

    void LogThreadMismatchOnce(
        const char* location) noexcept
    {
        bool expected = false;

        if (!g_threadMismatchLogged.
                compare_exchange_strong(
                    expected,
                    true,
                    std::memory_order_acq_rel))
        {
            return;
        }

        r3dOutToLog(
            "[Tasks] Runtime bridge thread mismatch "
            "at %s: owner=%u, current=%u\n",
            location,
            static_cast<unsigned int>(
                g_ownerThreadId),
            static_cast<unsigned int>(
                engine::platform::
                    GetCurrentThreadId()));
    }

    [[nodiscard]] std::size_t
        DispatchMainThreadCallbacks(
            const std::size_t maximumCount) noexcept
    {
        if (g_mainThreadDispatcher == nullptr)
        {
            return 0;
        }

        try
        {
            return
                g_mainThreadDispatcher->Dispatch(
                    maximumCount);
        }
        catch (...)
        {
            r3dOutToLog(
                "[Tasks] Main-thread callback "
                "threw an exception\n");

            return 0;
        }
    }

    void TasksFrameStartCallback()
    {
        /*
         * Сначала запускаем предыдущий hook.
         * Сейчас это должен быть Platform Input BeginFrame.
         */
        if (g_previousFrameStartCallback != nullptr)
        {
            g_previousFrameStartCallback();
        }

        if (!g_initialized.load(
                std::memory_order_acquire))
        {
            return;
        }

        if (g_shuttingDown.load(
                std::memory_order_acquire))
        {
            return;
        }

        if (!IsOwnerThread())
        {
            LogThreadMismatchOnce(
                "BeginFrame");

            return;
        }

        const std::size_t dispatchedCount =
            DispatchMainThreadCallbacks(
                MaximumCallbacksPerFrame);

        ++g_frameCount;

        g_dispatchedCallbackCount +=
            static_cast<std::uint64_t>(
                dispatchedCount);

        if (!g_firstFrameLogged)
        {
            g_firstFrameLogged = true;

            const std::size_t pendingCount =
                g_mainThreadDispatcher != nullptr
                    ? g_mainThreadDispatcher->
                        GetPendingCount()
                    : 0;

            r3dOutToLog(
                "[Tasks] Runtime bridge first frame: "
                "thread=%u, dispatched=%u, pending=%u\n",
                static_cast<unsigned int>(
                    g_ownerThreadId),
                static_cast<unsigned int>(
                    dispatchedCount),
                static_cast<unsigned int>(
                    pendingCount));
        }
    }

    class LegacyTaskCompletionGuard final
    {
    public:
        explicit LegacyTaskCompletionGuard(
            const r3dBackgroundTaskDispatcher::
                TaskDescriptor& descriptor) noexcept
            : descriptor_(descriptor)
        {
        }

        ~LegacyTaskCompletionGuard() noexcept
        {
            /*
             * Полностью повторяем завершение задачи
             * из старого dispatcher.
             */
            if (descriptor_.CompletionFlag != nullptr)
            {
                InterlockedExchange(
                    descriptor_.CompletionFlag,
                    1L);
            }

            if (descriptor_.Params != nullptr)
            {
                InterlockedExchange(
                    &descriptor_.Params->Taken,
                    0L);
            }

            g_finishedLegacyTaskCount.fetch_add(
                1,
                std::memory_order_relaxed);

            g_outstandingLegacyTaskCount.fetch_sub(
                1,
                std::memory_order_acq_rel);
        }

        LegacyTaskCompletionGuard(
            const LegacyTaskCompletionGuard&) = delete;

        LegacyTaskCompletionGuard& operator=(
            const LegacyTaskCompletionGuard&) = delete;

    private:
        r3dBackgroundTaskDispatcher::
            TaskDescriptor descriptor_;
    };

    void UpdatePeakOutstandingTaskCount(
    const std::uint64_t currentCount) noexcept
    {
        std::uint64_t currentPeak =
            g_peakOutstandingLegacyTaskCount.load(
                std::memory_order_relaxed);

        while (
            currentPeak < currentCount &&
            !g_peakOutstandingLegacyTaskCount.
                compare_exchange_weak(
                    currentPeak,
                    currentCount,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)
        )
        {
        }
    }

    [[nodiscard]] bool SubmitLegacyBackgroundTask(
        const r3dBackgroundTaskDispatcher::
            TaskDescriptor& descriptor)
    {
        if (!g_initialized.load(
                std::memory_order_acquire) ||
            g_shuttingDown.load(
                std::memory_order_acquire) ||
            !g_acceptingLegacyTasks.load(
                std::memory_order_acquire) ||
            g_legacyJobSystem == nullptr ||
            !g_legacyJobSystem->IsAcceptingTasks())
        {
            return false;
        }

        const std::uint64_t outstandingCount =
        g_outstandingLegacyTaskCount.fetch_add(
            1,
            std::memory_order_acq_rel) + 1;

            UpdatePeakOutstandingTaskCount(
                outstandingCount);

        try
        {
            const engine::tasks::TaskHandle handle =
                g_legacyJobSystem->Submit(
                    [descriptor](
                        const engine::tasks::
                            CancellationToken&)
                    {
                        LegacyTaskCompletionGuard
                            completionGuard(
                                descriptor);

                        if (descriptor.Fn != nullptr)
                        {
                            /*
                             * Старый dispatcher выполнял
                             * каждую задачу под этим lock.
                             */
                            r3dCSHolder resourceLock(
                                g_ResourceCritSection);

                            descriptor.Fn(
                                descriptor.Params);
                        }
                    },
                    engine::tasks::
                        TaskPriority::Normal);

            if (!handle.IsValid())
            {
                g_outstandingLegacyTaskCount.fetch_sub(
                    1,
                    std::memory_order_acq_rel);

                return false;
            }

            g_submittedLegacyTaskCount.fetch_add(
                1,
                std::memory_order_relaxed);

            bool expected = false;

            if (g_firstLegacyTaskLogged.
                    compare_exchange_strong(
                        expected,
                        true,
                        std::memory_order_acq_rel))
            {
                r3dOutToLog(
                    "[Tasks] Legacy background task "
                    "migrated to LTS.Tasks: taskId=%llu\n",
                    static_cast<
                        unsigned long long>(
                            handle.GetId()));
            }

            return true;
        }
        catch (...)
        {
            g_outstandingLegacyTaskCount.fetch_sub(
                1,
                std::memory_order_acq_rel);

            r3dOutToLog(
                "[Tasks] Legacy submission failed; "
                "falling back to old dispatcher\n");

            return false;
        }
    }

    void WaitForLegacyBackgroundTasks()
    {
        while (
            g_outstandingLegacyTaskCount.load(
                std::memory_order_acquire) != 0)
        {
            /*
             * Legacy asset loading может отправлять
             * D3D9-команды в main-thread device queue.
             */
            ProcessDeviceQueue(
                r3dGetTime(),
                1.0f);

            if (IsOwnerThread())
            {
                (void)DispatchMainThreadCallbacks(
                    MaximumCallbacksDuringWait);
            }

            engine::platform::
                YieldCurrentThread();
        }
    }

    void DrainMainThreadCallbacks() noexcept
    {
        if (g_mainThreadDispatcher == nullptr ||
            !IsOwnerThread())
        {
            return;
        }

        for (;;)
        {
            const std::size_t pendingCount =
                g_mainThreadDispatcher->
                    GetPendingCount();

            if (pendingCount == 0)
            {
                break;
            }

            const std::size_t dispatchedCount =
                DispatchMainThreadCallbacks(
                    MaximumCallbacksPerFrame);

            g_dispatchedCallbackCount +=
                static_cast<std::uint64_t>(
                    dispatchedCount);

            if (dispatchedCount == 0)
            {
                break;
            }
        }
    }

    void StartRuntimeProbe() noexcept
    {
        if (g_jobSystem == nullptr)
        {
            return;
        }

        try
        {
            const engine::tasks::TaskHandle probe =
                g_jobSystem->Submit(
                    [](
                        const engine::tasks::
                            CancellationToken&)
                    {
                        const std::uint32_t
                            workerThreadId =
                                engine::platform::
                                    GetCurrentThreadId();

                        if (g_mainThreadDispatcher ==
                            nullptr)
                        {
                            return;
                        }

                        try
                        {
                            (void)g_mainThreadDispatcher->
                                Post(
                                    [workerThreadId]()
                                    {
                                        const std::uint32_t
                                            mainThreadId =
                                                engine::
                                                    platform::
                                                    GetCurrentThreadId();

                                        r3dOutToLog(
                                            "[Tasks] Runtime probe: "
                                            "workerThread=%u, "
                                            "mainThread=%u, "
                                            "ownerThread=%u, "
                                            "onOwner=%d\n",
                                            static_cast<
                                                unsigned int>(
                                                    workerThreadId),
                                            static_cast<
                                                unsigned int>(
                                                    mainThreadId),
                                            static_cast<
                                                unsigned int>(
                                                    g_ownerThreadId),
                                            mainThreadId ==
                                                    g_ownerThreadId
                                                ? 1
                                                : 0);
                                    });
                        }
                        catch (...)
                        {
                        }
                    },
                    engine::tasks::
                        TaskPriority::Critical);

            if (!probe.IsValid())
            {
                r3dOutToLog(
                    "[Tasks] Runtime probe submission "
                    "failed\n");
            }
        }
        catch (...)
        {
            r3dOutToLog(
                "[Tasks] Runtime probe creation failed\n");
        }
    }
}

namespace studio
{
    bool InitializeTasksRuntimeBridge() noexcept
    {
        if (g_initialized.load(
                std::memory_order_acquire))
        {
            return true;
        }

        try
        {
            const std::uint32_t ownerThreadId =
                engine::platform::
                    GetCurrentThreadId();

            if (ownerThreadId == 0)
            {
                r3dOutToLog(
                    "[Tasks] Runtime bridge initialization "
                    "failed: invalid owner thread\n");

                return false;
            }

            auto mainThreadDispatcher =
                std::make_unique<
                    engine::tasks::
                        MainThreadDispatcher>();

            if (!mainThreadDispatcher->Initialize())
            {
                r3dOutToLog(
                    "[Tasks] Runtime bridge initialization "
                    "failed: dispatcher\n");

                return false;
            }

            auto jobSystem =
                std::make_unique<
                    engine::tasks::JobSystem>();

            engine::tasks::JobSystemConfig
                jobSystemConfig;

            jobSystemConfig.workerCount = 0;

            jobSystemConfig.workerNamePrefix =
                L"LTS.Worker";

            if (!jobSystem->Initialize(
                    jobSystemConfig))
            {
                r3dOutToLog(
                    "[Tasks] Runtime bridge initialization "
                    "failed: JobSystem\n");

                return false;
            }

            /*
             * Отдельный worker сохраняет FIFO-семантику
             * старого r3dBackgroundTaskDispatcher.
             */
            auto legacyJobSystem =
                std::make_unique<
                    engine::tasks::JobSystem>();

            engine::tasks::JobSystemConfig
                legacyConfig;

            legacyConfig.workerCount = 1;

            legacyConfig.workerNamePrefix =
                L"LTS.LegacyWorker";

            if (!legacyJobSystem->Initialize(
                    legacyConfig))
            {
                r3dOutToLog(
                    "[Tasks] Runtime bridge initialization "
                    "failed: legacy JobSystem\n");

                return false;
            }

            g_ownerThreadId =
                ownerThreadId;

            g_mainThreadDispatcher =
                std::move(mainThreadDispatcher);

            g_jobSystem =
                std::move(jobSystem);

            g_legacyJobSystem =
                std::move(legacyJobSystem);

            g_frameCount = 0;
            g_dispatchedCallbackCount = 0;

            g_firstFrameLogged = false;

            g_threadMismatchLogged.store(
                false,
                std::memory_order_release);

            g_firstLegacyTaskLogged.store(
                false,
                std::memory_order_release);

            g_outstandingLegacyTaskCount.store(
                0,
                std::memory_order_release);

            g_submittedLegacyTaskCount.store(
                0,
                std::memory_order_release);

            g_peakOutstandingLegacyTaskCount.store(
                0,
                std::memory_order_release);

            g_finishedLegacyTaskCount.store(
                0,
                std::memory_order_release);

            g_previousFrameStartCallback =
                r3dFrameStartCallback;

            if (g_previousFrameStartCallback ==
                &TasksFrameStartCallback)
            {
                g_previousFrameStartCallback =
                    nullptr;
            }

            g_shuttingDown.store(
                false,
                std::memory_order_release);

            g_initialized.store(
                true,
                std::memory_order_release);

            g_acceptingLegacyTasks.store(
                true,
                std::memory_order_release);

            /*
             * Устанавливаем optional legacy bridge.
             * При отказе Submit старый dispatcher
             * автоматически остаётся fallback.
             */
            g_r3dBackgroundTaskSubmitBridge =
                &SubmitLegacyBackgroundTask;

            g_r3dBackgroundTaskWaitBridge =
                &WaitForLegacyBackgroundTasks;

            r3dFrameStartCallback =
                &TasksFrameStartCallback;

            r3dOutToLog(
                "[Tasks] Runtime bridge initialized: "
                "workers=%u, legacyWorkers=%u, "
                "ownerThread=%u, frameHook=1\n",
                static_cast<unsigned int>(
                    g_jobSystem->GetWorkerCount()),
                static_cast<unsigned int>(
                    g_legacyJobSystem->
                        GetWorkerCount()),
                static_cast<unsigned int>(
                    g_ownerThreadId));

            StartRuntimeProbe();

            return true;
        }
        catch (...)
        {
            g_acceptingLegacyTasks.store(
                false,
                std::memory_order_release);

            g_initialized.store(
                false,
                std::memory_order_release);

            g_legacyJobSystem.reset();
            g_jobSystem.reset();
            g_mainThreadDispatcher.reset();

            g_ownerThreadId = 0;

            r3dOutToLog(
                "[Tasks] Runtime bridge initialization "
                "failed with exception\n");

            return false;
        }
    }

    void ShutdownTasksRuntimeBridge() noexcept
    {
        if (!g_initialized.load(
                std::memory_order_acquire))
        {
            return;
        }

        bool expected = false;

        if (!g_shuttingDown.
                compare_exchange_strong(
                    expected,
                    true,
                    std::memory_order_acq_rel))
        {
            return;
        }

        /*
         * Новые legacy submission теперь пойдут
         * обратно в старый dispatcher.
         */
        g_acceptingLegacyTasks.store(
            false,
            std::memory_order_release);

        if (g_r3dBackgroundTaskSubmitBridge ==
            &SubmitLegacyBackgroundTask)
        {
            g_r3dBackgroundTaskSubmitBridge =
                nullptr;
        }

        /*
         * Теперь функция ждёт и старую очередь,
         * и уже мигрированные задачи.
         */
        r3dFinishBackGroundTasks();

        if (g_r3dBackgroundTaskWaitBridge ==
            &WaitForLegacyBackgroundTasks)
        {
            g_r3dBackgroundTaskWaitBridge =
                nullptr;
        }

        /*
         * Tasks hook был установлен после Input hook,
         * поэтому снимается раньше него.
         */
        if (r3dFrameStartCallback ==
            &TasksFrameStartCallback)
        {
            r3dFrameStartCallback =
                g_previousFrameStartCallback;
        }

        g_previousFrameStartCallback =
            nullptr;

        if (g_legacyJobSystem != nullptr)
        {
            g_legacyJobSystem->Shutdown();
        }

        if (g_jobSystem != nullptr)
        {
            g_jobSystem->Shutdown();
        }

        /*
         * Worker threads уже остановлены, поэтому
         * новых callback больше появиться не может.
         */
        DrainMainThreadCallbacks();

        const std::size_t abandonedCallbacks =
            g_mainThreadDispatcher != nullptr
                ? g_mainThreadDispatcher->
                    GetPendingCount()
                : 0;

        if (g_mainThreadDispatcher != nullptr)
        {
            g_mainThreadDispatcher->Shutdown();
        }

        const engine::tasks::JobSystemStats
            modernStats =
                g_jobSystem != nullptr
                    ? g_jobSystem->GetStats()
                    : engine::tasks::
                        JobSystemStats{};

        r3dOutToLog(
            "[Tasks] Runtime bridge shutdown: "
            "frames=%llu, dispatched=%llu, "
            "modernSubmitted=%llu, "
            "modernCompleted=%llu, "
            "modernCancelled=%llu, "
            "legacySubmitted=%llu, "
            "legacyFinished=%llu, "
            "legacyPeak=%llu, "
            "legacyOutstanding=%llu, "
            "abandonedCallbacks=%u\n",
            static_cast<unsigned long long>(
                g_frameCount),
            static_cast<unsigned long long>(
                g_dispatchedCallbackCount),
            static_cast<unsigned long long>(
                modernStats.submittedTaskCount),
            static_cast<unsigned long long>(
                modernStats.completedTaskCount),
            static_cast<unsigned long long>(
                modernStats.cancelledTaskCount),
            static_cast<unsigned long long>(
                g_submittedLegacyTaskCount.load(
                    std::memory_order_acquire)),
            static_cast<unsigned long long>(
                g_finishedLegacyTaskCount.load(
                    std::memory_order_acquire)),
                    static_cast<unsigned long long>(
                g_peakOutstandingLegacyTaskCount.load(
                    std::memory_order_acquire)),
            static_cast<unsigned long long>(
                g_outstandingLegacyTaskCount.load(
                    std::memory_order_acquire)),
            static_cast<unsigned int>(
                abandonedCallbacks));

        r3d_assert(
            g_outstandingLegacyTaskCount.load(
                std::memory_order_acquire) == 0);

        g_mainThreadDispatcher.reset();
        g_legacyJobSystem.reset();
        g_jobSystem.reset();

        g_ownerThreadId = 0;
        g_frameCount = 0;
        g_dispatchedCallbackCount = 0;

        g_initialized.store(
            false,
            std::memory_order_release);

        g_shuttingDown.store(
            false,
            std::memory_order_release);
    }

    bool IsTasksRuntimeBridgeInitialized() noexcept
    {
        return
            g_initialized.load(
                std::memory_order_acquire) &&
            !g_shuttingDown.load(
                std::memory_order_acquire);
    }

    engine::tasks::JobSystem*
        TryGetJobSystem() noexcept
    {
        if (!IsTasksRuntimeBridgeInitialized())
        {
            return nullptr;
        }

        return g_jobSystem.get();
    }

    engine::tasks::MainThreadDispatcher*
        TryGetMainThreadDispatcher() noexcept
    {
        if (!IsTasksRuntimeBridgeInitialized())
        {
            return nullptr;
        }

        return g_mainThreadDispatcher.get();
    }

    bool PostToMainThread(
        MainThreadCallback callback)
    {
        if (!callback ||
            !IsTasksRuntimeBridgeInitialized() ||
            g_mainThreadDispatcher == nullptr)
        {
            return false;
        }

        try
        {
            return g_mainThreadDispatcher->Post(
                std::move(callback));
        }
        catch (...)
        {
            return false;
        }
    }

    std::uint64_t
        GetOutstandingLegacyTaskCount() noexcept
    {
        return
            g_outstandingLegacyTaskCount.load(
                std::memory_order_acquire);
    }
}