#pragma once

#include "Editor/EditorAssetBrowserPanel.h"
#include "Editor/EditorCameraController.h"
#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorCommandSystem.h"
#include "Editor/EditorGridRenderer.h"
#include "Editor/EditorInspectorPanel.h"
#include "Editor/EditorLevelDocument.h"
#include "Editor/EditorLauncherController.h"
#include "Editor/LevelEditorUiController.h"
#include "Editor/LevelEditorLayout.h"
#include "Editor/EditorSceneDocument.h"
#include "Editor/EditorSceneRenderer.h"
#include "Editor/EditorShell.h"
#include "Editor/EditorStaticMeshRenderer.h"
#include "Editor/EditorTransformController.h"

#include <Application/Application.h>

#include <Graphics/CommandContext.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/SwapChain.h>

#include <GraphicsDX11/D3D11Device.h>
#include <GraphicsDX11/RmlUiRenderInterfaceDX11.h>
#include <UI/RmlUiHost.h>
#include <ImGui/ImGuiHost.h>

#include <cstdint>
#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace lts::editor
{
    class EditorApplication final :
        public lts::application::Application
    {
    public:
        EditorApplication();
        ~EditorApplication() noexcept override;

        EditorApplication(
            const EditorApplication&) = delete;

        EditorApplication& operator=(
            const EditorApplication&) = delete;

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
        [[nodiscard]] bool InitializeUi() noexcept;
        [[nodiscard]] bool InitializeLauncherUi() noexcept;
        [[nodiscard]] bool ReturnToLauncher() noexcept;
        void ShutdownUi() noexcept;
        void RenderUi() noexcept;
        void RenderImGui() noexcept;
        void DrawImGuiWorkspace() noexcept;
        void ProcessEditorShortcuts() noexcept;
        bool ExecuteEditorCommand(EditorCommand command);
        void RefreshContentBrowser() noexcept;
        void DrawContentBrowser() noexcept;
        [[nodiscard]] bool StartImGuiWorkspace(EditorLauncherAction action) noexcept;
        void HandleLauncherAction(EditorLauncherAction action);
        void HandleLevelEditorAction(LevelEditorUiAction action);
        [[nodiscard]] bool LoadUiDocument(const char* path);

        void ShutdownGraphics() noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult
            CreateDepthStencil(
                std::uint32_t width,
                std::uint32_t height) noexcept;

        void DestroyDepthStencil() noexcept;

        void ResizeGraphics(
            std::uint32_t width,
            std::uint32_t height) noexcept;

        void ReportGraphicsFailure(
            const char* operation,
            engine::graphics::
                GraphicsResult result) noexcept;

        EditorSceneDocument sceneDocument_;
        EditorLevelDocument levelDocument_;
        EditorCommandHistory commandHistory_;
        EditorCommandSystem commandSystem_;

        EditorCameraController cameraController_;

        EditorTransformController
            transformController_;

        EditorGridRenderer gridRenderer_;

        EditorStaticMeshRenderer
            staticMeshRenderer_;

        EditorSceneRenderer sceneRenderer_;

        EditorShell editorShell_;
        EditorInspectorPanel inspectorPanel_;

        EditorAssetBrowserPanel
            assetBrowserPanel_;

        engine::graphics::d3d11::
            D3D11Device graphicsDevice_;

        std::unique_ptr<
            engine::graphics::SwapChain>
                swapChain_;

        std::unique_ptr<engine::graphics::SwapChain> uiSwapChain_;
        engine::graphics::d3d11::RmlUiRenderInterfaceDX11 uiRenderInterface_;
        engine::ui::RmlUiHost uiHost_;
        engine::ui::ImGuiHost imguiHost_;
        EditorLauncherController launcherController_;
        LevelEditorUiController levelEditorUiController_;
        LevelEditorLayout levelEditorLayout_;
        Rml::ElementDocument* uiDocument_ = nullptr;
        std::uint32_t uiWidth_ = 0;
        std::uint32_t uiHeight_ = 0;
        bool levelEditorUiActive_ = false;
        std::size_t snapSettingIndex_ = 1;
        bool playInNewWindow_ = false;
        EditorLauncherAction imguiWorkspace_ = EditorLauncherAction::LevelEditor;
        EditorLauncherAction pendingImGuiWorkspace_ = EditorLauncherAction::LevelEditor;
        bool imguiWorkspacePending_ = false;
        bool returnToLauncherPending_ = false;
        float imguiViewportX_ = 0.0F;
        float imguiViewportY_ = 0.0F;
        float imguiViewportWidth_ = 1.0F;
        float imguiViewportHeight_ = 1.0F;
        std::filesystem::path contentMeshesRoot_;
        std::filesystem::path contentSelectedDirectory_;
        std::vector<std::filesystem::path> contentMeshFiles_;
        std::array<char, 128U> contentSearch_{};
        std::array<char, 256U> entityRenameBuffer_{};
        EditorEntityId renameEntityId_ = 0U;
        std::uint32_t outlinerFolderCounter_ = 1U;

        engine::graphics::CommandContext*
            commandContext_ = nullptr;

        engine::graphics::TextureHandle
            depthStencil_;

        std::uint32_t viewportWidth_ = 0;
        std::uint32_t viewportHeight_ = 0;

        bool graphicsReady_ = false;
        bool graphicsFailureReported_ = false;
        bool swapChainOccluded_ = false;
    };
}
