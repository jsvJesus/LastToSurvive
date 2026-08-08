#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "Editors/StudioEditorUI.h"
#include "StudioGraphicsShell.h"
#include "StudioRuntimeBridge.h"

#include <Core/Log.h>

#include <ImGui/ImGuiHost.h>

#include <Graphics/CommandContext.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/SwapChain.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>

#include <GraphicsDX11/D3D11Device.h>

#include <Platform/Clock.h>
#include <Platform/MessagePump.h>
#include <Platform/Process.h>
#include <Platform/Window.h>

#include <Runtime/Engine.h>
#include <Runtime/RendererBackend.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>

extern void RegisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));

extern void UnregisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));

namespace
{
    using engine::graphics::GraphicsResult;
    using studio::StudioGraphicsShellResult;
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
        std::array<char, 512> buffer{};

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
                "Failed to format log message");

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

    StudioGraphicsShellResult
        ResultFromGraphicsFailure(
            const GraphicsResult result) noexcept
    {
        switch (result)
        {
        case GraphicsResult::DeviceLost:
            return
                StudioGraphicsShellResult::
                    DeviceLost;

        case GraphicsResult::DeviceRemoved:
            return
                StudioGraphicsShellResult::
                    DeviceRemoved;

        default:
            return
                StudioGraphicsShellResult::
                    FrameFailed;
        }
    }

    class StudioDX11Bootstrap final
    {
    public:
        StudioDX11Bootstrap() = default;

        ~StudioDX11Bootstrap() noexcept
        {
            Shutdown();
        }

        StudioDX11Bootstrap(
            const StudioDX11Bootstrap&) = delete;

        StudioDX11Bootstrap& operator=(
            const StudioDX11Bootstrap&) = delete;

        [[nodiscard]] bool Initialize(
            const std::uintptr_t nativeWindow) noexcept
        {
            Shutdown();

            HWND windowHandle =
                reinterpret_cast<HWND>(
                    nativeWindow);

            if (windowHandle == nullptr)
            {
                return FailInitialization(
                    "invalid window handle",
                    GraphicsResult::InvalidArgument);
            }

            if (!EnableWindowResize(
                    windowHandle))
            {
                return FailInitialization(
                    "window style",
                    GraphicsResult::BackendFailure);
            }

            window_ =
                engine::platform::Window(
                    engine::platform::
                        NativeWindowHandle::
                            FromValue(
                                nativeWindow));

            if (!window_.IsValid())
            {
                return FailInitialization(
                    "platform window",
                    GraphicsResult::InvalidArgument);
            }

            const engine::platform::WindowSize
                clientSize =
                    window_.GetClientSize();

            if (clientSize.IsEmpty())
            {
                return FailInitialization(
                    "window client size",
                    GraphicsResult::InvalidArgument);
            }

            engine::graphics::RenderDeviceDesc
                deviceDesc;

            deviceDesc.backend =
                engine::graphics::
                    GraphicsBackend::D3D11;

            deviceDesc.enableValidation =
                true;

            GraphicsResult result =
                device_.Initialize(
                    deviceDesc);

            if (engine::graphics::Failed(
                    result))
            {
                return FailInitialization(
                    "D3D11 device",
                    result);
            }

            context_ =
                device_.
                    GetImmediateCommandContext();

            if (
                context_ == nullptr ||
                !context_->IsValid())
            {
                return FailInitialization(
                    "immediate context",
                    GraphicsResult::InvalidState);
            }

            engine::graphics::SwapChainDesc
                swapChainDesc;

            swapChainDesc.window =
                window_.GetNativeHandle();

            swapChainDesc.width =
                clientSize.width;

            swapChainDesc.height =
                clientSize.height;

            swapChainDesc.bufferCount = 2;

            swapChainDesc.presentMode =
                engine::graphics::
                    PresentMode::VSync;

            result =
                device_.CreateSwapChain(
                    swapChainDesc,
                    swapChain_);

            if (engine::graphics::Failed(
                    result))
            {
                return FailInitialization(
                    "swap chain",
                    result);
            }

            if (!CreateDepthBuffer(
                    clientSize.width,
                    clientSize.height))
            {
                return false;
            }

            if (!imguiHost_.Initialize(windowHandle, device_.GetNativeDevice(), device_.GetNativeImmediateContext(), "StudioEditor.ini"))
            {
                return FailInitialization(
                    "Dear ImGui",
                    GraphicsResult::BackendFailure);
            }

            WriteLog(LogLevel::Information, "Editor.ImGui", "Dear ImGui initialized");

            width_ =
                clientSize.width;

            height_ =
                clientSize.height;

            minimized_ = false;
            resizePending_ = false;
            closeRequested_ = false;
            occluded_ = false;

            timerResetPending_ = true;

            initialized_ = true;

            WriteFormattedLog(LogLevel::Information, "Graphics.DX11", "Bootstrap initialized: %ux%u",
                static_cast<unsigned int>(width_), static_cast<unsigned int>(height_));

            return true;
        }

        void Shutdown() noexcept
        {
            initialized_ = false;

            imguiHost_.Shutdown();

            if (context_ != nullptr)
            {
                context_->UnbindRenderTargets();
                context_->ClearState();
                context_->Flush();
            }

            DestroyDepthBuffer();

            swapChain_.reset();

            context_ = nullptr;

            device_.Shutdown();

            window_ =
                engine::platform::Window();

            RestoreWindowStyle();

            width_ = 0;
            height_ = 0;

            pendingWidth_ = 0;
            pendingHeight_ = 0;

            resizePending_ = false;
            minimized_ = false;
            closeRequested_ = false;
            occluded_ = false;

            timerResetPending_ = true;
        }

        void OnWindowSize(
            const WPARAM sizeType,
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (!initialized_)
            {
                return;
            }

            if (
                sizeType == SIZE_MINIMIZED ||
                width == 0 ||
                height == 0)
            {
                minimized_ = true;
                return;
            }

            pendingWidth_ = width;
            pendingHeight_ = height;

            resizePending_ = true;

            if (minimized_)
            {
                timerResetPending_ = true;
            }

            minimized_ = false;
        }

        void RequestClose() noexcept
        {
            closeRequested_ = true;
        }

        [[nodiscard]] bool
            IsCloseRequested() const noexcept
        {
            return closeRequested_;
        }

        [[nodiscard]] bool
            ShouldWaitForMessage() const noexcept
        {
            return
                minimized_ ||
                occluded_;
        }

        [[nodiscard]] bool
            ConsumeTimerReset() noexcept
        {
            const bool reset =
                timerResetPending_;

            timerResetPending_ = false;

            return reset;
        }

        [[nodiscard]] StudioGraphicsShellResult
            GetFailureResult() const noexcept
        {
            return failureResult_;
        }

        [[nodiscard]] bool RenderFrame() noexcept
        {
            if (!initialized_)
            {
                return false;
            }

            if (resizePending_)
            {
                if (!Resize(
                        pendingWidth_,
                        pendingHeight_))
                {
                    return false;
                }
            }

            if (minimized_)
            {
                return true;
            }

            GraphicsResult result =
                context_->
                    SetSwapChainRenderTarget(
                        *swapChain_,
                        depthBuffer_);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "set render target",
                    result);
            }

            engine::graphics::Viewport
                viewport;

            viewport.width =
                static_cast<float>(
                    width_);

            viewport.height =
                static_cast<float>(
                    height_);

            result =
                context_->SetViewport(
                    viewport);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "viewport",
                    result);
            }

            engine::graphics::ScissorRect
                scissor;

            scissor.left = 0;
            scissor.top = 0;

            scissor.right =
                static_cast<std::int32_t>(
                    width_);

            scissor.bottom =
                static_cast<std::int32_t>(
                    height_);

            result =
                context_->SetScissorRect(
                    scissor);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "scissor",
                    result);
            }

            const engine::graphics::ClearColor
                clearColor
                {
                    0.025f,
                    0.035f,
                    0.050f,
                    1.0f
                };

            result =
                context_->
                    ClearSwapChainColor(
                        *swapChain_,
                        clearColor);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "clear backbuffer",
                    result);
            }

            result =
                context_->
                    ClearDepthStencilTarget(
                        depthBuffer_,
                        engine::graphics::
                            ClearDepthStencilFlags::
                                Depth |
                        engine::graphics::
                            ClearDepthStencilFlags::
                                Stencil,
                        1.0f,
                        0);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "clear depth",
                    result);
            }

            engine::graphics::PresentStatus
                presentStatus =
                    engine::graphics::
                        PresentStatus::Presented;

            imguiHost_.BeginFrame();

            studio::editor::DrawEditorUI();

            imguiHost_.Render();

            result =
                swapChain_->Present(
                    presentStatus);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "present",
                    result);
            }

            switch (presentStatus)
            {
            case engine::graphics::
                PresentStatus::DeviceLost:

                failureResult_ =
                    StudioGraphicsShellResult::
                        DeviceLost;

                return false;

            case engine::graphics::
                PresentStatus::DeviceRemoved:

                failureResult_ =
                    StudioGraphicsShellResult::
                        DeviceRemoved;

                return false;

            case engine::graphics::
                PresentStatus::Occluded:

                occluded_ = true;

                return true;

            default:
                break;
            }

            if (occluded_)
            {
                occluded_ = false;
                timerResetPending_ = true;
            }

            return true;
        }

        [[nodiscard]] bool ProcessNativeMessage(const UINT message, const WPARAM wordParameter, const LPARAM longParameter) noexcept
        {
            if (!imguiHost_.IsInitialized())
            {
                return false;
            }

            return imguiHost_.ProcessNativeMessage(
                windowHandle_,
                static_cast<std::uint32_t>(
                    message),
                static_cast<std::uintptr_t>(
                    wordParameter),
                static_cast<std::intptr_t>(
                    longParameter));
        }

    private:
        [[nodiscard]] bool CreateDepthBuffer(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            engine::graphics::TextureDesc
                depthDesc;

            depthDesc.width = width;
            depthDesc.height = height;

            depthDesc.format =
                engine::graphics::
                    Format::D24UNormS8UInt;

            depthDesc.bindFlags =
                engine::graphics::
                    TextureBindFlags::
                        DepthStencil;

            const GraphicsResult result =
                device_.CreateTexture(
                    depthDesc,
                    nullptr,
                    0,
                    depthBuffer_);

            if (engine::graphics::Failed(
                    result))
            {
                return FailInitialization(
                    "depth buffer",
                    result);
            }

            return true;
        }

        void DestroyDepthBuffer() noexcept
        {
            if (!depthBuffer_.IsValid())
            {
                return;
            }

            (void)device_.DestroyTexture(
                depthBuffer_);

            depthBuffer_ = {};
        }

        [[nodiscard]] bool Resize(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            resizePending_ = false;

            if (
                width == 0 ||
                height == 0)
            {
                return true;
            }

            if (
                width == width_ &&
                height == height_)
            {
                return true;
            }

            context_->UnbindRenderTargets();

            DestroyDepthBuffer();

            const GraphicsResult result =
                swapChain_->Resize(
                    width,
                    height);

            if (engine::graphics::Failed(
                    result))
            {
                return FailFrame(
                    "swap chain resize",
                    result);
            }

            if (!CreateDepthBuffer(
                    width,
                    height))
            {
                return false;
            }

            width_ = width;
            height_ = height;

            WriteFormattedLog(LogLevel::Information, "Graphics.DX11", "Backbuffer resized: %ux%u",
                static_cast<unsigned int>(width_), static_cast<unsigned int>(height_));

            return true;
        }

        [[nodiscard]] bool
            EnableWindowResize(
                HWND windowHandle) noexcept
        {
            windowHandle_ = windowHandle;

            originalWindowStyle_ =
                GetWindowLongPtr(
                    windowHandle_,
                    GWL_STYLE);

            const LONG_PTR newStyle =
                originalWindowStyle_ |
                WS_THICKFRAME |
                WS_MAXIMIZEBOX;

            if (newStyle == originalWindowStyle_)
            {
                return true;
            }

            SetLastError(
                ERROR_SUCCESS);

            const LONG_PTR previousStyle =
                SetWindowLongPtr(
                    windowHandle_,
                    GWL_STYLE,
                    newStyle);

            if (
                previousStyle == 0 &&
                GetLastError() != ERROR_SUCCESS)
            {
                windowHandle_ = nullptr;

                return false;
            }

            styleChanged_ = true;

            SetWindowPos(
                windowHandle_,
                nullptr,
                0,
                0,
                0,
                0,
                SWP_NOMOVE |
                SWP_NOSIZE |
                SWP_NOZORDER |
                SWP_NOACTIVATE |
                SWP_FRAMECHANGED);

            return true;
        }

        void RestoreWindowStyle() noexcept
        {
            if (
                styleChanged_ &&
                windowHandle_ != nullptr)
            {
                SetWindowLongPtr(
                    windowHandle_,
                    GWL_STYLE,
                    originalWindowStyle_);

                SetWindowPos(
                    windowHandle_,
                    nullptr,
                    0,
                    0,
                    0,
                    0,
                    SWP_NOMOVE |
                    SWP_NOSIZE |
                    SWP_NOZORDER |
                    SWP_NOACTIVATE |
                    SWP_FRAMECHANGED);
            }

            windowHandle_ = nullptr;
            originalWindowStyle_ = 0;
            styleChanged_ = false;
        }

        [[nodiscard]] bool FailInitialization(
        const char* phase,
        const GraphicsResult result) noexcept
        {
            WriteFormattedLog(LogLevel::Error, "Graphics.DX11", "Initialization failed at %s: %s", phase,
                engine::graphics::ToString(result));

            Shutdown();

            return false;
        }

        [[nodiscard]] bool FailFrame(const char* phase, const GraphicsResult result) noexcept
        {
            WriteFormattedLog(LogLevel::Error, "Graphics.DX11", "Frame failed at %s: %s", phase,
                engine::graphics::ToString(result));

            failureResult_ = ResultFromGraphicsFailure(result);

            return false;
        }

    private:
        engine::platform::Window window_;
        engine::ui::ImGuiHost imguiHost_;
        engine::graphics::d3d11::D3D11Device device_;
        engine::graphics::CommandContext* context_ = nullptr;
        std::unique_ptr<engine::graphics::SwapChain>swapChain_;
        engine::graphics::TextureHandle depthBuffer_;

        std::uint32_t width_ = 0;
        std::uint32_t height_ = 0;

        std::uint32_t pendingWidth_ = 0;
        std::uint32_t pendingHeight_ = 0;

        bool initialized_ = false;
        bool resizePending_ = false;
        bool minimized_ = false;
        bool closeRequested_ = false;
        bool occluded_ = false;

        bool timerResetPending_ = true;

        HWND windowHandle_ = nullptr;

        LONG_PTR originalWindowStyle_ = 0;

        bool styleChanged_ = false;

        StudioGraphicsShellResult failureResult_ =
            StudioGraphicsShellResult::
                FrameFailed;
    };

    StudioDX11Bootstrap*
        g_activeDX11Bootstrap =
            nullptr;

    bool StudioDX11MessageProc(const UINT message, const WPARAM wParam, const LPARAM lParam)
    {
        if (g_activeDX11Bootstrap == nullptr)
        {
            return false;
        }

        const bool handledByEditorUI =
            g_activeDX11Bootstrap->
                ProcessNativeMessage(
                    message,
                    wParam,
                    lParam);

        switch (message)
        {
        case WM_SIZE:
            g_activeDX11Bootstrap->
                OnWindowSize(
                    wParam,
                    static_cast<std::uint32_t>(
                        LOWORD(lParam)),
                    static_cast<std::uint32_t>(
                        HIWORD(lParam)));

            return false;

        case WM_CLOSE:
        case WM_DESTROY:
            g_activeDX11Bootstrap->
                RequestClose();

            return true;

        default:
            return handledByEditorUI;
        }
    }
}

