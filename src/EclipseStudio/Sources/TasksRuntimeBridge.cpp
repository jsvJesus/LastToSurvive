#include "r3dPCH.h"
#include "r3d.h"

#include "TasksRuntimeBridge.h"
#include "StudioRuntimeBridge.h"

#include "r3dBackgroundTaskDispatcher.h"
#include "r3dDeviceQueue.h"

#include <Runtime/Engine.h>

#include <Tasks/JobSystem.h>
#include <Tasks/MainThreadDispatcher.h>

#include <Platform/Thread.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

extern CRITICAL_SECTION
    g_ResourceCritSection;

namespace
{
    constexpr std::size_t
        MaximumCallbacksDuringWait = 64;

    std::unique_ptr<engine::tasks::JobSystem>
        g_legacyJobSystem;

    std::atomic<bool>
        g_initialized{false};

    std::atomic<bool>
        g_shuttingDown{false};

    std::atomic<bool>
        g_acceptingLegacyTasks{false};

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

    [[nodiscard]] engine::runtime::Engine*
        ResolveRuntimeEngine() noexcept
    {
        return studio::TryGetRuntimeEngine();
    }

    [[nodiscard]] engine::tasks::JobSystem*
        ResolveJobSystem() noexcept
    {
        engine::runtime::Engine* const engine =
            ResolveRuntimeEngine();

        if (engine == nullptr)
        {
            return nullptr;
        }

        return engine->GetServices().
            TryGet<
                engine::tasks::JobSystem>();
    }

