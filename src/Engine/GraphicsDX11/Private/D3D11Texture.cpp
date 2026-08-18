#include "D3D11Texture.h"

#include "D3D11Conversions.h"

#include <algorithm>
#include <limits>
#include <new>
#include <vector>

namespace engine::graphics::d3d11::detail
{
    namespace
    {
        [[nodiscard]] std::uint32_t MipExtent(
            const std::uint32_t value,
            const std::uint32_t mip) noexcept
        {
            return (std::max)(1U, value >> mip);
        }

        [[nodiscard]] std::size_t GetSubresourceCount(
            const TextureDesc& desc) noexcept
        {
            if (desc.dimension == TextureDimension::Texture3D)
                return desc.mipLevels;
            return static_cast<std::size_t>(desc.mipLevels) *
                   static_cast<std::size_t>(desc.arrayLayers);
        }

        [[nodiscard]] GraphicsResult ValidateInitialData(
            const TextureDesc& desc,
            const TextureSubresourceData* initialData,
            const std::size_t initialDataCount) noexcept
        {
            const std::size_t expectedCount = GetSubresourceCount(desc);
            if (initialDataCount == 0)
            {
                if (desc.usage == ResourceUsage::Immutable)
                    return GraphicsResult::InvalidArgument;
                return GraphicsResult::Success;
            }

            if (initialData == nullptr || initialDataCount != expectedCount ||
                desc.sampleCount > 1)
            {
                return GraphicsResult::InvalidArgument;
            }

            for (std::size_t index = 0; index < initialDataCount; ++index)
            {
                if (!initialData[index].IsValid())
                    return GraphicsResult::InvalidArgument;

                const std::uint32_t mip = static_cast<std::uint32_t>(
                    index % desc.mipLevels);
                const std::uint32_t width = MipExtent(desc.width, mip);
                const std::uint32_t height = MipExtent(desc.height, mip);
                const std::uint32_t depth = MipExtent(desc.depth, mip);
                const std::size_t requiredRow =
                    CalculateRowPitch(desc.format, width);
                const FormatInfo formatInfo = GetFormatInfo(desc.format);
                const std::size_t rowCount =
                    (static_cast<std::size_t>(height) +
                     formatInfo.blockHeight - 1U) /
                    formatInfo.blockHeight;

                if (requiredRow == 0 || rowCount == 0 ||
                    initialData[index].rowPitch < requiredRow ||
                    initialData[index].rowPitch >
                        (std::numeric_limits<std::size_t>::max)() / rowCount)
                {
                    return GraphicsResult::InvalidArgument;
                }

                const std::size_t minimumSlice =
                    initialData[index].rowPitch * rowCount;
                const std::size_t effectiveSlice =
                    desc.dimension == TextureDimension::Texture3D
                        ? initialData[index].slicePitch
                        : minimumSlice;

                if (effectiveSlice < minimumSlice ||
                    effectiveSlice >
                        (std::numeric_limits<std::size_t>::max)() / depth ||
                    initialData[index].dataSize < effectiveSlice * depth)
                {
                    return GraphicsResult::InvalidArgument;
                }
            }

            return GraphicsResult::Success;
        }

        [[nodiscard]] GraphicsResult BuildSubresources(
            const TextureSubresourceData* initialData,
            const std::size_t initialDataCount,
            std::vector<D3D11_SUBRESOURCE_DATA>& outData)
        {
            outData.clear();
            if (initialDataCount == 0)
                return GraphicsResult::Success;

            outData.resize(initialDataCount);
            for (std::size_t index = 0; index < initialDataCount; ++index)
            {
                if (initialData[index].rowPitch >
                        (std::numeric_limits<UINT>::max)() ||
                    initialData[index].slicePitch >
                        (std::numeric_limits<UINT>::max)())
                {
                    return GraphicsResult::Unsupported;
                }

                outData[index].pSysMem = initialData[index].data;
                outData[index].SysMemPitch =
                    static_cast<UINT>(initialData[index].rowPitch);
                outData[index].SysMemSlicePitch =
                    static_cast<UINT>(initialData[index].slicePitch);
            }
            return GraphicsResult::Success;
        }

