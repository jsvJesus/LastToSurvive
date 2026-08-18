#include "Runtime/TaskRuntimeModule.h"

#include "Runtime/Engine.h"

#include <utility>

namespace engine::runtime
{
    TaskRuntimeModule::TaskRuntimeModule(
        TaskRuntimeConfig config)
        : config_(std::move(config))
    {
        if (config_.maximumCallbacksPerFrame == 0)
        {
            config_.maximumCallbacksPerFrame = 1024;
        }

        if (
            config_.
                maximumCallbacksPerShutdownBatch == 0
        )
        {
            config_.
                maximumCallbacksPerShutdownBatch = 1024;
        }
    }

    const char*
        TaskRuntimeModule::GetName() const noexcept
    {
        return "Runtime.Tasks";
    }

    bool TaskRuntimeModule::Initialize(
        Engine& engine)
    {
        if (initialized_)
        {
            return false;
        }

        stats_ = {};

        bool jobSystemRegistered = false;

        bool dispatcherRegistered = false;

        ServiceRegistry& services =
            engine.GetServices();

        try
        {
            if (!mainThreadDispatcher_.Initialize())
            {
                return false;
            }

            if (!jobSystem_.Initialize(
                    config_.jobSystemConfig))
            {
                mainThreadDispatcher_.Shutdown();

                return false;
            }

            if (!services.Register<
                    engine::tasks::JobSystem>(
                        jobSystem_))
            {
                jobSystem_.Shutdown();

                mainThreadDispatcher_.Shutdown();

                return false;
            }

            jobSystemRegistered = true;

            if (!services.Register<
                    engine::tasks::
                        MainThreadDispatcher>(
                            mainThreadDispatcher_))
            {
                (void)services.Unregister<
                    engine::tasks::JobSystem>(
                        &jobSystem_);

                jobSystem_.Shutdown();

                mainThreadDispatcher_.Shutdown();

                return false;
            }

            dispatcherRegistered = true;

            initialized_ = true;

            return true;
        }
        catch (...)
        {
            if (dispatcherRegistered)
            {
                (void)services.Unregister<
                    engine::tasks::
                        MainThreadDispatcher>(
                            &mainThreadDispatcher_);
            }

            if (jobSystemRegistered)
            {
                (void)services.Unregister<
                    engine::tasks::JobSystem>(
                        &jobSystem_);
            }

            jobSystem_.Shutdown();

            mainThreadDispatcher_.Shutdown();

            initialized_ = false;

            return false;
        }
    }

    void TaskRuntimeModule::Shutdown(
        Engine& engine) noexcept
    {
        ServiceRegistry& services =
            engine.GetServices();

        /*
         * Сначала запрещаем получение новых
         * указателей через ServiceRegistry.
         */
        (void)services.Unregister<
            engine::tasks::MainThreadDispatcher>(
                &mainThreadDispatcher_);

        (void)services.Unregister<
            engine::tasks::JobSystem>(
                &jobSystem_);

        /*
         * После Shutdown workers больше не смогут
         * публиковать новые main-thread callbacks.
         */
        jobSystem_.Shutdown();

        DrainMainThreadCallbacks();

        stats_.abandonedCallbackCount =
            mainThreadDispatcher_.
                GetPendingCount();

        mainThreadDispatcher_.Shutdown();

        initialized_ = false;
    }

    void TaskRuntimeModule::BeginFrame(
        Engine& engine,
        const FrameContext& frameContext) noexcept
    {
        (void)engine;
        (void)frameContext;

        if (!initialized_)
        {
            return;
        }

        DispatchMainThreadCallbacks(
            config_.maximumCallbacksPerFrame);
    }

    bool TaskRuntimeModule::
        IsInitialized() const noexcept
    {
        return initialized_;
    }

    engine::tasks::JobSystem*
        TaskRuntimeModule::GetJobSystem() noexcept
    {
        return initialized_
            ? &jobSystem_
            : nullptr;
    }

    engine::tasks::MainThreadDispatcher*
        TaskRuntimeModule::
            GetMainThreadDispatcher() noexcept
    {
        return initialized_
            ? &mainThreadDispatcher_
            : nullptr;
    }

    const TaskRuntimeStats&
        TaskRuntimeModule::GetStats() const noexcept
    {
        return stats_;
    }

    void TaskRuntimeModule::
        DispatchMainThreadCallbacks(
            const std::size_t maximumCount) noexcept
    {
        if (
            maximumCount == 0 ||
            !mainThreadDispatcher_.IsInitialized()
        )
        {
            return;
        }

        try
        {
            const std::size_t dispatchedCount =
                mainThreadDispatcher_.Dispatch(
                    maximumCount);

            stats_.dispatchedCallbackCount +=
                static_cast<std::uint64_t>(
                    dispatchedCount);
        }
        catch (...)
        {
            ++stats_.dispatchFailureCount;
        }
    }

    void TaskRuntimeModule::
        DrainMainThreadCallbacks() noexcept
    {
        if (!mainThreadDispatcher_.IsInitialized())
        {
            return;
        }

        for (;;)
        {
            const std::size_t pendingCount =
                mainThreadDispatcher_.
                    GetPendingCount();

            if (pendingCount == 0)
            {
                break;
            }

            std::size_t dispatchedCount = 0;

            try
            {
                dispatchedCount =
                    mainThreadDispatcher_.Dispatch(
                        config_.
                            maximumCallbacksPerShutdownBatch);
            }
            catch (...)
            {
                ++stats_.dispatchFailureCount;

                break;
            }

            stats_.dispatchedCallbackCount +=
                static_cast<std::uint64_t>(
                    dispatchedCount);

            /*
             * Dispatch возвращает 0, если Shutdown
             * выполняется не на owner thread.
             */
            if (dispatchedCount == 0)
            {
                break;
            }
        }
    }
}