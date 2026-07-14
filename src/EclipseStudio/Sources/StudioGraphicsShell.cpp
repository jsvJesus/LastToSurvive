#include "r3dPCH.h"
#include "r3d.h"

#include "StudioGraphicsShell.h"
#include "StudioRuntimeBridge.h"

#include <Graphics/CommandContext.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/SwapChain.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>
#include <GraphicsDX11/D3D11Device.h>
#include <Platform/MessagePump.h>
#include <Platform/Clock.h>
#include <Platform/Window.h>
#include <Runtime/RendererBackend.h>
#include <Runtime/Engine.h>

#include <algorithm>
#include <cmath>
#include <memory>

extern void RegisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));
extern void UnregisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));
extern char __r3dCmdLine[1024];
PCHAR* CommandLineToArgvA(PCHAR commandLine, int* argumentCount);

namespace
{
    using engine::graphics::GraphicsResult;
    using studio::StudioGraphicsShellResult;

    bool HasCommandLineSwitch(const char* switchName) noexcept
    {
        if (switchName == nullptr || *switchName == '\0')
            return false;

        int argumentCount = 0;
        PCHAR* const arguments = CommandLineToArgvA(
            __r3dCmdLine, &argumentCount);
        if (arguments == nullptr)
            return false;
        bool found = false;
        for (int index = 0; index < argumentCount; ++index)
        {
            if (_stricmp(arguments[index], switchName) == 0)
            {
                found = true;
                break;
            }
        }
        GlobalFree(arguments);
        return found;
    }

    class StudioDX11Shell final
    {
    public:
        bool Initialize(const std::uintptr_t nativeWindow) noexcept
        {
            Shutdown();
            HWND const windowHandle = reinterpret_cast<HWND>(nativeWindow);
            if (windowHandle == nullptr)
                return Fail("invalid Studio HWND", GraphicsResult::InvalidArgument);

            if (!ApplyResizableWindowStyle(windowHandle))
                return Fail("Studio window style", GraphicsResult::BackendFailure);

            window_ = engine::platform::Window(
                engine::platform::NativeWindowHandle::FromValue(nativeWindow));
            const engine::platform::WindowSize size = window_.GetClientSize();
            if (!window_.IsValid() || size.IsEmpty())
                return Fail("invalid Studio HWND or zero client size", GraphicsResult::InvalidArgument);

            engine::graphics::RenderDeviceDesc deviceDesc;
            deviceDesc.backend = engine::graphics::GraphicsBackend::D3D11;
            deviceDesc.enableValidation = true;
            // Development-only fallback validation switch. It is inert unless
            // explicitly supplied and forces failure before device creation.
            if (HasCommandLineSwitch("-dx11shell-fail"))
                deviceDesc.backend = engine::graphics::GraphicsBackend::None;

            GraphicsResult result = device_.Initialize(deviceDesc);
            if (engine::graphics::Failed(result))
                return Fail("device creation", result);

            context_ = device_.GetImmediateCommandContext();
            if (context_ == nullptr || !context_->IsValid())
                return Fail("immediate CommandContext", GraphicsResult::InvalidState);

            engine::graphics::SwapChainDesc swapDesc;
            swapDesc.window = window_.GetNativeHandle();
            swapDesc.width = size.width;
            swapDesc.height = size.height;
            swapDesc.bufferCount = 2;
            swapDesc.presentMode = engine::graphics::PresentMode::VSync;
            result = device_.CreateSwapChain(swapDesc, swapChain_);
            if (engine::graphics::Failed(result))
                return Fail("swap-chain creation", result);

            if (!CreateSizeDependentResources(size.width, size.height))
                return false;

            width_ = size.width;
            height_ = size.height;
            minimized_ = false;
            initialized_ = true;
            r3dOutToLog("[Graphics] Selected DX11 Studio shell backend\n");
            r3dOutToLog("[Graphics][DX11] Device created: featureLevel=0x%04x, debugLayer=%s\n",
                static_cast<unsigned int>(device_.GetFeatureLevel()),
                deviceDesc.enableValidation ? "enabled" : "disabled");
            r3dOutToLog("[Graphics][DX11] Swap chain, backbuffer and depth buffer created: %ux%u\n",
                width_, height_);
            return true;
        }

