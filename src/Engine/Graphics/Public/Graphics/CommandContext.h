#pragma once

#include "Graphics/Buffer.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/ResourceHandle.h"
#include "Graphics/Sampler.h"
#include "Graphics/Shader.h"
#include "Graphics/Viewport.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    class SwapChain;

    inline constexpr std::size_t MaxVertexBufferSlots = 16U;
    inline constexpr std::size_t MaxShaderResourceSlots = 128U;
    inline constexpr std::size_t MaxSamplerSlots = 16U;
    inline constexpr std::size_t MaxConstantBufferSlots = 14U;

    struct ClearColor final
    {
        float red = 0.0F;
        float green = 0.0F;
        float blue = 0.0F;
        float alpha = 1.0F;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] constexpr std::array<float, 4U> ToArray() const noexcept
        {
            return {red, green, blue, alpha};
        }
    };

    enum class ClearDepthStencilFlags : std::uint8_t
    {
        None = 0,
        Depth = 1U << 0U,
        Stencil = 1U << 1U
    };

    [[nodiscard]] constexpr ClearDepthStencilFlags operator|(
        const ClearDepthStencilFlags left,
        const ClearDepthStencilFlags right) noexcept
    {
        return static_cast<ClearDepthStencilFlags>(
            static_cast<std::uint8_t>(left) |
            static_cast<std::uint8_t>(right));
    }

    [[nodiscard]] constexpr ClearDepthStencilFlags operator&(
        const ClearDepthStencilFlags left,
        const ClearDepthStencilFlags right) noexcept
    {
        return static_cast<ClearDepthStencilFlags>(
            static_cast<std::uint8_t>(left) &
            static_cast<std::uint8_t>(right));
    }

    [[nodiscard]] constexpr bool HasAnyFlag(
        const ClearDepthStencilFlags value,
        const ClearDepthStencilFlags flags) noexcept
    {
        return (value & flags) != ClearDepthStencilFlags::None;
    }

    struct VertexBufferBinding final
    {
        BufferHandle buffer;
        std::uint32_t stride = 0U;
        std::uint32_t offset = 0U;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return buffer.IsValid() && stride != 0U;
        }
    };

    struct IndexBufferBinding final
    {
        BufferHandle buffer;
        std::uint32_t offset = 0U;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return buffer.IsValid();
        }
    };

    // Backend-neutral immediate command interface.
    //
    // The first implementation is the D3D11 immediate context. The contract is
    // deliberately small and maps to the common operations required by the
    // initial Studio shell and by later render passes.
    class CommandContext
    {
    public:
        virtual ~CommandContext() noexcept = default;

        CommandContext(const CommandContext&) = delete;
        CommandContext& operator=(const CommandContext&) = delete;

        CommandContext(CommandContext&&) = delete;
        CommandContext& operator=(CommandContext&&) = delete;

        [[nodiscard]] virtual bool IsValid() const noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetViewport(
            const Viewport& viewport) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetScissorRect(
            const ScissorRect& scissorRect) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetSwapChainRenderTarget(
            SwapChain& swapChain) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetSwapChainRenderTarget(
            SwapChain& swapChain,
            TextureHandle depthStencilTarget) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetRenderTargets(
            const TextureHandle* colorTargets,
            std::size_t colorTargetCount,
            TextureHandle depthStencilTarget) noexcept = 0;

        virtual void UnbindRenderTargets() noexcept = 0;

        [[nodiscard]] virtual GraphicsResult ClearSwapChainColor(
            SwapChain& swapChain,
            const ClearColor& color) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult ClearColorTarget(
            TextureHandle texture,
            const ClearColor& color) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult ClearDepthStencilTarget(
            TextureHandle texture,
            ClearDepthStencilFlags flags,
            float depth,
            std::uint8_t stencil) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept = 0;

        virtual void UnbindGraphicsPipeline() noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetVertexBuffers(
            std::uint32_t firstSlot,
            const VertexBufferBinding* bindings,
            std::size_t bindingCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetIndexBuffer(
            const IndexBufferBinding& binding) noexcept = 0;

        virtual void UnbindIndexBuffer() noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetShaderResources(
            ShaderStage stage,
            std::uint32_t firstSlot,
            const TextureHandle* textures,
            std::size_t textureCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult UnbindShaderResources(
            ShaderStage stage,
            std::uint32_t firstSlot,
            std::size_t textureCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetSamplers(
            ShaderStage stage,
            std::uint32_t firstSlot,
            const SamplerHandle* samplers,
            std::size_t samplerCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult UnbindSamplers(
            ShaderStage stage,
            std::uint32_t firstSlot,
            std::size_t samplerCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult SetConstantBuffers(
            ShaderStage stage,
            std::uint32_t firstSlot,
            const BufferHandle* buffers,
            std::size_t bufferCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult UnbindConstantBuffers(
            ShaderStage stage,
            std::uint32_t firstSlot,
            std::size_t bufferCount) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult UpdateBuffer(
            BufferHandle buffer,
            const void* data,
            std::size_t dataSize) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult Draw(
            std::uint32_t vertexCount,
            std::uint32_t firstVertex) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DrawIndexed(
            std::uint32_t indexCount,
            std::uint32_t firstIndex,
            std::int32_t baseVertex) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult DrawIndexedInstanced(
            std::uint32_t,
            std::uint32_t,
            std::uint32_t,
            std::int32_t,
            std::uint32_t) noexcept
        {
            return GraphicsResult::Unsupported;
        }

        virtual void ClearState() noexcept = 0;
        virtual void Flush() noexcept = 0;

    protected:
        CommandContext() noexcept = default;
    };
}
