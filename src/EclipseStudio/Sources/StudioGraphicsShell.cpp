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
#include <Platform/Window.h>
#include <Runtime/RendererBackend.h>
#include <Runtime/Engine.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>

extern void RegisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));
extern void UnregisterMsgProc(
    bool (*proc)(UINT, WPARAM, LPARAM));

namespace
{
    using engine::graphics::GraphicsResult;

    bool HasCommandLineSwitch(const char* switchName) noexcept
    {
        if (switchName == nullptr || *switchName == '\0')
            return false;

        std::string commandLine = GetCommandLineA();
        std::string wanted = switchName;
        std::transform(commandLine.begin(), commandLine.end(), commandLine.begin(),
            [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
        std::transform(wanted.begin(), wanted.end(), wanted.begin(),
            [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

        std::string::size_type position = 0;
        while ((position = commandLine.find(wanted, position)) != std::string::npos)
        {
            const bool leftBoundary = position == 0 ||
                std::isspace(static_cast<unsigned char>(commandLine[position - 1])) != 0;
            const std::string::size_type end = position + wanted.size();
            const bool rightBoundary = end == commandLine.size() ||
                std::isspace(static_cast<unsigned char>(commandLine[end])) != 0;
            if (leftBoundary && rightBoundary)
                return true;
            position = end;
        }
        return false;
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

            LONG_PTR style = GetWindowLongPtr(windowHandle, GWL_STYLE);
            style |= WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_MAXIMIZEBOX;
            SetWindowLongPtr(windowHandle, GWL_STYLE, style);

            RECT clientRect = {};
            GetClientRect(windowHandle, &clientRect);
            if (clientRect.right - clientRect.left < 640 ||
                clientRect.bottom - clientRect.top < 480)
            {
                RECT windowRect = {0, 0, 1024, 768};
                AdjustWindowRect(&windowRect, static_cast<DWORD>(style), FALSE);
                SetWindowPos(windowHandle, nullptr, 0, 0,
                    windowRect.right - windowRect.left,
                    windowRect.bottom - windowRect.top,
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            SetWindowPos(windowHandle, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOACTIVATE | SWP_FRAMECHANGED);

            window_ = engine::platform::Window(
                engine::platform::NativeWindowHandle::FromValue(nativeWindow));
            const engine::platform::WindowSize size = window_.GetClientSize();
            if (!window_.IsValid() || size.IsEmpty())
                return Fail("invalid Studio HWND or zero client size", GraphicsResult::InvalidArgument);

            engine::graphics::RenderDeviceDesc deviceDesc;
            deviceDesc.backend = engine::graphics::GraphicsBackend::D3D11;
            deviceDesc.enableValidation = true;
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
            r3dOutToLog("[Graphics][DX11] Device created: featureLevel=0x%04x\n",
                static_cast<unsigned int>(device_.GetFeatureLevel()));
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
                r3dOutToLog("[Graphics][DX11] Studio window restored\n");
            minimized_ = false;
        }

        void RequestClose() noexcept { closeRequested_ = true; }

        [[nodiscard]] bool IsCloseRequested() const noexcept
        {
            return closeRequested_;
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

            GraphicsResult result = context_->ClearDepthStencilTarget(
                depth_, engine::graphics::ClearDepthStencilFlags::Depth |
                engine::graphics::ClearDepthStencilFlags::Stencil, 1.0F, 0);
            if (engine::graphics::Failed(result))
                return FailFrame("depth clear", result);
            result = context_->SetSwapChainRenderTarget(*swapChain_);
            if (engine::graphics::Failed(result))
                return FailFrame("backbuffer bind", result);
            result = context_->ClearSwapChainColor(*swapChain_, clearColor);
            if (engine::graphics::Failed(result))
                return FailFrame("backbuffer clear", result);

            engine::graphics::PresentStatus status;
            result = swapChain_->Present(status);
            if (engine::graphics::Failed(result) ||
                status == engine::graphics::PresentStatus::DeviceLost ||
                status == engine::graphics::PresentStatus::DeviceRemoved)
            {
                r3dOutToLog("[Graphics][DX11] Present failure: result=%s, status=%s\n",
                    engine::graphics::ToString(result), engine::graphics::ToString(status));
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
            if (initialized_)
                r3dOutToLog("[Graphics][DX11] Studio shell backend shutdown\n");
            initialized_ = false;
            resizePending_ = false;
            minimized_ = false;
            closeRequested_ = false;
            width_ = height_ = 0;
        }

        ~StudioDX11Shell() noexcept { Shutdown(); }

    private:
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
        bool closeRequested_ = false;
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

    bool RunDX11Shell(const std::uintptr_t nativeWindow) noexcept
    {
        StudioDX11Shell shell;
        if (!shell.Initialize(nativeWindow))
        {
            r3dOutToLog("[Graphics] Falling back to DX9 Studio backend\n");
            return false;
        }

        if (!InitializeStudioRuntimeBridge(engine::runtime::RendererBackend::D3D11))
        {
            r3dOutToLog("[Graphics][DX11] Runtime initialization failed; falling back to DX9\n");
            return false;
        }

        g_activeShell = &shell;
        RegisterMsgProc(StudioDX11ShellMsgProc);
        ShowWindow(reinterpret_cast<HWND>(nativeWindow), SW_SHOW);
        UpdateWindow(reinterpret_cast<HWND>(nativeWindow));

        bool frameSucceeded = true;
        bool quitRequested = false;
        while (!quitRequested && frameSucceeded)
        {
            const engine::platform::MessagePumpResult messages =
                engine::platform::MessagePump::ProcessPendingMessages();
            quitRequested = messages.quitRequested || shell.IsCloseRequested();
            if (!quitRequested)
            {
                engine::runtime::Engine* const runtime = TryGetRuntimeEngine();
                const bool beganFrame = runtime != nullptr && runtime->BeginFrame(0.0);
                frameSucceeded = beganFrame && shell.RenderFrame();
                if (beganFrame && !runtime->EndFrame())
                    frameSucceeded = false;
            }
        }

        UnregisterMsgProc(StudioDX11ShellMsgProc);
        g_activeShell = nullptr;
        shell.Shutdown();
        ShutdownStudioRuntimeBridge();
        return true;
    }
}
