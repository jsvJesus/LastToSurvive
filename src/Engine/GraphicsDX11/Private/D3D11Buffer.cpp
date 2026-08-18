#include "D3D11Buffer.h"

#include "D3D11Conversions.h"

#include <limits>
#include <utility>

namespace engine::graphics::d3d11::detail
{
    GraphicsResult CreateBufferResource(
        ID3D11Device* device,
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        D3D11BufferResource& outResource) noexcept
    {
        if (device == nullptr || !desc.IsValid())
            return GraphicsResult::InvalidArgument;
        if (desc.byteSize > (std::numeric_limits<UINT>::max)())
            return GraphicsResult::Unsupported;

        if (desc.usage == ResourceUsage::Immutable &&
            (initialData == nullptr || !initialData->IsValid()))
        {
            return GraphicsResult::InvalidArgument;
        }
        if (initialData != nullptr &&
            (!initialData->IsValid() || initialData->dataSize < desc.byteSize))
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT cpuAccess = 0;
        UINT miscFlags = 0;
        GraphicsResult result = ConvertBufferUsage(
            desc, usage, bindFlags, cpuAccess, miscFlags);
        if (Failed(result)) return result;

        D3D11_BUFFER_DESC nativeDesc{};
        nativeDesc.ByteWidth = static_cast<UINT>(desc.byteSize);
        nativeDesc.Usage = usage;
        nativeDesc.BindFlags = bindFlags;
        nativeDesc.CPUAccessFlags = cpuAccess;
        nativeDesc.MiscFlags = miscFlags;
        nativeDesc.StructureByteStride =
            HasAnyFlag(desc.miscFlags, BufferMiscFlags::Structured)
                ? desc.stride : 0;

        D3D11_SUBRESOURCE_DATA nativeInitial{};
        const D3D11_SUBRESOURCE_DATA* initialPointer = nullptr;
        if (initialData != nullptr)
        {
            nativeInitial.pSysMem = initialData->data;
            initialPointer = &nativeInitial;
        }

        D3D11BufferResource resource;
        resource.desc = desc;
        HRESULT hr = device->CreateBuffer(
            &nativeDesc, initialPointer, resource.native.Put());
        if (FAILED(hr)) return ConvertFailure(hr);

        const bool structured =
            HasAnyFlag(desc.miscFlags, BufferMiscFlags::Structured);
        const bool raw = HasAnyFlag(desc.miscFlags, BufferMiscFlags::Raw);
        const UINT elementCount = structured
            ? static_cast<UINT>(desc.byteSize / desc.stride)
            : static_cast<UINT>(desc.byteSize / 4U);

        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::ShaderResource))
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC view{};
            view.Format = raw ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
            view.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
            view.BufferEx.FirstElement = 0;
            view.BufferEx.NumElements = elementCount;
            view.BufferEx.Flags = raw ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;
            hr = device->CreateShaderResourceView(
                resource.native.Get(), &view,
                resource.shaderResourceView.Put());
            if (FAILED(hr)) return ConvertFailure(hr);
        }

        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::UnorderedAccess))
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC view{};
            view.Format = raw ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_UNKNOWN;
            view.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            view.Buffer.FirstElement = 0;
            view.Buffer.NumElements = elementCount;
            view.Buffer.Flags = raw ? D3D11_BUFFER_UAV_FLAG_RAW : 0;
            hr = device->CreateUnorderedAccessView(
                resource.native.Get(), &view,
                resource.unorderedAccessView.Put());
            if (FAILED(hr)) return ConvertFailure(hr);
        }

        outResource = std::move(resource);
        return GraphicsResult::Success;
    }
}
