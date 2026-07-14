#include "r3dPCH.h"
#include "r3d.h"

#include "StudioRuntimeBridge.h"
#include "TasksRuntimeBridge.h"

#include <Runtime/Engine.h>
#include <Runtime/TaskRuntimeModule.h>

#include <Tasks/JobSystem.h>
#include <Tasks/MainThreadDispatcher.h>

#include <Platform/Thread.h>

#include <cstdint>
#include <memory>
#include <utility>

using FrameCallback =
    void (*)();

/*
 * Объявления обязательно находятся
 * в глобальном namespace.
 */
extern FrameCallback
    r3dFrameStartCallback;

extern FrameCallback
    r3dFrameEndCallback;

namespace
{
    std::unique_ptr<
        engine::runtime::Engine>
            g_runtimeEngine;

    FrameCallback
        g_previousFrameStartCallback = nullptr;

    FrameCallback
        g_previousFrameEndCallback = nullptr;

    bool g_initialized = false;
    bool g_shuttingDown = false;

    bool g_firstFrameLogged = false;

    std::uint64_t g_startedFrameCount = 0;
    std::uint64_t g_endedFrameCount = 0;

    std::uint64_t g_beginFrameFailureCount = 0;
    std::uint64_t g_endFrameFailureCount = 0;

    std::uint64_t g_recoveredFrameCount = 0;

    void StudioRuntimeFrameStart()
    {
        /*
         * Порядок startup hooks:
         *
         * PlatformInput
         * TasksRuntime
         * StudioRuntime
         */
        if (g_previousFrameStartCallback != nullptr)
        {
            g_previousFrameStartCallback();
        }

        if (
            !g_initialized ||
            g_shuttingDown ||
            g_runtimeEngine == nullptr ||
            !g_runtimeEngine->IsInitialized()
        )
        {
            return;
        }

        /*
         * Это защита от legacy frame path,
         * который вызвал StartFrame дважды,
         * не вызвав EndFrame.
         */
        if (g_runtimeEngine->IsFrameActive())
        {
            if (g_runtimeEngine->EndFrame())
            {
                ++g_recoveredFrameCount;
                ++g_endedFrameCount;
            }
            else
            {
                ++g_endFrameFailureCount;
                return;
            }
        }

        const double deltaSeconds =
            static_cast<double>(
                r3dGetFrameTime2());

        if (!g_runtimeEngine->BeginFrame(
                deltaSeconds))
        {
            ++g_beginFrameFailureCount;

            if (g_beginFrameFailureCount == 1)
            {
                r3dOutToLog(
                    "[Runtime] Studio BeginFrame failed: "
                    "state=%s, active=%d, owner=%u, "
                    "current=%u, delta=%.6f\n",
                    engine::runtime::ToString(
                        g_runtimeEngine->GetState()),
                    g_runtimeEngine->
                        IsFrameActive()
                            ? 1
                            : 0,
                    static_cast<unsigned int>(
                        g_runtimeEngine->
                            GetOwnerThreadId()),
                    static_cast<unsigned int>(
                        engine::platform::
                            GetCurrentThreadId()),
                    deltaSeconds);
            }

            return;
        }

        ++g_startedFrameCount;

        if (!g_firstFrameLogged)
        {
            g_firstFrameLogged = true;

            const engine::runtime::FrameContext&
                frameContext =
                    g_runtimeEngine->
                        GetFrameContext();

            r3dOutToLog(
                "[Runtime] Studio first frame: "
                "index=%llu, delta=%.6f, "
                "elapsed=%.6f, thread=%u\n",
                static_cast<unsigned long long>(
                    frameContext.frameIndex),
                frameContext.deltaSeconds,
                frameContext.elapsedSeconds,
                static_cast<unsigned int>(
                    engine::platform::
                        GetCurrentThreadId()));
        }
    }

