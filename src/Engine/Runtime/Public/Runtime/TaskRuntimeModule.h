#pragma once

#include "Runtime/RuntimeModule.h"

#include <Tasks/JobSystem.h>
#include <Tasks/MainThreadDispatcher.h>

#include <cstddef>
#include <cstdint>

namespace engine::runtime
{
    struct TaskRuntimeConfig final
    {
        engine::tasks::JobSystemConfig jobSystemConfig;

        std::size_t maximumCallbacksPerFrame = 1024;

        std::size_t maximumCallbacksPerShutdownBatch =
            1024;
    };

    struct TaskRuntimeStats final
    {
        std::uint64_t dispatchedCallbackCount = 0;

        std::uint64_t dispatchFailureCount = 0;

        std::size_t abandonedCallbackCount = 0;
    };

    class TaskRuntimeModule final
        : public RuntimeModule
    {
    public:
        explicit TaskRuntimeModule(
            TaskRuntimeConfig config = {});

        ~TaskRuntimeModule() override = default;

        [[nodiscard]] const char*
            GetName() const noexcept override;

        [[nodiscard]] bool Initialize(
            Engine& engine) override;

        void Shutdown(
            Engine& engine) noexcept override;

        void BeginFrame(
            Engine& engine,
            const FrameContext& frameContext)
                noexcept override;

        [[nodiscard]] bool
            IsInitialized() const noexcept;

        [[nodiscard]] engine::tasks::JobSystem*
            GetJobSystem() noexcept;

        [[nodiscard]]
            engine::tasks::MainThreadDispatcher*
                GetMainThreadDispatcher() noexcept;

        [[nodiscard]] const TaskRuntimeStats&
            GetStats() const noexcept;

    private:
        void DispatchMainThreadCallbacks(
            std::size_t maximumCount) noexcept;

        void DrainMainThreadCallbacks() noexcept;

        TaskRuntimeConfig config_;

        engine::tasks::JobSystem jobSystem_;

        engine::tasks::MainThreadDispatcher
            mainThreadDispatcher_;

        TaskRuntimeStats stats_;

        bool initialized_ = false;
    };
}