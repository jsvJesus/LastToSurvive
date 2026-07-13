#include "D3D9Texture.h"

#include "D3D9Conversions.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace engine::graphics::d3d9::detail
{
    namespace
    {
        [[nodiscard]] std::uint32_t MipExtent(
            const std::uint32_t value,
            const std::uint32_t mipLevel) noexcept
        {
            return (std::max)(1U, value >> mipLevel);
        }

        [[nodiscard]] std::size_t ExpectedSubresourceCount(
            const TextureDesc& desc) noexcept
        {
            switch (desc.dimension)
            {
            case TextureDimension::Texture1D:
            case TextureDimension::Texture2D:
                return static_cast<std::size_t>(desc.mipLevels);
            case TextureDimension::Texture3D:
                return static_cast<std::size_t>(desc.mipLevels);
            case TextureDimension::TextureCube:
                return static_cast<std::size_t>(desc.mipLevels) * 6U;
            default:
                return 0;
            }
        }

        [[nodiscard]] GraphicsResult ValidateLegacyShape(
            const TextureDesc& desc) noexcept
        {
            if (desc.dimension == TextureDimension::TextureCube)
            {
                if (desc.arrayLayers != 6 || desc.width != desc.height)
                {
                    return GraphicsResult::Unsupported;
                }
            }
            else if (desc.arrayLayers != 1)
            {
                return GraphicsResult::Unsupported;
            }

            return GraphicsResult::Success;
        }

        [[nodiscard]] GraphicsResult CopyInitialData(
            const TextureDesc& desc,
            const TextureSubresourceData* initialData,
            const std::size_t initialDataCount,
            std::vector<OwnedTextureSubresource>& output) noexcept
        {
            output.clear();

            if (initialDataCount == 0)
            {
                if (desc.usage == ResourceUsage::Immutable)
                {
                    return GraphicsResult::InvalidArgument;
                }

                return GraphicsResult::Success;
            }

            if (initialData == nullptr ||
                initialDataCount != ExpectedSubresourceCount(desc))
            {
                return GraphicsResult::InvalidArgument;
            }

            output.reserve(initialDataCount);
            for (std::size_t index = 0; index < initialDataCount; ++index)
            {
                if (!initialData[index].IsValid())
                {
                    output.clear();
                    return GraphicsResult::InvalidArgument;
                }

                const std::uint32_t mipLevel = desc.dimension == TextureDimension::TextureCube
                    ? static_cast<std::uint32_t>(index % desc.mipLevels)
                    : static_cast<std::uint32_t>(index);
                const std::uint32_t width = MipExtent(desc.width, mipLevel);
                const std::uint32_t height = MipExtent(desc.height, mipLevel);
                const std::uint32_t depth = MipExtent(desc.depth, mipLevel);
                const FormatInfo info = GetFormatInfo(desc.format);
                const std::size_t expectedRowPitch = CalculateRowPitch(desc.format, width);
                const std::size_t rowCount =
                    (static_cast<std::size_t>(height) + info.blockHeight - 1U) /
                    info.blockHeight;

                if (expectedRowPitch == 0 ||
                    initialData[index].rowPitch < expectedRowPitch)
                {
                    output.clear();
                    return GraphicsResult::InvalidArgument;
                }

                const std::size_t minimumSlicePitch =
                    initialData[index].rowPitch * rowCount;
                const std::size_t sourceSlicePitch = initialData[index].slicePitch != 0
                    ? initialData[index].slicePitch
                    : minimumSlicePitch;

                if (sourceSlicePitch < minimumSlicePitch ||
                    initialData[index].dataSize < sourceSlicePitch * depth)
                {
                    output.clear();
                    return GraphicsResult::InvalidArgument;
                }

                OwnedTextureSubresource owned;
                owned.bytes.assign(
                    initialData[index].data,
                    initialData[index].data + initialData[index].dataSize);
                owned.rowPitch = initialData[index].rowPitch;
                owned.slicePitch = sourceSlicePitch;
                output.push_back(std::move(owned));
            }

            return GraphicsResult::Success;
        }

        [[nodiscard]] GraphicsResult UploadRect(
            IDirect3DTexture9* texture,
            const TextureDesc& desc,
            const std::vector<OwnedTextureSubresource>& data) noexcept
        {
            for (std::uint32_t mip = 0; mip < desc.mipLevels; ++mip)
            {
                D3DLOCKED_RECT locked{};
                const DWORD flags = desc.usage == ResourceUsage::Dynamic
                    ? D3DLOCK_DISCARD
                    : 0;

                const HRESULT hr = texture->LockRect(mip, &locked, nullptr, flags);
                if (FAILED(hr))
                {
                    return GraphicsResult::BackendFailure;
                }

                const OwnedTextureSubresource& source = data[mip];
                const std::uint32_t height = MipExtent(desc.height, mip);
                const FormatInfo formatInfo = GetFormatInfo(desc.format);
                const std::size_t rowCount =
                    (static_cast<std::size_t>(height) + formatInfo.blockHeight - 1U) /
                    formatInfo.blockHeight;
                const std::size_t destinationPitch =
                    static_cast<std::size_t>(locked.Pitch);
                const std::size_t copySize =
                    (std::min)(source.rowPitch, destinationPitch);

                auto* destination = static_cast<std::byte*>(locked.pBits);
                for (std::size_t row = 0; row < rowCount; ++row)
                {
                    std::memcpy(
                        destination + row * destinationPitch,
                        source.bytes.data() + row * source.rowPitch,
                        copySize);
                }

                texture->UnlockRect(mip);
            }

            return GraphicsResult::Success;
        }

        [[nodiscard]] GraphicsResult UploadCube(
            IDirect3DCubeTexture9* texture,
            const TextureDesc& desc,
            const std::vector<OwnedTextureSubresource>& data) noexcept
        {
            for (std::uint32_t face = 0; face < 6; ++face)
            {
                for (std::uint32_t mip = 0; mip < desc.mipLevels; ++mip)
                {
                    const std::size_t index =
                        static_cast<std::size_t>(face) * desc.mipLevels + mip;
                    D3DLOCKED_RECT locked{};
                    const HRESULT hr = texture->LockRect(
                        static_cast<D3DCUBEMAP_FACES>(face),
                        mip,
                        &locked,
                        nullptr,
                        0);

                    if (FAILED(hr))
                    {
                        return GraphicsResult::BackendFailure;
                    }

                    const OwnedTextureSubresource& source = data[index];
                    const std::uint32_t height = MipExtent(desc.height, mip);
                    const FormatInfo formatInfo = GetFormatInfo(desc.format);
                    const std::size_t rowCount =
                        (static_cast<std::size_t>(height) + formatInfo.blockHeight - 1U) /
                        formatInfo.blockHeight;
                    const std::size_t destinationPitch =
                        static_cast<std::size_t>(locked.Pitch);
                    const std::size_t copySize =
                        (std::min)(source.rowPitch, destinationPitch);
                    auto* destination = static_cast<std::byte*>(locked.pBits);

                    for (std::size_t row = 0; row < rowCount; ++row)
                    {
                        std::memcpy(
                            destination + row * destinationPitch,
                            source.bytes.data() + row * source.rowPitch,
                            copySize);
                    }

                    texture->UnlockRect(
                        static_cast<D3DCUBEMAP_FACES>(face),
                        mip);
                }
            }

            return GraphicsResult::Success;
        }

        [[nodiscard]] GraphicsResult UploadVolume(
            IDirect3DVolumeTexture9* texture,
            const TextureDesc& desc,
            const std::vector<OwnedTextureSubresource>& data) noexcept
        {
            for (std::uint32_t mip = 0; mip < desc.mipLevels; ++mip)
            {
                D3DLOCKED_BOX locked{};
                const HRESULT hr = texture->LockBox(mip, &locked, nullptr, 0);
                if (FAILED(hr))
                {
                    return GraphicsResult::BackendFailure;
                }

                const OwnedTextureSubresource& source = data[mip];
                const std::uint32_t height = MipExtent(desc.height, mip);
                const std::uint32_t depth = MipExtent(desc.depth, mip);
                const FormatInfo formatInfo = GetFormatInfo(desc.format);
                const std::size_t rowCount =
                    (static_cast<std::size_t>(height) + formatInfo.blockHeight - 1U) /
                    formatInfo.blockHeight;
                const std::size_t sourceSlicePitch = source.slicePitch != 0
                    ? source.slicePitch
                    : source.rowPitch * rowCount;
                const std::size_t destinationRowPitch =
                    static_cast<std::size_t>(locked.RowPitch);
                const std::size_t destinationSlicePitch =
                    static_cast<std::size_t>(locked.SlicePitch);
                const std::size_t copySize =
                    (std::min)(source.rowPitch, destinationRowPitch);
                auto* destination = static_cast<std::byte*>(locked.pBits);

                for (std::uint32_t slice = 0; slice < depth; ++slice)
                {
                    for (std::size_t row = 0; row < rowCount; ++row)
                    {
                        std::memcpy(
                            destination + slice * destinationSlicePitch + row * destinationRowPitch,
                            source.bytes.data() + slice * sourceSlicePitch + row * source.rowPitch,
                            copySize);
                    }
                }

                texture->UnlockBox(mip);
            }

            return GraphicsResult::Success;
        }
    }

    GraphicsResult BuildTextureResource(
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        const std::size_t initialDataCount,
        D3D9TextureResource& outResource) noexcept
    {
        if (!desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const GraphicsResult shapeResult = ValidateLegacyShape(desc);
        if (Failed(shapeResult))
        {
            return shapeResult;
        }

        DWORD usage = 0;
        D3DPOOL pool = D3DPOOL_MANAGED;
        const GraphicsResult usageResult = ConvertTextureUsage(desc, usage, pool);
        if (Failed(usageResult))
        {
            return usageResult;
        }

        D3DFORMAT format = D3DFMT_UNKNOWN;
        const GraphicsResult formatResult = ConvertFormat(desc.format, format);
        if (Failed(formatResult))
        {
            return formatResult;
        }

        D3D9TextureResource resource;
        resource.desc = desc;
        resource.pool = pool;

        const GraphicsResult copyResult = CopyInitialData(
            desc,
            initialData,
            initialDataCount,
            resource.initialData);
        if (Failed(copyResult))
        {
            return copyResult;
        }

        outResource = std::move(resource);
        return GraphicsResult::Success;
    }

    GraphicsResult CreateNativeTexture(
        IDirect3DDevice9* device,
        D3D9TextureResource& resource) noexcept
    {
        if (device == nullptr)
        {
            return GraphicsResult::InvalidArgument;
        }

        DWORD usage = 0;
        D3DPOOL pool = D3DPOOL_MANAGED;
        const GraphicsResult usageResult = ConvertTextureUsage(
            resource.desc,
            usage,
            pool);
        if (Failed(usageResult))
        {
            return usageResult;
        }

        D3DFORMAT format = D3DFMT_UNKNOWN;
        const GraphicsResult formatResult = ConvertFormat(
            resource.desc.format,
            format);
        if (Failed(formatResult))
        {
            return formatResult;
        }

        resource.native.Reset();
        HRESULT hr = E_FAIL;

        if (resource.desc.dimension == TextureDimension::Texture3D)
        {
            IDirect3DVolumeTexture9* texture = nullptr;
            hr = device->CreateVolumeTexture(
                resource.desc.width,
                resource.desc.height,
                resource.desc.depth,
                resource.desc.mipLevels,
                usage,
                format,
                pool,
                &texture,
                nullptr);
            resource.native.Attach(texture);
        }
        else if (resource.desc.dimension == TextureDimension::TextureCube)
        {
            IDirect3DCubeTexture9* texture = nullptr;
            hr = device->CreateCubeTexture(
                resource.desc.width,
                resource.desc.mipLevels,
                usage,
                format,
                pool,
                &texture,
                nullptr);
            resource.native.Attach(texture);
        }
        else
        {
            IDirect3DTexture9* texture = nullptr;
            hr = device->CreateTexture(
                resource.desc.width,
                resource.desc.height,
                resource.desc.mipLevels,
                usage,
                format,
                pool,
                &texture,
                nullptr);
            resource.native.Attach(texture);
        }

        if (FAILED(hr) || !resource.native)
        {
            resource.native.Reset();
            return hr == D3DERR_DEVICELOST
                ? GraphicsResult::DeviceLost
                : hr == E_OUTOFMEMORY
                    ? GraphicsResult::OutOfMemory
                    : GraphicsResult::BackendFailure;
        }

        if (!resource.initialData.empty())
        {
            GraphicsResult uploadResult = GraphicsResult::Unsupported;
            if (resource.desc.dimension == TextureDimension::Texture3D)
            {
                uploadResult = UploadVolume(
                    static_cast<IDirect3DVolumeTexture9*>(resource.native.Get()),
                    resource.desc,
                    resource.initialData);
            }
            else if (resource.desc.dimension == TextureDimension::TextureCube)
            {
                uploadResult = UploadCube(
                    static_cast<IDirect3DCubeTexture9*>(resource.native.Get()),
                    resource.desc,
                    resource.initialData);
            }
            else
            {
                uploadResult = UploadRect(
                    static_cast<IDirect3DTexture9*>(resource.native.Get()),
                    resource.desc,
                    resource.initialData);
            }

            if (Failed(uploadResult))
            {
                resource.native.Reset();
                return uploadResult;
            }
        }

        resource.pool = pool;
        resource.needsRestore = false;
        return GraphicsResult::Success;
    }

    void ReleaseTextureForDeviceLost(
        D3D9TextureResource& resource) noexcept
    {
        if (IsDefaultPool(resource.pool) && resource.native)
        {
            resource.native.Reset();
            resource.needsRestore = true;
        }
    }
}