    void StudioRuntimeFrameEnd()
    {
        if (
            g_initialized &&
            !g_shuttingDown &&
            g_runtimeEngine != nullptr &&
            g_runtimeEngine->IsInitialized() &&
            g_runtimeEngine->IsFrameActive()
        )
        {
            if (g_runtimeEngine->EndFrame())
            {
                ++g_endedFrameCount;
            }
            else
            {
                ++g_endFrameFailureCount;

                if (g_endFrameFailureCount == 1)
                {
                    r3dOutToLog(
                        "[Runtime] Studio EndFrame failed: "
                        "state=%s, owner=%u, current=%u\n",
                        engine::runtime::ToString(
                            g_runtimeEngine->
                                GetState()),
                        static_cast<unsigned int>(
                            g_runtimeEngine->
                                GetOwnerThreadId()),
                        static_cast<unsigned int>(
                            engine::platform::
                                GetCurrentThreadId()));
                }
            }
        }

        /*
         * End hooks идут в обратном порядке
         * относительно Begin hooks.
         */
        if (g_previousFrameEndCallback != nullptr)
        {
            g_previousFrameEndCallback();
        }
    }

    void RestoreFrameCallbacks() noexcept
    {
        if (
            r3dFrameStartCallback ==
            &StudioRuntimeFrameStart
        )
        {
            r3dFrameStartCallback =
                g_previousFrameStartCallback;
        }

        if (
            r3dFrameEndCallback ==
            &StudioRuntimeFrameEnd
        )
        {
            r3dFrameEndCallback =
                g_previousFrameEndCallback;
        }

        g_previousFrameStartCallback =
            nullptr;

        g_previousFrameEndCallback =
            nullptr;
    }
}

