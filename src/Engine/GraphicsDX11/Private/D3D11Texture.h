#pragma once

#include "D3D11ResourceTypes.h"
#include "Graphics/GraphicsResult.h"

#include <cstddef>

struct ID3D11Device;

namespace engine::graphics::d3d11::detail
{
    [[nodiscard]] GraphicsResult CreateTextureResource(
        ID3D11Device* device,
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        std::size_t initialDataCount,
        D3D11TextureResource& outResource) noexcept;
}
