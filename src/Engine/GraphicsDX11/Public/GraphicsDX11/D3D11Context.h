#pragma once

#include "Graphics/CommandContext.h"

struct ID3D11DeviceContext;

namespace engine::graphics::d3d11::detail
{
    class D3D11ResourceRegistry;
}

namespace engine::graphics::d3d11
{
    // Lightweight non-owning view over the immediate D3D11 context.
    // Lifetime is controlled by D3D11Device.
    class D3D11Context final : public CommandContext
    {
    public:
        D3D11Context() noexcept = default;

        D3D11Context(const D3D11Context&) = delete;
        D3D11Context& operator=(const D3D11Context&) = delete;

        [[nodiscard]] bool IsValid() const noexcept override;
        [[nodiscard]] ID3D11DeviceContext* GetNativeContext() const noexcept;

        [[nodiscard]] GraphicsResult SetViewport(
            const Viewport& viewport) noexcept override;

        [[nodiscard]] GraphicsResult SetScissorRect(
            const ScissorRect& scissorRect) noexcept override;

        [[nodiscard]] GraphicsResult SetSwapChainRenderTarget(
            SwapChain& swapChain) noexcept override;

        [[nodiscard]] GraphicsResult SetSwapChainRenderTarget(
            SwapChain& swapChain,
            TextureHandle depthStencilTarget) noexcept override;

        [[nodiscard]] GraphicsResult SetRenderTargets(
            const TextureHandle* colorTargets,
            std::size_t colorTargetCount,
            TextureHandle depthStencilTarget) noexcept override;

        void UnbindRenderTargets() noexcept override;

        [[nodiscard]] GraphicsResult ClearSwapChainColor(
            SwapChain& swapChain,
            const ClearColor& color) noexcept override;

        [[nodiscard]] GraphicsResult ClearColorTarget(
            TextureHandle texture,
            const ClearColor& color) noexcept override;

        [[nodiscard]] GraphicsResult ClearDepthStencilTarget(
            TextureHandle texture,
            ClearDepthStencilFlags flags,
            float depth,
            std::uint8_t stencil) noexcept override;

        [[nodiscard]] GraphicsResult SetGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept override;

        [[nodiscard]] GraphicsResult BindGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept;

        void UnbindGraphicsPipeline() noexcept override;

        [[nodiscard]] GraphicsResult SetVertexBuffers(
            std::uint32_t firstSlot,
            const VertexBufferBinding* bindings,
            std::size_t bindingCount) noexcept override;

        [[nodiscard]] GraphicsResult SetIndexBuffer(
            const IndexBufferBinding& binding) noexcept override;

        void UnbindIndexBuffer() noexcept override;

        [[nodiscard]] GraphicsResult Draw(
            std::uint32_t vertexCount,
            std::uint32_t firstVertex) noexcept override;

        [[nodiscard]] GraphicsResult DrawIndexed(
            std::uint32_t indexCount,
            std::uint32_t firstIndex,
            std::int32_t baseVertex) noexcept override;

        void ClearState() noexcept override;
        void Flush() noexcept override;

    private:
        friend class D3D11Device;

        void Attach(
            ID3D11DeviceContext* context,
            detail::D3D11ResourceRegistry* resources) noexcept;

        void Detach() noexcept;

        ID3D11DeviceContext* context_ = nullptr;
        detail::D3D11ResourceRegistry* resources_ = nullptr;
    };
}