        void OnSize(const WPARAM sizeType, const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (!initialized_)
                return;
            if (sizeType == SIZE_MINIMIZED || width == 0 || height == 0)
            {
                if (!minimized_)
                    r3dOutToLog("[Graphics][DX11] Studio window minimized\n");
                minimized_ = true;
                return;
            }

            pendingWidth_ = width;
            pendingHeight_ = height;
            resizePending_ = true;
            if (minimized_)
            {
                r3dOutToLog("[Graphics][DX11] Studio window restored\n");
                resetTimer_ = true;
            }
            minimized_ = false;
        }

        void RequestClose() noexcept
        {
            if (!closeRequested_)
                r3dOutToLog("[Graphics][DX11] Normal user close requested\n");
            closeRequested_ = true;
        }

        [[nodiscard]] bool IsCloseRequested() const noexcept
        {
            return closeRequested_;
        }

        [[nodiscard]] bool ShouldWaitForMessage() const noexcept
        {
            return minimized_ || occluded_;
        }

        [[nodiscard]] bool ConsumeTimerReset() noexcept
        {
            const bool reset = resetTimer_;
            resetTimer_ = false;
            return reset;
        }

        [[nodiscard]] StudioGraphicsShellResult GetFailureResult() const noexcept
        {
            return failureResult_;
        }

        bool RenderFrame() noexcept
        {
            if (!initialized_)
                return false;
            if (resizePending_ && !Resize(pendingWidth_, pendingHeight_))
                return false;
            if (minimized_)
                return true;

            engine::graphics::ClearColor clearColor;
            clearColor.red = 0.035F;
            clearColor.green = 0.055F;
            clearColor.blue = 0.085F;
            clearColor.alpha = 1.0F;

            GraphicsResult result = context_->SetSwapChainRenderTarget(
                *swapChain_, depth_);
            if (engine::graphics::Failed(result))
                return FailFrame("backbuffer/depth bind", result);
            result = context_->ClearSwapChainColor(*swapChain_, clearColor);
            if (engine::graphics::Failed(result))
                return FailFrame("backbuffer clear", result);
            result = context_->ClearDepthStencilTarget(
                depth_, engine::graphics::ClearDepthStencilFlags::Depth |
                engine::graphics::ClearDepthStencilFlags::Stencil, 1.0F, 0);
            if (engine::graphics::Failed(result))
                return FailFrame("depth clear", result);

            engine::graphics::PresentStatus status;
            result = swapChain_->Present(status);
            if (status == engine::graphics::PresentStatus::Occluded)
            {
                if (!occluded_)
                    r3dOutToLog("[Graphics][DX11] Studio window occluded\n");
                occluded_ = true;
                return true;
            }
            if (occluded_)
            {
                r3dOutToLog("[Graphics][DX11] Studio window visible after occlusion\n");
                occluded_ = false;
                resetTimer_ = true;
            }
            if (engine::graphics::Failed(result) ||
                status == engine::graphics::PresentStatus::DeviceLost ||
                status == engine::graphics::PresentStatus::DeviceRemoved)
            {
                r3dOutToLog("[Graphics][DX11] Present failure: result=%s, status=%s\n",
                    engine::graphics::ToString(result), engine::graphics::ToString(status));
                failureResult_ = status == engine::graphics::PresentStatus::DeviceLost
                    ? StudioGraphicsShellResult::DeviceLost
                    : status == engine::graphics::PresentStatus::DeviceRemoved
                        ? StudioGraphicsShellResult::DeviceRemoved
                        : StudioGraphicsShellResult::FrameFailed;
                return false;
            }
            return true;
        }

