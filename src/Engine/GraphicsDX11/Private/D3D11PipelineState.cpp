#include "D3D11PipelineState.h"

#include "D3D11Conversions.h"
#include "D3D11Debug.h"

#include <d3d11.h>

#include <utility>

namespace engine::graphics::d3d11::detail
{
    namespace
    {
        [[nodiscard]] D3D11_FILL_MODE ConvertFillMode(
            const FillMode value) noexcept
        {
            return value == FillMode::Wireframe
                ? D3D11_FILL_WIREFRAME
                : D3D11_FILL_SOLID;
        }

        [[nodiscard]] D3D11_CULL_MODE ConvertCullMode(
            const CullMode value) noexcept
        {
            switch (value)
            {
            case CullMode::None:
                return D3D11_CULL_NONE;
            case CullMode::Front:
                return D3D11_CULL_FRONT;
            case CullMode::Back:
            default:
                return D3D11_CULL_BACK;
            }
        }

        [[nodiscard]] D3D11_COMPARISON_FUNC ConvertComparison(
            const ComparisonFunction value) noexcept
        {
            switch (value)
            {
            case ComparisonFunction::Never:
                return D3D11_COMPARISON_NEVER;
            case ComparisonFunction::Less:
                return D3D11_COMPARISON_LESS;
            case ComparisonFunction::Equal:
                return D3D11_COMPARISON_EQUAL;
            case ComparisonFunction::LessEqual:
                return D3D11_COMPARISON_LESS_EQUAL;
            case ComparisonFunction::Greater:
                return D3D11_COMPARISON_GREATER;
            case ComparisonFunction::NotEqual:
                return D3D11_COMPARISON_NOT_EQUAL;
            case ComparisonFunction::GreaterEqual:
                return D3D11_COMPARISON_GREATER_EQUAL;
            case ComparisonFunction::Always:
            default:
                return D3D11_COMPARISON_ALWAYS;
            }
        }

        [[nodiscard]] D3D11_STENCIL_OP ConvertStencilOperation(
            const StencilOperation value) noexcept
        {
            switch (value)
            {
            case StencilOperation::Keep:
                return D3D11_STENCIL_OP_KEEP;
            case StencilOperation::Zero:
                return D3D11_STENCIL_OP_ZERO;
            case StencilOperation::Replace:
                return D3D11_STENCIL_OP_REPLACE;
            case StencilOperation::IncrementSaturate:
                return D3D11_STENCIL_OP_INCR_SAT;
            case StencilOperation::DecrementSaturate:
                return D3D11_STENCIL_OP_DECR_SAT;
            case StencilOperation::Invert:
                return D3D11_STENCIL_OP_INVERT;
            case StencilOperation::Increment:
                return D3D11_STENCIL_OP_INCR;
            case StencilOperation::Decrement:
            default:
                return D3D11_STENCIL_OP_DECR;
            }
        }

        [[nodiscard]] D3D11_BLEND ConvertBlendFactor(
            const BlendFactor value) noexcept
        {
            switch (value)
            {
            case BlendFactor::Zero:
                return D3D11_BLEND_ZERO;
            case BlendFactor::One:
                return D3D11_BLEND_ONE;
            case BlendFactor::SourceColor:
                return D3D11_BLEND_SRC_COLOR;
            case BlendFactor::InverseSourceColor:
                return D3D11_BLEND_INV_SRC_COLOR;
            case BlendFactor::SourceAlpha:
                return D3D11_BLEND_SRC_ALPHA;
            case BlendFactor::InverseSourceAlpha:
                return D3D11_BLEND_INV_SRC_ALPHA;
            case BlendFactor::DestinationAlpha:
                return D3D11_BLEND_DEST_ALPHA;
            case BlendFactor::InverseDestinationAlpha:
                return D3D11_BLEND_INV_DEST_ALPHA;
            case BlendFactor::DestinationColor:
                return D3D11_BLEND_DEST_COLOR;
            case BlendFactor::InverseDestinationColor:
                return D3D11_BLEND_INV_DEST_COLOR;
            case BlendFactor::SourceAlphaSaturate:
                return D3D11_BLEND_SRC_ALPHA_SAT;
            case BlendFactor::Constant:
                return D3D11_BLEND_BLEND_FACTOR;
            case BlendFactor::InverseConstant:
                return D3D11_BLEND_INV_BLEND_FACTOR;
            case BlendFactor::Source1Color:
                return D3D11_BLEND_SRC1_COLOR;
            case BlendFactor::InverseSource1Color:
                return D3D11_BLEND_INV_SRC1_COLOR;
            case BlendFactor::Source1Alpha:
                return D3D11_BLEND_SRC1_ALPHA;
            case BlendFactor::InverseSource1Alpha:
            default:
                return D3D11_BLEND_INV_SRC1_ALPHA;
            }
        }