        void FillSrvDesc(
            const TextureDesc& desc,
            const DXGI_FORMAT format,
            D3D11_SHADER_RESOURCE_VIEW_DESC& view) noexcept
        {
            view = {};
            view.Format = format;

            switch (desc.dimension)
            {
            case TextureDimension::Texture1D:
                if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                    view.Texture1DArray.MostDetailedMip = 0;
                    view.Texture1DArray.MipLevels = desc.mipLevels;
                    view.Texture1DArray.FirstArraySlice = 0;
                    view.Texture1DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                    view.Texture1D.MostDetailedMip = 0;
                    view.Texture1D.MipLevels = desc.mipLevels;
                }
                break;

            case TextureDimension::Texture2D:
                if (desc.sampleCount > 1)
                {
                    if (desc.arrayLayers > 1)
                    {
                        view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
                        view.Texture2DMSArray.FirstArraySlice = 0;
                        view.Texture2DMSArray.ArraySize = desc.arrayLayers;
                    }
                    else
                    {
                        view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
                    }
                }
                else if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    view.Texture2DArray.MostDetailedMip = 0;
                    view.Texture2DArray.MipLevels = desc.mipLevels;
                    view.Texture2DArray.FirstArraySlice = 0;
                    view.Texture2DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    view.Texture2D.MostDetailedMip = 0;
                    view.Texture2D.MipLevels = desc.mipLevels;
                }
                break;

            case TextureDimension::Texture3D:
                view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                view.Texture3D.MostDetailedMip = 0;
                view.Texture3D.MipLevels = desc.mipLevels;
                break;