        void Shutdown() noexcept
        {
            if (context_ != nullptr)
            {
                context_->UnbindRenderTargets();
                context_->ClearState();
                context_->Flush();
            }
            DestroyDepth();
            swapChain_.reset();
            context_ = nullptr;
            device_.Shutdown();
            RestoreWindowStyle();
            if (initialized_)
                r3dOutToLog("[Graphics][DX11] Studio shell backend shutdown\n");
            initialized_ = false;
            resizePending_ = false;
            minimized_ = false;
            occluded_ = false;
            resetTimer_ = true;
            closeRequested_ = false;
            failureResult_ = StudioGraphicsShellResult::FrameFailed;
            width_ = height_ = 0;
        }

        ~StudioDX11Shell() noexcept { Shutdown(); }

    private:
        bool ApplyResizableWindowStyle(HWND const windowHandle) noexcept
        {
            SetLastError(ERROR_SUCCESS);
            const LONG_PTR currentStyle =
                GetWindowLongPtr(windowHandle, GWL_STYLE);
            if (currentStyle == 0 && GetLastError() != ERROR_SUCCESS)
            {
                r3dOutToLog("[Graphics][DX11] GetWindowLongPtr failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
                return false;
            }

            windowHandle_ = windowHandle;
            originalWindowStyle_ = currentStyle;
            const LONG_PTR requiredStyle = currentStyle |
                WS_THICKFRAME | WS_MAXIMIZEBOX;
            if (requiredStyle == currentStyle)
                return true;

            SetLastError(ERROR_SUCCESS);
            const LONG_PTR previousStyle = SetWindowLongPtr(
                windowHandle, GWL_STYLE, requiredStyle);
            if (previousStyle == 0 && GetLastError() != ERROR_SUCCESS)
            {
                r3dOutToLog("[Graphics][DX11] SetWindowLongPtr failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
                windowHandle_ = nullptr;
                return false;
            }

            styleChanged_ = true;
            if (SetWindowPos(windowHandle, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                        SWP_NOACTIVATE | SWP_FRAMECHANGED) == FALSE)
            {
                r3dOutToLog("[Graphics][DX11] Applying window frame failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
                RestoreWindowStyle();
                return false;
            }
            return true;
        }

        void RestoreWindowStyle() noexcept
        {
            if (!styleChanged_ || windowHandle_ == nullptr)
            {
                windowHandle_ = nullptr;
                styleChanged_ = false;
                return;
            }

            SetLastError(ERROR_SUCCESS);
            const LONG_PTR previousStyle = SetWindowLongPtr(
                windowHandle_, GWL_STYLE, originalWindowStyle_);
            const DWORD styleError = GetLastError();
            if (previousStyle == 0 && styleError != ERROR_SUCCESS)
            {
                r3dOutToLog("[Graphics][DX11] Restoring window style failed: error=%lu\n",
                    static_cast<unsigned long>(styleError));
            }
            else if (SetWindowPos(windowHandle_, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                             SWP_NOACTIVATE | SWP_FRAMECHANGED) == FALSE)
            {
                r3dOutToLog("[Graphics][DX11] Restoring window frame failed: error=%lu\n",
                    static_cast<unsigned long>(GetLastError()));
            }
            windowHandle_ = nullptr;
            styleChanged_ = false;
        }

        bool CreateSizeDependentResources(const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (width == 0 || height == 0)
                return false;
            engine::graphics::TextureDesc depthDesc;
            depthDesc.width = width;
            depthDesc.height = height;
            depthDesc.format = engine::graphics::Format::D24UNormS8UInt;
            depthDesc.bindFlags = engine::graphics::TextureBindFlags::DepthStencil;
            GraphicsResult result = device_.CreateTexture(depthDesc, nullptr, 0, depth_);
            if (engine::graphics::Failed(result))
                return Fail("depth-buffer creation", result);

            engine::graphics::Viewport viewport;
            viewport.width = static_cast<float>(width);
            viewport.height = static_cast<float>(height);
            result = context_->SetViewport(viewport);
            if (engine::graphics::Failed(result))
                return Fail("viewport setup", result);
            engine::graphics::ScissorRect scissor;
            scissor.right = static_cast<std::int32_t>(width);
            scissor.bottom = static_cast<std::int32_t>(height);
            result = context_->SetScissorRect(scissor);
            if (engine::graphics::Failed(result))
                return Fail("scissor setup", result);
            return true;
        }

        bool Resize(const std::uint32_t width, const std::uint32_t height) noexcept
        {
            resizePending_ = false;
            if (width == 0 || height == 0 || (width == width_ && height == height_))
                return true;
            context_->UnbindRenderTargets();
            DestroyDepth();
            GraphicsResult result = swapChain_->Resize(width, height);
            if (engine::graphics::Failed(result))
                return FailFrame("swap-chain resize", result);
            if (!CreateSizeDependentResources(width, height))
                return false;
            width_ = width;
            height_ = height;
            r3dOutToLog("[Graphics][DX11] Studio shell resized: %ux%u\n", width_, height_);
            return true;
        }

        void DestroyDepth() noexcept
        {
            if (depth_.IsValid())
            {
                const GraphicsResult result = device_.DestroyTexture(depth_);
                if (engine::graphics::Failed(result))
                    r3dOutToLog("[Graphics][DX11] Depth destruction failed: %s\n",
                        engine::graphics::ToString(result));
                depth_ = {};
            }
        }

        bool Fail(const char* operation, const GraphicsResult result) noexcept
        {
            r3dOutToLog("[Graphics][DX11] Initialization failed at %s: %s\n",
                operation, engine::graphics::ToString(result));
            Shutdown();
            return false;
        }

        bool FailFrame(const char* operation, const GraphicsResult result) noexcept
        {
            r3dOutToLog("[Graphics][DX11] Frame failure at %s: %s\n",
                operation, engine::graphics::ToString(result));
            failureResult_ = result == GraphicsResult::DeviceLost
                ? StudioGraphicsShellResult::DeviceLost
                : result == GraphicsResult::DeviceRemoved
                    ? StudioGraphicsShellResult::DeviceRemoved
                    : StudioGraphicsShellResult::FrameFailed;
            return false;
        }

        engine::platform::Window window_;
        engine::graphics::d3d11::D3D11Device device_;
        engine::graphics::CommandContext* context_ = nullptr;
        std::unique_ptr<engine::graphics::SwapChain> swapChain_;
        engine::graphics::TextureHandle depth_;
        std::uint32_t width_ = 0, height_ = 0;
        std::uint32_t pendingWidth_ = 0, pendingHeight_ = 0;
        bool initialized_ = false, minimized_ = false, resizePending_ = false;
        bool closeRequested_ = false, occluded_ = false, resetTimer_ = true;
        HWND windowHandle_ = nullptr;
        LONG_PTR originalWindowStyle_ = 0;
        bool styleChanged_ = false;
        StudioGraphicsShellResult failureResult_ = StudioGraphicsShellResult::FrameFailed;
    };

    StudioDX11Shell* g_activeShell = nullptr;

    bool StudioDX11ShellMsgProc(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (g_activeShell == nullptr)
            return false;
        if (message == WM_SIZE)
        {
            g_activeShell->OnSize(wParam, LOWORD(lParam), HIWORD(lParam));
        }
        else if (message == WM_CLOSE || message == WM_DESTROY)
        {
            g_activeShell->RequestClose();
            return true;
        }
        return false;
    }
}

namespace studio
{
    bool WantsDX11Shell() noexcept
    {
        return HasCommandLineSwitch("-dx11shell") ||
            HasCommandLineSwitch("/dx11shell") ||
            HasCommandLineSwitch("-dx11shell-fail");
    }

    const char* ToString(const StudioGraphicsShellResult result) noexcept
    {
        switch (result)
        {
        case StudioGraphicsShellResult::NotRequested: return "NotRequested";
        case StudioGraphicsShellResult::Completed: return "Completed";
        case StudioGraphicsShellResult::InitializationFailed: return "InitializationFailed";
        case StudioGraphicsShellResult::RuntimeInitializationFailed: return "RuntimeInitializationFailed";
        case StudioGraphicsShellResult::FrameFailed: return "FrameFailed";
        case StudioGraphicsShellResult::DeviceLost: return "DeviceLost";
        case StudioGraphicsShellResult::DeviceRemoved: return "DeviceRemoved";
        default: return "Unknown";
        }
    }

    StudioGraphicsShellResult RunDX11Shell(
        const std::uintptr_t nativeWindow) noexcept
    {
        if (!WantsDX11Shell())
            return StudioGraphicsShellResult::NotRequested;

        StudioDX11Shell shell;
        if (!shell.Initialize(nativeWindow))
        {
            return StudioGraphicsShellResult::InitializationFailed;
        }

        if (!InitializeStudioRuntimeBridge(engine::runtime::RendererBackend::D3D11))
        {
            r3dOutToLog("[Graphics][DX11] Runtime initialization failed\n");
            shell.Shutdown();
            return StudioGraphicsShellResult::RuntimeInitializationFailed;
        }

        g_activeShell = &shell;
        RegisterMsgProc(StudioDX11ShellMsgProc);
        ShowWindow(reinterpret_cast<HWND>(nativeWindow), SW_SHOW);
        UpdateWindow(reinterpret_cast<HWND>(nativeWindow));

        bool frameSucceeded = true;
        bool quitRequested = false;
        engine::platform::Clock::Tick previousTick = engine::platform::Clock::Now();
        bool timerInitialized = previousTick != 0;
        while (!quitRequested && frameSucceeded)
        {
            if (shell.ShouldWaitForMessage())
            {
                const bool waitSucceeded =
                    engine::platform::MessagePump::WaitForMessage();
                if (!waitSucceeded)
                    r3dOutToLog("[Graphics][DX11] Message wait failed\n");
            }
            const engine::platform::MessagePumpResult messages =
                engine::platform::MessagePump::ProcessPendingMessages();
            quitRequested = messages.quitRequested || shell.IsCloseRequested();
            if (!quitRequested)
            {
                const engine::platform::Clock::Tick currentTick =
                    engine::platform::Clock::Now();
                double deltaSeconds = 0.0;
                if (shell.ConsumeTimerReset())
                    timerInitialized = false;
                if (timerInitialized && currentTick != 0)
                    deltaSeconds = engine::platform::Clock::ElapsedSeconds(
                        previousTick, currentTick);
                if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0)
                    deltaSeconds = 0.0;
                deltaSeconds = (std::min)(deltaSeconds, 0.25);
                previousTick = currentTick;
                timerInitialized = currentTick != 0;
                engine::runtime::Engine* const runtime = TryGetRuntimeEngine();
                const bool beganFrame = runtime != nullptr &&
                    runtime->BeginFrame(deltaSeconds);
                frameSucceeded = beganFrame && shell.RenderFrame();
                if (!beganFrame)
                    r3dOutToLog("[Graphics][DX11] Runtime BeginFrame failed\n");
                if (beganFrame && !runtime->EndFrame())
                {
                    r3dOutToLog("[Graphics][DX11] Runtime EndFrame failed\n");
                    frameSucceeded = false;
                }
            }
        }

        UnregisterMsgProc(StudioDX11ShellMsgProc);
        g_activeShell = nullptr;
        ShutdownStudioRuntimeBridge();
        const StudioGraphicsShellResult result = quitRequested
            ? StudioGraphicsShellResult::Completed
            : shell.GetFailureResult();
        if (result == StudioGraphicsShellResult::Completed)
            r3dOutToLog("[Graphics][DX11] Normal user close completed\n");
        shell.Shutdown();
        r3dOutToLog("[Graphics][DX11] Final shell result: %s\n", ToString(result));
        r3dOutToLog("[Graphics][DX11] Shutdown completed\n");
        return result;
    }
}
