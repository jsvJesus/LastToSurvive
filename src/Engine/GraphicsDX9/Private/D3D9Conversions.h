#pragma once

#include "Graphics/Buffer.h"
#include "Graphics/Format.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/Texture.h"

#include <d3d9.h>

namespace engine::graphics::d3d9::detail
{
    [[nodiscard]] GraphicsResult ConvertFormat(
        Format format,
        D3DFORMAT& outFormat) noexcept;

    [[nodiscard]] GraphicsResult ConvertTextureUsage(
        const TextureDesc& desc,
        DWORD& outUsage,
        D3DPOOL& outPool) noexcept;

    [[nodiscard]] GraphicsResult ConvertBufferUsage(
        const BufferDesc& desc,
        DWORD& outUsage,
        D3DPOOL& outPool) noexcept;

    [[nodiscard]] bool IsDefaultPool(D3DPOOL pool) noexcept;
}