namespace studio
{
    bool WantsDX11Shell() noexcept
    {
        return engine::platform::HasCurrentProcessArgument(L"-dx11");
    }

    const char* ToString(
        const StudioGraphicsShellResult result) noexcept
    {
        switch (result)
        {
        case StudioGraphicsShellResult::NotRequested:
            return "NotRequested";

        case StudioGraphicsShellResult::Completed:
            return "Completed";

        case StudioGraphicsShellResult::
            InitializationFailed:
            return "InitializationFailed";

        case StudioGraphicsShellResult::
            RuntimeInitializationFailed:
            return "RuntimeInitializationFailed";

        case StudioGraphicsShellResult::FrameFailed:
            return "FrameFailed";

        case StudioGraphicsShellResult::DeviceLost:
            return "DeviceLost";

        case StudioGraphicsShellResult::DeviceRemoved:
            return "DeviceRemoved";

        default:
            return "Unknown";
        }
    }

    StudioGraphicsShellResult RunDX11Shell(
        const std::uintptr_t nativeWindow) noexcept
    {
        if (!WantsDX11Shell())
        {
            return
                StudioGraphicsShellResult::
                    NotRequested;
        }

        WriteLog(LogLevel::Information, "Graphics", "Renderer backend: D3D11");

        /*
         * Сначала Runtime получает выбранный backend.
         * Затем создаётся конкретный GraphicsDX11 backend.
         */
        if (!InitializeStudioRuntimeBridge(
                engine::runtime::
                    RendererBackend::D3D11))
        {
            WriteLog(LogLevel::Error, "Graphics.DX11", "Runtime initialization failed");

            return StudioGraphicsShellResult::RuntimeInitializationFailed;
        }

        engine::runtime::Engine*
            runtimeEngine =
                TryGetRuntimeEngine();

        if (
            runtimeEngine == nullptr ||
            !runtimeEngine->IsInitialized())
        {
            ShutdownStudioRuntimeBridge();

            return
                StudioGraphicsShellResult::
                    RuntimeInitializationFailed;
        }

        StudioDX11Bootstrap bootstrap;

        if (!bootstrap.Initialize(
                nativeWindow))
        {
            ShutdownStudioRuntimeBridge();

            return
                StudioGraphicsShellResult::
                    InitializationFailed;
        }

        g_activeDX11Bootstrap =
            &bootstrap;

        RegisterMsgProc(
            &StudioDX11MessageProc);

        HWND windowHandle =
            reinterpret_cast<HWND>(
                nativeWindow);

        ShowWindow(
            windowHandle,
            SW_SHOW);

        UpdateWindow(
            windowHandle);

        StudioGraphicsShellResult finalResult =
            StudioGraphicsShellResult::
                Completed;

        bool running = true;

        engine::platform::Clock::Tick
            previousTime =
                engine::platform::Clock::Now();

        bool timerValid =
            previousTime != 0;

        while (running)
        {
            if (bootstrap.ShouldWaitForMessage())
            {
                (void)engine::platform::
                    MessagePump::
                        WaitForMessage();
            }

            const auto messages =
                engine::platform::
                    MessagePump::
                        ProcessPendingMessages();

            if (
                messages.quitRequested ||
                bootstrap.IsCloseRequested())
            {
                break;
            }

            const engine::platform::Clock::Tick
                currentTime =
                    engine::platform::
                        Clock::Now();

            double deltaSeconds = 0.0;

            if (bootstrap.ConsumeTimerReset())
            {
                timerValid = false;
            }

            if (
                timerValid &&
                currentTime != 0)
            {
                deltaSeconds =
                    engine::platform::
                        Clock::
                            ElapsedSeconds(
                                previousTime,
                                currentTime);
            }

            if (
                !std::isfinite(
                    deltaSeconds) ||
                deltaSeconds < 0.0)
            {
                deltaSeconds = 0.0;
            }

            deltaSeconds =
                (std::min)(
                    deltaSeconds,
                    0.25);

            previousTime =
                currentTime;

            timerValid =
                currentTime != 0;

            if (!runtimeEngine->BeginFrame(
                    deltaSeconds))
            {
                WriteLog(LogLevel::Error, "Runtime", "DX11 BeginFrame failed");

                finalResult = StudioGraphicsShellResult::FrameFailed;

                break;
            }

            const bool frameSucceeded =
                bootstrap.RenderFrame();

            const bool endFrameSucceeded =
                runtimeEngine->EndFrame();

            if (!frameSucceeded)
            {
                finalResult =
                    bootstrap.GetFailureResult();

                break;
            }

            if (!endFrameSucceeded)
            {
                WriteLog(LogLevel::Error, "Runtime", "DX11 EndFrame failed");

                finalResult = StudioGraphicsShellResult::FrameFailed;

                break;
            }
        }

        UnregisterMsgProc(
            &StudioDX11MessageProc);

        g_activeDX11Bootstrap =
            nullptr;

        /*
         * Сначала освобождаем GraphicsDX11:
         *
         * RT bindings
         * depth
         * swap chain
         * device
         *
         * После этого Runtime.
         */
        bootstrap.Shutdown();

        ShutdownStudioRuntimeBridge();

        WriteFormattedLog(LogLevel::Information, "Graphics.DX11", "Bootstrap stopped: %s", ToString(finalResult));

        return finalResult;
    }
}