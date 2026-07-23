#include "Editor/EditorApplication.h"

#include <Core/Log.h>
#include <Assets/LegacyTerrain2Importer.h>
#include <Assets/TerrainAsset.h>

#include <Graphics/Format.h>
#include <Graphics/GraphicsBackend.h>
#include <Graphics/GraphicsResult.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Element.h>
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

        [[nodiscard]] bool SelectFolder(
            const HWND owner, std::filesystem::path& output)
        {
            Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
            if (FAILED(CoCreateInstance(
                    CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&dialog)))) return false;
            DWORD options = 0U;
            if (FAILED(dialog->GetOptions(&options)) ||
                FAILED(dialog->SetOptions(options | FOS_PICKFOLDERS |
                    FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST)) ||
                FAILED(dialog->SetTitle(L"Select legacy Terrain2 directory")) ||
                FAILED(dialog->Show(owner))) return false;
            Microsoft::WRL::ComPtr<IShellItem> item;
            if (FAILED(dialog->GetResult(&item))) return false;
            PWSTR path = nullptr;
            if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) return false;
            output = path;
            CoTaskMemFree(path);
            return true;
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
            EditorCameraController& camera,
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

    EditorApplication::EditorApplication()
        : Application(CreateEditorDescription())
    {
    }

    EditorApplication::~EditorApplication() noexcept = default;

    lts::application::ApplicationResult
    EditorApplication::OnInitialize() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Initializing editor.");

        if (!editorShell_.Initialize(
                GetWindow().GetNativeHandle()))
        {
            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        try
        {
            sceneDocument_.Clear();
        }
        catch (...)
        {
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        commandHistory_.Clear();

        editorShell_.RefreshScene(
            sceneDocument_);

        if (!inspectorPanel_.Initialize(
                GetWindow().GetNativeHandle()))
        {
            sceneDocument_.Clear();
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        inspectorPanel_.Refresh(
            sceneDocument_);

        if (!levelDocument_.Initialize(
                GetWindow().GetNativeHandle(),
                sceneDocument_))
        {
            inspectorPanel_.Shutdown();
            sceneDocument_.Clear();
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!assetBrowserPanel_.Initialize(
                GetWindow().GetNativeHandle()))
        {
            levelDocument_.Shutdown();
            inspectorPanel_.Shutdown();
            sceneDocument_.Clear();
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        const engine::platform::
            NativeWindowHandle viewportWindow =
                editorShell_.
                    GetViewportWindowHandle();

        cameraController_.SetViewportWindow(
            viewportWindow);

        transformController_.SetViewportWindow(
            viewportWindow);

        editorShell_.SetStatusText(
            transformController_.
                BuildStatusText());

        if (!InitializeGraphics())
        {
            transformController_.
                SetViewportWindow({});

            cameraController_.
                SetViewportWindow({});

            assetBrowserPanel_.Shutdown();
            levelDocument_.Shutdown();
            inspectorPanel_.Shutdown();

            sceneDocument_.Clear();
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!staticMeshRenderer_.Initialize(
                graphicsDevice_))
        {
            transformController_.
                SetViewportWindow({});

            cameraController_.
                SetViewportWindow({});

            assetBrowserPanel_.Shutdown();
            levelDocument_.Shutdown();
            inspectorPanel_.Shutdown();

            sceneDocument_.Clear();

            ShutdownGraphics();
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!sceneRenderer_.Initialize(
                graphicsDevice_))
        {
            staticMeshRenderer_.Shutdown(
                graphicsDevice_);

            transformController_.
                SetViewportWindow({});

            cameraController_.
                SetViewportWindow({});

            assetBrowserPanel_.Shutdown();
            levelDocument_.Shutdown();
            inspectorPanel_.Shutdown();

            sceneDocument_.Clear();

            ShutdownGraphics();
            editorShell_.Shutdown();

            return lts::application::
                ApplicationResult::
                    ClientInitializationFailed;
        }

        if (!terrainRenderer_.Initialize(graphicsDevice_))
        {
            sceneRenderer_.Shutdown(graphicsDevice_);
            staticMeshRenderer_.Shutdown(graphicsDevice_);
            ShutdownGraphics();
            editorShell_.Shutdown();
            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        if (!InitializeUi())
        {
            terrainRenderer_.Shutdown(graphicsDevice_);
            sceneRenderer_.Shutdown(graphicsDevice_);
            staticMeshRenderer_.Shutdown(graphicsDevice_);
            ShutdownGraphics();
            editorShell_.Shutdown();
            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        assetBrowserPanel_.Shutdown();
        inspectorPanel_.Shutdown();

        if (!levelDocument_.SetWindowInterceptionEnabled(false))
        {
            ShutdownUi();
            return lts::application::ApplicationResult::ClientInitializationFailed;
        }

        EnumChildWindows(
            reinterpret_cast<HWND>(GetWindow().GetNativeHandle().Value()),
            [](HWND child, LPARAM) -> BOOL
            {
                ShowWindow(child, SW_HIDE);
                return TRUE;
            },
            0);

        SetMenu(
            reinterpret_cast<HWND>(GetWindow().GetNativeHandle().Value()),
            nullptr);
        DrawMenuBar(
            reinterpret_cast<HWND>(GetWindow().GetNativeHandle().Value()));

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Editor initialization completed.");

        return lts::application::
            ApplicationResult::Success;
    }

    void EditorApplication::OnShutdown() noexcept
    {
        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor",
            "Shutting down editor.");

        imguiHost_.Shutdown();
        ShutdownUi();

        transformController_.
            SetViewportWindow({});

        cameraController_.
            SetViewportWindow({});

        assetBrowserPanel_.Shutdown();
        levelDocument_.Shutdown();
        inspectorPanel_.Shutdown();

        commandHistory_.Clear();

        sceneRenderer_.Shutdown(
            graphicsDevice_);

        terrainRenderer_.Shutdown(graphicsDevice_);

        staticMeshRenderer_.Shutdown(
            graphicsDevice_);

        sceneDocument_.Clear();

        ShutdownGraphics();
        editorShell_.Shutdown();
    }

    void EditorApplication::OnUpdate(
    const double deltaSeconds) noexcept
    {
        if (imguiHost_.IsInitialized())
        {
            if (imguiWorkspace_ == EditorLauncherAction::LevelEditor)
            {
                const EditorLevelUpdateResult result =
                    levelDocument_.Update(sceneDocument_, commandHistory_);
                if (result.sceneReplaced)
                {
                    cameraController_.Reset();
                    loadedTerrainAssetPath_.clear();
                    static_cast<void>(terrainRenderer_.LoadTerrain(
                        graphicsDevice_, std::filesystem::path{}));
                }
                for (const EditorSceneEntity& sceneEntity : sceneDocument_.GetEntities())
                {
                    if (!sceneEntity.terrain.has_value() ||
                        !sceneEntity.terrain->visible) continue;
                    std::filesystem::path gameRoot = std::filesystem::current_path();
                    if (gameRoot.filename() != L"game") gameRoot /= L"game";
                    std::filesystem::path terrainPath = sceneEntity.terrain->assetPath;
                    if (terrainPath.is_relative()) terrainPath = gameRoot / terrainPath;
                    terrainPath = terrainPath.lexically_normal();
                    if (terrainPath !=
                        loadedTerrainAssetPath_)
                    {
                        /*
                         * Запоминаем саму попытку.
                         * Даже повреждённый файл не будет
                         * перечитываться каждый кадр.
                         */
                        loadedTerrainAssetPath_ =
                            terrainPath;

                        if (terrainRenderer_.
                                LoadTerrain(
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
                    if (returnToLauncherPending_)
                    {
                        if (!ReturnToLauncher()) RequestExit();
                    }
                    else
                    {
                        RequestExit();
                    }
                    return;
                }
                cameraController_.Update(
                    deltaSeconds,
                    static_cast<float>(GetInputSystem().GetMouseWheelDelta()) /
                        static_cast<float>(WHEEL_DELTA));

                const std::int32_t viewportX =
                    static_cast<std::int32_t>(imguiViewportX_);
                const std::int32_t viewportY =
                    static_cast<std::int32_t>(imguiViewportY_);
                const engine::platform::WindowSize viewportSize{
                    static_cast<std::uint32_t>(std::max(imguiViewportWidth_, 1.0F)),
                    static_cast<std::uint32_t>(std::max(imguiViewportHeight_, 1.0F))};
                transformController_.SetViewportRegion(
                    viewportX, viewportY, viewportSize.width, viewportSize.height);

                const engine::platform::MousePosition mouse =
                    GetInputSystem().GetMousePosition();
                const bool insideViewport =
                    mouse.x >= viewportX && mouse.y >= viewportY &&
                    mouse.x < viewportX + static_cast<std::int32_t>(viewportSize.width) &&
                    mouse.y < viewportY + static_cast<std::int32_t>(viewportSize.height);
                const bool clicked = insideViewport &&
                    GetInputSystem().WasMouseButtonPressed(
                        engine::platform::MouseButton::Left) &&
                    !GetInputSystem().IsMouseButtonDown(
                        engine::platform::MouseButton::Right);
                ViewportClick click;
                if (clicked)
                {
                    click.x = static_cast<std::uint32_t>(mouse.x - viewportX);
                    click.y = static_cast<std::uint32_t>(mouse.y - viewportY);
                }
                EditorInteractionResult transformResult{};
                if (!terrainPaintMode_)
                {
                    transformResult = transformController_.Update(
                        sceneDocument_, commandHistory_, cameraController_,
                        viewportSize, clicked ? &click : nullptr,
                        &staticMeshRenderer_);
                }

                const auto& transformState = transformController_.GetVisualState();
                if (transformResult.documentChanged &&
                    transformState.operation == EditorTransformOperation::Move &&
                    (transformState.activeAxis == EditorTransformAxis::X ||
                     transformState.activeAxis == EditorTransformAxis::Z))
                {
                    EditorSceneEntity* const entity = sceneDocument_.GetSelectedEntityMutable();
                    if (entity != nullptr && entity->staticMesh.has_value())
                    {
                        float terrainHeight = 0.0F;
                        if (terrainRenderer_.TryGetSurfaceHeight(
                                sceneDocument_, entity->transform.position[0],
                                entity->transform.position[2], terrainHeight))
                        {
                            DirectX::XMFLOAT3 boundsMinimum{}, boundsMaximum{};
                            float bottomOffset = 0.0F;
                            if (staticMeshRenderer_.TryGetMeshBounds(
                                    entity->staticMesh->assetPath,
                                    boundsMinimum, boundsMaximum))
                            {
                                bottomOffset = boundsMinimum.y * entity->transform.scale[1];
                            }
                            entity->transform.position[1] = terrainHeight - bottomOffset;
                        }
                    }
                }
            }
            return;
        }

        if (uiHost_.IsInitialized())
        {
            static_cast<void>(uiHost_.ProcessInput(GetInputSystem()));

            if (imguiWorkspacePending_)
            {
                imguiWorkspacePending_ = false;
                static_cast<void>(StartImGuiWorkspace(pendingImGuiWorkspace_));
                return;
            }

            if (levelEditorUiActive_)
            {
                const EditorLevelUpdateResult result =
                    levelDocument_.Update(sceneDocument_, commandHistory_);
                if (result.sceneReplaced)
                {
                    cameraController_.Reset();
                }
                if (result.closeApproved)
                {
                    RequestExit();
                }

                cameraController_.Update(
                    deltaSeconds,
                    static_cast<float>(GetInputSystem().GetMouseWheelDelta()) /
                        static_cast<float>(WHEEL_DELTA));

                if (uiDocument_ != nullptr)
                {
                    Rml::Element* const viewportElement =
                        uiDocument_->GetElementById("viewport");
                    if (viewportElement != nullptr)
                    {
                        const Rml::Vector2f offset = viewportElement->
                            GetAbsoluteOffset(Rml::BoxArea::Border);
                        const Rml::Vector2f size = viewportElement->GetBox().
                            GetSize(Rml::BoxArea::Border);
                        const std::int32_t viewportX =
                            static_cast<std::int32_t>(offset.x);
                        const std::int32_t viewportY =
                            static_cast<std::int32_t>(offset.y);
                        engine::platform::WindowSize viewportSize{
                            static_cast<std::uint32_t>(std::max(size.x, 1.0F)),
                            static_cast<std::uint32_t>(std::max(size.y, 1.0F))};

                        transformController_.SetViewportRegion(
                            viewportX, viewportY,
                            viewportSize.width, viewportSize.height);

                        ViewportClick click;
                        const engine::platform::MousePosition mouse =
                            GetInputSystem().GetMousePosition();
                        const bool inside =
                            mouse.x >= viewportX && mouse.y >= viewportY &&
                            mouse.x < viewportX + static_cast<std::int32_t>(viewportSize.width) &&
                            mouse.y < viewportY + static_cast<std::int32_t>(viewportSize.height);
                        const bool clicked = inside &&
                            GetInputSystem().WasMouseButtonPressed(
                                engine::platform::MouseButton::Left) &&
                            !GetInputSystem().IsMouseButtonDown(
                                engine::platform::MouseButton::Right);
                        if (clicked)
                        {
                            click.x = static_cast<std::uint32_t>(mouse.x - viewportX);
                            click.y = static_cast<std::uint32_t>(mouse.y - viewportY);
                        }

                        static_cast<void>(transformController_.Update(
                            sceneDocument_, commandHistory_, cameraController_,
                            viewportSize, clicked ? &click : nullptr,
                            &staticMeshRenderer_));
                    }
                }
            }

            uiHost_.Update();
            return;
        }

        const EditorLevelUpdateResult
            levelResult =
                levelDocument_.Update(
                    sceneDocument_,
                    commandHistory_);

        if (levelResult.sceneReplaced)
        {
            cameraController_.Reset();

            transformController_.
                SetViewportWindow(
                    editorShell_.
                        GetViewportWindowHandle());

            editorShell_.RefreshScene(
                sceneDocument_);

            inspectorPanel_.Refresh(
                sceneDocument_);
        }

        assetBrowserPanel_.Update();

        std::filesystem::path activatedAsset;

        if (assetBrowserPanel_.ConsumeActivatedAsset(
                activatedAsset))
        {
            const EditorSceneSnapshot before =
                sceneDocument_.CreateSnapshot();

            EditorTransform transform;

            const EditorSceneEntity* const selected =
                sceneDocument_.GetSelectedEntity();

            if (selected != nullptr)
            {
                transform.position =
                    selected->transform.position;

                transform.position[0] += 2.0F;
            }

            const std::wstring entityName =
                activatedAsset.
                    stem().
                    wstring();

            if (sceneDocument_.CreateStaticMeshEntity(
                    entityName,
                    activatedAsset.generic_wstring(),
                    transform))
            {
                static_cast<void>(
                    commandHistory_.Push(
                        before,
                        sceneDocument_.
                            CreateSnapshot()));

                editorShell_.RefreshScene(
                    sceneDocument_);

                inspectorPanel_.Refresh(
                    sceneDocument_);
            }
        }

        cameraController_.Update(
            deltaSeconds,
            editorShell_.
                ConsumeViewportWheelSteps());

        bool selectionChanged = false;
        bool hierarchyChanged = false;

        std::size_t selectedEntityIndex =
            InvalidEditorEntityIndex;

        if (
            editorShell_.
                ConsumeHierarchySelection(
                    selectedEntityIndex) &&
            sceneDocument_.
                SelectEntityByIndex(
                    selectedEntityIndex))
        {
            selectionChanged = true;
        }

        ViewportClick viewportClick;

        const bool hasViewportClick =
            editorShell_.
                ConsumeViewportClick(
                    viewportClick);

        const EditorInteractionResult
            interactionResult =
                transformController_.Update(
                    sceneDocument_,
                    commandHistory_,
                    cameraController_,
                    editorShell_.GetViewportSize(),
                    hasViewportClick
                        ? &viewportClick
                        : nullptr);

        selectionChanged =
            selectionChanged ||
            interactionResult.selectionChanged;

        hierarchyChanged =
            hierarchyChanged ||
            interactionResult.hierarchyChanged;

        if (hierarchyChanged)
        {
            editorShell_.RefreshScene(
                sceneDocument_);
        }
        else if (selectionChanged)
        {
            editorShell_.SelectHierarchyEntity(
                sceneDocument_.
                    GetSelectedIndex());
        }

        if (
            selectionChanged ||
            hierarchyChanged ||
            interactionResult.documentChanged)
        {
            editorShell_.ShowEntityDetails(
                sceneDocument_.
                    GetSelectedEntity());

            inspectorPanel_.Refresh(
                sceneDocument_);
        }

        if (inspectorPanel_.Update(
                sceneDocument_,
                commandHistory_))
        {
            editorShell_.ShowEntityDetails(
                sceneDocument_.
                    GetSelectedEntity());
        }

        if (interactionResult.statusChanged)
        {
            editorShell_.SetStatusText(
                transformController_.
                    BuildStatusText());
        }

        EditorMode changedMode =
            EditorMode::Level;

        if (editorShell_.ConsumeModeChanged(
                changedMode))
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

        levelDocument_.
            SynchronizeWindowTitle(
                sceneDocument_);

        if (swapChainOccluded_)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(16));
        }
    }

    void EditorApplication::OnRender() noexcept
    {
        if (imguiHost_.IsInitialized())
        {
            RenderImGui();
            return;
        }

        if (uiHost_.IsInitialized())
        {
            RenderUi();
            return;
        }

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
            ReportGraphicsFailure("SetViewport", result);
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

        result = terrainRenderer_.Render(*commandContext_, sceneDocument_, viewProjection,
            cameraController_.GetPosition());
        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure("EditorTerrainRenderer::Render", result);
            return;
        }

        result = staticMeshRenderer_.Render(
        *commandContext_,
        sceneDocument_,
        viewProjection);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "EditorStaticMeshRenderer::Render",
                result);

            return;
        }

        result = sceneRenderer_.Render(
            *commandContext_,
            sceneDocument_,
            viewProjection,
            transformController_.GetVisualState(),
            &staticMeshRenderer_);

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
            ReportGraphicsFailure("Present", result);
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
        const lts::application::ApplicationEvent& event) noexcept
    {
        switch (event.type)
        {
            case lts::application::ApplicationEventType::Resize:
            {
                if (uiSwapChain_ != nullptr && event.width > 0 && event.height > 0)
                {
                    commandContext_->UnbindRenderTargets();
                    const auto uiResizeResult = uiSwapChain_->Resize(event.width, event.height);
                    if (!engine::graphics::Failed(uiResizeResult))
                    {
                        DestroyDepthStencil();
                        const auto depthResizeResult =
                            CreateDepthStencil(event.width, event.height);
                        if (engine::graphics::Failed(depthResizeResult))
                        {
                            ReportGraphicsFailure(
                                "Resize UI depth stencil",
                                depthResizeResult);
                            break;
                        }
                        uiWidth_ = event.width;
                        uiHeight_ = event.height;
                        uiRenderInterface_.SetViewportSize(
                            static_cast<int>(uiWidth_), static_cast<int>(uiHeight_));
                        uiHost_.Resize(static_cast<int>(uiWidth_), static_cast<int>(uiHeight_));
                    }
                }

                if (uiHost_.IsInitialized() || imguiHost_.IsInitialized())
                {
                    break;
                }

                editorShell_.Resize(
                    event.width,
                    event.height);

                const auto viewportSize =
                    editorShell_.GetViewportSize();

                ResizeGraphics(
                    viewportSize.width,
                    viewportSize.height);
                break;
            }

            case lts::application::ApplicationEventType::Minimized:
                swapChainOccluded_ = true;
                break;

            case lts::application::ApplicationEventType::Restored:
            {
                swapChainOccluded_ = false;

                const auto viewportSize =
                    editorShell_.GetViewportSize();

                ResizeGraphics(
                    viewportSize.width,
                    viewportSize.height);
                break;
            }

            case lts::application::ApplicationEventType::Activated:
                swapChainOccluded_ = false;
                break;

            case lts::application::ApplicationEventType::DpiChanged:
            {
                const auto clientSize =
                    GetWindow().GetClientSize();

                editorShell_.Resize(
                    clientSize.width,
                    clientSize.height);

                const auto viewportSize =
                    editorShell_.GetViewportSize();

                ResizeGraphics(
                    viewportSize.width,
                    viewportSize.height);
                break;
            }

            case lts::application::ApplicationEventType::Deactivated:
            case lts::application::ApplicationEventType::CloseRequested:
            default:
                break;
        }
    }

    bool EditorApplication::OnNativeMessage(
        void* const nativeWindow,
        const std::uint32_t message,
        const std::uintptr_t wordParameter,
        const std::intptr_t longParameter) noexcept
    {
        return imguiHost_.ProcessNativeMessage(
            nativeWindow, message, wordParameter, longParameter);
    }

    bool EditorApplication::InitializeUi() noexcept
    {
        const auto clientSize = GetWindow().GetClientSize();
        uiWidth_ = std::max(clientSize.width, 1U);
        uiHeight_ = std::max(clientSize.height, 1U);

        engine::graphics::SwapChainDesc description;
        description.window = GetWindow().GetNativeHandle();
        description.width = uiWidth_;
        description.height = uiHeight_;
        description.bufferCount = 2;
        description.format = engine::graphics::Format::B8G8R8A8UNorm;
        description.presentMode = engine::graphics::PresentMode::VSync;

        const auto createResult = graphicsDevice_.CreateSwapChain(
            description,
            uiSwapChain_);
        if (engine::graphics::Failed(createResult))
        {
            ReportGraphicsFailure("Create UI swap chain", createResult);
            return false;
        }

        DestroyDepthStencil();
        const auto depthResult = CreateDepthStencil(uiWidth_, uiHeight_);
        if (engine::graphics::Failed(depthResult))
        {
            ReportGraphicsFailure("Create UI depth stencil", depthResult);
            return false;
        }

        return InitializeLauncherUi();
    }

    bool EditorApplication::InitializeLauncherUi() noexcept
    {
        const std::filesystem::path gameRoot =
            engine::ui::RmlUiHost::DiscoverGameRoot();
        if (gameRoot.empty())
        {
            return false;
        }

        const auto shaderPath =
            gameRoot / "Data" / "Shaders" / "Editor" / "RmlUi.hlsl";
        if (!uiRenderInterface_.Initialize(
                graphicsDevice_,
                shaderPath,
                static_cast<int>(uiWidth_),
                static_cast<int>(uiHeight_)))
        {
            return false;
        }

        if (!uiHost_.Initialize(
                engine::ui::UiDomain::Editor,
                gameRoot,
                uiRenderInterface_,
                static_cast<int>(uiWidth_),
                static_cast<int>(uiHeight_)))
        {
            uiRenderInterface_.Shutdown();
            return false;
        }

        if (!LoadUiDocument("Launcher/EditorLauncher.rml"))
        {
            ShutdownUi();
            return false;
        }

        return launcherController_.Attach(
            *uiDocument_,
            [this](const EditorLauncherAction action)
            {
                HandleLauncherAction(action);
            });
    }

    bool EditorApplication::ReturnToLauncher() noexcept
    {
        imguiHost_.Shutdown();
        cameraController_.SetViewportWindow({});
        transformController_.SetViewportWindow({});
        levelEditorUiActive_ = false;
        returnToLauncherPending_ = false;
        return InitializeLauncherUi();
    }

    void EditorApplication::ShutdownUi() noexcept
    {
        launcherController_.Detach();
        levelEditorUiController_.Detach();
        uiDocument_ = nullptr;
        uiHost_.Shutdown();
        uiRenderInterface_.Shutdown();
        uiSwapChain_.reset();
        uiWidth_ = 0;
        uiHeight_ = 0;
    }

    bool EditorApplication::StartImGuiWorkspace(
        const EditorLauncherAction action) noexcept
    {
        if (uiDocument_ != nullptr)
        {
            launcherController_.Detach();
            uiDocument_->Close();
            uiDocument_ = nullptr;
        }
        uiHost_.Shutdown();
        uiRenderInterface_.Shutdown();

        std::error_code settingsError;
        std::filesystem::create_directories(
            "Data/Editor/Settings", settingsError);
        const char* iniFilename = "Data/Editor/Settings/LevelEditor.layout.ini";
        if (action == EditorLauncherAction::CharacterEditor)
            iniFilename = "Data/Editor/Settings/CharacterEditor.layout.ini";
        else if (action == EditorLauncherAction::PhysicsEditor)
            iniFilename = "Data/Editor/Settings/PhysicsEditor.layout.ini";
        else if (action == EditorLauncherAction::FbxImporter)
            iniFilename = "Data/Editor/Settings/FbxImporter.layout.ini";
        else if (action == EditorLauncherAction::IconGenerator)
            iniFilename = "Data/Editor/Settings/IconGenerator.layout.ini";

        if (!imguiHost_.Initialize(
                reinterpret_cast<void*>(GetWindow().GetNativeHandle().Value()),
                graphicsDevice_.GetNativeDevice(),
                graphicsDevice_.GetNativeImmediateContext(),
                iniFilename))
        {
            return false;
        }

        imguiWorkspace_ = action;
        RefreshContentBrowser();
        levelEditorUiActive_ = action == EditorLauncherAction::LevelEditor;
        cameraController_.SetViewportWindow(GetWindow().GetNativeHandle());
        transformController_.SetViewportWindow(GetWindow().GetNativeHandle());
        return true;
    }

    void EditorApplication::RefreshContentBrowser() noexcept
    {
        const std::filesystem::path previousDirectory = contentSelectedDirectory_;
        contentMeshFiles_.clear();
        try
        {
            std::filesystem::path gameRoot = std::filesystem::current_path();
            if (gameRoot.filename() != L"game")
            {
                gameRoot /= L"game";
            }
            contentMeshesRoot_ = (gameRoot / L"Data" / L"Meshes").lexically_normal();
            contentSelectedDirectory_ = previousDirectory.empty()
                ? contentMeshesRoot_
                : previousDirectory;

            std::error_code error;
            for (std::filesystem::recursive_directory_iterator iterator(
                     contentMeshesRoot_,
                     std::filesystem::directory_options::skip_permission_denied,
                     error), end;
                 iterator != end;
                 iterator.increment(error))
            {
                if (error)
                {
                    error.clear();
                    continue;
                }
                if (iterator->is_regular_file(error) && !error &&
                    iterator->path().extension() == L".ltsmesh")
                {
                    contentMeshFiles_.push_back(iterator->path().lexically_normal());
                }
            }
            std::sort(contentMeshFiles_.begin(), contentMeshFiles_.end());
            if (!std::filesystem::is_directory(contentSelectedDirectory_))
            {
                contentSelectedDirectory_ = contentMeshesRoot_;
            }
        }
        catch (...)
        {
            contentMeshFiles_.clear();
        }
    }

    void EditorApplication::DrawContentBrowser() noexcept
    {
        if (ImGui::Button("Refresh"))
        {
            RefreshContentBrowser();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(260.0F);
        ImGui::InputTextWithHint(
            "##ContentSearch",
            "Search meshes...",
            contentSearch_.data(),
            contentSearch_.size());
        ImGui::SameLine();
        if (ImGui::Button(contentGridView_ ? "List" : "Tiles"))
        {
            contentGridView_ = !contentGridView_;
        }
        ImGui::Separator();

        ImGui::BeginChild("ContentFolders", ImVec2(210.0F, 0.0F), ImGuiChildFlags_Borders);
        std::function<void(const std::filesystem::path&, const char*)> drawDirectory;
        drawDirectory = [&](const std::filesystem::path& directory, const char* label)
        {
            std::vector<std::filesystem::path> children;
            std::error_code error;
            for (std::filesystem::directory_iterator iterator(
                     directory, std::filesystem::directory_options::skip_permission_denied,
                     error), end;
                 iterator != end;
                 iterator.increment(error))
            {
                if (error) { error.clear(); continue; }
                if (iterator->is_directory(error) && !error)
                {
                    children.push_back(iterator->path().lexically_normal());
                }
            }
            std::sort(children.begin(), children.end());
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth;
            if (children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
            if (directory == contentSelectedDirectory_) flags |= ImGuiTreeNodeFlags_Selected;
            if (directory == contentMeshesRoot_) flags |= ImGuiTreeNodeFlags_DefaultOpen;
            const bool open = ImGui::TreeNodeEx(directory.generic_u8string().c_str(), flags, "%s", label);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {
                contentSelectedDirectory_ = directory;
            }
            if (open)
            {
                for (const std::filesystem::path& child : children)
                {
                    const std::string childLabel = child.filename().u8string();
                    drawDirectory(child, childLabel.c_str());
                }
                ImGui::TreePop();
            }
        };
        drawDirectory(contentMeshesRoot_, "Meshes");
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("ContentAssets", ImVec2(0.0F, 0.0F), ImGuiChildFlags_Borders);
        std::error_code breadcrumbError;
        const std::filesystem::path relativeDirectory = std::filesystem::relative(
            contentSelectedDirectory_, contentMeshesRoot_, breadcrumbError);
        if (ImGui::SmallButton("Meshes")) contentSelectedDirectory_ = contentMeshesRoot_;
        if (!breadcrumbError && relativeDirectory != L".")
        {
            std::filesystem::path current = contentMeshesRoot_;
            for (const auto& part : relativeDirectory)
            {
                current /= part;
                ImGui::SameLine(); ImGui::TextDisabled(">"); ImGui::SameLine();
                const std::string partLabel = part.u8string();
                ImGui::PushID(current.generic_u8string().c_str());
                if (ImGui::SmallButton(partLabel.c_str())) contentSelectedDirectory_ = current;
                ImGui::PopID();
            }
        }
        ImGui::Separator();
        std::string search = contentSearch_.data();
        std::transform(search.begin(), search.end(), search.begin(),
            [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
        bool refreshRequested = false;
        for (const std::filesystem::path& file : contentMeshFiles_)
        {
            if (contentSelectedDirectory_ != contentMeshesRoot_ &&
                file.parent_path() != contentSelectedDirectory_)
            {
                continue;
            }
            const std::string name = file.stem().u8string();
            std::error_code displayPathError;
            const std::string displayPath = std::filesystem::relative(
                file, contentMeshesRoot_, displayPathError).generic_u8string();
            std::string loweredName = displayPathError ? name : displayPath;
            std::transform(loweredName.begin(), loweredName.end(), loweredName.begin(),
                [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
            if (!search.empty() && loweredName.find(search) == std::string::npos)
            {
                continue;
            }

            ImGui::PushID(file.generic_u8string().c_str());
            const float itemWidth = contentGridView_ ? 150.0F : -1.0F;
            const float itemHeight = contentGridView_ ? 58.0F : 24.0F;
            const bool selected = ImGui::Selectable(
                name.c_str(), contentSelectedAsset_ == file,
                ImGuiSelectableFlags_AllowDoubleClick,
                ImVec2(itemWidth, itemHeight));
            if (selected) contentSelectedAsset_ = file;
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::error_code relativeError;
                const std::filesystem::path relative = std::filesystem::relative(
                    file, contentMeshesRoot_.parent_path().parent_path(), relativeError);
                if (!relativeError)
                {
                    const std::string payload = relative.generic_u8string();
                    ImGui::SetDragDropPayload(
                        "LTS_MESH_ASSET",
                        payload.c_str(),
                        payload.size() + 1U);
                    ImGui::Text("Place %s", name.c_str());
                }
                ImGui::EndDragDropSource();
            }
            if (selected && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                std::error_code error;
                const std::filesystem::path relative = std::filesystem::relative(
                    file, contentMeshesRoot_.parent_path().parent_path(), error);
                if (!error)
                {
                    const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                    EditorTransform transform{};
                    EditorPickRay ray{};
                    if (cameraController_.BuildPickRay(
                            static_cast<std::uint32_t>(imguiViewportWidth_ * 0.5F),
                            static_cast<std::uint32_t>(imguiViewportHeight_ * 0.5F),
                            static_cast<std::uint32_t>(imguiViewportWidth_),
                            static_cast<std::uint32_t>(imguiViewportHeight_), ray))
                    {
                        float distance = 10.0F;
                        if (std::abs(ray.direction.y) > 0.00001F)
                        {
                            const float planeDistance = -ray.origin.y / ray.direction.y;
                            if (planeDistance >= 0.0F) distance = planeDistance;
                            float terrainHeight = 0.0F;
                            for (std::uint32_t iteration = 0; iteration < 8U; ++iteration)
                            {
                                const float x = ray.origin.x + ray.direction.x * distance;
                                const float z = ray.origin.z + ray.direction.z * distance;
                                if (!terrainRenderer_.TryGetSurfaceHeight(
                                        sceneDocument_, x, z, terrainHeight)) break;
                                const float refined =
                                    (terrainHeight - ray.origin.y) / ray.direction.y;
                                if (refined < 0.0F) break;
                                distance = refined;
                            }
                        }
                        transform.position = {
                            ray.origin.x + ray.direction.x * distance,
                            ray.origin.y + ray.direction.y * distance,
                            ray.origin.z + ray.direction.z * distance};
                        float terrainHeight = 0.0F;
                        if (terrainRenderer_.TryGetSurfaceHeight(
                                sceneDocument_, transform.position[0],
                                transform.position[2], terrainHeight))
                            transform.position[1] = terrainHeight;
                        DirectX::XMFLOAT3 boundsMinimum{}, boundsMaximum{};
                        if (staticMeshRenderer_.TryGetMeshBounds(
                                relative.generic_wstring(), boundsMinimum, boundsMaximum))
                            transform.position[1] -= boundsMinimum.y;
                    }
                    if (sceneDocument_.CreateStaticMeshEntity(
                            file.stem().wstring(),
                            relative.generic_wstring(),
                            transform))
                    {
                        static_cast<void>(commandHistory_.Push(
                            before, sceneDocument_.CreateSnapshot()));
                    }
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s\nDouble-click to add to scene",
                    file.generic_u8string().c_str());
            }
            if (ImGui::BeginPopupContextItem("AssetContext"))
            {
                contentSelectedAsset_ = file;
                if (ImGui::MenuItem("Copy Asset Path"))
                {
                    ImGui::SetClipboardText(file.generic_u8string().c_str());
                }
                if (ImGui::MenuItem("Copy Game Path"))
                {
                    std::error_code relativeError;
                    const std::filesystem::path relative = std::filesystem::relative(
                        file, contentMeshesRoot_.parent_path().parent_path(), relativeError);
                    if (!relativeError)
                    {
                        ImGui::SetClipboardText(relative.generic_u8string().c_str());
                    }
                }
                if (ImGui::MenuItem("Refresh")) refreshRequested = true;
                ImGui::EndPopup();
            }
            ImGui::PopID();
            if (contentGridView_ && ImGui::GetContentRegionAvail().x > 310.0F)
            {
                ImGui::SameLine();
            }
        }
        if (refreshRequested) RefreshContentBrowser();
        if (contentMeshFiles_.empty())
        {
            ImGui::TextDisabled("No .ltsmesh files found in Data/Meshes");
        }
        ImGui::EndChild();
    }

    void EditorApplication::DrawImGuiWorkspace() noexcept
    {
        if (imguiWorkspace_ == EditorLauncherAction::LevelEditor)
        {
            static_cast<void>(levelEditorLayout_.DrawDockSpace());
            ProcessEditorShortcuts();
        }
        else
            static_cast<void>(ImGui::DockSpaceOverViewport(
                0, ImGui::GetMainViewport(),
                ImGuiDockNodeFlags_PassthruCentralNode));

        const char* workspaceName = "Level Editor";
        switch (imguiWorkspace_)
        {
            case EditorLauncherAction::CharacterEditor: workspaceName = "Character Editor"; break;
            case EditorLauncherAction::PhysicsEditor: workspaceName = "Physics Editor"; break;
            case EditorLauncherAction::FbxImporter: workspaceName = "FBX Importer"; break;
            case EditorLauncherAction::IconGenerator: workspaceName = "Icon Generator"; break;
            default: break;
        }

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N")) static_cast<void>(ExecuteEditorCommand(EditorCommand::NewLevel));
                if (ImGui::MenuItem("Open", "Ctrl+O")) static_cast<void>(ExecuteEditorCommand(EditorCommand::OpenLevel));
                if (ImGui::MenuItem("Save", "Ctrl+S")) static_cast<void>(ExecuteEditorCommand(EditorCommand::SaveLevel));
                if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) static_cast<void>(ExecuteEditorCommand(EditorCommand::SaveLevelAs));
                ImGui::Separator();
                if (ImGui::MenuItem(
                        "Import Legacy Terrain2..."))
                {
                    std::filesystem::path selectedFolder;

                    const HWND owner =
                        reinterpret_cast<HWND>(
                            GetWindow().
                                GetNativeHandle().
                                Value());

                    if (SelectFolder(
                            owner,
                            selectedFolder))
                    {
                        /*
                         * Можно выбрать либо сам Terrain2,
                         * либо корневую папку карты.
                         */
                        std::filesystem::path
                            terrain2Directory =
                                selectedFolder;

                        std::error_code pathError;

                        if (!std::filesystem::
                                is_regular_file(
                                    terrain2Directory /
                                        L"terrain2.ini",
                                    pathError))
                        {
                            pathError.clear();

                            const auto nestedDirectory =
                                selectedFolder /
                                L"Terrain2";

                            if (std::filesystem::
                                    is_regular_file(
                                        nestedDirectory /
                                            L"terrain2.ini",
                                        pathError))
                            {
                                terrain2Directory =
                                    nestedDirectory;
                            }
                        }

                        std::filesystem::path gameRoot =
                            std::filesystem::
                                current_path();

                        if (gameRoot.filename() !=
                            L"game")
                        {
                            gameRoot /= L"game";
                        }

                        std::wstring terrainName;

                        if (terrain2Directory.
                                filename() ==
                            L"Terrain2")
                        {
                            terrainName =
                                terrain2Directory.
                                    parent_path().
                                    filename().
                                    wstring();
                        }
                        else
                        {
                            terrainName =
                                terrain2Directory.
                                    filename().
                                    wstring();
                        }

                        if (terrainName.empty())
                        {
                            terrainName =
                                L"ImportedTerrain";
                        }

                        const std::filesystem::path
                            destination =
                                gameRoot /
                                L"Data" /
                                L"Terrains" /
                                (
                                    terrainName +
                                    L".ltsterrain"
                                );

                        const engine::assets::
                            AssetResult importResult =
                                engine::assets::
                                    LegacyTerrain2Importer::
                                        Import(
                                            terrain2Directory,
                                            destination);

                        bool terrainReady = false;

                        std::wstring message;

                        if (engine::assets::Failed(
                                importResult))
                        {
                            message =
                                L"Terrain import failed: " +
                                std::wstring(
                                    FromUtf8(
                                        engine::assets::
                                            ToString(
                                                importResult)));
                        }
                        else
                        {
                            engine::assets::TerrainAsset
                                terrainAsset;

                            const auto assetResult =
                                engine::assets::
                                    TerrainAsset::Load(
                                        destination,
                                        terrainAsset);

                            if (engine::assets::Failed(
                                    assetResult))
                            {
                                message =
                                    L"Terrain was imported, "
                                    L"but validation failed: " +
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
                                        destination))
                            {
                                message =
                                    L"Terrain was imported, "
                                    L"but GPU resources "
                                    L"could not be created.";
                            }
                            else
                            {
                                loadedTerrainAssetPath_ =
                                    destination.
                                        lexically_normal();

                                std::error_code
                                    relativeError;

                                const auto relativePath =
                                    std::filesystem::
                                        relative(
                                            destination,
                                            gameRoot,
                                            relativeError);

                                if (relativeError)
                                {
                                    message =
                                        L"Terrain was imported, "
                                        L"but its game-relative "
                                        L"path could not be built.";
                                }
                                else
                                {
                                    const EditorSceneSnapshot
                                        before =
                                            sceneDocument_.
                                                CreateSnapshot();

                                    EditorTransform
                                        transform{};

                                    if (sceneDocument_.
                                            CreateTerrainEntity(
                                                destination.
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
                                            L"Terrain imported "
                                            L"successfully:\n" +
                                            destination.wstring();
                                    }
                                    else
                                    {
                                        message =
                                            L"Terrain was imported, "
                                            L"but the Terrain actor "
                                            L"could not be created.";
                                    }
                                }
                            }
                        }

                        MessageBoxW(
                            owner,
                            message.c_str(),
                            L"Legacy Terrain2 Import",
                            MB_OK |
                                (
                                    terrainReady
                                        ? MB_ICONINFORMATION
                                        : MB_ICONERROR
                                ));
                    }
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
                if (ImGui::MenuItem("Back to Editor Launcher"))
                {
                    returnToLauncherPending_ = true;
                    levelDocument_.RequestCloseLevel();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    returnToLauncherPending_ = false;
                    levelDocument_.RequestCloseLevel();
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
            ImGui::TextDisabled("%s", workspaceName);
            ImGui::EndMainMenuBar();
        }

        if (imguiWorkspace_ == EditorLauncherAction::LevelEditor)
        {
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
            if (ImGui::Button("Play")) HandleLauncherAction(EditorLauncherAction::TestGame);
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
                    ImGui::SeparatorText("Terrain Paint");
                    ImGui::Checkbox("Enable Paint Mode", &terrainPaintMode_);
                    ImGui::SliderFloat("Brush Size", &terrainBrushRadius_, 1.0F, 1024.0F, "%.1f");
                    ImGui::SliderFloat("Strength", &terrainBrushStrength_, 0.01F, 1.0F, "%.2f");
                    ImGui::SliderFloat("Falloff", &terrainBrushFalloff_, 0.0F, 1.0F, "%.2f");
                    const auto& paintLayers=entity->terrain->layers;
                    const char* preview=paintLayers.empty()?"Base Layer":paintLayers[0].name.c_str();
                    if(terrainPaintLayer_<paintLayers.size())
                        preview=paintLayers[terrainPaintLayer_].name.c_str();
                    if(ImGui::BeginCombo("Paint Layer",preview))
                    {
                        for(std::size_t index=0;index<paintLayers.size()&&index<18U;++index)
                        {
                            ImGui::PushID(static_cast<int>(index));
                            if(ImGui::Selectable(paintLayers[index].name.c_str(),terrainPaintLayer_==index))
                                terrainPaintLayer_=index;
                            ImGui::PopID();
                        }
                        ImGui::EndCombo();
                    }
                    if(ImGui::Button("Undo Paint")&&terrainRenderer_.CanUndoPaint())
                        static_cast<void>(terrainRenderer_.UndoPaint());
                    ImGui::SameLine();
                    if(ImGui::Button("Redo Paint")&&terrainRenderer_.CanRedoPaint())
                        static_cast<void>(terrainRenderer_.RedoPaint());
                    ImGui::TextDisabled("LMB paint | Shift+LMB erase");
                    ImGui::SeparatorText("Terrain Materials");
                    ImGui::Separator();
                    ImGui::TextUnformatted("Terrain");
                    ImGui::TextWrapped("Asset: %s",
                        ToUtf8(entity->terrain->assetPath).c_str());
                    ImGui::TextDisabled("%llu material layers",
                        static_cast<unsigned long long>(entity->terrain->layers.size()));
                    for (std::size_t layerIndex = 0U;
                         layerIndex < entity->terrain->layers.size(); ++layerIndex)
                    {
                        const auto& layer = entity->terrain->layers[layerIndex];
                        ImGui::PushID(static_cast<int>(layerIndex));
                        const std::string label = std::to_string(layerIndex) + ". " + layer.name;
                        if (ImGui::TreeNode(label.c_str()))
                        {
                            bool visible = layer.visible;
                            float scale[2]{layer.scaleU, layer.scaleV};
                            float offset[2]{layer.offsetU, layer.offsetV};
                            std::string diffuse = layer.diffusePath;
                            std::string normal = layer.normalPath;
                            if (ImGui::Checkbox("Visible", &visible))
                            {
                                const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                                if (sceneDocument_.UpdateSelectedTerrainLayer(
                                        layerIndex, diffuse, normal, scale[0], scale[1],
                                        offset[0], offset[1], visible))
                                    static_cast<void>(commandHistory_.Push(
                                        before, sceneDocument_.CreateSnapshot()));
                            }
                            const auto editPlacement = [this, layerIndex, &diffuse, &normal,
                                &scale, &offset, visible](const char* caption, float* values,
                                    const float speed, const float minimum, const float maximum)
                            {
                                const bool changed = ImGui::DragFloat2(
                                    caption, values, speed, minimum, maximum);
                                if (ImGui::IsItemActivated())
                                {
                                    terrainLayerEditBefore_ = sceneDocument_.CreateSnapshot();
                                    terrainLayerEditIndex_ = layerIndex;
                                    terrainLayerEditActive_ = true;
                                }
                                if (changed)
                                {
                                    static_cast<void>(sceneDocument_.UpdateSelectedTerrainLayer(
                                        layerIndex, diffuse, normal, scale[0], scale[1],
                                        offset[0], offset[1], visible));
                                }
                                if (ImGui::IsItemDeactivatedAfterEdit() &&
                                    terrainLayerEditActive_ && terrainLayerEditIndex_ == layerIndex)
                                {
                                    static_cast<void>(commandHistory_.Push(
                                        terrainLayerEditBefore_, sceneDocument_.CreateSnapshot()));
                                    terrainLayerEditBefore_ = {};
                                    terrainLayerEditIndex_ = InvalidEditorEntityIndex;
                                    terrainLayerEditActive_ = false;
                                }
                            };
                            editPlacement("Texture Size", scale, 1.0F, 0.001F, 100000.0F);
                            editPlacement("Texture Offset", offset, 0.01F, -100000.0F, 100000.0F);
                            ImGui::TextWrapped("Diffuse: %s", diffuse.c_str());
                            if (ImGui::Button("Browse Diffuse..."))
                            {
                                std::filesystem::path selected;
                                if (SelectTerrainTexture(reinterpret_cast<HWND>(
                                        GetWindow().GetNativeHandle().Value()), selected))
                                {
                                    std::filesystem::path gameRoot = std::filesystem::current_path();
                                    if (gameRoot.filename() != L"game") gameRoot /= L"game";
                                    std::error_code relativeError;
                                    const auto relative = std::filesystem::relative(
                                        selected, gameRoot, relativeError);
                                    diffuse = relativeError ? selected.generic_u8string() :
                                        relative.generic_u8string();
                                    const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                                    if (sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex, diffuse, normal, scale[0], scale[1],
                                            offset[0], offset[1], visible))
                                        static_cast<void>(commandHistory_.Push(
                                            before, sceneDocument_.CreateSnapshot()));
                                }
                            }
                            ImGui::TextWrapped("Normal: %s", normal.c_str());
                            if (ImGui::Button("Browse Normal..."))
                            {
                                std::filesystem::path selected;
                                if (SelectTerrainTexture(reinterpret_cast<HWND>(
                                        GetWindow().GetNativeHandle().Value()), selected))
                                {
                                    std::filesystem::path gameRoot = std::filesystem::current_path();
                                    if (gameRoot.filename() != L"game") gameRoot /= L"game";
                                    std::error_code relativeError;
                                    const auto relative = std::filesystem::relative(
                                        selected, gameRoot, relativeError);
                                    normal = relativeError ? selected.generic_u8string() :
                                        relative.generic_u8string();
                                    const EditorSceneSnapshot before = sceneDocument_.CreateSnapshot();
                                    if (sceneDocument_.UpdateSelectedTerrainLayer(
                                            layerIndex, diffuse, normal, scale[0], scale[1],
                                            offset[0], offset[1], visible))
                                        static_cast<void>(commandHistory_.Push(
                                            before, sceneDocument_.CreateSnapshot()));
                                }
                            }
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("No selection");
            }
            ImGui::End();

            ImGui::Begin("Content Browser");
            DrawContentBrowser();
            ImGui::End();

            ImGui::SetNextWindowBgAlpha(0.0F);
            ImGui::Begin(
                "Viewport",
                nullptr,
                ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse);
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
            ImGui::End();
        }
        else
        {
            ImGui::Begin(workspaceName);
            ImGui::Text("%s workspace", workspaceName);
            ImGui::Separator();
            ImGui::TextDisabled("Dear ImGui tool host is active.");
            ImGui::End();
        }
    }

    bool EditorApplication::ExecuteEditorCommand(const EditorCommand command)
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

    void EditorApplication::ProcessEditorShortcuts() noexcept
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput) return;

        const auto pressed = [](const ImGuiKey key)
        {
            return ImGui::IsKeyPressed(key, false);
        };

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

    void EditorApplication::RenderImGui() noexcept
    {
        if (uiSwapChain_ == nullptr ||
            commandContext_ == nullptr ||
            IsMinimized())
        {
            return;
        }

        /*
         * Сначала строим Dear ImGui.
         * На этом этапе также вычисляются реальные координаты
         * и размер docked-окна Viewport.
         */
        imguiHost_.BeginFrame();
        DrawImGuiWorkspace();

        engine::graphics::Viewport fullViewport{};

        fullViewport.x = 0.0F;
        fullViewport.y = 0.0F;
        fullViewport.width =
            static_cast<float>(uiWidth_);
        fullViewport.height =
            static_cast<float>(uiHeight_);
        fullViewport.minDepth = 0.0F;
        fullViewport.maxDepth = 1.0F;

        auto result =
            commandContext_->SetViewport(
                fullViewport);

        if (!engine::graphics::Failed(result))
        {
            result =
                commandContext_->
                    SetSwapChainRenderTarget(
                        *uiSwapChain_);
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
            result =
                commandContext_->
                    ClearSwapChainColor(
                        *uiSwapChain_,
                        clear);
        }

        /*
         * В текущей архитектуре Dear ImGui рисуется первым.
         * D3D-сцена затем заполняет только прямоугольник
         * прозрачного окна Viewport.
         */
        if (!engine::graphics::Failed(result))
        {
            imguiHost_.Render();
        }

        if (!engine::graphics::Failed(result) &&
            imguiWorkspace_ ==
                EditorLauncherAction::LevelEditor &&
            depthStencil_.IsValid())
        {
            engine::graphics::Viewport sceneViewport{};

            sceneViewport.x =
                imguiViewportX_;

            sceneViewport.y =
                imguiViewportY_;

            sceneViewport.width =
                (std::max)(
                    imguiViewportWidth_,
                    1.0F);

            sceneViewport.height =
                (std::max)(
                    imguiViewportHeight_,
                    1.0F);

            sceneViewport.minDepth = 0.0F;
            sceneViewport.maxDepth = 1.0F;

            result =
                commandContext_->SetViewport(
                    sceneViewport);

            if (!engine::graphics::Failed(result))
            {
                result =
                    commandContext_->
                        SetSwapChainRenderTarget(
                            *uiSwapChain_,
                            depthStencil_);
            }

            if (!engine::graphics::Failed(result))
            {
                result =
                    commandContext_->
                        ClearDepthStencilTarget(
                            depthStencil_,
                            engine::graphics::
                                ClearDepthStencilFlags::Depth |
                            engine::graphics::
                                ClearDepthStencilFlags::Stencil,
                            1.0F,
                            0U);
            }

            DirectX::XMFLOAT4X4 viewProjection{};

            const std::uint32_t viewportWidth =
                static_cast<std::uint32_t>(
                    (std::max)(
                        imguiViewportWidth_,
                        1.0F));

            const std::uint32_t viewportHeight =
                static_cast<std::uint32_t>(
                    (std::max)(
                        imguiViewportHeight_,
                        1.0F));

            if (!engine::graphics::Failed(result) &&
                cameraController_.BuildViewProjection(
                    viewportWidth,
                    viewportHeight,
                    viewProjection))
            {
                result =
                    gridRenderer_.Render(
                        *commandContext_,
                        viewProjection);

                if (!engine::graphics::Failed(result))
                {
                    result =
                        terrainRenderer_.Render(
                            *commandContext_,
                            sceneDocument_,
                            viewProjection,
                            cameraController_.
                                GetPosition());
                }

                if (!engine::graphics::Failed(result))
                {
                    result =
                        staticMeshRenderer_.Render(
                            *commandContext_,
                            sceneDocument_,
                            viewProjection);
                }

                if (!engine::graphics::Failed(result))
                {
                    result =
                        sceneRenderer_.Render(
                            *commandContext_,
                            sceneDocument_,
                            viewProjection,
                            transformController_.
                                GetVisualState(),
                            &staticMeshRenderer_);
                }

                if (!engine::graphics::Failed(result) &&
                    terrainPaintMode_ &&
                    terrainBrushHitValid_)
                {
                    result =
                        terrainRenderer_.RenderBrush(
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
            engine::graphics::
                PresentStatus::Failed;

        result =
            uiSwapChain_->Present(
                presentStatus);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "Present Dear ImGui",
                result);
        }
    }

    bool EditorApplication::LoadUiDocument(const char* const path)
    {
        if (path == nullptr)
        {
            return false;
        }

        if (uiDocument_ != nullptr)
        {
            launcherController_.Detach();
            levelEditorUiController_.Detach();
            uiDocument_->Close();
            uiDocument_ = nullptr;
        }

        uiDocument_ = uiHost_.LoadDocument(path);
        return uiDocument_ != nullptr;
    }

    void EditorApplication::HandleLauncherAction(
        const EditorLauncherAction action)
    {
        switch (action)
        {
            case EditorLauncherAction::LevelEditor:
            case EditorLauncherAction::CharacterEditor:
            case EditorLauncherAction::PhysicsEditor:
            case EditorLauncherAction::FbxImporter:
            case EditorLauncherAction::IconGenerator:
                pendingImGuiWorkspace_ = action;
                imguiWorkspacePending_ = true;
                break;
            case EditorLauncherAction::TestGame:
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Warning,
                    "LTS.Editor.Launcher",
                    "Test Game executable integration is not available yet.");
                break;
            case EditorLauncherAction::Exit:
                RequestExit();
                break;
            default:
                break;
        }
    }

    void EditorApplication::HandleLevelEditorAction(
        const LevelEditorUiAction action)
    {
        const auto setStatus = [this](const char* const message)
        {
            if (uiDocument_ == nullptr || message == nullptr)
            {
                return;
            }
            if (Rml::Element* const status = uiDocument_->GetElementById("status"))
            {
                status->SetInnerRML(
                    Rml::String("<span class=\"status-ok\"></span><span>") +
                    message +
                    "</span><span class=\"status-divider\"></span><span>DX11</span>"
                    "<div class=\"status-right\">LAST TO SURVIVE EDITOR</div>");
            }
        };

        const auto selectTool = [this](
            const char* const activeId,
            const EditorTransformOperation operation)
        {
            transformController_.SetOperation(operation);
            constexpr const char* ids[]{
                "select-tool", "move-tool", "rotate-tool", "scale-tool"};
            for (const char* const id : ids)
            {
                if (Rml::Element* const element = uiDocument_->GetElementById(id))
                {
                    element->SetClass("active-tool", std::strcmp(id, activeId) == 0);
                }
            }
        };

        switch (action)
        {
            case LevelEditorUiAction::Back:
                levelEditorUiActive_ = false;
                cameraController_.SetViewportWindow({});
                transformController_.SetViewportWindow({});
                if (LoadUiDocument("Launcher/EditorLauncher.rml"))
                {
                    static_cast<void>(launcherController_.Attach(
                        *uiDocument_,
                        [this](const EditorLauncherAction launcherAction)
                        {
                            HandleLauncherAction(launcherAction);
                        }));
                }
                break;
            case LevelEditorUiAction::NewLevel:
                levelDocument_.RequestNewLevel();
                setStatus("NEW LEVEL REQUESTED");
                break;
            case LevelEditorUiAction::OpenLevel:
                levelDocument_.RequestOpenLevel();
                setStatus("OPEN LEVEL");
                break;
            case LevelEditorUiAction::SaveLevel:
                levelDocument_.RequestSaveLevel();
                setStatus("SAVE LEVEL");
                break;
            case LevelEditorUiAction::Undo:
                static_cast<void>(ExecuteEditorCommand(EditorCommand::Undo));
                setStatus("UNDO");
                break;
            case LevelEditorUiAction::Redo:
                static_cast<void>(ExecuteEditorCommand(EditorCommand::Redo));
                setStatus("REDO");
                break;
            case LevelEditorUiAction::Move:
                selectTool("move-tool", EditorTransformOperation::Move);
                setStatus("MOVE TOOL");
                break;
            case LevelEditorUiAction::Rotate:
                selectTool("rotate-tool", EditorTransformOperation::Rotate);
                setStatus("ROTATE TOOL");
                break;
            case LevelEditorUiAction::Scale:
                selectTool("scale-tool", EditorTransformOperation::Scale);
                setStatus("SCALE TOOL");
                break;
            case LevelEditorUiAction::ToggleSpace:
                transformController_.ToggleSpace();
                if (Rml::Element* const element =
                        uiDocument_->GetElementById("coordinate-space"))
                {
                    const bool local = transformController_.GetVisualState().space ==
                        EditorTransformSpace::Local;
                    element->SetInnerRML(local ? "LOCAL <span>v</span>" :
                                                 "WORLD <span>v</span>");
                }
                break;
            case LevelEditorUiAction::Select:
                selectTool("select-tool", EditorTransformOperation::Select);
                setStatus("SELECT TOOL");
                break;
            case LevelEditorUiAction::Snap:
            {
                constexpr int snapValues[]{1, 10, 50, 100};
                snapSettingIndex_ = (snapSettingIndex_ + 1) % std::size(snapValues);
                if (Rml::Element* const element =
                        uiDocument_->GetElementById("snap-settings"))
                {
                    element->SetInnerRML(
                        "SNAP " + std::to_string(snapValues[snapSettingIndex_]) +
                        " <span>v</span>");
                }
                setStatus("SNAP SETTING CHANGED");
                break;
            }
            case LevelEditorUiAction::Play:
                HandleLauncherAction(EditorLauncherAction::TestGame);
                setStatus("GAME EXECUTABLE NOT FOUND");
                break;
            case LevelEditorUiAction::PlayOptions:
                playInNewWindow_ = !playInNewWindow_;
                setStatus(playInNewWindow_ ? "PLAY MODE: NEW WINDOW" :
                                            "PLAY MODE: SELECTED VIEWPORT");
                break;
            case LevelEditorUiAction::FileMenu:
                levelDocument_.RequestOpenLevel();
                setStatus("FILE: OPEN LEVEL");
                break;
            case LevelEditorUiAction::EditMenu:
                static_cast<void>(commandHistory_.Undo(sceneDocument_));
                setStatus("EDIT: UNDO");
                break;
            case LevelEditorUiAction::WindowMenu:
                setStatus("WINDOW LAYOUT ACTIVE");
                break;
            case LevelEditorUiAction::ToolsMenu:
                transformController_.ToggleSpace();
                setStatus("TOOLS: COORDINATE SPACE TOGGLED");
                break;
            case LevelEditorUiAction::BuildMenu:
                levelDocument_.RequestSaveLevel();
                setStatus("BUILD: LEVEL VALIDATION AND SAVE");
                break;
            case LevelEditorUiAction::HelpMenu:
                setStatus("HELP: Q SELECT / W MOVE / E ROTATE / R SCALE");
                break;
            default:
                break;
        }
    }

    void EditorApplication::RenderUi() noexcept
    {
        if (uiSwapChain_ == nullptr || !uiHost_.IsInitialized() ||
            commandContext_ == nullptr)
        {
            return;
        }

        engine::graphics::Viewport viewport;
        viewport.width = static_cast<float>(uiWidth_);
        viewport.height = static_cast<float>(uiHeight_);
        viewport.maxDepth = 1.0F;
        auto result = commandContext_->SetViewport(viewport);
        if (!engine::graphics::Failed(result))
        {
            result = commandContext_->SetSwapChainRenderTarget(*uiSwapChain_);
        }
        if (!engine::graphics::Failed(result))
        {
            engine::graphics::ClearColor clear;
            clear.red = 0.035F;
            clear.green = 0.045F;
            clear.blue = 0.043F;
            clear.alpha = 1.0F;
            result = commandContext_->ClearSwapChainColor(*uiSwapChain_, clear);
        }
        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure("Render UI setup", result);
            return;
        }

        uiRenderInterface_.PrepareRender();
        uiHost_.Render();
        uiRenderInterface_.FinishRender();

        if (levelEditorUiActive_ && uiDocument_ != nullptr && depthStencil_.IsValid())
        {
            Rml::Element* const viewportElement =
                uiDocument_->GetElementById("viewport");
            if (viewportElement != nullptr)
            {
                const Rml::Vector2f offset =
                    viewportElement->GetAbsoluteOffset(Rml::BoxArea::Border);
                const Rml::Vector2f size =
                    viewportElement->GetBox().GetSize(Rml::BoxArea::Border);
                const std::uint32_t sceneWidth = static_cast<std::uint32_t>(
                    std::max(size.x, 1.0F));
                const std::uint32_t sceneHeight = static_cast<std::uint32_t>(
                    std::max(size.y, 1.0F));

                engine::graphics::Viewport sceneViewport;
                sceneViewport.x = std::max(offset.x, 0.0F);
                sceneViewport.y = std::max(offset.y, 0.0F);
                sceneViewport.width = static_cast<float>(sceneWidth);
                sceneViewport.height = static_cast<float>(sceneHeight);
                sceneViewport.maxDepth = 1.0F;

                result = commandContext_->SetViewport(sceneViewport);
                if (!engine::graphics::Failed(result))
                {
                    result = commandContext_->SetSwapChainRenderTarget(
                        *uiSwapChain_, depthStencil_);
                }
                if (!engine::graphics::Failed(result))
                {
                    result = commandContext_->ClearDepthStencilTarget(
                        depthStencil_,
                        engine::graphics::ClearDepthStencilFlags::Depth |
                            engine::graphics::ClearDepthStencilFlags::Stencil,
                        1.0F,
                        0);
                }

                DirectX::XMFLOAT4X4 viewProjection{};
                if (!engine::graphics::Failed(result) &&
                    cameraController_.BuildViewProjection(
                        sceneWidth, sceneHeight, viewProjection))
                {
                    result = gridRenderer_.Render(*commandContext_, viewProjection);
                    if (!engine::graphics::Failed(result))
                    {
                        result = terrainRenderer_.Render(*commandContext_, sceneDocument_, viewProjection,
                            cameraController_.GetPosition());
                    }
                    if (!engine::graphics::Failed(result))
                    {
                        result = staticMeshRenderer_.Render(
                            *commandContext_, sceneDocument_, viewProjection);
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
                }

                if (engine::graphics::Failed(result))
                {
                    ReportGraphicsFailure("Render RML viewport scene", result);
                    return;
                }
            }
        }

        commandContext_->UnbindRenderTargets();

        engine::graphics::PresentStatus status =
            engine::graphics::PresentStatus::Failed;
        result = uiSwapChain_->Present(status);
        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure("Present UI", result);
        }
    }

    bool EditorApplication::InitializeGraphics() noexcept
    {
        const engine::platform::WindowSize viewportSize =
            editorShell_.GetViewportSize();

        viewportWidth_ = std::max(viewportSize.width, 1U);
        viewportHeight_ = std::max(viewportSize.height, 1U);

        engine::graphics::RenderDeviceDesc deviceDescription;

        deviceDescription.backend =
            engine::graphics::GraphicsBackend::D3D11;

        deviceDescription.enableValidation =
            GetEngine().GetConfig().enableValidation;

        deviceDescription.enableDebugMarkers = true;
        deviceDescription.forceSoftwareAdapter = false;

        auto result =
            graphicsDevice_.Initialize(deviceDescription);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "D3D11Device::Initialize",
                result);

            ShutdownGraphics();
            return false;
        }

        commandContext_ =
            graphicsDevice_.GetImmediateCommandContext();

        if (commandContext_ == nullptr)
        {
            ReportGraphicsFailure(
                "GetImmediateCommandContext",
                engine::graphics::GraphicsResult::InvalidState);

            ShutdownGraphics();
            return false;
        }

        engine::graphics::SwapChainDesc swapChainDescription;

        swapChainDescription.window =
            editorShell_.GetViewportWindowHandle();

        swapChainDescription.width = viewportWidth_;
        swapChainDescription.height = viewportHeight_;
        swapChainDescription.bufferCount = 2;
        swapChainDescription.format =
            engine::graphics::Format::B8G8R8A8UNorm;

        swapChainDescription.presentMode =
            engine::graphics::PresentMode::VSync;

        swapChainDescription.windowed = true;
        swapChainDescription.allowModeSwitch = false;
        swapChainDescription.enableTearing = false;

        result = graphicsDevice_.CreateSwapChain(
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

        result = CreateDepthStencil(
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

        if (!gridRenderer_.Initialize(graphicsDevice_))
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

    void EditorApplication::ShutdownGraphics() noexcept
    {
        graphicsReady_ = false;

        if (commandContext_ != nullptr)
        {
            commandContext_->UnbindRenderTargets();
            commandContext_->ClearState();
            commandContext_->Flush();
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

    void EditorApplication::DestroyDepthStencil() noexcept
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

    void EditorApplication::ResizeGraphics(
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (
            !graphicsReady_ ||
            swapChain_ == nullptr ||
            commandContext_ == nullptr ||
            width == 0U ||
            height == 0U)
        {
            return;
        }

        if (
            width == viewportWidth_ &&
            height == viewportHeight_)
        {
            return;
        }

        commandContext_->UnbindRenderTargets();
        DestroyDepthStencil();

        auto result = swapChain_->Resize(width, height);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "SwapChain::Resize",
                result);
            return;
        }

        viewportWidth_ = width;
        viewportHeight_ = height;

        result = CreateDepthStencil(width, height);

        if (engine::graphics::Failed(result))
        {
            ReportGraphicsFailure(
                "Resize CreateDepthStencil",
                result);
            return;
        }

        swapChainOccluded_ = false;
    }

    void EditorApplication::ReportGraphicsFailure(
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
