#include "StudioRuntimeBridge.h"

#include <Core/Log.h>

#include <Runtime/Engine.h>
#include <Runtime/TaskRuntimeModule.h>

#include <Tasks/JobSystem.h>
#include <Tasks/MainThreadDispatcher.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>
#include <utility>

namespace
{
    std::unique_ptr<
        engine::runtime::Engine>
            g_runtimeEngine;

    bool g_initialized = false;
    bool g_shuttingDown = false;

    using engine::core::LogLevel;

    void WriteLog(
        const LogLevel level,
        const std::string_view category,
        const std::string_view text) noexcept
    {
        engine::core::GetLogger().Write(
            level,
            category,
            text);
    }

    template <typename... Args>
    void WriteFormattedLog(
        const LogLevel level,
        const std::string_view category,
        const char* const format,
        Args... args) noexcept
    {
        std::array<char, 768> buffer{};

        const int written =
            std::snprintf(
                buffer.data(),
                buffer.size(),
                format,
                args...);

        if (written <= 0)
        {
            WriteLog(
                LogLevel::Error,
                "Logging",
                "Failed to format runtime log message");

            return;
        }

        const std::size_t length =
            (std::min)(
                static_cast<std::size_t>(
                    written),
                buffer.size() - 1);

        WriteLog(
            level,
            category,
            std::string_view(
                buffer.data(),
                length));
    }
}

namespace studio
{
    bool InitializeStudioRuntimeBridge(
        const engine::runtime::RendererBackend backend) noexcept
    {
        if (g_initialized)
        {
            if (
                g_runtimeEngine != nullptr &&
                g_runtimeEngine->IsInitialized() &&
                g_runtimeEngine->
                    GetConfig().
                    rendererBackend == backend
            )
            {
                return true;
            }

            WriteLog(
                LogLevel::Error,
                "Runtime",
                "Runtime is already initialized "
                "with another renderer backend");

            return false;
        }

        if (g_shuttingDown)
        {
            return false;
        }

        try
        {
            auto runtimeEngine =
                std::make_unique<
                    engine::runtime::Engine>();

            engine::runtime::TaskRuntimeConfig
                taskRuntimeConfig;

            taskRuntimeConfig.
                jobSystemConfig.workerCount = 0;

            taskRuntimeConfig.
                jobSystemConfig.workerNamePrefix =
                    L"Engine.Worker";

            taskRuntimeConfig.
                maximumCallbacksPerFrame = 1024;

            taskRuntimeConfig.
                maximumCallbacksPerShutdownBatch = 1024;

            const bool taskModuleAdded =
                runtimeEngine->AddModule(
                    std::make_unique<
                        engine::runtime::
                            TaskRuntimeModule>(
                                std::move(
                                    taskRuntimeConfig)));

            if (!taskModuleAdded)
            {
                WriteLog(
                    LogLevel::Error,
                    "Runtime",
                    "Failed to add TaskRuntimeModule");

                return false;
            }

            engine::runtime::EngineConfig config;

            config.applicationName =
                "DX11 Studio";

            config.mode =
                engine::runtime::
                    EngineMode::Studio;

            config.rendererBackend =
                backend;

            config.enableValidation =
                true;

            config.enableMainThreadChecks =
                true;

            if (!runtimeEngine->Initialize(
                    std::move(config)))
            {
                WriteFormattedLog(
                    LogLevel::Error,
                    "Runtime",
                    "Studio runtime initialization "
                    "failed: state=%s",
                    engine::runtime::ToString(
                        runtimeEngine->
                            GetState()));

                return false;
            }

            engine::runtime::ServiceRegistry&
                services =
                    runtimeEngine->
                        GetServices();

            const bool taskServicesRegistered =
                services.TryGet<
                    engine::tasks::JobSystem>() != nullptr &&
                services.TryGet<
                    engine::tasks::
                        MainThreadDispatcher>() != nullptr;

            if (!taskServicesRegistered)
            {
                WriteLog(
                    LogLevel::Error,
                    "Runtime",
                    "Task runtime services "
                    "were not registered");

                runtimeEngine->Shutdown();

                return false;
            }

            g_runtimeEngine =
                std::move(runtimeEngine);

            g_shuttingDown = false;
            g_initialized = true;

            WriteFormattedLog(
                LogLevel::Information,
                "Runtime",
                "Studio runtime initialized: "
                "state=%s, mode=%s, renderer=%s, "
                "ownerThread=%u, services=%u",
                engine::runtime::ToString(
                    g_runtimeEngine->
                        GetState()),
                engine::runtime::ToString(
                    g_runtimeEngine->
                        GetConfig().mode),
                engine::runtime::ToString(
                    g_runtimeEngine->
                        GetConfig().
                        rendererBackend),
                static_cast<unsigned int>(
                    g_runtimeEngine->
                        GetOwnerThreadId()),
                static_cast<unsigned int>(
                    g_runtimeEngine->
                        GetServices().
                        GetServiceCount()));

            return true;
        }
        catch (...)
        {
            if (g_runtimeEngine != nullptr)
            {
                g_runtimeEngine->Shutdown();
                g_runtimeEngine.reset();
            }

            g_initialized = false;
            g_shuttingDown = false;

            WriteLog(
                LogLevel::Error,
                "Runtime",
                "Studio runtime initialization "
                "failed with exception");

            return false;
        }
    }

