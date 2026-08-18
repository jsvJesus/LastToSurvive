#pragma once

#include <RmlUi/Core/RenderInterface.h>

#include <filesystem>
#include <memory>

namespace engine::graphics::d3d11
{
    class D3D11Device;

    class RmlUiRenderInterfaceDX11 final : public Rml::RenderInterface
    {
    public:
        RmlUiRenderInterfaceDX11();
        ~RmlUiRenderInterfaceDX11() noexcept override;

        RmlUiRenderInterfaceDX11(const RmlUiRenderInterfaceDX11&) = delete;
        RmlUiRenderInterfaceDX11& operator=(const RmlUiRenderInterfaceDX11&) = delete;

        [[nodiscard]] bool Initialize(
            D3D11Device& device,
            const std::filesystem::path& shaderPath,
            int viewportWidth,
            int viewportHeight) noexcept;
        void Shutdown() noexcept;
        void SetViewportSize(int width, int height) noexcept;
        void PrepareRender() noexcept;
        void FinishRender() noexcept;

        Rml::CompiledGeometryHandle CompileGeometry(
            Rml::Span<const Rml::Vertex> vertices,
            Rml::Span<const int> indices) override;
        void RenderGeometry(
            Rml::CompiledGeometryHandle geometry,
            Rml::Vector2f translation,
            Rml::TextureHandle texture) override;
        void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

        Rml::TextureHandle LoadTexture(
            Rml::Vector2i& dimensions,
            const Rml::String& source) override;
        Rml::TextureHandle GenerateTexture(
            Rml::Span<const Rml::byte> source,
            Rml::Vector2i dimensions) override;
        void ReleaseTexture(Rml::TextureHandle texture) override;

        void EnableScissorRegion(bool enable) override;
        void SetScissorRegion(Rml::Rectanglei region) override;
        void SetTransform(const Rml::Matrix4f* transform) override;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
