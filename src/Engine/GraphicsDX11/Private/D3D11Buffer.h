#pragma once

#include "D3D11ResourceTypes.h"
#include "Graphics/GraphicsResult.h"

struct ID3D11Device;

namespace engine::graphics::d3d11::detail
{
    [[nodiscard]] GraphicsResult CreateBufferResource(
        ID3D11Device* device,
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        D3D11BufferResource& outResource) noexcept;
}
