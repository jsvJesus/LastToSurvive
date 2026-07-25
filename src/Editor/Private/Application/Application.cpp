#include "Editor/Application/Application.h"
#include "Editor/Tools/TestGame/Launcher.h"

#include <Core/Log.h>
#include <Assets/TerrainAsset.h>

#include <Graphics/Format.h>
#include <Graphics/GraphicsBackend.h>
#include <Graphics/GraphicsResult.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <Runtime/EngineMode.h>
#include <Runtime/RendererBackend.h>

#include <cmath>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <functional>
#include <Windows.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]] std::string ToUtf8(const std::wstring& value)
        {
            if (value.empty()) return {};
            const int size = WideCharToMultiByte(
                CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                nullptr, 0, nullptr, nullptr);
            if (size <= 0) return {};
            std::string output(static_cast<std::size_t>(size), '\0');
            WideCharToMultiByte(
                CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                output.data(), size, nullptr, nullptr);
            return output;
        }

        [[nodiscard]] std::wstring FromUtf8(const char* const value)
        {
            if (value == nullptr || *value == '\0') return {};
            const int length = static_cast<int>(std::strlen(value));
            const int size = MultiByteToWideChar(
                CP_UTF8, 0, value, length, nullptr, 0);
            if (size <= 0) return {};
            std::wstring output(static_cast<std::size_t>(size), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, value, length, output.data(), size);
            return output;
        }

        [[nodiscard]] bool SelectTerrainFile(
            const HWND owner, std::filesystem::path& output)
        {
            Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
            if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return false;
            constexpr COMDLG_FILTERSPEC filter[] =
            {
                {L"LTS Terrain (*.ltsterrain)", L"*.ltsterrain"},
                {L"All files (*.*)", L"*.*"}
            };
            if (FAILED(dialog->SetFileTypes(2U, filter)) ||
                FAILED(dialog->SetDefaultExtension(L"ltsterrain")) ||
                FAILED(dialog->SetTitle(L"Open LTS Terrain")) ||
                FAILED(dialog->Show(owner))) return false;
            Microsoft::WRL::ComPtr<IShellItem> item;
            if (FAILED(dialog->GetResult(&item))) return false;
            PWSTR path = nullptr;
            if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) return false;
            output = path; CoTaskMemFree(path); return true;
        }

        [[nodiscard]] bool SelectTerrainTexture(
            const HWND owner, std::filesystem::path& output)
        {
            Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
            if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) return false;
            constexpr COMDLG_FILTERSPEC filters[] =
            {
                {
                    L"DirectDraw Surface (*.dds)",
                    L"*.dds"
                },
                {
                    L"All files (*.*)",
                    L"*.*"
                }
            };
            if (FAILED(dialog->SetFileTypes(2U, filters)) ||
                FAILED(dialog->SetTitle(L"Select terrain layer texture")) ||
                FAILED(dialog->Show(owner))) return false;
            Microsoft::WRL::ComPtr<IShellItem> item;
            if (FAILED(dialog->GetResult(&item))) return false;
            PWSTR path = nullptr;
            if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) return false;
            output = path; CoTaskMemFree(path); return true;
        }

        [[nodiscard]] std::vector<engine::scene::TerrainComponent::LayerOverride>
            BuildTerrainLayerOverrides(const engine::assets::TerrainAsset& terrain)
        {
            std::vector<engine::scene::TerrainComponent::LayerOverride> result;
            result.reserve(terrain.layers.size());
            for (const auto& source : terrain.layers)
            {
                engine::scene::TerrainComponent::LayerOverride layer;
                layer.name = source.name;
                layer.diffusePath = source.diffusePath;
                layer.normalPath = source.normalPath;
                layer.scaleU = source.scaleU;
                layer.scaleV = source.scaleV;
                result.push_back(std::move(layer));
            }
            return result;
        }

        void FocusCameraOnTerrain(
            CameraController& camera,
            const engine::assets::TerrainAsset& terrain,
            const EditorTransform& transform)
        {
            const float localWidth =
                static_cast<float>(
                    terrain.width - 1U) *
                terrain.tileSize;

            const float localDepth =
                static_cast<float>(
                    terrain.height - 1U) *
                terrain.tileSize;

            const float worldWidth =
                localWidth *
                std::fabs(
                    transform.scale[0]);

            const float worldDepth =
                localDepth *
                std::fabs(
                    transform.scale[2]);

            const float amplitude =
                (std::max)(
                    std::fabs(
                        terrain.heightOffset),
                    std::fabs(
                        terrain.heightOffset +
                        terrain.heightScale));

            camera.FocusOn(
                {
                    transform.position[0] +
                        localWidth *
                        transform.scale[0] *
                        0.5F,

                    transform.position[1] +
                        amplitude *
                        std::fabs(
                            transform.scale[1]) *
                        0.35F,

                    transform.position[2] +
                        localDepth *
                        transform.scale[2] *
                        0.5F
                },
                (std::max)(
                    worldWidth,
                    worldDepth) *
                0.64F);
        }

        [[nodiscard]]
        lts::application::ApplicationDesc CreateEditorDescription()
        {
            lts::application::ApplicationDesc description;

            description.title = L"LastToSurvive Editor";
            description.logFileName = "Logs/LTS.Editor.log";
            description.width = 1600;
            description.height = 900;
            description.resizable = true;
            description.startMaximized = false;
            description.enableDpiAwareness = true;

            description.engineConfig.applicationName = "LTS.Editor";
            description.engineConfig.mode =
                engine::runtime::EngineMode::Studio;

            description.engineConfig.rendererBackend =
                engine::runtime::RendererBackend::D3D11;

            description.engineConfig.enableValidation = true;
            description.engineConfig.enableMainThreadChecks = true;

            return description;
        }
    }

    Application::Application()
    : lts::application::Application(CreateEditorDescription())
    {
    }

    Application::~Application() noexcept = default;

    lts::application::ApplicationResult Application::OnInitialize() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Initializing editor.");

        try
        {
            sceneDocument_.Clear();
        }
        catch (...)
        {
            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        commandHistory_.Clear();

        if (!levelDocument_.Initialize(
                GetWindow().GetNativeHandle(),
                sceneDocument_))
        {
            sceneDocument_.Clear();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!InitializeGraphics())
        {
            levelDocument_.Shutdown();
            sceneDocument_.Clear();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!staticMeshRenderer_.Initialize(
                graphicsDevice_))
        {
            levelDocument_.Shutdown();
            sceneDocument_.Clear();
            ShutdownGraphics();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!sceneRenderer_.Initialize(
                graphicsDevice_))
        {
            staticMeshRenderer_.Shutdown(
                graphicsDevice_);

            levelDocument_.Shutdown();
            sceneDocument_.Clear();
            ShutdownGraphics();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!terrainRenderer_.Initialize(
                graphicsDevice_))
        {
            sceneRenderer_.Shutdown(
                graphicsDevice_);

            staticMeshRenderer_.Shutdown(
                graphicsDevice_);

            levelDocument_.Shutdown();
            sceneDocument_.Clear();
            ShutdownGraphics();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!InitializeEditorUi())
        {
            terrainRenderer_.Shutdown(
                graphicsDevice_);

            sceneRenderer_.Shutdown(
                graphicsDevice_);

            staticMeshRenderer_.Shutdown(
                graphicsDevice_);

            levelDocument_.Shutdown();
            sceneDocument_.Clear();
            ShutdownGraphics();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        /*
         * Старое Win32-меню и обработка команд документа
         * больше не управляют интерфейсом редактора.
         * Все команды приходят из Dear ImGui.
         */
        if (!levelDocument_.
                SetWindowInterceptionEnabled(false))
        {
            ShutdownEditorUi();

            terrainRenderer_.Shutdown(
                graphicsDevice_);

            sceneRenderer_.Shutdown(
                graphicsDevice_);

            staticMeshRenderer_.Shutdown(
                graphicsDevice_);

            levelDocument_.Shutdown();
            sceneDocument_.Clear();
            ShutdownGraphics();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        const HWND mainWindow =
            reinterpret_cast<HWND>(
                GetWindow().
                    GetNativeHandle().
                    Value());

        /*
         * EditorLevelDocument пока ещё может устанавливать
         * старое системное меню. Убираем его, поскольку меню
         * редактора теперь полностью рисуется через ImGui.
         */
        SetMenu(
            mainWindow,
            nullptr);

        DrawMenuBar(
            mainWindow);

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Editor initialization completed.");

        return lts::application::
            ApplicationResult::Success;
    }

    void Application::OnShutdown() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Shutting down editor.");

        ShutdownEditorUi();

        transformController_.
            SetViewportWindow({});

        cameraController_.
            SetViewportWindow({});

        levelDocument_.Shutdown();
        commandHistory_.Clear();

        terrainRenderer_.Shutdown(
            graphicsDevice_);

        sceneRenderer_.Shutdown(
            graphicsDevice_);

        staticMeshRenderer_.Shutdown(
            graphicsDevice_);

        sceneDocument_.Clear();

        ShutdownGraphics();
    }

    void Application::OnUpdate(const double deltaSeconds) noexcept
    {
        if (!imguiHost_.IsInitialized())
        {
            return;
        }

        const EditorLevelUpdateResult result =
            levelDocument_.Update(
                sceneDocument_,
                commandHistory_);

        if (result.sceneReplaced)
        {
            cameraController_.Reset();

            loadedTerrainAssetPath_.clear();

            static_cast<void>(
                terrainRenderer_.LoadTerrain(
                    graphicsDevice_,
                    std::filesystem::path{}));
        }

        /*
         * Загружаем terrain, который указан
         * в открытом документе уровня.
         */
        for (
            const EditorSceneEntity& sceneEntity :
                sceneDocument_.GetEntities())
        {
            if (
                !sceneEntity.terrain.has_value() ||
                !sceneEntity.terrain->visible)
            {
                continue;
            }

            std::filesystem::path gameRoot =
                std::filesystem::current_path();

            if (gameRoot.filename() != L"game")
            {
                gameRoot /= L"game";
            }

            std::filesystem::path terrainPath =
                sceneEntity.terrain->assetPath;

            if (terrainPath.is_relative())
            {
                terrainPath =
                    gameRoot /
                    terrainPath;
            }

            terrainPath =
                terrainPath.lexically_normal();

            if (
                terrainPath !=
                loadedTerrainAssetPath_)
            {
                loadedTerrainAssetPath_ =
                    terrainPath;

                if (terrainRenderer_.LoadTerrain(
                        graphicsDevice_,
                        terrainPath) &&
                    result.sceneReplaced)
                {
                    engine::assets::TerrainAsset
                        terrainAsset;

                    if (engine::assets::Succeeded(
                            engine::assets::
                                TerrainAsset::Load(
                                    terrainPath,
                                    terrainAsset)))
                    {
                        FocusCameraOnTerrain(
                            cameraController_,
                            terrainAsset,
                            sceneEntity.transform);
                    }
                }
            }

            break;
        }

        if (result.closeApproved)
        {
            RequestExit();
            return;
        }

        levelDocument_.SynchronizeWindowTitle(
            sceneDocument_);

        /*
         * Документ продолжает обновляться при открытых
         * Character/Physics/FBX/Icon вкладках, но управление
         * сценой работает только в активном Viewport.
         */
        if (!imguiViewportVisible_)
        {
            terrainBrushHitValid_ = false;

            if (terrainPaintStrokeActive_)
            {
                static_cast<void>(
                    terrainRenderer_.
                        EndPaintStroke());

                terrainPaintStrokeActive_ = false;
            }

            return;
        }

        cameraController_.Update(
            deltaSeconds,
            static_cast<float>(
                GetInputSystem().
                    GetMouseWheelDelta()) /
                static_cast<float>(
                    WHEEL_DELTA));

        const std::int32_t viewportX =
            static_cast<std::int32_t>(
                imguiViewportX_);

        const std::int32_t viewportY =
            static_cast<std::int32_t>(
                imguiViewportY_);

        const engine::platform::WindowSize
            viewportSize
            {
                static_cast<std::uint32_t>(
                    std::max(
                        imguiViewportWidth_,
                        1.0F)),

                static_cast<std::uint32_t>(
                    std::max(
                        imguiViewportHeight_,
                        1.0F))
            };

        transformController_.SetViewportRegion(
            viewportX,
            viewportY,
            viewportSize.width,
            viewportSize.height);

        const engine::platform::MousePosition mouse =
            GetInputSystem().
                GetMousePosition();

        const bool insideViewport =
            mouse.x >= viewportX &&
            mouse.y >= viewportY &&
            mouse.x <
                viewportX +
                    static_cast<std::int32_t>(
                        viewportSize.width) &&
            mouse.y <
                viewportY +
                    static_cast<std::int32_t>(
                        viewportSize.height);

        const bool clicked =
            insideViewport &&
            GetInputSystem().
                WasMouseButtonPressed(
                    engine::platform::
                        MouseButton::Left) &&
            !GetInputSystem().
                IsMouseButtonDown(
                    engine::platform::
                        MouseButton::Right);

        ViewportClick click{};

        if (clicked)
        {
            click.x =
                static_cast<std::uint32_t>(
                    mouse.x -
                    viewportX);

            click.y =
                static_cast<std::uint32_t>(
                    mouse.y -
                    viewportY);
        }

        EditorInteractionResult
            transformResult{};

        if (!terrainPaintMode_)
        {
            transformResult =
                transformController_.Update(
                    sceneDocument_,
                    commandHistory_,
                    cameraController_,
                    viewportSize,
                    clicked
                        ? &click
                        : nullptr,
                    &staticMeshRenderer_);
        }

        /*
         * При перемещении объекта по X/Z
         * удерживаем его на поверхности terrain.
         */
        const auto& transformState =
            transformController_.
                GetVisualState();

        if (
            transformResult.documentChanged &&
            transformState.operation ==
                EditorTransformOperation::Move &&
            (
                transformState.activeAxis ==
                    EditorTransformAxis::X ||
                transformState.activeAxis ==
                    EditorTransformAxis::Z
            ))
        {
            EditorSceneEntity* const entity =
                sceneDocument_.
                    GetSelectedEntityMutable();

            if (
                entity != nullptr &&
                entity->staticMesh.has_value())
            {
                float terrainHeight = 0.0F;

                if (terrainRenderer_.
                        TryGetSurfaceHeight(
                            sceneDocument_,
                            entity->transform.
                                position[0],
                            entity->transform.
                                position[2],
                            terrainHeight))
                {
                    DirectX::XMFLOAT3
                        boundsMinimum{};

                    DirectX::XMFLOAT3
                        boundsMaximum{};

                    float bottomOffset = 0.0F;

                    if (staticMeshRenderer_.
                            TryGetMeshBounds(
                                entity->staticMesh->
                                    assetPath,
                                boundsMinimum,
                                boundsMaximum))
                    {
                        bottomOffset =
                            boundsMinimum.y *
                            entity->transform.
                                scale[1];
                    }

                    entity->transform.position[1] =
                        terrainHeight -
                        bottomOffset;
                }
            }
        }
    }

    void Application::OnRender() noexcept
    {
        if (!imguiHost_.IsInitialized())
        {
            return;
        }

        RenderImGui();
    }

    void Application::OnEvent(const lts::application::ApplicationEvent& event) noexcept
    {
        switch (event.type)
        {
        case lts::application::
            ApplicationEventType::Resize:
            {
                ResizeEditorUi(
                    event.width,
                    event.height);

                break;
            }

        case lts::application::
            ApplicationEventType::DpiChanged:
            {
                const auto clientSize =
                    GetWindow().
                        GetClientSize();

                ResizeEditorUi(
                    clientSize.width,
                    clientSize.height);

                break;
            }

        case lts::application::
            ApplicationEventType::Minimized:

        case lts::application::
            ApplicationEventType::Restored:

        case lts::application::
            ApplicationEventType::Activated:

        case lts::application::
            ApplicationEventType::Deactivated:

        case lts::application::
            ApplicationEventType::CloseRequested:

        default:
            break;
        }
    }

    bool Application::OnNativeMessage(
        void* const nativeWindow,
        const std::uint32_t message,
        const std::uintptr_t wordParameter,
        const std::intptr_t longParameter) noexcept
    {
        return imguiHost_.ProcessNativeMessage(
            nativeWindow, message, wordParameter, longParameter);
    }

    bool Application::InitializeEditorUi() noexcept
    {
        const auto clientSize =
            GetWindow().GetClientSize();

        uiWidth_ =
            std::max(
                clientSize.width,
                1U);

        uiHeight_ =
            std::max(
                clientSize.height,
                1U);

        engine::graphics::SwapChainDesc
            description{};

        description.window =
            GetWindow().
                GetNativeHandle();

        description.width =
            uiWidth_;

        description.height =
            uiHeight_;

        description.bufferCount = 2U;

        description.format =
            engine::graphics::
                Format::B8G8R8A8UNorm;

        description.presentMode =
            engine::graphics::
                PresentMode::VSync;

        const auto swapChainResult =
            graphicsDevice_.CreateSwapChain(
                description,
                uiSwapChain_);

        if (engine::graphics::Failed(
                swapChainResult))
        {
            ReportGraphicsFailure(
                "Create editor swap chain",
                swapChainResult);

            return false;
        }

        DestroyDepthStencil();

        const auto depthResult =
            CreateDepthStencil(
                uiWidth_,
                uiHeight_);

        if (engine::graphics::Failed(
                depthResult))
        {
            ReportGraphicsFailure(
                "Create editor depth stencil",
                depthResult);

            uiSwapChain_.reset();
            uiWidth_ = 0U;
            uiHeight_ = 0U;

            return false;
        }

        std::error_code settingsError;

        std::filesystem::create_directories(
            "Data/Editor/Settings",
            settingsError);

        constexpr const char*
            layoutFile =
                "Data/Editor/Settings/"
                "LevelEditor.layout.ini";

        if (!imguiHost_.Initialize(
                reinterpret_cast<void*>(
                    GetWindow().
                        GetNativeHandle().
                        Value()),
                graphicsDevice_.
                    GetNativeDevice(),
                graphicsDevice_.
                    GetNativeImmediateContext(),
                layoutFile))
        {
            DestroyDepthStencil();

            uiSwapChain_.reset();
            uiWidth_ = 0U;
            uiHeight_ = 0U;

            return false;
        }

        contentBrowserPanel_.Refresh();

        cameraController_.
            SetViewportWindow(
                GetWindow().
                    GetNativeHandle());

        transformController_.
            SetViewportWindow(
                GetWindow().
                    GetNativeHandle());

        return true;
    }

    void Application::
        ShutdownEditorUi() noexcept
    {
        if (commandContext_ != nullptr)
        {
            commandContext_->
                UnbindRenderTargets();
        }

        imguiHost_.Shutdown();

        DestroyDepthStencil();

        uiSwapChain_.reset();

        uiWidth_ = 0U;
        uiHeight_ = 0U;

        imguiViewportVisible_ = false;
        terrainBrushHitValid_ = false;
    }

    void Application::ResizeEditorUi(const std::uint32_t width, const std::uint32_t height) noexcept
    {
        if (
        uiSwapChain_ == nullptr ||
        commandContext_ == nullptr ||
        width == 0U ||
        height == 0U)
        {
            return;
        }

        if (
            width == uiWidth_ &&
            height == uiHeight_)
        {
            return;
        }

        commandContext_->
            UnbindRenderTargets();

        DestroyDepthStencil();

        const auto resizeResult =
            uiSwapChain_->Resize(
                width,
                height);

        if (engine::graphics::Failed(
                resizeResult))
        {
            ReportGraphicsFailure(
                "Resize editor swap chain",
                resizeResult);

            return;
        }

        uiWidth_ = width;
        uiHeight_ = height;

        const auto depthResult =
            CreateDepthStencil(
                uiWidth_,
                uiHeight_);

        if (engine::graphics::Failed(
                depthResult))
        {
            ReportGraphicsFailure(
                "Resize editor depth stencil",
                depthResult);
        }
    }

    void Application::LaunchTestGame() noexcept
    {
        const HWND owner =
            reinterpret_cast<HWND>(
                GetWindow().
                    GetNativeHandle().
                    Value());

        if (sceneDocument_.IsDirty())
        {
            MessageBoxW(
                owner,
                L"Save the current level before "
                L"starting Test Game.",
                L"Test Game",
                MB_OK |
                    MB_ICONWARNING);

            return;
        }

        const std::filesystem::path&
            currentLevelPath =
                levelDocument_.GetCurrentPath();

        if (currentLevelPath.empty())
        {
            MessageBoxW(
                owner,
                L"The current level has not been saved.\n\n"
                L"Use File -> Save As before starting "
                L"Test Game.",
                L"Test Game",
                MB_OK |
                    MB_ICONWARNING);

            return;
        }

        std::wstring error;

        if (!Launcher::Launch(
                currentLevelPath,
                error))
        {
            MessageBoxW(
                owner,
                error.c_str(),
                L"Test Game",
                MB_OK |
                    MB_ICONERROR);

            return;
        }

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.TestGame",
            "LTS.Game.exe started in editor test mode.");
    }

    void Application::DrawImGuiWorkspace() noexcept
    {
        /*
         * Каждый новый ImGui frame считаем Viewport скрытым.
         * Ниже он установит true только при реально активной вкладке.
         */
        imguiViewportVisible_ = false;

        static_cast<void>(levelEditorLayout_.DrawDockSpace());

        ProcessEditorShortcuts();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N")) static_cast<void>(ExecuteEditorCommand(EditorCommand::NewLevel));
                if (ImGui::MenuItem("Open", "Ctrl+O")) static_cast<void>(ExecuteEditorCommand(EditorCommand::OpenLevel));
                if (ImGui::MenuItem("Save", "Ctrl+S")) static_cast<void>(ExecuteEditorCommand(EditorCommand::SaveLevel));
                if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) static_cast<void>(ExecuteEditorCommand(EditorCommand::SaveLevelAs));

                ImGui::Separator();

                if (ImGui::MenuItem("Import Terrain (.r16)..."))
                {
                    terrainImporter_.Open();
                }
                
                if (ImGui::MenuItem(
                        "Open Terrain..."))
                {
                    const HWND owner =
                        reinterpret_cast<HWND>(
                            GetWindow().
                                GetNativeHandle().
                                Value());

                    std::filesystem::path terrainPath;

                    if (SelectTerrainFile(
                            owner,
                            terrainPath))
                    {
                        engine::assets::TerrainAsset
                            terrainAsset;

                        const auto assetResult =
                            engine::assets::
                                TerrainAsset::Load(
                                    terrainPath,
                                    terrainAsset);

                        bool terrainReady = false;
                        std::wstring message;

                        if (engine::assets::Failed(
                                assetResult))
                        {
                            message =
                                L"Terrain validation failed: " +
                                std::wstring(
                                    FromUtf8(
                                        engine::assets::
                                            ToString(
                                                assetResult)));
                        }
                        else if (
                            !terrainRenderer_.
                                LoadTerrain(
                                    graphicsDevice_,
                                    terrainPath))
                        {
                            message =
                                L"Terrain asset is valid, "
                                L"but GPU resources "
                                L"could not be created.";
                        }
                        else
                        {
                            loadedTerrainAssetPath_ =
                                terrainPath.
                                    lexically_normal();

                            std::filesystem::path gameRoot =
                                std::filesystem::
                                    current_path();

                            if (gameRoot.filename() !=
                                L"game")
                            {
                                gameRoot /= L"game";
                            }

                            std::error_code
                                relativeError;

                            const auto relativePath =
                                std::filesystem::
                                    relative(
                                        terrainPath,
                                        gameRoot,
                                        relativeError);

                            if (relativeError)
                            {
                                message =
                                    L"Terrain was loaded, "
                                    L"but its game-relative "
                                    L"path could not be built.";
                            }
                            else
                            {
                                const EditorSceneSnapshot
                                    before =
                                        sceneDocument_.
                                            CreateSnapshot();

                                EditorTransform transform{};

                                if (sceneDocument_.
                                        CreateTerrainEntity(
                                            terrainPath.
                                                stem().
                                                wstring(),
                                            relativePath.
                                                generic_wstring(),
                                            transform))
                                {
                                    static_cast<void>(
                                        sceneDocument_.
                                            SetSelectedTerrainLayers(
                                                BuildTerrainLayerOverrides(
                                                    terrainAsset)));

                                    static_cast<void>(
                                        commandHistory_.Push(
                                            before,
                                            sceneDocument_.
                                                CreateSnapshot()));

                                    FocusCameraOnTerrain(
                                        cameraController_,
                                        terrainAsset,
                                        transform);

                                    terrainReady = true;

                                    message =
                                        L"Terrain opened "
                                        L"successfully:\n" +
                                        terrainPath.wstring();
                                }
                                else
                                {
                                    message =
                                        L"Terrain was loaded, "
                                        L"but the Terrain actor "
                                        L"could not be created.";
                                }
                            }
                        }

                        MessageBoxW(
                            owner,
                            message.c_str(),
                            L"Open Terrain",
                            MB_OK |
                                (
                                    terrainReady
                                        ? MB_ICONINFORMATION
                                        : MB_ICONERROR
                                ));
                    }
                }
                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    levelDocument_.
                        RequestCloseLevel();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false,
                        commandHistory_.CanUndo() || terrainRenderer_.CanUndoPaint()))
                    static_cast<void>(ExecuteEditorCommand(EditorCommand::Undo));
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false,
                        commandHistory_.CanRedo() || terrainRenderer_.CanRedoPaint()))
                    static_cast<void>(ExecuteEditorCommand(EditorCommand::Redo));
                ImGui::Separator();
                if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, sceneDocument_.GetSelectedEntity() != nullptr)) static_cast<void>(ExecuteEditorCommand(EditorCommand::DuplicateSelection));
                if (ImGui::MenuItem("Delete", "Delete", false, sceneDocument_.GetSelectedEntity() != nullptr)) static_cast<void>(ExecuteEditorCommand(EditorCommand::DeleteSelection));
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Reset Layout"))
                    levelEditorLayout_.RequestReset();
                ImGui::EndMenu();
            }

            const EditorToolAction toolAction = toolWindowManager_.DrawToolsMenu();
            if (
                toolAction ==
                EditorToolAction::TestGame)
            {
                LaunchTestGame();
            }

            ImGui::TextDisabled("Level Editor");
            ImGui::EndMainMenuBar();
        }
        
        //{
            ImGui::Begin(
                "Toolbar",
                nullptr,
                ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse);
            if (ImGui::Button("New")) static_cast<void>(ExecuteEditorCommand(EditorCommand::NewLevel));
            ImGui::SameLine();
            if (ImGui::Button("Open")) static_cast<void>(ExecuteEditorCommand(EditorCommand::OpenLevel));
            ImGui::SameLine();
            if (ImGui::Button("Save")) static_cast<void>(ExecuteEditorCommand(EditorCommand::SaveLevel));
            ImGui::SameLine(); ImGui::Separator(); ImGui::SameLine();
            if (ImGui::Button("Undo")) static_cast<void>(ExecuteEditorCommand(EditorCommand::Undo));
            ImGui::SameLine();
            if (ImGui::Button("Redo")) static_cast<void>(ExecuteEditorCommand(EditorCommand::Redo));
            ImGui::SameLine(); ImGui::Separator(); ImGui::SameLine();
            const auto drawToolButton = [this](
                const char* const label,
                const EditorTransformOperation operation,
                const char* const description)
            {
                const bool active = transformController_.GetVisualState().operation == operation;
                if (active)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10F, 0.38F, 0.47F, 1.0F));
                }
                const bool clicked = ImGui::Button(label);
                if (active) ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", description);
                if (clicked)
                {
                    EditorCommand command = EditorCommand::SelectTool;
                    if (operation == EditorTransformOperation::Move)
                        command = EditorCommand::MoveTool;
                    else if (operation == EditorTransformOperation::Rotate)
                        command = EditorCommand::RotateTool;
                    else if (operation == EditorTransformOperation::Scale)
                        command = EditorCommand::ScaleTool;
                    static_cast<void>(ExecuteEditorCommand(command));
                }
            };
            drawToolButton(
                "Q Select", EditorTransformOperation::Select,
                "Selection mode: click objects without showing a transform gizmo");
            ImGui::SameLine();
            drawToolButton("W Move", EditorTransformOperation::Move, "Move selected object");
            ImGui::SameLine();
            drawToolButton("E Rotate", EditorTransformOperation::Rotate, "Rotate selected object");
            ImGui::SameLine();
            drawToolButton("R Scale", EditorTransformOperation::Scale, "Scale selected object");
            ImGui::SameLine();
            if (terrainPaintMode_)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42F, 0.24F, 0.08F, 1.0F));
            if (ImGui::Button("Terrain Paint"))
                terrainPaintMode_ = !terrainPaintMode_;
            if (terrainPaintMode_) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Paint terrain material weights. Hold Shift to erase.");
            ImGui::SameLine(); ImGui::Separator(); ImGui::SameLine();
            if (ImGui::Button(
                    transformController_.GetVisualState().space == EditorTransformSpace::World
                        ? "World" : "Local"))
            {
                transformController_.ToggleSpace();
            }
            ImGui::SameLine();
            bool snappingEnabled = transformController_.IsSnappingEnabled();
            if (ImGui::Checkbox("Snap", &snappingEnabled))
                transformController_.SetSnappingEnabled(snappingEnabled);
            ImGui::SameLine();
            std::array<float, 3U> snapSteps = transformController_.GetSnapSteps();
            ImGui::SetNextItemWidth(165.0F);
            if (ImGui::DragFloat3(
                    "##SnapSteps", snapSteps.data(), 0.1F, 0.001F, 1000.0F,
                    "%.2f"))
                transformController_.SetSnapSteps(
                    snapSteps[0], snapSteps[1], snapSteps[2]);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Move / Rotate / Scale snap steps");
            ImGui::SameLine();
            if (ImGui::Button("Play")) LaunchTestGame();
            ImGui::End();

            ImGui::Begin("Place Actors");
            if (ImGui::Button("Empty Actor", ImVec2(-1.0F, 0.0F)))
            {
                const EditorTransform transform{};
                static_cast<void>(sceneDocument_.CreateEntity(
                    L"Empty Actor", EditorEntityKind::Empty, transform));
            }
            if (ImGui::Button("Directional Light", ImVec2(-1.0F, 0.0F)))
            {
                const EditorTransform transform{};
                static_cast<void>(sceneDocument_.CreateEntity(
                    L"Directional Light", EditorEntityKind::DirectionalLight, transform));
            }
            if (ImGui::Button("Spawn Point", ImVec2(-1.0F, 0.0F)))
            {
                const EditorTransform transform{};
                static_cast<void>(sceneDocument_.CreateEntity(
                    L"Spawn Point", EditorEntityKind::SpawnPoint, transform));
            }
            ImGui::End();

            ImGui::Begin("World Outliner");
            worldOutlinerPanel_.Draw(sceneDocument_, commandHistory_);
            ImGui::End();

            ImGui::Begin("Inspector");
            if (const EditorSceneEntity* entity = sceneDocument_.GetSelectedEntity())
            {
                if (renameEntityId_ != entity->id)
                {
                    renameEntityId_ = entity->id;
                    entityRenameBuffer_.fill('\0');
                    const std::string name = ToUtf8(entity->name);
                    const std::size_t count = std::min(
                        name.size(), entityRenameBuffer_.size() - 1U);
                    std::memcpy(entityRenameBuffer_.data(), name.data(), count);
                }
                ImGui::TextDisabled("Name");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1.0F);
                if (ImGui::InputText(
                        "##EntityName",
                        entityRenameBuffer_.data(),
                        entityRenameBuffer_.size(),
                        ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                    if (sceneDocument_.RenameSelectedEntity(
                            FromUtf8(entityRenameBuffer_.data())))
                    {
                        static_cast<void>(commandHistory_.Push(
                            before, sceneDocument_.CreateSnapshot()));
                    }
                    renameEntityId_ = 0U;
                }
                ImGui::Separator();
                const std::size_t selectionCount =
                    sceneDocument_.GetSelectedEntityIds().size();
                if (selectionCount > 1U)
                    ImGui::TextDisabled("%llu objects selected",
                        static_cast<unsigned long long>(selectionCount));

                EditorTransform transform = entity->transform;
                const auto hasMixed = [this](
                    const std::array<float, 3U>& values,
                    const int component)
                {
                    for (const EditorEntityId id : sceneDocument_.GetSelectedEntityIds())
                    {
                        const EditorSceneEntity* candidate =
                            sceneDocument_.GetSceneWorld().FindEntity(id);
                        if (candidate == nullptr) continue;
                        const std::array<float, 3U>* candidateValues =
                            component == 0 ? &candidate->transform.position :
                            (component == 1 ? &candidate->transform.rotationDegrees :
                                              &candidate->transform.scale);
                        if (*candidateValues != values) return true;
                    }
                    return false;
                };
                const auto editTransform = [this, &entity, &transform, &hasMixed](
                    const char* label, std::array<float, 3U>& values,
                    const float speed, const int component)
                {
                    const std::array<float, 3U> beforeValues = values;
                    if (hasMixed(beforeValues, component))
                        ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);
                    const bool changed = ImGui::DragFloat3(
                        label, values.data(), speed,
                        component == 2 ? 0.001F : 0.0F,
                        component == 2 ? 1000.0F : 0.0F);
                    if (hasMixed(beforeValues, component)) ImGui::PopItemFlag();
                    if (ImGui::IsItemActivated() && !inspectorEditActive_)
                    {
                        inspectorEditBefore_ = sceneDocument_.CreateSnapshot();
                        inspectorEditActive_ = true;
                    }
                    if (changed)
                    {
                        if (transformController_.IsSnappingEnabled())
                        {
                            const std::array<float, 3U> steps =
                                transformController_.GetSnapSteps();
                            const float step = steps[static_cast<std::size_t>(component)];
                            for (float& value : values)
                                value = std::round(value / step) * step;
                        }
                        std::array<float, 3U> positionDelta{};
                        std::array<float, 3U> rotationDelta{};
                        std::array<float, 3U> scaleRatio{1.0F, 1.0F, 1.0F};
                        for (std::size_t axis = 0U; axis < 3U; ++axis)
                        {
                            if (component == 0)
                                positionDelta[axis] = values[axis] - beforeValues[axis];
                            else if (component == 1)
                                rotationDelta[axis] = values[axis] - beforeValues[axis];
                            else
                                scaleRatio[axis] = beforeValues[axis] != 0.0F
                                    ? values[axis] / beforeValues[axis] : 1.0F;
                        }
                        static_cast<void>(sceneDocument_.ApplySelectionTransformDelta(
                            positionDelta, rotationDelta, scaleRatio));
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit() && inspectorEditActive_)
                    {
                        static_cast<void>(commandHistory_.Push(
                            inspectorEditBefore_, sceneDocument_.CreateSnapshot()));
                        inspectorEditBefore_ = {};
                        inspectorEditActive_ = false;
                    }
                };
                editTransform("Location", transform.position, 0.1F, 0);
                editTransform("Rotation", transform.rotationDegrees, 0.5F, 1);
                editTransform("Scale", transform.scale, 0.01F, 2);

                if (ImGui::Button("Reset Transform"))
                {
                    const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                    if (sceneDocument_.SetSelectionTransform(EditorTransform{}))
                        static_cast<void>(commandHistory_.Push(
                            before, sceneDocument_.CreateSnapshot()));
                }
                ImGui::SameLine();
                if (ImGui::Button("Copy"))
                {
                    transformClipboard_ = entity->transform;
                    transformClipboardValid_ = true;
                }
                ImGui::SameLine();
                if (!transformClipboardValid_) ImGui::BeginDisabled();
                if (ImGui::Button("Paste"))
                {
                    const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                    if (sceneDocument_.SetSelectionTransform(transformClipboard_))
                        static_cast<void>(commandHistory_.Push(
                            before, sceneDocument_.CreateSnapshot()));
                }
                if (!transformClipboardValid_) ImGui::EndDisabled();

                if (entity->terrain.has_value())
                {
                    const auto& layers = entity->terrain->layers;

                    if (!layers.empty() && terrainPaintLayer_ >= layers.size())
                    {
                        terrainPaintLayer_ = layers.size() - 1U;
                    }

                    const auto selectTexturePath = [this](std::string& outputPath)
                    {
                        std::filesystem::path selectedPath;

                        const HWND owner = reinterpret_cast<HWND>(
                            GetWindow().GetNativeHandle().Value());

                        if (!SelectTerrainTexture(owner, selectedPath))
                        {
                            return false;
                        }

                        std::filesystem::path gameRoot = std::filesystem::current_path();

                        if (gameRoot.filename() != L"game")
                        {
                            gameRoot /= L"game";
                        }

                        std::error_code error;
                        const std::filesystem::path relativePath =
                            std::filesystem::relative(selectedPath, gameRoot, error);

                        outputPath = error
                            ? selectedPath.generic_u8string()
                            : relativePath.generic_u8string();

                        return true;
                    };

                    /*
                     * TERRAIN MATERIALS
                     */
                    ImGui::SeparatorText("Terrain Materials");

                    ImGui::TextWrapped(
                        "Asset: %s",
                        ToUtf8(entity->terrain->assetPath).c_str());

                    ImGui::TextDisabled(
                        "%llu / 18 layers",
                        static_cast<unsigned long long>(layers.size()));

                    const bool canAddLayer = layers.size() < 18U;

                    if (!canAddLayer)
                    {
                        ImGui::BeginDisabled();
                    }

                    if (ImGui::Button("Add Layer"))
                    {
                        const std::size_t oldLayerCount = layers.size();
                        const std::size_t newLayerCount = oldLayerCount + 1U;

                        if (terrainRenderer_.SetMaterialLayerCount(newLayerCount))
                        {
                            const EditorSceneSnapshot before =
                                sceneDocument_.CreateSnapshot();

                            const std::string layerName = oldLayerCount == 0U
                                ? "Base"
                                : "Layer " + std::to_string(oldLayerCount);

                            if (sceneDocument_.AddSelectedTerrainLayer(layerName))
                            {
                                static_cast<void>(commandHistory_.Push(
                                    before,
                                    sceneDocument_.CreateSnapshot()));

                                terrainPaintLayer_ = oldLayerCount;
                            }
                            else
                            {
                                static_cast<void>(
                                    terrainRenderer_.SetMaterialLayerCount(oldLayerCount));
                            }
                        }
                    }

                    if (!canAddLayer)
                    {
                        ImGui::EndDisabled();
                    }

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                        !canAddLayer)
                    {
                        ImGui::SetTooltip("Maximum terrain layer count is 18.");
                    }

                    ImGui::SameLine();

                    const bool canRemoveLayer =
                        terrainPaintLayer_ > 0U &&
                        terrainPaintLayer_ < layers.size();

                    if (!canRemoveLayer)
                    {
                        ImGui::BeginDisabled();
                    }

                    if (ImGui::Button("Remove Selected Layer"))
                    {
                        const std::size_t oldLayerCount = layers.size();
                        const std::size_t removeIndex = terrainPaintLayer_;

                        const EditorSceneSnapshot before =
                            sceneDocument_.CreateSnapshot();

                        if (terrainRenderer_.RemoveMaterialLayer(
                                removeIndex,
                                oldLayerCount) &&
                            sceneDocument_.RemoveSelectedTerrainLayer(removeIndex))
                        {
                            static_cast<void>(commandHistory_.Push(
                                before,
                                sceneDocument_.CreateSnapshot()));

                            terrainPaintLayer_ = (std::min)(
                                removeIndex,
                                layers.size() - 1U);
                        }
                    }

                    if (!canRemoveLayer)
                    {
                        ImGui::EndDisabled();
                    }

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) &&
                        !canRemoveLayer)
                    {
                        ImGui::SetTooltip(
                            terrainPaintLayer_ == 0U
                                ? "Base Layer cannot be removed."
                                : "Select a material layer first.");
                    }

                    ImGui::TextDisabled(
                        "Layer 0 is Base. Each RGB splat-mask stores 3 painted layers.");

                    ImGui::Separator();

                    for (std::size_t layerIndex = 0U;
                         layerIndex < layers.size();
                         ++layerIndex)
                    {
                        const auto& layer = layers[layerIndex];

                        ImGui::PushID(static_cast<int>(layerIndex));

                        const std::string header =
                            std::to_string(layerIndex) + ". " + layer.name;

                        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;

                        if (layerIndex == terrainPaintLayer_)
                        {
                            flags |= ImGuiTreeNodeFlags_Selected;
                        }

                        if (ImGui::TreeNodeEx(header.c_str(), flags))
                        {
                            bool visible = layer.visible;
                            float scale[2]{layer.scaleU, layer.scaleV};
                            float offset[2]{layer.offsetU, layer.offsetV};

                            std::string diffuse = layer.diffusePath;
                            std::string normal = layer.normalPath;

                            if (ImGui::Button(
                                    layerIndex == terrainPaintLayer_
                                        ? "Active Paint Layer"
                                        : "Use for Paint"))
                            {
                                terrainPaintLayer_ = layerIndex;
                            }

                            ImGui::SameLine();

                            if (ImGui::Checkbox("Visible", &visible))
                            {
                                const EditorSceneSnapshot before =
                                    sceneDocument_.CreateSnapshot();

                                if (sceneDocument_.UpdateSelectedTerrainLayer(
                                        layerIndex,
                                        diffuse,
                                        normal,
                                        scale[0],
                                        scale[1],
                                        offset[0],
                                        offset[1],
                                        visible))
                                {
                                    static_cast<void>(commandHistory_.Push(
                                        before,
                                        sceneDocument_.CreateSnapshot()));
                                }
                            }

                            const auto editPlacement =
                                [this, layerIndex, &diffuse, &normal, &scale, &offset, visible](
                                    const char* label,
                                    float* values,
                                    const float speed,
                                    const float minimum,
                                    const float maximum)
                            {
                                const bool changed = ImGui::DragFloat2(
                                    label,
                                    values,
                                    speed,
                                    minimum,
                                    maximum,
                                    "%.3f");

                                if (ImGui::IsItemActivated() && !terrainLayerEditActive_)
                                {
                                    terrainLayerEditBefore_ =
                                        sceneDocument_.CreateSnapshot();

                                    terrainLayerEditIndex_ = layerIndex;
                                    terrainLayerEditActive_ = true;
                                }

                                if (changed)
                                {
                                    static_cast<void>(
                                        sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex,
                                            diffuse,
                                            normal,
                                            scale[0],
                                            scale[1],
                                            offset[0],
                                            offset[1],
                                            visible));
                                }

                                if (ImGui::IsItemDeactivatedAfterEdit() &&
                                    terrainLayerEditActive_ &&
                                    terrainLayerEditIndex_ == layerIndex)
                                {
                                    static_cast<void>(commandHistory_.Push(
                                        terrainLayerEditBefore_,
                                        sceneDocument_.CreateSnapshot()));

                                    terrainLayerEditBefore_ = {};
                                    terrainLayerEditIndex_ = InvalidEditorEntityIndex;
                                    terrainLayerEditActive_ = false;
                                }
                            };

                            editPlacement(
                                "Scale U / V",
                                scale,
                                0.25F,
                                0.001F,
                                100000.0F);

                            editPlacement(
                                "Offset U / V",
                                offset,
                                0.01F,
                                -100000.0F,
                                100000.0F);

                            ImGui::SeparatorText("Diffuse");

                            ImGui::TextWrapped(
                                "%s",
                                diffuse.empty()
                                    ? "<fallback diffuse>"
                                    : diffuse.c_str());

                            if (ImGui::Button("Browse Diffuse..."))
                            {
                                std::string selectedPath;

                                if (selectTexturePath(selectedPath))
                                {
                                    const EditorSceneSnapshot before =
                                        sceneDocument_.CreateSnapshot();

                                    diffuse = std::move(selectedPath);

                                    if (sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex,
                                            diffuse,
                                            normal,
                                            scale[0],
                                            scale[1],
                                            offset[0],
                                            offset[1],
                                            visible))
                                    {
                                        static_cast<void>(commandHistory_.Push(
                                            before,
                                            sceneDocument_.CreateSnapshot()));
                                    }
                                }
                            }

                            if (!diffuse.empty())
                            {
                                ImGui::SameLine();

                                if (ImGui::Button("Clear Diffuse"))
                                {
                                    const EditorSceneSnapshot before =
                                        sceneDocument_.CreateSnapshot();

                                    diffuse.clear();

                                    if (sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex,
                                            diffuse,
                                            normal,
                                            scale[0],
                                            scale[1],
                                            offset[0],
                                            offset[1],
                                            visible))
                                    {
                                        static_cast<void>(commandHistory_.Push(
                                            before,
                                            sceneDocument_.CreateSnapshot()));
                                    }
                                }
                            }

                            ImGui::SeparatorText("Normal");

                            ImGui::TextWrapped(
                                "%s",
                                normal.empty()
                                    ? "<flat normal>"
                                    : normal.c_str());

                            if (ImGui::Button("Browse Normal..."))
                            {
                                std::string selectedPath;

                                if (selectTexturePath(selectedPath))
                                {
                                    const EditorSceneSnapshot before =
                                        sceneDocument_.CreateSnapshot();

                                    normal = std::move(selectedPath);

                                    if (sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex,
                                            diffuse,
                                            normal,
                                            scale[0],
                                            scale[1],
                                            offset[0],
                                            offset[1],
                                            visible))
                                    {
                                        static_cast<void>(commandHistory_.Push(
                                            before,
                                            sceneDocument_.CreateSnapshot()));
                                    }
                                }
                            }

                            if (!normal.empty())
                            {
                                ImGui::SameLine();

                                if (ImGui::Button("Clear Normal"))
                                {
                                    const EditorSceneSnapshot before =
                                        sceneDocument_.CreateSnapshot();

                                    normal.clear();

                                    if (sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex,
                                            diffuse,
                                            normal,
                                            scale[0],
                                            scale[1],
                                            offset[0],
                                            offset[1],
                                            visible))
                                    {
                                        static_cast<void>(commandHistory_.Push(
                                            before,
                                            sceneDocument_.CreateSnapshot()));
                                    }
                                }
                            }

                            if (layerIndex == 0U)
                            {
                                ImGui::TextDisabled(
                                    "Base Layer fills every unpainted terrain area.");
                            }
                            else
                            {
                                const std::size_t paintedIndex = layerIndex - 1U;
                                const std::size_t maskIndex = paintedIndex / 3U;
                                const std::size_t channelIndex = paintedIndex % 3U;

                                const char* channelName =
                                    channelIndex == 0U ? "R" :
                                    channelIndex == 1U ? "G" : "B";

                                ImGui::TextDisabled(
                                    "Splat-mask %llu, channel %s",
                                    static_cast<unsigned long long>(maskIndex),
                                    channelName);
                            }

                            ImGui::TreePop();
                        }

                        ImGui::PopID();
                    }

                    /*
                     * TERRAIN PAINT
                     */
                    ImGui::SeparatorText("Terrain Paint");

                    const bool canPaint = layers.size() > 1U;

                    if (!canPaint)
                    {
                        terrainPaintMode_ = false;
                        ImGui::BeginDisabled();
                    }

                    ImGui::Checkbox("Enable Paint Mode", &terrainPaintMode_);

                    if (!canPaint)
                    {
                        ImGui::EndDisabled();
                        ImGui::TextDisabled("Add at least one layer above Base.");
                    }

                    ImGui::SliderFloat(
                        "Brush Size",
                        &terrainBrushRadius_,
                        1.0F,
                        1024.0F,
                        "%.1f");

                    ImGui::SliderFloat(
                        "Strength",
                        &terrainBrushStrength_,
                        0.01F,
                        1.0F,
                        "%.2f");

                    ImGui::SliderFloat(
                        "Falloff",
                        &terrainBrushFalloff_,
                        0.0F,
                        1.0F,
                        "%.2f");

                    const char* paintPreview = layers.empty()
                        ? "No Layers"
                        : layers[terrainPaintLayer_].name.c_str();

                    if (layers.empty())
                    {
                        ImGui::BeginDisabled();
                    }

                    if (ImGui::BeginCombo("Paint Layer", paintPreview))
                    {
                        for (std::size_t index = 0U;
                             index < layers.size();
                             ++index)
                        {
                            const bool selected = terrainPaintLayer_ == index;

                            if (ImGui::Selectable(
                                    layers[index].name.c_str(),
                                    selected))
                            {
                                terrainPaintLayer_ = index;
                            }

                            if (selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }

                        ImGui::EndCombo();
                    }

                    if (layers.empty())
                    {
                        ImGui::EndDisabled();
                    }

                    if (!terrainRenderer_.CanUndoPaint())
                    {
                        ImGui::BeginDisabled();
                    }

                    if (ImGui::Button("Undo Paint"))
                    {
                        static_cast<void>(terrainRenderer_.UndoPaint());
                    }

                    if (!terrainRenderer_.CanUndoPaint())
                    {
                        ImGui::EndDisabled();
                    }

                    ImGui::SameLine();

                    if (!terrainRenderer_.CanRedoPaint())
                    {
                        ImGui::BeginDisabled();
                    }

                    if (ImGui::Button("Redo Paint"))
                    {
                        static_cast<void>(terrainRenderer_.RedoPaint());
                    }

                    if (!terrainRenderer_.CanRedoPaint())
                    {
                        ImGui::EndDisabled();
                    }

                    ImGui::TextDisabled(
                        "LMB paints selected layer. Shift + LMB erases it.");
                }
            }
            else
            {
                ImGui::TextDisabled("No selection");
            }
            ImGui::End();

            EditorContentBrowserContext
            contentBrowserContext
            {
                sceneDocument_,
                commandHistory_,
                cameraController_,
                staticMeshRenderer_,
                terrainRenderer_,
                imguiViewportWidth_,
                imguiViewportHeight_
            };

            contentBrowserPanel_.
                Draw(
                    contentBrowserContext);

            ImGui::SetNextWindowBgAlpha(0.0F);

            const bool drawLevelViewport =
                ImGui::Begin(
                    "Viewport",
                    nullptr,
                    ImGuiWindowFlags_NoBackground |
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse);

            if (drawLevelViewport)
            {
                /*
                 * Begin() возвращает false для скрытой dock-вкладки.
                 * Поэтому Level Scene разрешаем только здесь.
                 */
                imguiViewportVisible_ = true;
            
                if(const ImGuiWindow* toolbarWindow=ImGui::FindWindowByName("Toolbar"))
                {
                    const ImGuiWindow* viewportWindow=ImGui::FindWindowByName("Viewport");
                    if(viewportWindow!=nullptr&&toolbarWindow->DockId!=0U&&
                        toolbarWindow->DockId==viewportWindow->DockId)
                        levelEditorLayout_.RequestReset();
                }
                
                const ImVec2 position = ImGui::GetCursorScreenPos();
                const ImVec2 available = ImGui::GetContentRegionAvail();
                imguiViewportX_ = position.x;
                imguiViewportY_ = position.y;
                imguiViewportWidth_ = std::max(available.x, 1.0F);
                imguiViewportHeight_ = std::max(available.y, 1.0F);
                ImGui::InvisibleButton("SceneViewport", available);
                const bool paintHovered=terrainPaintMode_&&ImGui::IsItemHovered();
                terrainBrushHitValid_=false;
                if(paintHovered)
                {
                    const ImVec2 mouse=ImGui::GetMousePos();
                    const float localX=mouse.x-imguiViewportX_;
                    const float localY=mouse.y-imguiViewportY_;
                    EditorPickRay ray{};
                    if(localX>=0.0F&&localY>=0.0F&&
                        cameraController_.BuildPickRay(
                            static_cast<std::uint32_t>(localX),
                            static_cast<std::uint32_t>(localY),
                            static_cast<std::uint32_t>(imguiViewportWidth_),
                            static_cast<std::uint32_t>(imguiViewportHeight_),ray)&&
                        std::abs(ray.direction.y)>0.00001F)
                    {
                        float distance=-ray.origin.y/ray.direction.y;
                        if(distance>=0.0F)
                        {
                            float terrainHeight=0.0F;
                            bool hit=false;
                            for(std::uint32_t iteration=0;iteration<10U;++iteration)
                            {
                                const float x=ray.origin.x+ray.direction.x*distance;
                                const float z=ray.origin.z+ray.direction.z*distance;
                                if(!terrainRenderer_.TryGetSurfaceHeight(
                                        sceneDocument_,x,z,terrainHeight))break;
                                hit=true;
                                const float refined=(terrainHeight-ray.origin.y)/ray.direction.y;
                                if(refined<0.0F)break;
                                distance=refined;
                            }
                            if(hit)
                            {
                                terrainBrushWorldX_=ray.origin.x+ray.direction.x*distance;
                                terrainBrushWorldZ_=ray.origin.z+ray.direction.z*distance;
                                terrainBrushHitValid_=true;
                            }
                        }
                    }
                    if(terrainBrushHitValid_&&ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        terrainPaintStrokeActive_=terrainRenderer_.BeginPaintStroke();
                }
                if(terrainPaintStrokeActive_&&terrainBrushHitValid_&&
                    ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    static_cast<void>(terrainRenderer_.Paint(
                        sceneDocument_,terrainBrushWorldX_,terrainBrushWorldZ_,terrainBrushRadius_,
                        terrainBrushStrength_,terrainBrushFalloff_,terrainPaintLayer_,
                        ImGui::GetIO().KeyShift));
                }
                if(terrainPaintStrokeActive_&&ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    static_cast<void>(terrainRenderer_.EndPaintStroke());
                    terrainPaintStrokeActive_=false;
                }
                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* const payload =
                            ImGui::AcceptDragDropPayload("LTS_MESH_ASSET"))
                    {
                        if (payload->IsDelivery() && payload->Data != nullptr)
                        {
                            const char* const assetPath =
                                static_cast<const char*>(payload->Data);
                            const ImVec2 mouse = ImGui::GetMousePos();
                            EditorPickRay ray;
                            const float localX = mouse.x - imguiViewportX_;
                            const float localY = mouse.y - imguiViewportY_;
                            if (localX >= 0.0F && localY >= 0.0F &&
                                cameraController_.BuildPickRay(
                                    static_cast<std::uint32_t>(localX),
                                    static_cast<std::uint32_t>(localY),
                                    static_cast<std::uint32_t>(imguiViewportWidth_),
                                    static_cast<std::uint32_t>(imguiViewportHeight_),
                                    ray))
                            {
                                const std::filesystem::path path = FromUtf8(assetPath);
                                constexpr float fallbackDistance = 10.0F;
                                float distance = fallbackDistance;
                                if (std::abs(ray.direction.y) > 0.00001F)
                                {
                                    const float groundDistance =
                                        -ray.origin.y / ray.direction.y;
                                    if (groundDistance >= 0.0F)
                                        distance = groundDistance;
                                }

                                // Refine the old Y=0 plane hit against the actual
                                // terrain height. A few fixed-point iterations are
                                // enough because the heightfield is continuous.
                                float terrainHeight = 0.0F;
                                if (std::abs(ray.direction.y) > 0.00001F)
                                {
                                    for (std::uint32_t iteration = 0; iteration < 8U; ++iteration)
                                    {
                                        const float x = ray.origin.x + ray.direction.x * distance;
                                        const float z = ray.origin.z + ray.direction.z * distance;
                                        if (!terrainRenderer_.TryGetSurfaceHeight(
                                                sceneDocument_, x, z, terrainHeight)) break;
                                        const float refinedDistance =
                                            (terrainHeight - ray.origin.y) / ray.direction.y;
                                        if (refinedDistance < 0.0F) break;
                                        distance = refinedDistance;
                                    }
                                }

                                EditorTransform transform{};
                                transform.position = {
                                    ray.origin.x + ray.direction.x * distance,
                                    terrainHeight,
                                    ray.origin.z + ray.direction.z * distance};
                                DirectX::XMFLOAT3 boundsMinimum{}, boundsMaximum{};
                                if (staticMeshRenderer_.TryGetMeshBounds(
                                        path.generic_wstring(), boundsMinimum, boundsMaximum))
                                {
                                    transform.position[1] -= boundsMinimum.y;
                                }
                                const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                                if (sceneDocument_.CreateStaticMeshEntity(
                                        path.stem().wstring(),
                                        path.generic_wstring(),
                                        transform))
                                {
                                    static_cast<void>(commandHistory_.Push(
                                        before, sceneDocument_.CreateSnapshot()));
                                }
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
            }
            else
            {
                /*
                 * При переключении вкладки прекращаем работу
                 * кисти terrain и запрещаем её визуализацию.
                 */
                terrainBrushHitValid_ = false;

                if (terrainPaintStrokeActive_)
                {
                    static_cast<void>(
                        terrainRenderer_.EndPaintStroke());

                    terrainPaintStrokeActive_ = false;
                }
            }

            ImGui::End();
        //}
        TerrainImportContext terrainImportContext
        {
            graphicsDevice_,
            sceneDocument_,
            commandHistory_,
            cameraController_,
            terrainRenderer_,
            loadedTerrainAssetPath_,
            reinterpret_cast<void*>(GetWindow().GetNativeHandle().Value())
        };

        terrainImporter_.Draw(terrainImportContext);
        toolWindowManager_.DrawOpenWindows();
    }

    bool Application::ExecuteEditorCommand(const EditorCommand command)
    {
        if (terrainPaintMode_ && command == EditorCommand::Undo &&
            terrainRenderer_.CanUndoPaint())
            return terrainRenderer_.UndoPaint();
        if (terrainPaintMode_ && command == EditorCommand::Redo &&
            terrainRenderer_.CanRedoPaint())
            return terrainRenderer_.RedoPaint();
        return commandSystem_.Execute(
            command, sceneDocument_, commandHistory_, levelDocument_,
            transformController_);
    }

    void Application::ProcessEditorShortcuts() noexcept
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput) return;

        const auto pressed = [](const ImGuiKey key)
        {
            return ImGui::IsKeyPressed(key, false);
        };

        if (pressed(ImGuiKey_F5))
        {
            LaunchTestGame();
            return;
        }

        EditorCommand command{};
        bool hasCommand = true;
        if (io.KeyCtrl && pressed(ImGuiKey_N)) command = EditorCommand::NewLevel;
        else if (io.KeyCtrl && pressed(ImGuiKey_O)) command = EditorCommand::OpenLevel;
        else if (io.KeyCtrl && io.KeyShift && pressed(ImGuiKey_S)) command = EditorCommand::SaveLevelAs;
        else if (io.KeyCtrl && pressed(ImGuiKey_S)) command = EditorCommand::SaveLevel;
        else if (io.KeyCtrl && io.KeyShift && pressed(ImGuiKey_Z)) command = EditorCommand::Redo;
        else if (io.KeyCtrl && pressed(ImGuiKey_Z)) command = EditorCommand::Undo;
        else if (io.KeyCtrl && pressed(ImGuiKey_Y)) command = EditorCommand::Redo;
        else if (io.KeyCtrl && pressed(ImGuiKey_D)) command = EditorCommand::DuplicateSelection;
        else if (pressed(ImGuiKey_Delete)) command = EditorCommand::DeleteSelection;
        else if (!io.KeyCtrl && !io.KeyAlt && !io.MouseDown[ImGuiMouseButton_Right] && pressed(ImGuiKey_Q)) command = EditorCommand::SelectTool;
        else if (!io.KeyCtrl && !io.KeyAlt && !io.MouseDown[ImGuiMouseButton_Right] && pressed(ImGuiKey_W)) command = EditorCommand::MoveTool;
        else if (!io.KeyCtrl && !io.KeyAlt && !io.MouseDown[ImGuiMouseButton_Right] && pressed(ImGuiKey_E)) command = EditorCommand::RotateTool;
        else if (!io.KeyCtrl && !io.KeyAlt && !io.MouseDown[ImGuiMouseButton_Right] && pressed(ImGuiKey_R)) command = EditorCommand::ScaleTool;
        else hasCommand = false;

        if (hasCommand) static_cast<void>(ExecuteEditorCommand(command));
    }

    void Application::RenderImGui() noexcept
    {
        if (uiSwapChain_ == nullptr ||
            commandContext_ == nullptr ||
            IsMinimized())
        {
            return;
        }

        imguiHost_.BeginFrame();
        DrawImGuiWorkspace();

        /*
         * 3D-сцена сейчас рисуется поверх ImGui, чтобы прозрачное окно
         * Viewport не перекрывало grid, terrain и объекты.
         *
         * Но при открытом popup или modal-окне сцену временно не рисуем,
         * иначе она перекроет окно импорта, меню или контекстное меню.
         */
        const bool popupOpen = ImGui::IsPopupOpen(
            nullptr,
            ImGuiPopupFlags_AnyPopupId |
            ImGuiPopupFlags_AnyPopupLevel);

        engine::graphics::Viewport fullViewport{};
        fullViewport.x = 0.0F;
        fullViewport.y = 0.0F;
        fullViewport.width = static_cast<float>(uiWidth_);
        fullViewport.height = static_cast<float>(uiHeight_);
        fullViewport.minDepth = 0.0F;
        fullViewport.maxDepth = 1.0F;

        auto result = commandContext_->SetViewport(fullViewport);

        if (!engine::graphics::Failed(result))
        {
            result = commandContext_->SetSwapChainRenderTarget(*uiSwapChain_);
        }

        const engine::graphics::ClearColor clear
        {
            0.025F,
            0.030F,
            0.036F,
            1.0F
        };

        if (!engine::graphics::Failed(result))
        {
            result = commandContext_->ClearSwapChainColor(*uiSwapChain_, clear);
        }

        /*
         * Сначала отрисовываем весь ImGui.
         * Затем 3D-сцена заполняет только прозрачную область Viewport.
         */
        if (!engine::graphics::Failed(result))
        {
            imguiHost_.Render();
        }

        const bool renderScene =
            !engine::graphics::Failed(result) &&
            imguiViewportVisible_ &&
            !popupOpen &&
            depthStencil_.IsValid();

        if (renderScene)
        {
            engine::graphics::Viewport sceneViewport{};
            sceneViewport.x = imguiViewportX_;
            sceneViewport.y = imguiViewportY_;
            sceneViewport.width = (std::max)(imguiViewportWidth_, 1.0F);
            sceneViewport.height = (std::max)(imguiViewportHeight_, 1.0F);
            sceneViewport.minDepth = 0.0F;
            sceneViewport.maxDepth = 1.0F;

            result = commandContext_->SetViewport(sceneViewport);

            if (!engine::graphics::Failed(result))
            {
                result = commandContext_->SetSwapChainRenderTarget(
                    *uiSwapChain_,
                    depthStencil_);
            }

            if (!engine::graphics::Failed(result))
            {
                result = commandContext_->ClearDepthStencilTarget(
                    depthStencil_,
                    engine::graphics::ClearDepthStencilFlags::Depth |
                    engine::graphics::ClearDepthStencilFlags::Stencil,
                    1.0F,
                    0U);
            }

            DirectX::XMFLOAT4X4 viewProjection{};

            const std::uint32_t viewportWidth =
                static_cast<std::uint32_t>(
                    (std::max)(imguiViewportWidth_, 1.0F));

            const std::uint32_t viewportHeight =
                static_cast<std::uint32_t>(
                    (std::max)(imguiViewportHeight_, 1.0F));

            if (!engine::graphics::Failed(result) &&
                cameraController_.BuildViewProjection(
                    viewportWidth,
                    viewportHeight,
                    viewProjection))
            {
                result = gridRenderer_.Render(
                    *commandContext_,
                    viewProjection);

                if (!engine::graphics::Failed(result))
                {
                    result = terrainRenderer_.Render(
                        *commandContext_,
                        sceneDocument_,
                        viewProjection,
                        cameraController_.GetPosition());
                }

                if (!engine::graphics::Failed(result))
                {
                    result = staticMeshRenderer_.Render(
                        *commandContext_,
                        sceneDocument_,
                        viewProjection);
                }

                if (!engine::graphics::Failed(result))
                {
                    result = sceneRenderer_.Render(
                        *commandContext_,
                        sceneDocument_,
                        viewProjection,
                        transformController_.GetVisualState(),
                        &staticMeshRenderer_);
                }

                if (!engine::graphics::Failed(result) &&
                    terrainPaintMode_ &&
                    terrainBrushHitValid_)
                {
                    result = terrainRenderer_.RenderBrush(
                        *commandContext_,
                        sceneDocument_,
                        viewProjection,
                        terrainBrushWorldX_,
                        terrainBrushWorldZ_,
                        terrainBrushRadius_,
                        ImGui::GetIO().KeyShift);
                }
            }
        }

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "Render Dear ImGui workspace",
                result);

            commandContext_->UnbindRenderTargets();
            return;
        }

        commandContext_->UnbindRenderTargets();

        engine::graphics::PresentStatus presentStatus =
            engine::graphics::PresentStatus::Failed;

        result = uiSwapChain_->Present(presentStatus);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "Present Dear ImGui",
                result);
        }
    }

    bool Application::InitializeGraphics() noexcept
    {
        engine::graphics::RenderDeviceDesc
        deviceDescription{};

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

        graphicsFailureReported_ = false;

        return true;
    }

    void Application::ShutdownGraphics() noexcept
    {
        if (commandContext_ != nullptr)
        {
            commandContext_->
                UnbindRenderTargets();

            commandContext_->
                ClearState();

            commandContext_->
                Flush();
        }

        gridRenderer_.Shutdown(
            graphicsDevice_);

        DestroyDepthStencil();

        commandContext_ = nullptr;

        graphicsDevice_.Shutdown();

        graphicsFailureReported_ = false;
    }

    engine::graphics::GraphicsResult
        Application::CreateDepthStencil(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
    {
        if (width == 0U || height == 0U)
        {
            return engine::graphics::GraphicsResult::InvalidArgument;
        }

        engine::graphics::TextureDesc depthDescription;

        depthDescription.dimension =
            engine::graphics::TextureDimension::Texture2D;

        depthDescription.width = width;
        depthDescription.height = height;
        depthDescription.depth = 1;
        depthDescription.arrayLayers = 1;
        depthDescription.mipLevels = 1;
        depthDescription.sampleCount = 1;

        depthDescription.format =
            engine::graphics::Format::D24UNormS8UInt;

        depthDescription.usage =
            engine::graphics::ResourceUsage::Default;

        depthDescription.bindFlags =
            engine::graphics::TextureBindFlags::DepthStencil;

        depthDescription.cpuAccess =
            engine::graphics::CpuAccessFlags::None;

        depthDescription.generateMipmaps = false;

        return graphicsDevice_.CreateTexture(
            depthDescription,
            nullptr,
            0,
            depthStencil_);
    }

    void Application::DestroyDepthStencil() noexcept
    {
        if (!depthStencil_.IsValid())
        {
            return;
        }

        const auto result =
            graphicsDevice_.DestroyTexture(depthStencil_);

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
            engine::graphics::TextureHandle{};
    }

    void Application::ReportGraphicsFailure(
        const char* const operation,
        const engine::graphics::GraphicsResult result) noexcept
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
