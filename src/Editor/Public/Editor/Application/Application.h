#pragma once

#include "Editor/LevelEditor/Viewport/CameraController.h"
#include "Editor/Commands/CommandHistory.h"
#include "Editor/Commands/CommandSystem.h"
#include "Editor/LevelEditor/UI/ContentBrowserPanel.h"
#include "Editor/LevelEditor/Rendering/GridRenderer.h"
#include "Editor/LevelEditor/Rendering/SkyRenderer.h"
#include "Editor/LevelEditor/Documents/LevelDocument.h"
#include "Editor/LevelEditor/UI/DockLayout.h"
#include "Editor/LevelEditor/UI/WorldOutlinerPanel.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Rendering/SceneRenderer.h"
#include "Editor/LevelEditor/Rendering/StaticMeshRenderer.h"
#include "Editor/LevelEditor/Viewport/TransformController.h"
#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"
#include "Editor/LevelEditor/Terrain/TerrainImporter.h"
#include "Editor/Tools/ToolWindowManager.h"

#include "Editor/LevelEditor/UI/MaterialInspector.h"

#include <Application/Application.h>

#include <Graphics/CommandContext.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/SwapChain.h>

#include <GraphicsDX11/D3D11Device.h>
#include <ImGui/ImGuiHost.h>

#include <cstdint>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace lts::editor
{
    class Application final :
        public lts::application::Application
    {
    public:
        Application();
        ~Application() noexcept override;

        Application(
            const Application&) = delete;

        Application& operator=(
            const Application&) = delete;

    protected:
        [[nodiscard]]
        lts::application::ApplicationResult
            OnInitialize() noexcept override;

        void OnShutdown() noexcept override;

        void OnUpdate(
            double deltaSeconds) noexcept override;

        void OnRender() noexcept override;

        void OnEvent(
            const lts::application::
                ApplicationEvent& event) noexcept override;

        [[nodiscard]] bool OnNativeMessage(
            void* nativeWindow,
            std::uint32_t message,
            std::uintptr_t wordParameter,
            std::intptr_t longParameter) noexcept override;

    private:
        [[nodiscard]]
        bool InitializeGraphics() noexcept;

        [[nodiscard]]
        bool InitializeEditorUi() noexcept;
        void ShutdownEditorUi() noexcept;
        void RenderImGui() noexcept;
        void DrawImGuiWorkspace() noexcept;
        
        void LaunchTestGame() noexcept;
        void ProcessEditorShortcuts() noexcept;
        bool ExecuteEditorCommand(EditorCommand command);

        void ShutdownGraphics() noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult
            CreateDepthStencil(
                std::uint32_t width,
                std::uint32_t height) noexcept;

        void DestroyDepthStencil() noexcept;

        void ResizeEditorUi(
            std::uint32_t width,
            std::uint32_t height) noexcept;

        void ReportGraphicsFailure(
            const char* operation,
            engine::graphics::
                GraphicsResult result) noexcept;

        SceneDocument sceneDocument_;
        LevelDocument levelDocument_;
        CommandHistory commandHistory_;
        CommandSystem commandSystem_;
        CameraController cameraController_;
        TransformController transformController_;
        GridRenderer gridRenderer_;
        SkyRenderer skyRenderer_;
        StaticMeshRenderer staticMeshRenderer_;
        TerrainRenderer terrainRenderer_;
        TerrainImporter terrainImporter_;
        SceneRenderer sceneRenderer_;
        MaterialInspector materialInspector_;
        
        engine::graphics::d3d11::
            D3D11Device graphicsDevice_;

        std::unique_ptr<engine::graphics::SwapChain>
        uiSwapChain_;

        engine::ui::ImGuiHost imguiHost_;

        DockLayout levelEditorLayout_;
        ToolWindowManager toolWindowManager_;

        std::uint32_t uiWidth_ = 0U;
        std::uint32_t uiHeight_ = 0U;
        
        float imguiViewportX_ = 0.0F;
        float imguiViewportY_ = 0.0F;
        float imguiViewportWidth_ = 1.0F;
        float imguiViewportHeight_ = 1.0F;

        /*
         * true только когда dock-вкладка Viewport
         * действительно отображается.
         */
        bool imguiViewportVisible_ = false;
        
        std::filesystem::path loadedTerrainAssetPath_;
        std::filesystem::path failedTerrainAssetPath_;
        std::array<char, 256U> entityRenameBuffer_{};
        EditorEntityId renameEntityId_ = 0U;
        WorldOutlinerPanel worldOutlinerPanel_;
        ContentBrowserPanel contentBrowserPanel_;
        EditorSceneSnapshot inspectorEditBefore_;
        EditorTransform transformClipboard_;
        bool inspectorEditActive_ = false;
        EditorSceneSnapshot terrainLayerEditBefore_;
        std::size_t terrainLayerEditIndex_ = InvalidEditorEntityIndex;
        bool terrainLayerEditActive_ = false;
        bool terrainPaintMode_ = false;
        bool terrainPaintStrokeActive_ = false;
        std::size_t terrainPaintLayer_ = 0U;
        float terrainBrushRadius_ = 64.0F;
        float terrainBrushStrength_ = 0.25F;
        float terrainBrushFalloff_ = 0.5F;
        float terrainBrushWorldX_ = 0.0F;
        float terrainBrushWorldZ_ = 0.0F;
        bool terrainBrushHitValid_ = false;
        bool transformClipboardValid_ = false;

        engine::graphics::CommandContext*
            commandContext_ = nullptr;

        engine::graphics::TextureHandle
            depthStencil_;
        
        bool graphicsFailureReported_ = false;
    };
}
