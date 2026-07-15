#pragma once

#include "Graphics/RenderDevice.h"
#include "GraphicsDX11/D3D11Context.h"

#include <cstddef>
#include <cstdint>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11DeviceChild;
struct ID3D11Resource;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;
struct ID3D11InputLayout;
struct IDXGIFactory2;

namespace engine::graphics::d3d11
{
    // Primary graphics backend. Unlike the temporary DX9 adapter, this class
    // owns the native device and immediate context created during Initialize.
    class D3D11Device final : public RenderDevice
    {
    public:
        D3D11Device();
        ~D3D11Device() noexcept override;

        D3D11Device(const D3D11Device&) = delete;
        D3D11Device& operator=(const D3D11Device&) = delete;

        D3D11Device(D3D11Device&&) = delete;
        D3D11Device& operator=(D3D11Device&&) = delete;

        [[nodiscard]] GraphicsBackend GetBackend() const noexcept override;
        [[nodiscard]] DeviceState GetState() const noexcept override;

        [[nodiscard]] GraphicsResult Initialize(
            const RenderDeviceDesc& desc) noexcept override;

        [[nodiscard]] GraphicsResult AttachExternal(
            ID3D11Device* device,
            ID3D11DeviceContext* immediateContext,
            const RenderDeviceDesc& desc) noexcept;

        void Shutdown() noexcept override;

        [[nodiscard]] CommandContext*
            GetImmediateCommandContext() noexcept override
        {
            return GetImmediateContext();
        }

        [[nodiscard]] const CommandContext*
            GetImmediateCommandContext() const noexcept override
        {
            return GetImmediateContext();
        }

        [[nodiscard]] GraphicsResult CreateSwapChain(
            const SwapChainDesc& desc,
            std::unique_ptr<SwapChain>& outSwapChain) noexcept override;

        [[nodiscard]] GraphicsResult CreateTexture(
            const TextureDesc& desc,
            const TextureSubresourceData* initialData,
            std::size_t initialDataCount,
            TextureHandle& outTexture) noexcept override;

        [[nodiscard]] GraphicsResult DestroyTexture(
            TextureHandle texture) noexcept override;

        [[nodiscard]] GraphicsResult CreateBuffer(
            const BufferDesc& desc,
            const BufferInitialData* initialData,
            BufferHandle& outBuffer) noexcept override;

        [[nodiscard]] GraphicsResult DestroyBuffer(
            BufferHandle buffer) noexcept override;

        [[nodiscard]] GraphicsResult CreateSampler(
            const SamplerDesc& desc,
            SamplerHandle& outSampler) noexcept override;

        [[nodiscard]] GraphicsResult DestroySampler(
            SamplerHandle sampler) noexcept override;

        [[nodiscard]] GraphicsResult CreateShader(
            const ShaderDesc& desc,
            ShaderHandle& outShader) noexcept override;

        [[nodiscard]] GraphicsResult DestroyShader(
            ShaderHandle shader) noexcept override;

        [[nodiscard]] GraphicsResult CreateInputLayout(
            const InputLayoutDesc& desc,
            InputLayoutHandle& outInputLayout) noexcept override;

        [[nodiscard]] GraphicsResult DestroyInputLayout(
            InputLayoutHandle inputLayout) noexcept override;

        [[nodiscard]] GraphicsResult CreateGraphicsPipeline(
            const GraphicsPipelineDesc& desc,
            PipelineStateHandle& outPipeline) noexcept override;

        [[nodiscard]] GraphicsResult DestroyGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept override;

        [[nodiscard]] ID3D11Device* GetNativeDevice() const noexcept;
        [[nodiscard]] ID3D11DeviceContext*
            GetNativeImmediateContext() const noexcept;
        [[nodiscard]] IDXGIFactory2* GetNativeFactory() const noexcept;
        [[nodiscard]] D3D11Context* GetImmediateContext() noexcept;
        [[nodiscard]] const D3D11Context* GetImmediateContext() const noexcept;

        [[nodiscard]] std::uint32_t GetFeatureLevel() const noexcept;
        [[nodiscard]] bool IsTearingSupported() const noexcept;
        [[nodiscard]] bool IsDebugLayerEnabled() const noexcept;

        // Checks GetDeviceRemovedReason and updates DeviceState when needed.
        [[nodiscard]] GraphicsResult CheckDeviceStatus() noexcept;

        [[nodiscard]] ID3D11Resource* GetNativeTexture(
            TextureHandle texture) const noexcept;
        [[nodiscard]] ID3D11ShaderResourceView* GetTextureShaderResourceView(
            TextureHandle texture) const noexcept;
        [[nodiscard]] ID3D11RenderTargetView* GetTextureRenderTargetView(
            TextureHandle texture) const noexcept;
        [[nodiscard]] ID3D11DepthStencilView* GetTextureDepthStencilView(
            TextureHandle texture) const noexcept;
        [[nodiscard]] ID3D11UnorderedAccessView* GetTextureUnorderedAccessView(
            TextureHandle texture) const noexcept;

        [[nodiscard]] ID3D11Buffer* GetNativeBuffer(
            BufferHandle buffer) const noexcept;
        [[nodiscard]] ID3D11ShaderResourceView* GetBufferShaderResourceView(
            BufferHandle buffer) const noexcept;
        [[nodiscard]] ID3D11UnorderedAccessView* GetBufferUnorderedAccessView(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] ID3D11DeviceChild* GetNativeShader(
            ShaderHandle shader) const noexcept;

        [[nodiscard]] ID3D11InputLayout* GetNativeInputLayout(
            InputLayoutHandle inputLayout) const noexcept;

        [[nodiscard]] std::size_t GetTextureCount() const noexcept;
        [[nodiscard]] std::size_t GetBufferCount() const noexcept;
        [[nodiscard]] std::size_t GetSamplerCount() const noexcept;
        [[nodiscard]] std::size_t GetShaderCount() const noexcept;
        [[nodiscard]] std::size_t GetInputLayoutCount() const noexcept;
        [[nodiscard]] std::size_t GetGraphicsPipelineCount() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