        [[nodiscard]] D3D11_BLEND_OP ConvertBlendOperation(
            const BlendOperation value) noexcept
        {
            switch (value)
            {
            case BlendOperation::Add:
                return D3D11_BLEND_OP_ADD;
            case BlendOperation::Subtract:
                return D3D11_BLEND_OP_SUBTRACT;
            case BlendOperation::ReverseSubtract:
                return D3D11_BLEND_OP_REV_SUBTRACT;
            case BlendOperation::Minimum:
                return D3D11_BLEND_OP_MIN;
            case BlendOperation::Maximum:
            default:
                return D3D11_BLEND_OP_MAX;
            }
        }

        [[nodiscard]] D3D11_PRIMITIVE_TOPOLOGY ConvertTopology(
            const PrimitiveTopology value) noexcept
        {
            switch (value)
            {
            case PrimitiveTopology::PointList:
                return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            case PrimitiveTopology::LineList:
                return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case PrimitiveTopology::LineStrip:
                return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case PrimitiveTopology::TriangleList:
                return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case PrimitiveTopology::TriangleStrip:
                return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case PrimitiveTopology::Undefined:
            default:
                return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
            }
        }

        [[nodiscard]] D3D11_DEPTH_STENCILOP_DESC ConvertStencilFace(
            const DepthStencilOperationDesc& source) noexcept
        {
            D3D11_DEPTH_STENCILOP_DESC destination{};
            destination.StencilFailOp =
                ConvertStencilOperation(source.stencilFail);
            destination.StencilDepthFailOp =
                ConvertStencilOperation(source.depthFail);
            destination.StencilPassOp =
                ConvertStencilOperation(source.pass);
            destination.StencilFunc =
                ConvertComparison(source.function);
            return destination;
        }

        [[nodiscard]] bool HasStage(
            const D3D11ShaderResource* const resource,
            const ShaderStage stage) noexcept
        {
            return resource == nullptr || resource->stage == stage;
        }
    }

