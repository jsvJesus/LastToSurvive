#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include "Editors/StudioEditorUI.h"
#include "StudioGraphicsShell.h"

#include <Application/Application.h>

#include <Core/Log.h>

#include <ImGui/ImGuiHost.h>

#include <Graphics/CommandContext.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/SwapChain.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>

#include <GraphicsDX11/D3D11Device.h>

#include <Platform/Clock.h>
#include <Platform/Process.h>
#include <Platform/Thread.h>
#include <Platform/Window.h>

#include <Runtime/EngineConfig.h>
#include <Runtime/EngineMode.h>
#include <Runtime/RendererBackend.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>
#include <string>

namespace
{
    using engine::core::LogLevel;
    using engine::graphics::GraphicsResult;
    using studio::StudioGraphicsShellResult;

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

    [[nodiscard]]
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

    [[nodiscard]]
    lts::application::ApplicationDesc
        CreateStudioApplicationDescription()
    {
        lts::application::ApplicationDesc
            description;

        description.title =
            L"DX11 Studio";

        description.logFileName =
            "Studio.log";

        description.width = 1600;
        description.height = 900;

        description.resizable = true;
        description.startMaximized = false;
        description.enableDpiAwareness = true;

        description.engineConfig.applicationName =
            "DX11 Studio";

        description.engineConfig.mode =
            engine::runtime::
                EngineMode::Studio;

        description.engineConfig.rendererBackend =
            engine::runtime::
                RendererBackend::D3D11;

        description.engineConfig.enableValidation =
            true;

        description.engineConfig.
            enableMainThreadChecks =
                true;

        return description;
    }

    [[nodiscard]]
    StudioGraphicsShellResult
        MapApplicationResult(
            const lts::application::
                ApplicationResult result) noexcept
    {
        switch (result)
        {
        case lts::application::
            ApplicationResult::Success:

            return
                StudioGraphicsShellResult::
                    Completed;

        case lts::application::
            ApplicationResult::
                RuntimeInitializationFailed:

            return
                StudioGraphicsShellResult::
                    RuntimeInitializationFailed;

        case lts::application::
            ApplicationResult::
                RuntimeFrameFailed:

            return
                StudioGraphicsShellResult::
                    FrameFailed;

        default:
            return
                StudioGraphicsShellResult::
                    InitializationFailed;
        }
    }

    class StudioDX11Bootstrap final
    {
        void ResetFpsCounter() noexcept
        {
            fpsSampleStart_ =
                engine::platform::Clock::Now();

            fpsFrameCount_ = 0;
        }
        
        void UpdateWindowTitleFps() noexcept
        {
            if (windowHandle_ == nullptr)
            {
                return;
            }

            ++fpsFrameCount_;

            const engine::platform::Clock::Tick now =
                engine::platform::Clock::Now();

            if (fpsSampleStart_ == 0)
            {
                fpsSampleStart_ = now;
                fpsFrameCount_ = 0;
                return;
            }

            const double elapsedSeconds =
                engine::platform::Clock::ElapsedSeconds(
                    fpsSampleStart_,
                    now);

            constexpr double updateIntervalSeconds = 0.5;

            if (elapsedSeconds < updateIntervalSeconds)
            {
                return;
            }

            const double fps =
                static_cast<double>(fpsFrameCount_) /
                elapsedSeconds;

            const std::uint32_t roundedFps =
                static_cast<std::uint32_t>(
                    fps + 0.5);

            const engine::platform::NativeWindowHandle nativeWindow =
                engine::platform::NativeWindowHandle::FromValue(
                    reinterpret_cast<std::uintptr_t>(
                        windowHandle_));

            engine::platform::Window window(
                nativeWindow);

            std::wstring title =
                L"DX11 Studio | FPS: ";

            title += std::to_wstring(
                roundedFps);

            window.SetTitle(title);

            fpsFrameCount_ = 0;
            fpsSampleStart_ = now;
        }
        
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

        [[nodiscard]]
        bool Initialize(
            engine::platform::Window&
                window) noexcept
        {
            Shutdown();

            failureResult_ =
                StudioGraphicsShellResult::
                    InitializationFailed;

            const engine::platform::
                NativeWindowHandle nativeWindow =
                    window.GetNativeHandle();

            if (!nativeWindow.IsValid())
            {
                return FailInitialization(
                    "invalid native window",
                    GraphicsResult::
                        InvalidArgument);
            }

            windowHandle_ =
                reinterpret_cast<HWND>(
                    nativeWindow.Value());

            if (windowHandle_ == nullptr)
            {
                return FailInitialization(
                    "invalid Win32 window",
                    GraphicsResult::
                        InvalidArgument);
            }

            const engine::platform::WindowSize
                clientSize =
                    window.GetClientSize();

            if (clientSize.IsEmpty())
            {
                return FailInitialization(
                    "window client size",
                    GraphicsResult::
                        InvalidArgument);
            }

            engine::graphics::
                RenderDeviceDesc deviceDesc;

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
                !context_->IsValid()
            )
            {
                return FailInitialization(
                    "immediate context",
                    GraphicsResult::
                        InvalidState);
            }

