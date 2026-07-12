#include "r3dPCH.h"
#include "r3d.h"

#include "PlatformInputBridge.h"

#include <Platform/Input.h>
#include <Platform/Thread.h>

#include <cstdint>

using Win32MessageProcedure =
    bool (*)(
        UINT message,
        WPARAM wordParameter,
        LPARAM longParameter);

using FrameStartCallback =
    void (*)();

/*
 * Эти функции и переменная определены в Eternity,
 * поэтому объявления должны находиться в глобальном
 * namespace, а не внутри anonymous namespace.
 */
extern void RegisterMsgProc(
    Win32MessageProcedure procedure);

extern void UnregisterMsgProc(
    Win32MessageProcedure procedure);

extern void (*r3dFrameStartCallback)();

namespace
{
    engine::platform::InputSystem
        g_platformInputSystem;

    bool g_platformInputBridgeInitialized = false;

    std::uint32_t g_platformInputOwnerThreadId = 0;

    std::uint64_t g_platformInputFrameCount = 0;
    std::uint64_t g_platformInputMessageCount = 0;

    bool g_firstFrameLogged = false;
    bool g_firstMessageLogged = false;
    bool g_threadMismatchLogged = false;

    FrameStartCallback g_previousFrameStartCallback = nullptr;

    [[nodiscard]] bool IsOwnerThread() noexcept
    {
        return
            g_platformInputOwnerThreadId != 0 &&
            engine::platform::GetCurrentThreadId() ==
                g_platformInputOwnerThreadId;
    }

    void LogThreadMismatchOnce(
        const char* location) noexcept
    {
        if (g_threadMismatchLogged)
        {
            return;
        }

        g_threadMismatchLogged = true;

        r3dOutToLog(
            "[Platform] Input bridge thread mismatch "
            "at %s: owner=%u, current=%u\n",
            location,
            static_cast<unsigned int>(
                g_platformInputOwnerThreadId),
            static_cast<unsigned int>(
                engine::platform::GetCurrentThreadId()));
    }

    void PlatformInputBeginFrame()
    {
        if (!g_platformInputBridgeInitialized)
        {
            if (g_previousFrameStartCallback != nullptr)
            {
                g_previousFrameStartCallback();
            }

            return;
        }

        if (!IsOwnerThread())
        {
            LogThreadMismatchOnce(
                "BeginFrame");

            if (g_previousFrameStartCallback != nullptr)
            {
                g_previousFrameStartCallback();
            }

            return;
        }

        g_platformInputSystem.BeginFrame();

        ++g_platformInputFrameCount;

        if (!g_firstFrameLogged)
        {
            g_firstFrameLogged = true;

            r3dOutToLog(
                "[Platform] Input bridge: "
                "first frame began, thread=%u\n",
                static_cast<unsigned int>(
                    g_platformInputOwnerThreadId));
        }

        /*
         * Не уничтожаем другой frame callback,
         * если он появится во время миграции.
         */
        if (g_previousFrameStartCallback != nullptr)
        {
            g_previousFrameStartCallback();
        }
    }

    bool PlatformInputMessageProcedure(
        const UINT message,
        const WPARAM wordParameter,
        const LPARAM longParameter)
    {
        if (!g_platformInputBridgeInitialized)
        {
            return false;
        }

        if (!IsOwnerThread())
        {
            LogThreadMismatchOnce(
                "MessageProc");

            return false;
        }

        const bool handledByPlatformInput =
            g_platformInputSystem.HandleNativeMessage(
                static_cast<std::uint32_t>(
                    message),
                static_cast<std::uintptr_t>(
                    wordParameter),
                static_cast<std::intptr_t>(
                    longParameter));

        if (handledByPlatformInput)
        {
            ++g_platformInputMessageCount;

            if (!g_firstMessageLogged)
            {
                g_firstMessageLogged = true;

                r3dOutToLog(
                    "[Platform] Input bridge: "
                    "first native event received, "
                    "message=0x%04X, events=%u\n",
                    static_cast<unsigned int>(
                        message),
                    static_cast<unsigned int>(
                        g_platformInputSystem.
                            GetEventCount()));
            }
        }

        /*
         * Критически важно:
         *
         * новый Input только наблюдает за сообщением.
         * false позволяет старому WndProc, RmlUI,
         * консоли и DirectInput продолжить обработку.
         */
        return false;
    }
}

namespace studio
{
    bool InitializePlatformInputBridge() noexcept
    {
        if (g_platformInputBridgeInitialized)
        {
            return true;
        }

        g_platformInputSystem.Reset();

        g_platformInputOwnerThreadId =
            engine::platform::GetCurrentThreadId();

        if (g_platformInputOwnerThreadId == 0)
        {
            r3dOutToLog(
                "[Platform] Input bridge initialization "
                "failed: invalid owner thread\n");

            return false;
        }

        g_platformInputFrameCount = 0;
        g_platformInputMessageCount = 0;

        g_firstFrameLogged = false;
        g_firstMessageLogged = false;
        g_threadMismatchLogged = false;

        g_previousFrameStartCallback =
            r3dFrameStartCallback;

        /*
         * Защита от случайной повторной установки
         * нашего же callback.
         */
        if (g_previousFrameStartCallback ==
            &PlatformInputBeginFrame)
        {
            g_previousFrameStartCallback = nullptr;
        }

        g_platformInputBridgeInitialized = true;

        r3dFrameStartCallback =
            &PlatformInputBeginFrame;

        RegisterMsgProc(
            &PlatformInputMessageProcedure);

        r3dOutToLog(
            "[Platform] Input bridge initialized: "
            "thread=%u, messageProc=1, frameHook=1\n",
            static_cast<unsigned int>(
                g_platformInputOwnerThreadId));

        return true;
    }

    void ShutdownPlatformInputBridge() noexcept
    {
        if (!g_platformInputBridgeInitialized)
        {
            return;
        }

        /*
         * Сначала запрещаем callback работать с состоянием.
         */
        g_platformInputBridgeInitialized = false;

        UnregisterMsgProc(
            &PlatformInputMessageProcedure);

        if (r3dFrameStartCallback ==
            &PlatformInputBeginFrame)
        {
            r3dFrameStartCallback =
                g_previousFrameStartCallback;
        }

        g_previousFrameStartCallback = nullptr;

        g_platformInputSystem.Reset();

        r3dOutToLog(
            "[Platform] Input bridge shutdown: "
            "frames=%llu, nativeEvents=%llu\n",
            static_cast<unsigned long long>(
                g_platformInputFrameCount),
            static_cast<unsigned long long>(
                g_platformInputMessageCount));

        g_platformInputOwnerThreadId = 0;
        g_platformInputFrameCount = 0;
        g_platformInputMessageCount = 0;
    }

    bool IsPlatformInputBridgeInitialized() noexcept
    {
        return g_platformInputBridgeInitialized;
    }

    engine::platform::InputSystem&
        GetPlatformInputSystem() noexcept
    {
        return g_platformInputSystem;
    }
}