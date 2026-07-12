#include "Runtime/Engine.h"

#include "Runtime/RuntimeModule.h"

#include <Platform/Thread.h>

#include <cmath>
#include <utility>

namespace engine::runtime
{
    const char* ToString(
        const EngineState state) noexcept
    {
        switch (state)
        {
            case EngineState::Uninitialized:
                return "uninitialized";

            case EngineState::Initializing:
                return "initializing";

            case EngineState::Running:
                return "running";

            case EngineState::Stopping:
                return "stopping";

            case EngineState::Stopped:
                return "stopped";

            case EngineState::Failed:
                return "failed";

            default:
                return "unknown";
        }
    }

    Engine::Engine() = default;

    Engine::~Engine() noexcept
    {
        Shutdown();
    }

    bool Engine::AddModule(
        std::unique_ptr<
            RuntimeModule> module)
    {
        if (module == nullptr)
        {
            return false;
        }

        if (
            state_ == EngineState::Initializing ||
            state_ == EngineState::Running ||
            state_ == EngineState::Stopping ||
            frameActive_
        )
        {
            return false;
        }

        modules_.push_back(
            std::move(module));

        return true;
    }

    bool Engine::Initialize(
        EngineConfig config)
    {
        if (
            state_ == EngineState::Initializing ||
            state_ == EngineState::Running ||
            state_ == EngineState::Stopping
        )
        {
            return false;
        }

        const std::uint32_t currentThreadId =
            engine::platform::GetCurrentThreadId();

        if (currentThreadId == 0)
        {
            state_ = EngineState::Failed;
            return false;
        }

        state_ =
            EngineState::Initializing;

        ownerThreadId_ =
            currentThreadId;

        config_ =
            std::move(config);

        frameContext_ = {};

        frameActive_ = false;

        initializedModuleCount_ = 0;

        exitRequested_.store(
            false,
            std::memory_order_release);

        services_.Clear();

        RuntimeModule* activeModule =
            nullptr;

        try
        {
            if (!services_.Register<Engine>(*this))
            {
                state_ = EngineState::Failed;
                ownerThreadId_ = 0;
                return false;
            }

            if (!services_.Register<ServiceRegistry>(
                    services_))
            {
                services_.Clear();
                state_ = EngineState::Failed;
                ownerThreadId_ = 0;
                return false;
            }

            for (
                std::size_t moduleIndex = 0;
                moduleIndex < modules_.size();
                ++moduleIndex
            )
            {
                RuntimeModule* const module =
                    modules_[moduleIndex].get();

                if (module == nullptr)
                {
                    RollbackInitializedModules();
                    services_.Clear();

                    state_ =
                        EngineState::Failed;

                    ownerThreadId_ = 0;

                    return false;
                }

                activeModule =
                    module;

                if (!module->Initialize(*this))
                {
                    /*
                     * Даже Initialize(false) мог успеть
                     * частично создать ресурсы.
                     */
                    module->Shutdown(*this);

                    activeModule =
                        nullptr;

                    RollbackInitializedModules();

                    services_.Clear();

                    state_ =
                        EngineState::Failed;

                    ownerThreadId_ = 0;

                    return false;
                }

                ++initializedModuleCount_;

                activeModule =
                    nullptr;
            }
        }
        catch (...)
        {
            if (activeModule != nullptr)
            {
                activeModule->Shutdown(*this);
            }

            RollbackInitializedModules();

            services_.Clear();

            state_ =
                EngineState::Failed;

            ownerThreadId_ = 0;

            return false;
        }

        state_ =
            EngineState::Running;

        return true;
    }

    void Engine::Shutdown() noexcept
    {
        if (
            state_ == EngineState::Uninitialized ||
            state_ == EngineState::Stopped
        )
        {
            return;
        }

        if (state_ == EngineState::Stopping)
        {
            return;
        }

        if (state_ == EngineState::Failed)
        {
            services_.Clear();

            ownerThreadId_ = 0;

            frameActive_ = false;

            state_ =
                EngineState::Stopped;

            return;
        }

        FinishActiveFrameForShutdown();

        state_ =
            EngineState::Stopping;

        RollbackInitializedModules();

        services_.Clear();

        ownerThreadId_ = 0;

        frameContext_ = {};

        frameActive_ = false;

        state_ =
            EngineState::Stopped;
    }

