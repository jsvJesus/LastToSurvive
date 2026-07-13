#pragma once

#include "D3D9ResourceTypes.h"

#include "Graphics/GraphicsResult.h"

struct IDirect3DDevice9;

namespace engine::graphics::d3d9::detail
{
    [[nodiscard]] GraphicsResult BuildBufferResource(
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        D3D9BufferResource& outResource) noexcept;

    [[nodiscard]] GraphicsResult CreateNativeBuffer(
        IDirect3DDevice9* device,
        D3D9BufferResource& resource) noexcept;

    void ReleaseBufferForDeviceLost(
        D3D9BufferResource& resource) noexcept;
}
