#pragma once

#include "D3D11HandlePool.h"
#include "D3D11ResourceTypes.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/InputLayout.h"
#include "Graphics/PipelineState.h"
#include "Graphics/ResourceHandle.h"
#include "Graphics/Sampler.h"
#include "Graphics/Shader.h"

#include <cstddef>

struct ID3D11Device;

namespace engine::graphics::d3d11::detail
{
    class D3D11ResourceRegistry final
    {
    public:
        [[nodiscard]] GraphicsResult CreateTexture(
            ID3D11Device* device,
            const TextureDesc& desc,
            const TextureSubresourceData* initialData,
            std::size_t initialDataCount,
            TextureHandle& outTexture) noexcept;

        [[nodiscard]] GraphicsResult DestroyTexture(
            TextureHandle texture) noexcept;

        [[nodiscard]] GraphicsResult CreateBuffer(
            ID3D11Device* device,
            const BufferDesc& desc,
            const BufferInitialData* initialData,
            BufferHandle& outBuffer) noexcept;

        [[nodiscard]] GraphicsResult DestroyBuffer(
            BufferHandle buffer) noexcept;

        [[nodiscard]] GraphicsResult CreateSampler(
            ID3D11Device* device,
            const SamplerDesc& desc,
            SamplerHandle& outSampler) noexcept;

        [[nodiscard]] GraphicsResult DestroySampler(
            SamplerHandle sampler) noexcept;

        [[nodiscard]] GraphicsResult CreateShader(
            ID3D11Device* device,
            const ShaderDesc& desc,
            ShaderHandle& outShader) noexcept;

        [[nodiscard]] GraphicsResult DestroyShader(
            ShaderHandle shader) noexcept;

        [[nodiscard]] GraphicsResult CreateInputLayout(
            ID3D11Device* device,
            const InputLayoutDesc& desc,
            InputLayoutHandle& outInputLayout) noexcept;

        [[nodiscard]] GraphicsResult DestroyInputLayout(
            InputLayoutHandle inputLayout) noexcept;

        [[nodiscard]] GraphicsResult CreateGraphicsPipeline(
            ID3D11Device* device,
            const GraphicsPipelineDesc& desc,
            PipelineStateHandle& outPipeline) noexcept;

        [[nodiscard]] GraphicsResult DestroyGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept;

        void Clear() noexcept;

        [[nodiscard]] const D3D11TextureResource* GetTexture(
            TextureHandle texture) const noexcept;

        [[nodiscard]] const D3D11BufferResource* GetBuffer(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] const D3D11SamplerResource* GetSampler(
            SamplerHandle sampler) const noexcept;

        [[nodiscard]] const D3D11ShaderResource* GetShader(
            ShaderHandle shader) const noexcept;

        [[nodiscard]] const D3D11InputLayoutResource* GetInputLayout(
            InputLayoutHandle inputLayout) const noexcept;

        [[nodiscard]] const D3D11GraphicsPipelineResource*
            GetGraphicsPipeline(
                PipelineStateHandle pipeline) const noexcept;

        [[nodiscard]] std::size_t GetTextureCount() const noexcept;
        [[nodiscard]] std::size_t GetBufferCount() const noexcept;
        [[nodiscard]] std::size_t GetSamplerCount() const noexcept;
        [[nodiscard]] std::size_t GetShaderCount() const noexcept;
        [[nodiscard]] std::size_t GetInputLayoutCount() const noexcept;
        [[nodiscard]] std::size_t GetGraphicsPipelineCount() const noexcept;

    private:
        HandlePool<TextureHandle, D3D11TextureResource> textures_;
        HandlePool<BufferHandle, D3D11BufferResource> buffers_;
        HandlePool<SamplerHandle, D3D11SamplerResource> samplers_;
        HandlePool<ShaderHandle, D3D11ShaderResource> shaders_;
        HandlePool<InputLayoutHandle, D3D11InputLayoutResource>
            inputLayouts_;
        HandlePool<PipelineStateHandle, D3D11GraphicsPipelineResource>
            graphicsPipelines_;
    };
}
