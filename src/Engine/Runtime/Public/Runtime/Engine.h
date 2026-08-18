#pragma once

#include "Runtime/EngineConfig.h"
#include "Runtime/FrameContext.h"
#include "Runtime/ServiceRegistry.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace engine::runtime
{
    class RuntimeModule;

    enum class EngineState : std::uint8_t
    {
        Uninitialized = 0,
        Initializing,
        Running,
        Stopping,
        Stopped,
        Failed
    };

    [[nodiscard]] const char* ToString(
        EngineState state) noexcept;

    class Engine final
    {
    public:
        Engine();

        ~Engine() noexcept;

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        Engine(Engine&&) = delete;
        Engine& operator=(Engine&&) = delete;

        [[nodiscard]] bool AddModule(
            std::unique_ptr<
                RuntimeModule> module);

        [[nodiscard]] bool Initialize(
            EngineConfig config);

        void Shutdown() noexcept;

        [[nodiscard]] bool BeginFrame(
            double deltaSeconds) noexcept;

        [[nodiscard]] bool EndFrame() noexcept;

        void RequestExit() noexcept;

        [[nodiscard]] bool
            IsExitRequested() const noexcept;

        [[nodiscard]] bool
            IsInitialized() const noexcept;

        [[nodiscard]] bool
            IsFrameActive() const noexcept;

        [[nodiscard]] bool
            IsOwnerThread() const noexcept;

        [[nodiscard]] EngineState
            GetState() const noexcept;

        [[nodiscard]] const EngineConfig&
            GetConfig() const noexcept;

        [[nodiscard]] const FrameContext&
            GetFrameContext() const noexcept;

        [[nodiscard]] std::uint32_t
            GetOwnerThreadId() const noexcept;

        [[nodiscard]] std::size_t
            GetModuleCount() const noexcept;

        [[nodiscard]] std::size_t
            GetInitializedModuleCount() const noexcept;

        [[nodiscard]] ServiceRegistry&
            GetServices() noexcept;

        [[nodiscard]] const ServiceRegistry&
            GetServices() const noexcept;

    private:
        void RollbackInitializedModules() noexcept;

        void FinishActiveFrameForShutdown() noexcept;

        EngineConfig config_;

        FrameContext frameContext_;

        ServiceRegistry services_;

        std::vector<
            std::unique_ptr<RuntimeModule>>
                modules_;

        std::size_t initializedModuleCount_ = 0;

        std::uint32_t ownerThreadId_ = 0;

        EngineState state_ =
            EngineState::Uninitialized;

        bool frameActive_ = false;

        std::atomic<bool> exitRequested_{
            false
        };
    };
}