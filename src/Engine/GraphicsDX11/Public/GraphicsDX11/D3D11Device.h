#pragma once

#include "Graphics/RenderDevice.h"
#include "GraphicsDX11/D3D11Context.h"

#include <cstddef>
#include <cstdint>
#include <memory>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Resource;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;
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

        void Shutdown() noexcept override;

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

        [[nodiscard]] ID3D11Device* GetNativeDevice() const noexcept;
        [[nodiscard]] ID3D11DeviceContext*
            GetNativeImmediateContext() const noexcept;
        [[nodiscard]] IDXGIFactory2* GetNativeFactory() const noexcept;
        [[nodiscard]] D3D11Context* GetImmediateContext() noexcept;
        [[nodiscard]] const D3D11Context* GetImmediateContext() const noexcept;

        [[nodiscard]] std::uint32_t GetFeatureLevel() const noexcept;
        [[nodiscard]] bool IsTearingSupported() const noexcept;

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

        [[nodiscard]] std::size_t GetTextureCount() const noexcept;
        [[nodiscard]] std::size_t GetBufferCount() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
