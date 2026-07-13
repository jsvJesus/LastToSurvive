#pragma once

#include "Graphics/Buffer.h"
#include "Graphics/Format.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/Texture.h"

#include <d3d11.h>
#include <dxgi.h>

namespace engine::graphics::d3d11::detail
{
    enum class TextureViewKind : unsigned char
    {
        Resource = 0,
        ShaderResource,
        RenderTarget,
        DepthStencil,
        UnorderedAccess
    };

    [[nodiscard]] GraphicsResult ConvertFormat(
        Format format,
        TextureViewKind viewKind,
        bool depthShaderResource,
        DXGI_FORMAT& outFormat) noexcept;

    [[nodiscard]] GraphicsResult ConvertTextureUsage(
        const TextureDesc& desc,
        D3D11_USAGE& outUsage,
        UINT& outBindFlags,
        UINT& outCpuAccessFlags,
        UINT& outMiscFlags) noexcept;

    [[nodiscard]] GraphicsResult ConvertBufferUsage(
        const BufferDesc& desc,
        D3D11_USAGE& outUsage,
        UINT& outBindFlags,
        UINT& outCpuAccessFlags,
        UINT& outMiscFlags) noexcept;

    [[nodiscard]] GraphicsResult ConvertFailure(
        HRESULT result) noexcept;

    [[nodiscard]] bool IsDeviceRemovedResult(
        HRESULT result) noexcept;
}
