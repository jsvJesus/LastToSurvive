#pragma once

#include "D3D11ComPtr.h"
#include "Graphics/Buffer.h"
#include "Graphics/PipelineState.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"

#include <d3d11.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace engine::graphics::d3d11::detail
{
    struct D3D11TextureResource final
    {
        TextureDesc desc;
        ComPtr<ID3D11Resource> native;
        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11DepthStencilView> depthStencilView;
        ComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
    };

    struct D3D11BufferResource final
    {
        BufferDesc desc;
        ComPtr<ID3D11Buffer> native;
        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        ComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
    };

    struct D3D11ShaderResource final
    {
        ShaderStage stage = ShaderStage::Unknown;

        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11GeometryShader> geometryShader;
        ComPtr<ID3D11HullShader> hullShader;
        ComPtr<ID3D11DomainShader> domainShader;
        ComPtr<ID3D11ComputeShader> computeShader;

        // Retained only for stages that need bytecode-dependent resources.
        // The first consumer is ID3D11InputLayout for vertex shaders.
        std::vector<std::byte> bytecode;
    };

    struct D3D11InputLayoutResource final
    {
        ComPtr<ID3D11InputLayout> native;
    };

    struct D3D11GraphicsPipelineResource final
    {
        ComPtr<ID3D11VertexShader> vertexShader;
        ComPtr<ID3D11PixelShader> pixelShader;
        ComPtr<ID3D11GeometryShader> geometryShader;
        ComPtr<ID3D11HullShader> hullShader;
        ComPtr<ID3D11DomainShader> domainShader;
        ComPtr<ID3D11InputLayout> inputLayout;

        ComPtr<ID3D11RasterizerState> rasterizerState;
        ComPtr<ID3D11BlendState> blendState;
        ComPtr<ID3D11DepthStencilState> depthStencilState;

        D3D11_PRIMITIVE_TOPOLOGY topology =
            D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

        std::array<float, 4U> blendConstants{
            1.0F,
            1.0F,
            1.0F,
            1.0F};

        std::uint32_t sampleMask = 0xFFFFFFFFU;
        std::uint32_t stencilReference = 0U;
    };
}