    void ShutdownStudioRuntimeBridge() noexcept
    {
        if (
            !g_initialized &&
            g_runtimeEngine == nullptr
        )
        {
            return;
        }

        if (g_shuttingDown)
        {
            return;
        }

        g_shuttingDown = true;

        engine::runtime::EngineState
            stateBeforeShutdown =
                engine::runtime::
                    EngineState::Uninitialized;

        std::uint64_t runtimeFrameIndex = 0;

        bool activeFrameFinished = true;

        if (g_runtimeEngine != nullptr)
        {
            stateBeforeShutdown =
                g_runtimeEngine->
                    GetState();

            runtimeFrameIndex =
                g_runtimeEngine->
                    GetFrameContext().
                    frameIndex;

            if (g_runtimeEngine->IsFrameActive())
            {
                activeFrameFinished =
                    g_runtimeEngine->
                        EndFrame();

                if (!activeFrameFinished)
                {
                    WriteLog(
                        LogLevel::Error,
                        "Runtime",
                        "Failed to finish active "
                        "frame during shutdown");
                }
            }

            g_runtimeEngine->Shutdown();
        }

        const engine::runtime::EngineState
            stateAfterShutdown =
                g_runtimeEngine != nullptr
                    ? g_runtimeEngine->
                        GetState()
                    : engine::runtime::
                        EngineState::Stopped;

        WriteFormattedLog(
            LogLevel::Information,
            "Runtime",
            "Studio runtime shutdown: "
            "stateBefore=%s, stateAfter=%s, "
            "frames=%llu, activeFrameFinished=%d",
            engine::runtime::ToString(
                stateBeforeShutdown),
            engine::runtime::ToString(
                stateAfterShutdown),
            static_cast<unsigned long long>(
                runtimeFrameIndex),
            activeFrameFinished
                ? 1
                : 0);

        if (
            g_runtimeEngine != nullptr &&
            g_runtimeEngine->GetState() !=
                engine::runtime::
                    EngineState::Stopped
        )
        {
            WriteFormattedLog(
                LogLevel::Error,
                "Runtime",
                "Unexpected runtime state "
                "after shutdown: %s",
                engine::runtime::ToString(
                    g_runtimeEngine->
                        GetState()));
        }

        g_runtimeEngine.reset();

        g_initialized = false;
        g_shuttingDown = false;
    }

    bool IsStudioRuntimeBridgeInitialized() noexcept
    {
        return
            g_initialized &&
            !g_shuttingDown &&
            g_runtimeEngine != nullptr &&
            g_runtimeEngine->
                IsInitialized();
    }

    engine::runtime::Engine*
        TryGetRuntimeEngine() noexcept
    {
        if (!IsStudioRuntimeBridgeInitialized())
        {
            return nullptr;
        }

        return g_runtimeEngine.get();
    }
}