    GraphicsResult CreateGraphicsPipelineResource(
        ID3D11Device* const device,
        const GraphicsPipelineDesc& desc,
        const D3D11ShaderResource& vertexShader,
        const D3D11ShaderResource* const pixelShader,
        const D3D11ShaderResource* const geometryShader,
        const D3D11ShaderResource* const hullShader,
        const D3D11ShaderResource* const domainShader,
        const D3D11InputLayoutResource* const inputLayout,
        D3D11GraphicsPipelineResource& outResource) noexcept
    {
        outResource = D3D11GraphicsPipelineResource{};

        if (
            device == nullptr ||
            !desc.IsValid() ||
            vertexShader.stage != ShaderStage::Vertex ||
            !vertexShader.vertexShader ||
            !HasStage(pixelShader, ShaderStage::Pixel) ||
            !HasStage(geometryShader, ShaderStage::Geometry) ||
            !HasStage(hullShader, ShaderStage::Hull) ||
            !HasStage(domainShader, ShaderStage::Domain))
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11GraphicsPipelineResource resource;
        resource.vertexShader.CopyFrom(vertexShader.vertexShader.Get());

        if (pixelShader != nullptr)
        {
            resource.pixelShader.CopyFrom(pixelShader->pixelShader.Get());
        }

        if (geometryShader != nullptr)
        {
            resource.geometryShader.CopyFrom(
                geometryShader->geometryShader.Get());
        }

        if (hullShader != nullptr)
        {
            resource.hullShader.CopyFrom(hullShader->hullShader.Get());
        }

        if (domainShader != nullptr)
        {
            resource.domainShader.CopyFrom(
                domainShader->domainShader.Get());
        }

        if (inputLayout != nullptr)
        {
            resource.inputLayout.CopyFrom(inputLayout->native.Get());
        }

        D3D11_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = ConvertFillMode(desc.rasterizer.fillMode);
        rasterizerDesc.CullMode = ConvertCullMode(desc.rasterizer.cullMode);
        rasterizerDesc.FrontCounterClockwise =
            desc.rasterizer.frontCounterClockwise ? TRUE : FALSE;
        rasterizerDesc.DepthBias = desc.rasterizer.depthBias;
        rasterizerDesc.DepthBiasClamp = desc.rasterizer.depthBiasClamp;
        rasterizerDesc.SlopeScaledDepthBias =
            desc.rasterizer.slopeScaledDepthBias;
        rasterizerDesc.DepthClipEnable =
            desc.rasterizer.depthClipEnable ? TRUE : FALSE;
        rasterizerDesc.ScissorEnable =
            desc.rasterizer.scissorEnable ? TRUE : FALSE;
        rasterizerDesc.MultisampleEnable =
            desc.rasterizer.multisampleEnable ? TRUE : FALSE;
        rasterizerDesc.AntialiasedLineEnable =
            desc.rasterizer.antialiasedLineEnable ? TRUE : FALSE;

        HRESULT result = device->CreateRasterizerState(
            &rasterizerDesc,
            resource.rasterizerState.Put());

        if (FAILED(result))
        {
            return ConvertFailure(result);
        }

        D3D11_BLEND_DESC blendDesc{};
        blendDesc.AlphaToCoverageEnable =
            desc.blend.alphaToCoverageEnable ? TRUE : FALSE;
        blendDesc.IndependentBlendEnable =
            desc.blend.independentBlendEnable ? TRUE : FALSE;

        for (std::size_t index = 0U;
             index < desc.blend.renderTargets.size();
             ++index)
        {
            const RenderTargetBlendDesc& source =
                desc.blend.renderTargets[index];
            D3D11_RENDER_TARGET_BLEND_DESC& destination =
                blendDesc.RenderTarget[index];

            destination.BlendEnable =
                source.blendEnable ? TRUE : FALSE;
            destination.SrcBlend =
                ConvertBlendFactor(source.sourceColor);
            destination.DestBlend =
                ConvertBlendFactor(source.destinationColor);
            destination.BlendOp =
                ConvertBlendOperation(source.colorOperation);
            destination.SrcBlendAlpha =
                ConvertBlendFactor(source.sourceAlpha);
            destination.DestBlendAlpha =
                ConvertBlendFactor(source.destinationAlpha);
            destination.BlendOpAlpha =
                ConvertBlendOperation(source.alphaOperation);
            destination.RenderTargetWriteMask =
                static_cast<UINT8>(source.writeMask);
        }

        result = device->CreateBlendState(
            &blendDesc,
            resource.blendState.Put());

        if (FAILED(result))
        {
            return ConvertFailure(result);
        }

        D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable =
            desc.depthStencil.depthEnable ? TRUE : FALSE;
        depthStencilDesc.DepthWriteMask =
            desc.depthStencil.depthWriteEnable
                ? D3D11_DEPTH_WRITE_MASK_ALL
                : D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc =
            ConvertComparison(desc.depthStencil.depthFunction);
        depthStencilDesc.StencilEnable =
            desc.depthStencil.stencilEnable ? TRUE : FALSE;
        depthStencilDesc.StencilReadMask =
            desc.depthStencil.stencilReadMask;
        depthStencilDesc.StencilWriteMask =
            desc.depthStencil.stencilWriteMask;
        depthStencilDesc.FrontFace =
            ConvertStencilFace(desc.depthStencil.frontFace);
        depthStencilDesc.BackFace =
            ConvertStencilFace(desc.depthStencil.backFace);

        result = device->CreateDepthStencilState(
            &depthStencilDesc,
            resource.depthStencilState.Put());

        if (FAILED(result))
        {
            return ConvertFailure(result);
        }

        resource.topology = ConvertTopology(desc.topology);
        resource.blendConstants = desc.blendConstants;
        resource.sampleMask = desc.sampleMask;
        resource.stencilReference = desc.stencilReference;

        if (
            resource.topology == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED ||
            !resource.vertexShader ||
            !resource.rasterizerState ||
            !resource.blendState ||
            !resource.depthStencilState)
        {
            return GraphicsResult::BackendFailure;
        }

        SetDebugName(resource.rasterizerState.Get(), desc.debugName);
        SetDebugName(resource.blendState.Get(), desc.debugName);
        SetDebugName(resource.depthStencilState.Get(), desc.debugName);

        outResource = std::move(resource);
        return GraphicsResult::Success;
    }
}