            engine::graphics::
                SwapChainDesc swapChainDesc;

            swapChainDesc.window =
                nativeWindow;

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

            if (!CreateSceneColorBuffer(
                    clientSize.width,
                    clientSize.height))
            {
                return false;
            }

            if (!imguiHost_.Initialize(
                    windowHandle_,
                    device_.GetNativeDevice(),
                    device_.
                        GetNativeImmediateContext(),
                    "StudioEditor.ini"))
            {
                return FailInitialization(
                    "Dear ImGui",
                    GraphicsResult::
                        BackendFailure);
            }

            if (!studio::editor::InitializeEditorUI(
                    device_,
                    nativeWindow))
            {
                return FailInitialization(
                    "DX11 terrain editor",
                    GraphicsResult::BackendFailure);
            }

            width_ =
                clientSize.width;

            height_ =
                clientSize.height;

            pendingWidth_ = 0;
            pendingHeight_ = 0;

            resizePending_ = false;
            minimized_ = false;
            occluded_ = false;

            ResetFpsCounter();
            initialized_ = true;

            failureResult_ =
                StudioGraphicsShellResult::
                    FrameFailed;

            WriteFormattedLog(
                LogLevel::Information,
                "Graphics.DX11",
                "Bootstrap initialized: %ux%u",
                static_cast<unsigned int>(
                    width_),
                static_cast<unsigned int>(
                    height_));

            WriteLog(
                LogLevel::Information,
                "Editor.ImGui",
                "Dear ImGui initialized");

            return true;
        }

        void Shutdown() noexcept
        {
            initialized_ = false;

            studio::editor::ShutdownEditorUI(device_);

            imguiHost_.Shutdown();

            if (context_ != nullptr)
            {
                context_->
                    UnbindRenderTargets();

                context_->ClearState();
                context_->Flush();
            }

            DestroySceneColorBuffer();
            DestroyDepthBuffer();

            swapChain_.reset();

            context_ = nullptr;

            device_.Shutdown();

            windowHandle_ = nullptr;
            
            fpsSampleStart_ = 0;
            fpsFrameCount_ = 0;

            width_ = 0;
            height_ = 0;

            pendingWidth_ = 0;
            pendingHeight_ = 0;

            resizePending_ = false;
            minimized_ = false;
            occluded_ = false;
        }

        void OnResize(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            if (!initialized_)
            {
                return;
            }

            if (
                width == 0 ||
                height == 0
            )
            {
                return;
            }

            pendingWidth_ = width;
            pendingHeight_ = height;

            resizePending_ = true;
            minimized_ = false;
        }

        void OnMinimized() noexcept
        {
            minimized_ = true;
            fpsSampleStart_ = 0;
            fpsFrameCount_ = 0;
        }

        void OnRestored() noexcept
        {
            minimized_ = false;
            occluded_ = false;
            ResetFpsCounter();
        }

        [[nodiscard]]
        bool IsOccluded() const noexcept
        {
            return occluded_;
        }

        [[nodiscard]]
        StudioGraphicsShellResult
            GetFailureResult() const noexcept
        {
            return failureResult_;
        }