namespace studio
{
    bool InitializeStudioRuntimeBridge(
        const engine::runtime::RendererBackend backend) noexcept
    {
        if (g_initialized)
        {
            return true;
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
                    L"LTS.Worker";

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
                r3dOutToLog(
                    "[Runtime] Failed to add "
                    "TaskRuntimeModule\n");

                return false;
            }

            engine::runtime::EngineConfig config;

            config.applicationName =
                "LastToSurvive Studio";

            config.mode =
                engine::runtime::
                    EngineMode::Studio;

            /*
             * Studio пока реально работает через
             * legacy DX9 renderer.
             */
            config.rendererBackend = backend;

            config.enableValidation =
                true;

            config.enableMainThreadChecks =
                true;

            if (!runtimeEngine->Initialize(
                    std::move(config)))
            {
                r3dOutToLog(
                    "[Runtime] Studio runtime "
                    "initialization failed: state=%s\n",
                    engine::runtime::ToString(
                        runtimeEngine->GetState()));

                return false;
            }

            g_runtimeEngine =
                std::move(runtimeEngine);

            engine::runtime::ServiceRegistry&
                services =
                    g_runtimeEngine->GetServices();

            const bool taskServicesRegistered =
                services.TryGet<
                    engine::tasks::JobSystem>() != nullptr &&
                services.TryGet<
                    engine::tasks::
                        MainThreadDispatcher>() != nullptr;

            g_previousFrameStartCallback =
                r3dFrameStartCallback;

            g_previousFrameEndCallback =
                r3dFrameEndCallback;

            if (
                g_previousFrameStartCallback ==
                &StudioRuntimeFrameStart
            )
            {
                g_previousFrameStartCallback =
                    nullptr;
            }

            if (
                g_previousFrameEndCallback ==
                &StudioRuntimeFrameEnd
            )
            {
                g_previousFrameEndCallback =
                    nullptr;
            }

            g_firstFrameLogged = false;

            g_startedFrameCount = 0;
            g_endedFrameCount = 0;

            g_beginFrameFailureCount = 0;
            g_endFrameFailureCount = 0;

            g_recoveredFrameCount = 0;

            g_shuttingDown = false;
            g_initialized = true;

            r3dFrameStartCallback =
                &StudioRuntimeFrameStart;

            r3dFrameEndCallback =
                &StudioRuntimeFrameEnd;

            r3dOutToLog(
                "[Runtime] Studio bridge initialized: "
                "state=%s, mode=%s, renderer=%s, "
                "ownerThread=%u, services=%u, "
                "taskServices=%d, frameHooks=2\n",
                engine::runtime::ToString(
                    g_runtimeEngine->GetState()),
                engine::runtime::ToString(
                    g_runtimeEngine->
                        GetConfig().mode),
                engine::runtime::ToString(
                    g_runtimeEngine->
                        GetConfig().rendererBackend),
                static_cast<unsigned int>(
                    g_runtimeEngine->
                        GetOwnerThreadId()),
                static_cast<unsigned int>(
                    g_runtimeEngine->
                        GetServices().
                            GetServiceCount()),
                            taskServicesRegistered
                    ? 1
                    : 0);

            return true;
        }
        catch (...)
        {
            if (g_runtimeEngine != nullptr)
            {
                g_runtimeEngine->Shutdown();
                g_runtimeEngine.reset();
            }

            RestoreFrameCallbacks();

            g_initialized = false;
            g_shuttingDown = false;

            r3dOutToLog(
                "[Runtime] Studio bridge "
                "initialization failed with exception\n");

            return false;
        }
    }

    void ShutdownStudioRuntimeBridge() noexcept
    {
        if (!g_initialized)
        {
            return;
        }

        if (g_shuttingDown)
        {
            return;
        }

        g_shuttingDown = true;

        /*
         * Сначала запрещаем новые frame callbacks.
         */
        RestoreFrameCallbacks();

        engine::runtime::EngineState stateBeforeShutdown =
            engine::runtime::
                EngineState::Uninitialized;

        std::uint64_t runtimeFrameIndex = 0;

        if (g_runtimeEngine != nullptr)
        {
            stateBeforeShutdown =
                g_runtimeEngine->GetState();

            runtimeFrameIndex =
                g_runtimeEngine->
                    GetFrameContext().
                        frameIndex;

            /*
             * На случай shutdown между StartFrame
             * и EndFrame.
             */
            if (g_runtimeEngine->IsFrameActive())
            {
                if (g_runtimeEngine->EndFrame())
                {
                    ++g_endedFrameCount;
                }
                else
                {
                    ++g_endFrameFailureCount;
                }
            }

            g_runtimeEngine->Shutdown();
        }

        const engine::runtime::EngineState
            finalState =
                g_runtimeEngine != nullptr
                    ? g_runtimeEngine->
                        GetState()
                    : engine::runtime::
                        EngineState::Stopped;

        r3dOutToLog(
            "[Runtime] Studio bridge shutdown: "
            "stateBefore=%s, stateAfter=%s, "
            "runtimeFrames=%llu, "
            "started=%llu, ended=%llu, "
            "recovered=%llu, "
            "beginFailures=%llu, "
            "endFailures=%llu\n",
            engine::runtime::ToString(
                stateBeforeShutdown),
            engine::runtime::ToString(
                finalState),
            static_cast<unsigned long long>(
                runtimeFrameIndex),
            static_cast<unsigned long long>(
                g_startedFrameCount),
            static_cast<unsigned long long>(
                g_endedFrameCount),
            static_cast<unsigned long long>(
                g_recoveredFrameCount),
            static_cast<unsigned long long>(
                g_beginFrameFailureCount),
            static_cast<unsigned long long>(
                g_endFrameFailureCount));

        r3d_assert(
            g_startedFrameCount ==
            g_endedFrameCount);

        r3d_assert(
            g_beginFrameFailureCount == 0);

        r3d_assert(
            g_endFrameFailureCount == 0);

        if (g_runtimeEngine != nullptr)
        {
            r3d_assert(
                g_runtimeEngine->GetState() ==
                engine::runtime::
                    EngineState::Stopped);
        }

        g_runtimeEngine.reset();

        g_initialized = false;
        g_shuttingDown = false;

        g_firstFrameLogged = false;

        g_startedFrameCount = 0;
        g_endedFrameCount = 0;

        g_beginFrameFailureCount = 0;
        g_endFrameFailureCount = 0;

        g_recoveredFrameCount = 0;
    }

    bool IsStudioRuntimeBridgeInitialized() noexcept
    {
        return
            g_initialized &&
            !g_shuttingDown &&
            g_runtimeEngine != nullptr &&
            g_runtimeEngine->IsInitialized();
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