    [[nodiscard]]
        engine::tasks::MainThreadDispatcher*
            ResolveMainThreadDispatcher() noexcept
    {
        engine::runtime::Engine* const engine =
            ResolveRuntimeEngine();

        if (engine == nullptr)
        {
            return nullptr;
        }

        return engine->GetServices().
            TryGet<
                engine::tasks::
                    MainThreadDispatcher>();
    }

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
            const LegacyTaskCompletionGuard&) =
                delete;

        LegacyTaskCompletionGuard& operator=(
            const LegacyTaskCompletionGuard&) =
                delete;

    private:
        r3dBackgroundTaskDispatcher::
            TaskDescriptor descriptor_;
    };

    [[nodiscard]] bool SubmitLegacyBackgroundTask(
        const r3dBackgroundTaskDispatcher::
            TaskDescriptor& descriptor)
    {
        if (
            !g_initialized.load(
                std::memory_order_acquire) ||
            g_shuttingDown.load(
                std::memory_order_acquire) ||
            !g_acceptingLegacyTasks.load(
                std::memory_order_acquire) ||
            g_legacyJobSystem == nullptr ||
            !g_legacyJobSystem->
                IsAcceptingTasks()
        )
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

                        if (descriptor.Fn == nullptr)
                        {
                            return;
                        }

                        /*
                         * Старый dispatcher выполнял
                         * задачи под этим lock.
                         */
                        r3dCSHolder resourceLock(
                            g_ResourceCritSection);

                        descriptor.Fn(
                            descriptor.Params);
                    },
                    engine::tasks::
                        TaskPriority::Normal);

            if (!handle.IsValid())
            {
                g_outstandingLegacyTaskCount.
                    fetch_sub(
                        1,
                        std::memory_order_acq_rel);

                return false;
            }

            const engine::tasks::TaskState state =
                handle.GetState();

            if (
                state ==
                    engine::tasks::
                        TaskState::Failed ||
                state ==
                    engine::tasks::
                        TaskState::Cancelled
            )
            {
                g_outstandingLegacyTaskCount.
                    fetch_sub(
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
                    "[Tasks] Legacy task routed "
                    "through Runtime task services: "
                    "taskId=%llu\n",
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
                "[Tasks] Legacy task submission "
                "failed; using old dispatcher\n");

            return false;
        }
    }

    void DispatchCallbacksDuringLegacyWait() noexcept
    {
        engine::tasks::MainThreadDispatcher*
            const dispatcher =
                ResolveMainThreadDispatcher();

        if (dispatcher == nullptr)
        {
            return;
        }

        try
        {
            (void)dispatcher->Dispatch(
                MaximumCallbacksDuringWait);
        }
        catch (...)
        {
            r3dOutToLog(
                "[Tasks] Main-thread callback "
                "failed during legacy wait\n");
        }
    }

    void WaitForLegacyBackgroundTasks()
    {
        while (
            g_outstandingLegacyTaskCount.load(
                std::memory_order_acquire) != 0)
        {
            /*
             * Старые asset tasks могут ждать
             * выполнения D3D device queue.
             */
            ProcessDeviceQueue(
                r3dGetTime(),
                1.0f);

            DispatchCallbacksDuringLegacyWait();

            engine::platform::
                YieldCurrentThread();
        }
    }

    void StartRuntimeProbe() noexcept
    {
        engine::tasks::JobSystem* const jobSystem =
            ResolveJobSystem();

        if (jobSystem == nullptr)
        {
            return;
        }

        try
        {
            const engine::tasks::TaskHandle probe =
                jobSystem->Submit(
                    [](
                        const engine::tasks::
                            CancellationToken&)
                    {
                        const std::uint32_t
                            workerThreadId =
                                engine::platform::
                                    GetCurrentThreadId();

                        (void)studio::PostToMainThread(
                            [workerThreadId]()
                            {
                                engine::runtime::Engine*
                                    const runtimeEngine =
                                        studio::
                                            TryGetRuntimeEngine();

                                const std::uint32_t
                                    currentThreadId =
                                        engine::platform::
                                            GetCurrentThreadId();

                                const std::uint32_t
                                    ownerThreadId =
                                        runtimeEngine !=
                                            nullptr
                                            ? runtimeEngine->
                                                GetOwnerThreadId()
                                            : 0;

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
                                            currentThreadId),
                                    static_cast<
                                        unsigned int>(
                                            ownerThreadId),
                                    currentThreadId ==
                                            ownerThreadId
                                        ? 1
                                        : 0);
                            });
                    },
                    engine::tasks::
                        TaskPriority::Critical);

            if (!probe.IsValid())
            {
                r3dOutToLog(
                    "[Tasks] Runtime probe "
                    "submission failed\n");
            }
        }
        catch (...)
        {
            r3dOutToLog(
                "[Tasks] Runtime probe "
                "creation failed\n");
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

        if (!IsStudioRuntimeBridgeInitialized())
        {
            r3dOutToLog(
                "[Tasks] Runtime bridge requires "
                "an initialized Studio Runtime\n");

            return false;
        }

        engine::tasks::JobSystem* const jobSystem =
            ResolveJobSystem();

        engine::tasks::MainThreadDispatcher*
            const dispatcher =
                ResolveMainThreadDispatcher();

        if (
            jobSystem == nullptr ||
            dispatcher == nullptr
        )
        {
            r3dOutToLog(
                "[Tasks] Runtime task services "
                "are unavailable\n");

            return false;
        }

        try
        {
            auto legacyJobSystem =
                std::make_unique<
                    engine::tasks::JobSystem>();

            engine::tasks::JobSystemConfig
                legacyConfig;

            /*
             * Один worker сохраняет FIFO-поведение
             * старого background dispatcher.
             */
            legacyConfig.workerCount = 1;

            legacyConfig.workerNamePrefix =
                L"LTS.LegacyWorker";

            if (!legacyJobSystem->Initialize(
                    legacyConfig))
            {
                r3dOutToLog(
                    "[Tasks] Legacy compatibility "
                    "worker initialization failed\n");

                return false;
            }

            g_legacyJobSystem =
                std::move(legacyJobSystem);

            g_shuttingDown.store(
                false,
                std::memory_order_release);

            g_acceptingLegacyTasks.store(
                true,
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

            g_finishedLegacyTaskCount.store(
                0,
                std::memory_order_release);

            g_peakOutstandingLegacyTaskCount.store(
                0,
                std::memory_order_release);

            g_initialized.store(
                true,
                std::memory_order_release);

            g_r3dBackgroundTaskSubmitBridge =
                &SubmitLegacyBackgroundTask;

            g_r3dBackgroundTaskWaitBridge =
                &WaitForLegacyBackgroundTasks;

            r3dOutToLog(
                "[Tasks] Runtime bridge initialized: "
                "modernWorkers=%u, "
                "legacyWorkers=%u, "
                "runtimeOwned=1\n",
                static_cast<unsigned int>(
                    jobSystem->GetWorkerCount()),
                static_cast<unsigned int>(
                    g_legacyJobSystem->
                        GetWorkerCount()));

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

            if (g_legacyJobSystem != nullptr)
            {
                g_legacyJobSystem->Shutdown();
            }

            g_legacyJobSystem.reset();

            r3dOutToLog(
                "[Tasks] Runtime bridge "
                "initialization failed "
                "with exception\n");

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

        g_acceptingLegacyTasks.store(
            false,
            std::memory_order_release);

        if (
            g_r3dBackgroundTaskSubmitBridge ==
            &SubmitLegacyBackgroundTask
        )
        {
            g_r3dBackgroundTaskSubmitBridge =
                nullptr;
        }

        /*
         * Ждём старый dispatcher и уже принятые
         * compatibility jobs до остановки worker.
         */
        r3dFinishBackGroundTasks();

        if (
            g_r3dBackgroundTaskWaitBridge ==
            &WaitForLegacyBackgroundTasks
        )
        {
            g_r3dBackgroundTaskWaitBridge =
                nullptr;
        }

        if (g_legacyJobSystem != nullptr)
        {
            g_legacyJobSystem->Shutdown();
        }

        const engine::tasks::JobSystem* const
            modernJobSystem =
                ResolveJobSystem();

        const engine::tasks::JobSystemStats
            modernStats =
                modernJobSystem != nullptr
                    ? modernJobSystem->GetStats()
                    : engine::tasks::
                        JobSystemStats{};

        r3dOutToLog(
            "[Tasks] Runtime bridge shutdown: "
            "runtimeOwned=1, "
            "modernSubmitted=%llu, "
            "modernCompleted=%llu, "
            "modernCancelled=%llu, "
            "legacySubmitted=%llu, "
            "legacyFinished=%llu, "
            "legacyPeak=%llu, "
            "legacyOutstanding=%llu\n",
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
                g_peakOutstandingLegacyTaskCount.
                    load(
                        std::memory_order_acquire)),
            static_cast<unsigned long long>(
                g_outstandingLegacyTaskCount.load(
                    std::memory_order_acquire)));

        r3d_assert(
            g_outstandingLegacyTaskCount.load(
                std::memory_order_acquire) == 0);

        r3d_assert(
            g_submittedLegacyTaskCount.load(
                std::memory_order_acquire) ==
            g_finishedLegacyTaskCount.load(
                std::memory_order_acquire));

        g_legacyJobSystem.reset();

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
        return ResolveJobSystem();
    }

    engine::tasks::MainThreadDispatcher*
        TryGetMainThreadDispatcher() noexcept
    {
        return ResolveMainThreadDispatcher();
    }

    bool PostToMainThread(
        MainThreadCallback callback)
    {
        if (!callback)
        {
            return false;
        }

        engine::tasks::MainThreadDispatcher*
            const dispatcher =
                ResolveMainThreadDispatcher();

        if (dispatcher == nullptr)
        {
            return false;
        }

        try
        {
            return dispatcher->Post(
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