            case TextureDimension::TextureCube:
                if (desc.arrayLayers == 6)
                {
                    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                    view.TextureCube.MostDetailedMip = 0;
                    view.TextureCube.MipLevels = desc.mipLevels;
                }
                else
                {
                    view.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
                    view.TextureCubeArray.MostDetailedMip = 0;
                    view.TextureCubeArray.MipLevels = desc.mipLevels;
                    view.TextureCubeArray.First2DArrayFace = 0;
                    view.TextureCubeArray.NumCubes = desc.arrayLayers / 6;
                }
                break;
            }
        }

        void FillRtvDesc(
            const TextureDesc& desc,
            const DXGI_FORMAT format,
            D3D11_RENDER_TARGET_VIEW_DESC& view) noexcept
        {
            view = {};
            view.Format = format;
            switch (desc.dimension)
            {
            case TextureDimension::Texture1D:
                if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
                    view.Texture1DArray.MipSlice = 0;
                    view.Texture1DArray.FirstArraySlice = 0;
                    view.Texture1DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
                    view.Texture1D.MipSlice = 0;
                }
                break;
            case TextureDimension::Texture2D:
            case TextureDimension::TextureCube:
                if (desc.sampleCount > 1)
                {
                    if (desc.arrayLayers > 1)
                    {
                        view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
                        view.Texture2DMSArray.FirstArraySlice = 0;
                        view.Texture2DMSArray.ArraySize = desc.arrayLayers;
                    }
                    else
                    {
                        view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                    }
                }
                else if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    view.Texture2DArray.MipSlice = 0;
                    view.Texture2DArray.FirstArraySlice = 0;
                    view.Texture2DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    view.Texture2D.MipSlice = 0;
                }
                break;
            case TextureDimension::Texture3D:
                view.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                view.Texture3D.MipSlice = 0;
                view.Texture3D.FirstWSlice = 0;
                view.Texture3D.WSize = desc.depth;
                break;
            }
        }

        void FillDsvDesc(
            const TextureDesc& desc,
            const DXGI_FORMAT format,
            D3D11_DEPTH_STENCIL_VIEW_DESC& view) noexcept
        {
            view = {};
            view.Format = format;
            switch (desc.dimension)
            {
            case TextureDimension::Texture1D:
                if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
                    view.Texture1DArray.MipSlice = 0;
                    view.Texture1DArray.FirstArraySlice = 0;
                    view.Texture1DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
                    view.Texture1D.MipSlice = 0;
                }
                break;
            case TextureDimension::Texture2D:
            case TextureDimension::TextureCube:
                if (desc.sampleCount > 1)
                {
                    if (desc.arrayLayers > 1)
                    {
                        view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
                        view.Texture2DMSArray.FirstArraySlice = 0;
                        view.Texture2DMSArray.ArraySize = desc.arrayLayers;
                    }
                    else
                    {
                        view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                    }
                }
                else if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                    view.Texture2DArray.MipSlice = 0;
                    view.Texture2DArray.FirstArraySlice = 0;
                    view.Texture2DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    view.Texture2D.MipSlice = 0;
                }
                break;
            case TextureDimension::Texture3D:
                break;
            }
        }

        void FillUavDesc(
            const TextureDesc& desc,
            const DXGI_FORMAT format,
            D3D11_UNORDERED_ACCESS_VIEW_DESC& view) noexcept
        {
            view = {};
            view.Format = format;
            switch (desc.dimension)
            {
            case TextureDimension::Texture1D:
                if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                    view.Texture1DArray.MipSlice = 0;
                    view.Texture1DArray.FirstArraySlice = 0;
                    view.Texture1DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                    view.Texture1D.MipSlice = 0;
                }
                break;
            case TextureDimension::Texture2D:
            case TextureDimension::TextureCube:
                if (desc.arrayLayers > 1)
                {
                    view.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    view.Texture2DArray.MipSlice = 0;
                    view.Texture2DArray.FirstArraySlice = 0;
                    view.Texture2DArray.ArraySize = desc.arrayLayers;
                }
                else
                {
                    view.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    view.Texture2D.MipSlice = 0;
                }
                break;
            case TextureDimension::Texture3D:
                view.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                view.Texture3D.MipSlice = 0;
                view.Texture3D.FirstWSlice = 0;
                view.Texture3D.WSize = desc.depth;
                break;
            }
        }

        [[nodiscard]] GraphicsResult CreateViews(
            ID3D11Device* device,
            D3D11TextureResource& resource) noexcept
        {
            const TextureDesc& desc = resource.desc;
            const bool depthSrv = IsDepthFormat(desc.format) &&
                HasAnyFlag(desc.bindFlags, TextureBindFlags::ShaderResource);

            if (HasAnyFlag(desc.bindFlags, TextureBindFlags::ShaderResource))
            {
                DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
                GraphicsResult result = ConvertFormat(
                    desc.format, TextureViewKind::ShaderResource,
                    depthSrv, format);
                if (Failed(result)) return result;
                D3D11_SHADER_RESOURCE_VIEW_DESC view{};
                FillSrvDesc(desc, format, view);
                const HRESULT hr = device->CreateShaderResourceView(
                    resource.native.Get(), &view,
                    resource.shaderResourceView.Put());
                if (FAILED(hr)) return ConvertFailure(hr);
            }

            if (HasAnyFlag(desc.bindFlags, TextureBindFlags::RenderTarget))
            {
                DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
                GraphicsResult result = ConvertFormat(
                    desc.format, TextureViewKind::RenderTarget,
                    false, format);
                if (Failed(result)) return result;
                D3D11_RENDER_TARGET_VIEW_DESC view{};
                FillRtvDesc(desc, format, view);
                const HRESULT hr = device->CreateRenderTargetView(
                    resource.native.Get(), &view,
                    resource.renderTargetView.Put());
                if (FAILED(hr)) return ConvertFailure(hr);
            }

            if (HasAnyFlag(desc.bindFlags, TextureBindFlags::DepthStencil))
            {
                if (desc.dimension == TextureDimension::Texture3D)
                    return GraphicsResult::Unsupported;
                DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
                GraphicsResult result = ConvertFormat(
                    desc.format, TextureViewKind::DepthStencil,
                    depthSrv, format);
                if (Failed(result)) return result;
                D3D11_DEPTH_STENCIL_VIEW_DESC view{};
                FillDsvDesc(desc, format, view);
                const HRESULT hr = device->CreateDepthStencilView(
                    resource.native.Get(), &view,
                    resource.depthStencilView.Put());
                if (FAILED(hr)) return ConvertFailure(hr);
            }

            if (HasAnyFlag(desc.bindFlags, TextureBindFlags::UnorderedAccess))
            {
                if (desc.sampleCount > 1 || IsDepthFormat(desc.format))
                    return GraphicsResult::Unsupported;
                DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
                GraphicsResult result = ConvertFormat(
                    desc.format, TextureViewKind::UnorderedAccess,
                    false, format);
                if (Failed(result)) return result;
                D3D11_UNORDERED_ACCESS_VIEW_DESC view{};
                FillUavDesc(desc, format, view);
                const HRESULT hr = device->CreateUnorderedAccessView(
                    resource.native.Get(), &view,
                    resource.unorderedAccessView.Put());
                if (FAILED(hr)) return ConvertFailure(hr);
            }

            return GraphicsResult::Success;
        }
    }

    GraphicsResult CreateTextureResource(
        ID3D11Device* device,
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        const std::size_t initialDataCount,
        D3D11TextureResource& outResource) noexcept
    {
        if (device == nullptr || !desc.IsValid())
            return GraphicsResult::InvalidArgument;

        const GraphicsResult initialResult = ValidateInitialData(
            desc, initialData, initialDataCount);
        if (Failed(initialResult)) return initialResult;

        D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
        UINT bindFlags = 0;
        UINT cpuAccess = 0;
        UINT miscFlags = 0;
        GraphicsResult result = ConvertTextureUsage(
            desc, usage, bindFlags, cpuAccess, miscFlags);
        if (Failed(result)) return result;

        const bool depthSrv = IsDepthFormat(desc.format) &&
            HasAnyFlag(desc.bindFlags, TextureBindFlags::ShaderResource);
        DXGI_FORMAT resourceFormat = DXGI_FORMAT_UNKNOWN;
        result = ConvertFormat(desc.format, TextureViewKind::Resource,
                               depthSrv, resourceFormat);
        if (Failed(result)) return result;

        try
        {
            std::vector<D3D11_SUBRESOURCE_DATA> subresources;
            result = BuildSubresources(
                initialData, initialDataCount, subresources);
            if (Failed(result)) return result;
            const D3D11_SUBRESOURCE_DATA* nativeInitial =
                subresources.empty() ? nullptr : subresources.data();

            D3D11TextureResource resource;
            resource.desc = desc;
            HRESULT hr = E_FAIL;

            if (desc.dimension == TextureDimension::Texture1D)
            {
                D3D11_TEXTURE1D_DESC nativeDesc{};
                nativeDesc.Width = desc.width;
                nativeDesc.MipLevels = desc.mipLevels;
                nativeDesc.ArraySize = desc.arrayLayers;
                nativeDesc.Format = resourceFormat;
                nativeDesc.Usage = usage;
                nativeDesc.BindFlags = bindFlags;
                nativeDesc.CPUAccessFlags = cpuAccess;
                nativeDesc.MiscFlags = miscFlags;

                ComPtr<ID3D11Texture1D> texture;
                hr = device->CreateTexture1D(
                    &nativeDesc, nativeInitial, texture.Put());
                if (SUCCEEDED(hr)) resource.native.Attach(texture.Detach());
            }
            else if (desc.dimension == TextureDimension::Texture3D)
            {
                D3D11_TEXTURE3D_DESC nativeDesc{};
                nativeDesc.Width = desc.width;
                nativeDesc.Height = desc.height;
                nativeDesc.Depth = desc.depth;
                nativeDesc.MipLevels = desc.mipLevels;
                nativeDesc.Format = resourceFormat;
                nativeDesc.Usage = usage;
                nativeDesc.BindFlags = bindFlags;
                nativeDesc.CPUAccessFlags = cpuAccess;
                nativeDesc.MiscFlags = miscFlags;

                ComPtr<ID3D11Texture3D> texture;
                hr = device->CreateTexture3D(
                    &nativeDesc, nativeInitial, texture.Put());
                if (SUCCEEDED(hr)) resource.native.Attach(texture.Detach());
            }
            else
            {
                D3D11_TEXTURE2D_DESC nativeDesc{};
                nativeDesc.Width = desc.width;
                nativeDesc.Height = desc.height;
                nativeDesc.MipLevels = desc.mipLevels;
                nativeDesc.ArraySize = desc.arrayLayers;
                nativeDesc.Format = resourceFormat;
                nativeDesc.SampleDesc.Count = desc.sampleCount;
                nativeDesc.SampleDesc.Quality = 0;
                nativeDesc.Usage = usage;
                nativeDesc.BindFlags = bindFlags;
                nativeDesc.CPUAccessFlags = cpuAccess;
                nativeDesc.MiscFlags = miscFlags;

                ComPtr<ID3D11Texture2D> texture;
                hr = device->CreateTexture2D(
                    &nativeDesc, nativeInitial, texture.Put());
                if (SUCCEEDED(hr)) resource.native.Attach(texture.Detach());
            }

            if (FAILED(hr)) return ConvertFailure(hr);
            result = CreateViews(device, resource);
            if (Failed(result)) return result;

            outResource = std::move(resource);
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
    }
}
