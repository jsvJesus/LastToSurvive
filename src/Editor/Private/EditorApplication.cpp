#include "Editor/EditorApplication.h"
#include "Editor/EditorScenePicker.h"

#include <Core/Log.h>

#include <Graphics/Format.h>
#include <Graphics/GraphicsBackend.h>
#include <Graphics/GraphicsResult.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>

#include <Runtime/EngineMode.h>
#include <Runtime/RendererBackend.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        lts::application::ApplicationDesc
            CreateEditorDescription()
        {
            lts::application::ApplicationDesc description;

            description.title =
                L"LastToSurvive Editor";

            description.logFileName = "Logs/LTS.Editor.log";

            description.width = 1600;
            description.height = 900;

            description.resizable = true;
            description.startMaximized = false;
            description.enableDpiAwareness = true;

            description.engineConfig.applicationName =
                "LTS.Editor";

            description.engineConfig.mode =
                engine::runtime::EngineMode::Studio;

            description.engineConfig.rendererBackend =
                engine::runtime::
                    RendererBackend::D3D11;

            description.engineConfig.enableValidation =
                true;

            description.engineConfig.enableMainThreadChecks =
                true;

            return description;
        }
    }

    EditorApplication::EditorApplication()
        : Application(
            CreateEditorDescription())
    {
    }

    EditorApplication::~EditorApplication() noexcept =
        default;

    lts::application::ApplicationResult
    EditorApplication::OnInitialize() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Initializing editor.");

        if (!editorShell_.Initialize(GetWindow().GetNativeHandle()))
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Critical,
                "LTS.Editor",
                "Failed to initialize editor shell.");

            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        try
        {
            sceneDocument_.CreateDefaultLevel();
        }
        catch (...)
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Critical,
                "LTS.Editor.Scene",
                "Failed to create the default editor level.");

            editorShell_.Shutdown();

            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        editorShell_.RefreshScene(sceneDocument_);

        cameraController_.SetViewportWindow(
            editorShell_.GetViewportWindowHandle());

        if (!InitializeGraphics())
        {
            cameraController_.SetViewportWindow({});
            sceneDocument_.Clear();
            editorShell_.Shutdown();

            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        if (!sceneRenderer_.Initialize(graphicsDevice_))
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Critical,
                "LTS.Editor.SceneRenderer",
                "Failed to initialize editor scene renderer.");

            sceneRenderer_.Shutdown(graphicsDevice_);
            cameraController_.SetViewportWindow({});
            sceneDocument_.Clear();
            ShutdownGraphics();
            editorShell_.Shutdown();

            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Editor initialization completed.");

        return lts::application::ApplicationResult::Success;
    }

    void EditorApplication::OnShutdown() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Shutting down editor.");

        cameraController_.SetViewportWindow({});

        sceneRenderer_.Shutdown(graphicsDevice_);
        sceneDocument_.Clear();

        ShutdownGraphics();
        editorShell_.Shutdown();
    }

    void EditorApplication::OnUpdate(
    const double deltaSeconds) noexcept
    {
        cameraController_.Update(
            deltaSeconds,
            editorShell_.ConsumeViewportWheelSteps());

        std::size_t selectedEntityIndex =
            InvalidEditorEntityIndex;

        /*
         * Выбор через World Outliner.
         */
        if (
            editorShell_.ConsumeHierarchySelection(
                selectedEntityIndex) &&
            sceneDocument_.SelectEntityByIndex(
                selectedEntityIndex))
        {
            editorShell_.ShowEntityDetails(
                sceneDocument_.GetSelectedEntity());
        }

        /*
         * Выбор кликом во viewport.
         */
        ViewportClick viewportClick;

        if (editorShell_.ConsumeViewportClick(viewportClick))
        {
            const engine::platform::WindowSize viewportSize =
                editorShell_.GetViewportSize();

            EditorPickRay pickRay;

            const bool rayBuilt =
                cameraController_.BuildPickRay(
                    viewportClick.x,
                    viewportClick.y,
                    viewportSize.width,
                    viewportSize.height,
                    pickRay);

            std::size_t pickedEntityIndex =
                InvalidEditorEntityIndex;

            float pickedDistance = 0.0F;

            if (
                rayBuilt &&
                EditorScenePicker::Pick(
                    sceneDocument_,
                    pickRay,
                    pickedEntityIndex,
                    pickedDistance) &&
                sceneDocument_.SelectEntityByIndex(
                    pickedEntityIndex))
            {
                editorShell_.SelectHierarchyEntity(
                    pickedEntityIndex);

                editorShell_.ShowEntityDetails(
                    sceneDocument_.GetSelectedEntity());
            }
            else
            {
                sceneDocument_.ClearSelection();

                editorShell_.SelectHierarchyEntity(
                    InvalidEditorEntityIndex);

                editorShell_.ShowEntityDetails(nullptr);
            }
        }

        EditorMode changedMode =
            EditorMode::Level;

        if (editorShell_.ConsumeModeChanged(changedMode))
        {
            switch (changedMode)
            {
                case EditorMode::Level:
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Information,
                        "LTS.Editor",
                        "Level mode selected.");
                    break;

                case EditorMode::Character:
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Information,
                        "LTS.Editor",
                        "Character mode selected.");
                    break;

                case EditorMode::Icon:
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Information,
                        "LTS.Editor",
                        "Icon mode selected.");
                    break;

                case EditorMode::Physics:
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Information,
                        "LTS.Editor",
                        "Physics mode selected.");
                    break;

                case EditorMode::Particles:
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Information,
                        "LTS.Editor",
                        "Particles mode selected.");
                    break;

                case EditorMode::Play:
                default:
                    break;
            }
        }

        if (swapChainOccluded_)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(16));
        }
    }

    void EditorApplication::OnRender() noexcept
    {
        if (
            !graphicsReady_ ||
            IsMinimized() ||
            swapChain_ == nullptr ||
            commandContext_ == nullptr)
        {
            return;
        }

        engine::graphics::Viewport viewport;

        viewport.x = 0.0F;
        viewport.y = 0.0F;
        viewport.width = static_cast<float>(viewportWidth_);
        viewport.height = static_cast<float>(viewportHeight_);
        viewport.minDepth = 0.0F;
        viewport.maxDepth = 1.0F;

        auto result = commandContext_->SetViewport(viewport);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "SetViewport",
                result);

            return;
        }

        result = commandContext_->SetSwapChainRenderTarget(
            *swapChain_,
            depthStencil_);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "SetSwapChainRenderTarget",
                result);

            return;
        }

        engine::graphics::ClearColor clearColor;

        clearColor.red = 0.018F;
        clearColor.green = 0.022F;
        clearColor.blue = 0.026F;
        clearColor.alpha = 1.0F;

        result = commandContext_->ClearSwapChainColor(
            *swapChain_,
            clearColor);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "ClearSwapChainColor",
                result);

            return;
        }

        result = commandContext_->ClearDepthStencilTarget(
            depthStencil_,
            engine::graphics::ClearDepthStencilFlags::Depth |
                engine::graphics::ClearDepthStencilFlags::Stencil,
            1.0F,
            0);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "ClearDepthStencilTarget",
                result);

            return;
        }

        DirectX::XMFLOAT4X4 viewProjection{};

        if (!cameraController_.BuildViewProjection(
                viewportWidth_,
                viewportHeight_,
                viewProjection))
        {
            return;
        }

        result = gridRenderer_.Render(
            *commandContext_,
            viewProjection);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "EditorGridRenderer::Render",
                result);

            return;
        }

        result = sceneRenderer_.Render(
            *commandContext_,
            sceneDocument_,
            viewProjection);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "EditorSceneRenderer::Render",
                result);

            return;
        }

        commandContext_->UnbindRenderTargets();

        engine::graphics::PresentStatus presentStatus =
            engine::graphics::PresentStatus::Failed;

        result = swapChain_->Present(presentStatus);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "Present",
                result);

            return;
        }

        switch (presentStatus)
        {
            case engine::graphics::PresentStatus::Presented:
                swapChainOccluded_ = false;
                break;

            case engine::graphics::PresentStatus::Occluded:
                swapChainOccluded_ = true;
                break;

            case engine::graphics::PresentStatus::DeviceLost:
                ReportGraphicsFailure(
                    "Present: device lost",
                    engine::graphics::GraphicsResult::DeviceLost);
                break;

            case engine::graphics::PresentStatus::DeviceRemoved:
                ReportGraphicsFailure(
                    "Present: device removed",
                    engine::graphics::GraphicsResult::DeviceRemoved);
                break;

            case engine::graphics::PresentStatus::Failed:
            default:
                ReportGraphicsFailure(
                    "Present",
                    engine::graphics::GraphicsResult::BackendFailure);
                break;
        }
    }

    void EditorApplication::OnEvent(
    const lts::application::
        ApplicationEvent& event) noexcept
    {
        switch (event.type)
        {
        case lts::application::
            ApplicationEventType::Resize:
            {
                editorShell_.Resize(
                    event.width,
                    event.height);

                const auto viewportSize =
                    editorShell_.
                        GetViewportSize();

                ResizeGraphics(
                    viewportSize.width,
                    viewportSize.height);

                break;
            }

        case lts::application::
            ApplicationEventType::Minimized:
            {
                swapChainOccluded_ = true;
                break;
            }

        case lts::application::
            ApplicationEventType::Restored:
            {
                swapChainOccluded_ = false;

                const auto viewportSize =
                    editorShell_.
                        GetViewportSize();

                ResizeGraphics(
                    viewportSize.width,
                    viewportSize.height);

                break;
            }

        case lts::application::
            ApplicationEventType::Activated:
            {
                swapChainOccluded_ = false;
                break;
            }

        case lts::application::
            ApplicationEventType::DpiChanged:
            {
                const auto clientSize =
                    GetWindow().
                        GetClientSize();

                editorShell_.Resize(
                    clientSize.width,
                    clientSize.height);

                const auto viewportSize =
                    editorShell_.
                        GetViewportSize();

                ResizeGraphics(
                    viewportSize.width,
                    viewportSize.height);

                break;
            }

        case lts::application::
            ApplicationEventType::Deactivated:
        case lts::application::
            ApplicationEventType::CloseRequested:
        default:
            break;
        }
    }

    bool EditorApplication::
        InitializeGraphics() noexcept
    {
        const engine::platform::WindowSize viewportSize =
        editorShell_.
            GetViewportSize();

        viewportWidth_ =
            std::max(
                viewportSize.width,
                1U);

        viewportHeight_ =
            std::max(
                viewportSize.height,
                1U);

        engine::graphics::RenderDeviceDesc
            deviceDescription;

        deviceDescription.backend =
            engine::graphics::
                GraphicsBackend::D3D11;

        deviceDescription.enableValidation =
            GetEngine().
                GetConfig().
                enableValidation;

        deviceDescription.enableDebugMarkers =
            true;

        deviceDescription.forceSoftwareAdapter =
            false;

        auto result =
            graphicsDevice_.Initialize(
                deviceDescription);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "D3D11Device::Initialize",
                result);

            ShutdownGraphics();
            return false;
        }

        commandContext_ =
            graphicsDevice_.
                GetImmediateCommandContext();

        if (commandContext_ == nullptr)
        {
            ReportGraphicsFailure(
                "GetImmediateCommandContext",
                engine::graphics::
                    GraphicsResult::InvalidState);

            ShutdownGraphics();
            return false;
        }

        engine::graphics::SwapChainDesc
            swapChainDescription;

        swapChainDescription.window =
            editorShell_.
                GetViewportWindowHandle();

        swapChainDescription.width =
            viewportWidth_;

        swapChainDescription.height =
            viewportHeight_;

        swapChainDescription.bufferCount = 2;

        swapChainDescription.format =
            engine::graphics::
                Format::B8G8R8A8UNorm;

        swapChainDescription.presentMode =
            engine::graphics::
                PresentMode::VSync;

        swapChainDescription.windowed = true;
        swapChainDescription.allowModeSwitch = false;
        swapChainDescription.enableTearing = false;

        result =
            graphicsDevice_.CreateSwapChain(
                swapChainDescription,
                swapChain_);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "CreateSwapChain",
                result);

            ShutdownGraphics();
            return false;
        }

        result =
            CreateDepthStencil(
                viewportWidth_,
                viewportHeight_);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "CreateDepthStencil",
                result);

            ShutdownGraphics();
            return false;
        }

        if (!gridRenderer_.Initialize(
        graphicsDevice_))
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Critical,
                "LTS.Editor.Grid",
                "Failed to initialize editor world grid.");

            ShutdownGraphics();
            return false;
        }

        graphicsReady_ = true;
        graphicsFailureReported_ = false;
        swapChainOccluded_ = false;

        return true;
    }

    void EditorApplication::
        ShutdownGraphics() noexcept
    {
        graphicsReady_ = false;

        if (commandContext_ != nullptr)
        {
            commandContext_->
                UnbindRenderTargets();

            commandContext_->
                ClearState();

            commandContext_->
                Flush();
        }

        gridRenderer_.Shutdown(graphicsDevice_);

        DestroyDepthStencil();

        swapChain_.reset();
        commandContext_ = nullptr;

        graphicsDevice_.Shutdown();

        viewportWidth_ = 0;
        viewportHeight_ = 0;

        swapChainOccluded_ = false;
    }

    engine::graphics::GraphicsResult
        EditorApplication::CreateDepthStencil(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
    {
        if (width == 0 || height == 0)
        {
            return engine::graphics::
                GraphicsResult::InvalidArgument;
        }

        engine::graphics::TextureDesc
            depthDescription;

        depthDescription.dimension =
            engine::graphics::
                TextureDimension::Texture2D;

        depthDescription.width = width;
        depthDescription.height = height;
        depthDescription.depth = 1;

        depthDescription.arrayLayers = 1;
        depthDescription.mipLevels = 1;
        depthDescription.sampleCount = 1;

        depthDescription.format =
            engine::graphics::
                Format::D24UNormS8UInt;

        depthDescription.usage =
            engine::graphics::
                ResourceUsage::Default;

        depthDescription.bindFlags =
            engine::graphics::
                TextureBindFlags::DepthStencil;

        depthDescription.cpuAccess =
            engine::graphics::
                CpuAccessFlags::None;

        depthDescription.generateMipmaps =
            false;

        return graphicsDevice_.CreateTexture(
            depthDescription,
            nullptr,
            0,
            depthStencil_);
    }

    void EditorApplication::
        DestroyDepthStencil() noexcept
    {
        if (!depthStencil_.IsValid())
        {
            return;
        }

        const auto result =
            graphicsDevice_.DestroyTexture(
                depthStencil_);

        if (engine::graphics::Failed(result))
        {
            char buffer[256]{};

            std::snprintf(
                buffer,
                sizeof(buffer),
                "DestroyDepthStencil failed: %s",
                engine::graphics::ToString(result));

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Warning,
                "LTS.Editor.Graphics",
                buffer);
        }

        depthStencil_ =
            engine::graphics::
                TextureHandle{};
    }

    void EditorApplication::ResizeGraphics(
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (
            !graphicsReady_ ||
            swapChain_ == nullptr ||
            commandContext_ == nullptr ||
            width == 0 ||
            height == 0
        )
        {
            return;
        }

        if (
            width == viewportWidth_ &&
            height == viewportHeight_
        )
        {
            return;
        }

        commandContext_->
            UnbindRenderTargets();

        DestroyDepthStencil();

        auto result =
            swapChain_->Resize(
                width,
                height);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "SwapChain::Resize",
                result);

            return;
        }

        viewportWidth_ = width;
        viewportHeight_ = height;

        result =
            CreateDepthStencil(
                width,
                height);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "Resize CreateDepthStencil",
                result);

            return;
        }

        swapChainOccluded_ = false;
    }

    void EditorApplication::
        ReportGraphicsFailure(
            const char* operation,
            const engine::graphics::
                GraphicsResult result) noexcept
    {
        if (graphicsFailureReported_)
        {
            RequestExit();
            return;
        }

        graphicsFailureReported_ = true;

        char buffer[512]{};

        std::snprintf(
            buffer,
            sizeof(buffer),
            "%s failed: %s",
            operation != nullptr
                ? operation
                : "Unknown graphics operation",
            engine::graphics::ToString(result));

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Critical,
            "LTS.Editor.Graphics",
            buffer);

        RequestExit();
    }
}