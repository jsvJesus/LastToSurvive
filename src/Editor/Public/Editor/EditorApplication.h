#pragma once

#include "Editor/EditorShell.h"

#include <Application/Application.h>

#include <Graphics/CommandContext.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/SwapChain.h>

#include <GraphicsDX11/D3D11Device.h>

#include <cstdint>
#include <memory>

namespace lts::editor
{
    class EditorApplication final
        : public lts::application::Application
    {
    public:
        EditorApplication();

        ~EditorApplication() noexcept override;

        EditorApplication(
            const EditorApplication&) = delete;

        EditorApplication&
            operator=(
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

    private:
        [[nodiscard]]
        bool InitializeGraphics() noexcept;

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

        EditorShell editorShell_;

        engine::graphics::d3d11::
            D3D11Device graphicsDevice_;

        std::unique_ptr<
            engine::graphics::SwapChain>
                swapChain_;

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