        [[nodiscard]]
        bool RenderFrame() noexcept
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
                context_->SetRenderTargets(
                    &sceneColorBuffer_,
                    1U,
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

            const engine::graphics::
                ClearColor clearColor
                {
                    0.025f,
                    0.035f,
                    0.050f,
                    1.0f
                };

            result =
                context_->ClearColorTarget(
                    sceneColorBuffer_,
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

            engine::graphics::
                PresentStatus presentStatus =
                    engine::graphics::
                        PresentStatus::
                            Presented;

            imguiHost_.BeginFrame();

            result = studio::editor::RenderEditorWorld(
                *context_,
                width_,
                height_);

            if (engine::graphics::Failed(result))
            {
                return FailFrame(
                    "terrain editor render",
                    result);
            }

            context_->UnbindRenderTargets();

            result = context_->SetSwapChainRenderTarget(*swapChain_);
            if (engine::graphics::Failed(result))
            {
                return FailFrame("set post-process target", result);
            }

            result = context_->ClearSwapChainColor(*swapChain_, clearColor);
            if (engine::graphics::Failed(result))
            {
                return FailFrame("clear post-process target", result);
            }

            result = studio::editor::RenderEditorColorCorrection(
                *context_,
                sceneColorBuffer_,
                width_,
                height_);
            if (engine::graphics::Failed(result))
            {
                return FailFrame("color correction", result);
            }

            studio::editor::
                DrawEditorUI();

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

            occluded_ = false;

            UpdateWindowTitleFps();

            return true;
        }

        [[nodiscard]]
        bool ProcessNativeMessage(
            void* const nativeWindow,
            const std::uint32_t message,
            const std::uintptr_t wordParameter,
            const std::intptr_t longParameter) noexcept
        {
            if (!imguiHost_.IsInitialized())
            {
                return false;
            }

            return
                imguiHost_.ProcessNativeMessage(
                    nativeWindow,
                    message,
                    wordParameter,
                    longParameter);
        }

    private:
        [[nodiscard]]
        bool CreateDepthBuffer(
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

        [[nodiscard]]
        bool CreateSceneColorBuffer(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            engine::graphics::TextureDesc description;
            description.width = width;
            description.height = height;
            description.format =
                engine::graphics::Format::R16G16B16A16Float;
            description.bindFlags =
                engine::graphics::TextureBindFlags::RenderTarget |
                engine::graphics::TextureBindFlags::ShaderResource;

            const GraphicsResult result = device_.CreateTexture(
                description,
                nullptr,
                0U,
                sceneColorBuffer_);
            if (engine::graphics::Failed(result))
            {
                return FailInitialization("scene color buffer", result);
            }
            return true;
        }

        void DestroySceneColorBuffer() noexcept
        {
            if (!sceneColorBuffer_.IsValid())
            {
                return;
            }
            static_cast<void>(device_.DestroyTexture(sceneColorBuffer_));
            sceneColorBuffer_ = {};
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

        [[nodiscard]]
        bool Resize(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            resizePending_ = false;

            if (
                width == 0 ||
                height == 0
            )
            {
                return true;
            }

            if (
                width == width_ &&
                height == height_
            )
            {
                return true;
            }

            context_->
                UnbindRenderTargets();

            DestroySceneColorBuffer();
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

            if (!CreateSceneColorBuffer(width, height))
            {
                return false;
            }

            width_ = width;
            height_ = height;

            WriteFormattedLog(
                LogLevel::Information,
                "Graphics.DX11",
                "Backbuffer resized: %ux%u",
                static_cast<unsigned int>(
                    width_),
                static_cast<unsigned int>(
                    height_));

            return true;
        }

        [[nodiscard]]
        bool FailInitialization(
            const char* const phase,
            const GraphicsResult result) noexcept
        {
            failureResult_ =
                StudioGraphicsShellResult::
                    InitializationFailed;

            WriteFormattedLog(
                LogLevel::Error,
                "Graphics.DX11",
                "Initialization failed at %s: %s",
                phase,
                engine::graphics::
                    ToString(result));

            Shutdown();

            return false;
        }

        [[nodiscard]]
        bool FailFrame(
            const char* const phase,
            const GraphicsResult result) noexcept
        {
            failureResult_ =
                ResultFromGraphicsFailure(
                    result);

            WriteFormattedLog(
                LogLevel::Error,
                "Graphics.DX11",
                "Frame failed at %s: %s",
                phase,
                engine::graphics::
                    ToString(result));

            return false;
        }

    private:
        engine::ui::ImGuiHost
            imguiHost_;

        engine::graphics::d3d11::
            D3D11Device device_;

        engine::graphics::
            CommandContext* context_ =
                nullptr;

        std::unique_ptr<
            engine::graphics::SwapChain>
                swapChain_;

        engine::graphics::TextureHandle
            depthBuffer_;

        engine::graphics::TextureHandle
            sceneColorBuffer_;

        HWND windowHandle_ = nullptr;

        std::uint32_t width_ = 0;
        std::uint32_t height_ = 0;

        engine::platform::Clock::Tick fpsSampleStart_ = 0;
        std::uint32_t fpsFrameCount_ = 0;

        std::uint32_t pendingWidth_ = 0;
        std::uint32_t pendingHeight_ = 0;

        bool initialized_ = false;
        bool resizePending_ = false;
        bool minimized_ = false;
        bool occluded_ = false;

        StudioGraphicsShellResult
            failureResult_ =
                StudioGraphicsShellResult::
                    InitializationFailed;
    };

    class StudioApplication final
        : public lts::application::Application
    {
    public:
        StudioApplication()
            : Application(
                CreateStudioApplicationDescription())
        {
        }

        [[nodiscard]]
        StudioGraphicsShellResult
            GetShellResult() const noexcept
        {
            return shellResult_;
        }

    protected:
        [[nodiscard]]
        lts::application::ApplicationResult
            OnInitialize() noexcept override
        {
            if (!bootstrap_.Initialize(
                    GetWindow()))
            {
                shellResult_ =
                    bootstrap_.
                        GetFailureResult();

                return
                    lts::application::
                        ApplicationResult::
                            ClientInitializationFailed;
            }

            shellResult_ =
                StudioGraphicsShellResult::
                    Completed;

            return
                lts::application::
                    ApplicationResult::
                        Success;
        }

        void OnShutdown() noexcept override
        {
            bootstrap_.Shutdown();
        }

        void OnUpdate(
            const double) noexcept override
        {
            if (bootstrap_.IsOccluded())
            {
                engine::platform::
                    SleepForMilliseconds(16);
            }
        }

        void OnRender() noexcept override
        {
            if (
                shellResult_ !=
                StudioGraphicsShellResult::
                    Completed
            )
            {
                return;
            }

            if (!bootstrap_.RenderFrame())
            {
                shellResult_ =
                    bootstrap_.
                        GetFailureResult();

                RequestExit();
            }
        }

        void OnEvent(
            const lts::application::
                ApplicationEvent& event) noexcept override
        {
            switch (event.type)
            {
            case lts::application::
                ApplicationEventType::Resize:

                bootstrap_.OnResize(
                    event.width,
                    event.height);

                break;

            case lts::application::
                ApplicationEventType::Minimized:

                bootstrap_.OnMinimized();

                break;

            case lts::application::
                ApplicationEventType::Restored:

                bootstrap_.OnRestored();

                break;

            default:
                break;
            }
        }

        [[nodiscard]]
        bool OnNativeMessage(
            void* const nativeWindow,
            const std::uint32_t message,
            const std::uintptr_t wordParameter,
            const std::intptr_t longParameter) noexcept override
        {
            return
                bootstrap_.ProcessNativeMessage(
                    nativeWindow,
                    message,
                    wordParameter,
                    longParameter);
        }

    private:
        StudioDX11Bootstrap bootstrap_;

        StudioGraphicsShellResult
            shellResult_ =
                StudioGraphicsShellResult::
                    InitializationFailed;
    };
}

namespace studio
{
    bool WantsDX11Shell() noexcept
    {
        return
            engine::platform::
                HasCurrentProcessArgument(
                    L"-dx11");
    }

    const char* ToString(
        const StudioGraphicsShellResult result) noexcept
    {
        switch (result)
        {
        case StudioGraphicsShellResult::
            NotRequested:

            return "NotRequested";

        case StudioGraphicsShellResult::
            Completed:

            return "Completed";

        case StudioGraphicsShellResult::
            InitializationFailed:

            return "InitializationFailed";

        case StudioGraphicsShellResult::
            RuntimeInitializationFailed:

            return "RuntimeInitializationFailed";

        case StudioGraphicsShellResult::
            FrameFailed:

            return "FrameFailed";

        case StudioGraphicsShellResult::
            DeviceLost:

            return "DeviceLost";

        case StudioGraphicsShellResult::
            DeviceRemoved:

            return "DeviceRemoved";

        default:
            return "Unknown";
        }
    }

    StudioGraphicsShellResult
        RunDX11Shell() noexcept
    {
        if (!WantsDX11Shell())
        {
            return
                StudioGraphicsShellResult::
                    NotRequested;
        }

        WriteLog(
            LogLevel::Information,
            "Graphics",
            "Starting native DX11 Studio application");

        try
        {
            StudioApplication application;

            const lts::application::
                ApplicationResult applicationResult =
                    application.Run();

            const StudioGraphicsShellResult
                shellResult =
                    application.
                        GetShellResult();

            if (
                shellResult !=
                StudioGraphicsShellResult::
                    Completed
            )
            {
                return shellResult;
            }

            return
                MapApplicationResult(
                    applicationResult);
        }
        catch (...)
        {
            WriteLog(
                LogLevel::Critical,
                "Graphics.DX11",
                "Unhandled exception while "
                "creating Studio application");

            return
                StudioGraphicsShellResult::
                    InitializationFailed;
        }
    }
}
