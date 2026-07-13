#pragma once

#include "D3D9HandlePool.h"
#include "D3D9ResourceTypes.h"

#include "Graphics/GraphicsResult.h"
#include "Graphics/ResourceHandle.h"

#include <cstddef>

struct IDirect3DDevice9;
struct IDirect3DBaseTexture9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

namespace engine::graphics::d3d9::detail
{
    class D3D9ResourceRegistry final
    {
    public:
        [[nodiscard]] GraphicsResult CreateTexture(
            IDirect3DDevice9* device,
            const TextureDesc& desc,
            const TextureSubresourceData* initialData,
            std::size_t initialDataCount,
            TextureHandle& outTexture) noexcept;

        [[nodiscard]] GraphicsResult DestroyTexture(
            TextureHandle texture) noexcept;

        [[nodiscard]] GraphicsResult CreateBuffer(
            IDirect3DDevice9* device,
            const BufferDesc& desc,
            const BufferInitialData* initialData,
            BufferHandle& outBuffer) noexcept;

        [[nodiscard]] GraphicsResult DestroyBuffer(
            BufferHandle buffer) noexcept;

        void OnDeviceLost() noexcept;

        [[nodiscard]] GraphicsResult OnDeviceReset(
            IDirect3DDevice9* device) noexcept;

        void Clear() noexcept;

        [[nodiscard]] IDirect3DBaseTexture9* GetTexture(
            TextureHandle texture) const noexcept;

        [[nodiscard]] IDirect3DVertexBuffer9* GetVertexBuffer(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] IDirect3DIndexBuffer9* GetIndexBuffer(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] std::size_t GetTextureCount() const noexcept;
        [[nodiscard]] std::size_t GetBufferCount() const noexcept;

    private:
        HandlePool<TextureHandle, D3D9TextureResource> textures_;
        HandlePool<BufferHandle, D3D9BufferResource> buffers_;
    };
}
