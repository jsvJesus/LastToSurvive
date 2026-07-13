#include "D3D11Shader.h"

#include "D3D11Conversions.h"
#include "D3D11Debug.h"

#include <d3d11.h>

#include <array>
#include <cstring>
#include <new>
#include <utility>

namespace engine::graphics::d3d11::detail
{
    ID3D11DeviceChild* GetNativeShader(
        const D3D11ShaderResource& resource) noexcept
    {
        switch (resource.stage)
        {
        case ShaderStage::Vertex:
            return resource.vertexShader.Get();
        case ShaderStage::Pixel:
            return resource.pixelShader.Get();
        case ShaderStage::Geometry:
            return resource.geometryShader.Get();
        case ShaderStage::Hull:
            return resource.hullShader.Get();
        case ShaderStage::Domain:
            return resource.domainShader.Get();
        case ShaderStage::Compute:
            return resource.computeShader.Get();
        case ShaderStage::Unknown:
        default:
            return nullptr;
        }
    }

    GraphicsResult CreateShaderResource(
        ID3D11Device* const device,
        const ShaderDesc& desc,
        D3D11ShaderResource& outResource) noexcept
    {
        outResource = D3D11ShaderResource{};

        if (device == nullptr || !desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11ShaderResource resource;
        resource.stage = desc.stage;

        HRESULT result = E_INVALIDARG;

        switch (desc.stage)
        {
        case ShaderStage::Vertex:
            result = device->CreateVertexShader(
                desc.bytecode.data,
                desc.bytecode.size,
                nullptr,
                resource.vertexShader.Put());
            break;

        case ShaderStage::Pixel:
            result = device->CreatePixelShader(
                desc.bytecode.data,
                desc.bytecode.size,
                nullptr,
                resource.pixelShader.Put());
            break;

        case ShaderStage::Geometry:
            result = device->CreateGeometryShader(
                desc.bytecode.data,
                desc.bytecode.size,
                nullptr,
                resource.geometryShader.Put());
            break;

        case ShaderStage::Hull:
            result = device->CreateHullShader(
                desc.bytecode.data,
                desc.bytecode.size,
                nullptr,
                resource.hullShader.Put());
            break;

        case ShaderStage::Domain:
            result = device->CreateDomainShader(
                desc.bytecode.data,
                desc.bytecode.size,
                nullptr,
                resource.domainShader.Put());
            break;

        case ShaderStage::Compute:
            result = device->CreateComputeShader(
                desc.bytecode.data,
                desc.bytecode.size,
                nullptr,
                resource.computeShader.Put());
            break;

        case ShaderStage::Unknown:
        default:
            return GraphicsResult::InvalidArgument;
        }

        if (FAILED(result))
        {
            return ConvertFailure(result);
        }

        if (desc.stage == ShaderStage::Vertex)
        {
            try
            {
                resource.bytecode.resize(desc.bytecode.size);
                std::memcpy(
                    resource.bytecode.data(),
                    desc.bytecode.data,
                    desc.bytecode.size);
            }
            catch (const std::bad_alloc&)
            {
                return GraphicsResult::OutOfMemory;
            }
            catch (...)
            {
                return GraphicsResult::BackendFailure;
            }
        }

        SetDebugName(GetNativeShader(resource), desc.debugName);
        outResource = std::move(resource);
        return GraphicsResult::Success;
    }

    GraphicsResult CreateInputLayoutResource(
        ID3D11Device* const device,
        const InputLayoutDesc& desc,
        const D3D11ShaderResource& vertexShader,
        D3D11InputLayoutResource& outResource) noexcept
    {
        outResource = D3D11InputLayoutResource{};

        if (
            device == nullptr ||
            !desc.IsValid() ||
            vertexShader.stage != ShaderStage::Vertex ||
            !vertexShader.vertexShader ||
            vertexShader.bytecode.empty())
        {
            return GraphicsResult::InvalidArgument;
        }

        std::array<
            D3D11_INPUT_ELEMENT_DESC,
            MaxVertexInputElements> nativeElements{};

        for (std::size_t index = 0U; index < desc.elementCount; ++index)
        {
            const VertexElementDesc& source = desc.elements[index];
            D3D11_INPUT_ELEMENT_DESC& destination = nativeElements[index];

            DXGI_FORMAT nativeFormat = DXGI_FORMAT_UNKNOWN;
            const GraphicsResult formatResult = ConvertVertexFormat(
                source.format,
                nativeFormat);

            if (Failed(formatResult))
            {
                return formatResult;
            }

            destination.SemanticName = source.semanticName;
            destination.SemanticIndex = source.semanticIndex;
            destination.Format = nativeFormat;
            destination.InputSlot = source.inputSlot;
            destination.AlignedByteOffset =
                source.alignedByteOffset == AppendAlignedVertexElement
                    ? D3D11_APPEND_ALIGNED_ELEMENT
                    : source.alignedByteOffset;
            destination.InputSlotClass =
                source.inputRate == VertexInputRate::PerInstance
                    ? D3D11_INPUT_PER_INSTANCE_DATA
                    : D3D11_INPUT_PER_VERTEX_DATA;
            destination.InstanceDataStepRate = source.instanceStepRate;
        }

        D3D11InputLayoutResource resource;

        const HRESULT result = device->CreateInputLayout(
            nativeElements.data(),
            static_cast<UINT>(desc.elementCount),
            vertexShader.bytecode.data(),
            vertexShader.bytecode.size(),
            resource.native.Put());

        if (FAILED(result))
        {
            return ConvertFailure(result);
        }

        SetDebugName(resource.native.Get(), desc.debugName);
        outResource = std::move(resource);
        return GraphicsResult::Success;
    }
}
