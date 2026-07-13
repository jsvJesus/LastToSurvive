#pragma once

#include "D3D9ResourceTypes.h"

#include "Graphics/GraphicsResult.h"

struct IDirect3DDevice9;

namespace engine::graphics::d3d9::detail
{
    [[nodiscard]] GraphicsResult BuildTextureResource(
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        std::size_t initialDataCount,
        D3D9TextureResource& outResource) noexcept;

    [[nodiscard]] GraphicsResult CreateNativeTexture(
        IDirect3DDevice9* device,
        D3D9TextureResource& resource) noexcept;

    void ReleaseTextureForDeviceLost(
        D3D9TextureResource& resource) noexcept;
}
