#pragma once

#include "Graphics/RenderDevice.h"

#include <cstddef>
#include <cstdint>
#include <memory>

struct IDirect3DDevice9;
struct IDirect3DBaseTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

namespace engine::graphics::d3d9
{
    // Compatibility adapter over the already existing Studio D3D9 device.
    // The native device is non-owning: the legacy renderer remains its owner. by
    class D3D9Device final : public RenderDevice
    {
    public:
        D3D9Device();
        ~D3D9Device() noexcept override;

        D3D9Device(const D3D9Device&) = delete;
        D3D9Device& operator=(const D3D9Device&) = delete;

        D3D9Device(D3D9Device&&) = delete;
        D3D9Device& operator=(D3D9Device&&) = delete;

        [[nodiscard]] GraphicsResult AttachExternalDevice(
            IDirect3DDevice9* device) noexcept;

        [[nodiscard]] IDirect3DDevice9* DetachExternalDevice() noexcept;

        [[nodiscard]] bool IsExternalDeviceAttached() const noexcept;

        [[nodiscard]] IDirect3DDevice9* GetNativeDevice() const noexcept;

        [[nodiscard]] IDirect3DBaseTexture9* GetNativeTexture(
            TextureHandle texture) const noexcept;

        [[nodiscard]] IDirect3DVertexBuffer9* GetNativeVertexBuffer(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] IDirect3DIndexBuffer9* GetNativeIndexBuffer(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] std::size_t GetTextureCount() const noexcept;
        [[nodiscard]] std::size_t GetBufferCount() const noexcept;

        // Call immediately before the legacy renderer releases D3DPOOL_DEFAULT
        // resources and invokes IDirect3DDevice9::Reset.
        void OnDeviceLost() noexcept;

        // Call only after the legacy renderer has successfully reset the device.
        // Passing a device is optional because Reset normally keeps the same COM object.
        [[nodiscard]] GraphicsResult OnDeviceReset(
            IDirect3DDevice9* device = nullptr) noexcept;

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

        // Shader and pipeline migration for DX9 is intentionally deferred.
        // The compatibility backend reports Unsupported instead of silently
        // routing new graphics resources through legacy shader state.
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

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
