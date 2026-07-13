#pragma once

#include "D3D11ResourceTypes.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/PipelineState.h"

struct ID3D11Device;

namespace engine::graphics::d3d11::detail
{
    [[nodiscard]] GraphicsResult CreateGraphicsPipelineResource(
        ID3D11Device* device,
        const GraphicsPipelineDesc& desc,
        const D3D11ShaderResource& vertexShader,
        const D3D11ShaderResource* pixelShader,
        const D3D11ShaderResource* geometryShader,
        const D3D11ShaderResource* hullShader,
        const D3D11ShaderResource* domainShader,
        const D3D11InputLayoutResource* inputLayout,
        D3D11GraphicsPipelineResource& outResource) noexcept;
}