    bool Engine::BeginFrame(
        const double deltaSeconds) noexcept
    {
        if (
            state_ != EngineState::Running ||
            frameActive_
        )
        {
            return false;
        }

        if (
            config_.enableMainThreadChecks &&
            !IsOwnerThread()
        )
        {
            return false;
        }

        if (
            !std::isfinite(deltaSeconds) ||
            deltaSeconds < 0.0
        )
        {
            return false;
        }

        ++frameContext_.frameIndex;

        frameContext_.deltaSeconds =
            deltaSeconds;

        frameContext_.elapsedSeconds +=
            deltaSeconds;

        frameActive_ = true;

        for (
            std::size_t moduleIndex = 0;
            moduleIndex < initializedModuleCount_;
            ++moduleIndex
        )
        {
            RuntimeModule* const module =
                modules_[moduleIndex].get();

            if (module != nullptr)
            {
                module->BeginFrame(
                    *this,
                    frameContext_);
            }
        }

        return true;
    }

    bool Engine::EndFrame() noexcept
    {
        if (
            state_ != EngineState::Running ||
            !frameActive_
        )
        {
            return false;
        }

        if (
            config_.enableMainThreadChecks &&
            !IsOwnerThread()
        )
        {
            return false;
        }

        for (
            std::size_t moduleIndex =
                initializedModuleCount_;
            moduleIndex > 0;
            --moduleIndex
        )
        {
            RuntimeModule* const module =
                modules_[moduleIndex - 1].get();

            if (module != nullptr)
            {
                module->EndFrame(
                    *this,
                    frameContext_);
            }
        }

        frameActive_ = false;

        return true;
    }

    void Engine::RequestExit() noexcept
    {
        exitRequested_.store(
            true,
            std::memory_order_release);
    }

    bool Engine::IsExitRequested() const noexcept
    {
        return exitRequested_.load(
            std::memory_order_acquire);
    }

    bool Engine::IsInitialized() const noexcept
    {
        return state_ == EngineState::Running;
    }

    bool Engine::IsFrameActive() const noexcept
    {
        return frameActive_;
    }

    bool Engine::IsOwnerThread() const noexcept
    {
        return
            ownerThreadId_ != 0 &&
            engine::platform::GetCurrentThreadId() ==
                ownerThreadId_;
    }

    EngineState Engine::GetState() const noexcept
    {
        return state_;
    }

    const EngineConfig&
        Engine::GetConfig() const noexcept
    {
        return config_;
    }

    const FrameContext&
        Engine::GetFrameContext() const noexcept
    {
        return frameContext_;
    }

    std::uint32_t
        Engine::GetOwnerThreadId() const noexcept
    {
        return ownerThreadId_;
    }

    std::size_t
        Engine::GetModuleCount() const noexcept
    {
        return modules_.size();
    }

    std::size_t
        Engine::GetInitializedModuleCount() const noexcept
    {
        return initializedModuleCount_;
    }

    ServiceRegistry&
        Engine::GetServices() noexcept
    {
        return services_;
    }

    const ServiceRegistry&
        Engine::GetServices() const noexcept
    {
        return services_;
    }

    void Engine::RollbackInitializedModules() noexcept
    {
        while (initializedModuleCount_ > 0)
        {
            const std::size_t moduleIndex =
                initializedModuleCount_ - 1;

            RuntimeModule* const module =
                modules_[moduleIndex].get();

            if (module != nullptr)
            {
                module->Shutdown(*this);
            }

            --initializedModuleCount_;
        }
    }

    void Engine::
        FinishActiveFrameForShutdown() noexcept
    {
        if (!frameActive_)
        {
            return;
        }

        for (
            std::size_t moduleIndex =
                initializedModuleCount_;
            moduleIndex > 0;
            --moduleIndex
        )
        {
            RuntimeModule* const module =
                modules_[moduleIndex - 1].get();

            if (module != nullptr)
            {
                module->EndFrame(
                    *this,
                    frameContext_);
            }
        }

        frameActive_ = false;
    }
}