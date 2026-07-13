#pragma once

#include "D3D11HandlePool.h"
#include "D3D11ResourceTypes.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/ResourceHandle.h"

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

        void Clear() noexcept;

        [[nodiscard]] const D3D11TextureResource* GetTexture(
            TextureHandle texture) const noexcept;
        [[nodiscard]] const D3D11BufferResource* GetBuffer(
            BufferHandle buffer) const noexcept;

        [[nodiscard]] std::size_t GetTextureCount() const noexcept;
        [[nodiscard]] std::size_t GetBufferCount() const noexcept;

    private:
        HandlePool<TextureHandle, D3D11TextureResource> textures_;
        HandlePool<BufferHandle, D3D11BufferResource> buffers_;
    };
}
