#pragma once

#include "D3D11ResourceTypes.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/InputLayout.h"
#include "Graphics/Shader.h"

struct ID3D11Device;
struct ID3D11DeviceChild;

namespace engine::graphics::d3d11::detail
{
    [[nodiscard]] GraphicsResult CreateShaderResource(
        ID3D11Device* device,
        const ShaderDesc& desc,
        D3D11ShaderResource& outResource) noexcept;

    [[nodiscard]] GraphicsResult CreateInputLayoutResource(
        ID3D11Device* device,
        const InputLayoutDesc& desc,
        const D3D11ShaderResource& vertexShader,
        D3D11InputLayoutResource& outResource) noexcept;

    [[nodiscard]] ID3D11DeviceChild* GetNativeShader(
        const D3D11ShaderResource& resource) noexcept;
}
