#include "Assets/DdsTextureDecoder.h"

#include "Graphics/Format.h"
#include "Graphics/ResourceUsage.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        constexpr std::uint32_t DdsMagic =
            0x20534444U;

        constexpr std::uint32_t DdpfAlphaPixels =
            0x00000001U;

        constexpr std::uint32_t DdpfFourCc =
            0x00000004U;

        constexpr std::uint32_t DdpfRgb =
            0x00000040U;

        constexpr std::uint32_t DdpfLuminance =
            0x00020000U;

        constexpr std::uint32_t DdsCaps2CubeMap =
            0x00000200U;

        constexpr std::uint32_t DdsCaps2CubeMapAllFaces =
            0x0000FC00U;

        constexpr std::uint32_t DdsCaps2Volume =
            0x00200000U;

        constexpr std::uint32_t D3D10ResourceMiscTextureCube =
            0x00000004U;

        constexpr std::uint32_t D3D10ResourceDimensionTexture1D =
            2U;

        constexpr std::uint32_t D3D10ResourceDimensionTexture2D =
            3U;

        constexpr std::uint32_t D3D10ResourceDimensionTexture3D =
            4U;

        [[nodiscard]] constexpr std::uint32_t MakeFourCc(
            const char a,
            const char b,
            const char c,
            const char d) noexcept
        {
            return
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(a)) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(b)) << 8U) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(c)) << 16U) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(d)) << 24U);
        }

        constexpr std::uint32_t FourCcDxt1 =
            MakeFourCc('D', 'X', 'T', '1');

        constexpr std::uint32_t FourCcDxt2 =
            MakeFourCc('D', 'X', 'T', '2');

        constexpr std::uint32_t FourCcDxt3 =
            MakeFourCc('D', 'X', 'T', '3');

        constexpr std::uint32_t FourCcDxt4 =
            MakeFourCc('D', 'X', 'T', '4');

        constexpr std::uint32_t FourCcDxt5 =
            MakeFourCc('D', 'X', 'T', '5');

        constexpr std::uint32_t FourCcAti1 =
            MakeFourCc('A', 'T', 'I', '1');

        constexpr std::uint32_t FourCcAti2 =
            MakeFourCc('A', 'T', 'I', '2');

        constexpr std::uint32_t FourCcBc4U =
            MakeFourCc('B', 'C', '4', 'U');

        constexpr std::uint32_t FourCcBc5U =
            MakeFourCc('B', 'C', '5', 'U');

        constexpr std::uint32_t FourCcDx10 =
            MakeFourCc('D', 'X', '1', '0');

        struct DdsPixelFormat final
        {
            std::uint32_t size;
            std::uint32_t flags;
            std::uint32_t fourCc;
            std::uint32_t rgbBitCount;

            std::uint32_t redMask;
            std::uint32_t greenMask;
            std::uint32_t blueMask;
            std::uint32_t alphaMask;
        };

        struct DdsHeader final
        {
            std::uint32_t size;
            std::uint32_t flags;

            std::uint32_t height;
            std::uint32_t width;

            std::uint32_t pitchOrLinearSize;
            std::uint32_t depth;
            std::uint32_t mipMapCount;

            std::uint32_t reserved1[11];

            DdsPixelFormat pixelFormat;

            std::uint32_t caps;
            std::uint32_t caps2;
            std::uint32_t caps3;
            std::uint32_t caps4;
            std::uint32_t reserved2;
        };

        struct DdsHeaderDx10 final
        {
            std::uint32_t dxgiFormat;
            std::uint32_t resourceDimension;
            std::uint32_t miscFlag;
            std::uint32_t arraySize;
            std::uint32_t miscFlags2;
        };

        static_assert(sizeof(DdsPixelFormat) == 32U);
        static_assert(sizeof(DdsHeader) == 124U);
        static_assert(sizeof(DdsHeaderDx10) == 20U);

        template<typename T>
        [[nodiscard]] bool ReadStructure(
            const std::byte* data,
            const std::size_t dataSize,
            std::size_t& offset,
            T& outValue) noexcept
        {
            if (data == nullptr)
            {
                return false;
            }

            if (
                offset > dataSize ||
                sizeof(T) > dataSize - offset
            )
            {
                return false;
            }

            std::memcpy(
                &outValue,
                data + offset,
                sizeof(T));

            offset += sizeof(T);
            return true;
        }

        [[nodiscard]] bool TryAdd(
            const std::size_t left,
            const std::size_t right,
            std::size_t& outValue) noexcept
        {
            if (
                left >
                (std::numeric_limits<std::size_t>::max)() -
                    right
            )
            {
                outValue = 0U;
                return false;
            }

            outValue = left + right;
            return true;
        }

        [[nodiscard]] bool TryMultiply(
            const std::size_t left,
            const std::size_t right,
            std::size_t& outValue) noexcept
        {
            if (left == 0U || right == 0U)
            {
                outValue = 0U;
                return true;
            }

            if (
                left >
                (std::numeric_limits<std::size_t>::max)() /
                    right
            )
            {
                outValue = 0U;
                return false;
            }

            outValue = left * right;
            return true;
        }

        [[nodiscard]] std::uint32_t NextMipDimension(
            const std::uint32_t value) noexcept
        {
            return (std::max)(1U, value / 2U);
        }

        [[nodiscard]] engine::graphics::Format ToSrgb(
            const engine::graphics::Format format) noexcept
        {
            using engine::graphics::Format;

            switch (format)
            {
            case Format::R8G8B8A8UNorm:
                return Format::R8G8B8A8UNormSrgb;

            case Format::B8G8R8A8UNorm:
                return Format::B8G8R8A8UNormSrgb;

            case Format::BC1UNorm:
                return Format::BC1UNormSrgb;

            case Format::BC2UNorm:
                return Format::BC2UNormSrgb;

            case Format::BC3UNorm:
                return Format::BC3UNormSrgb;

            case Format::BC7UNorm:
                return Format::BC7UNormSrgb;

            default:
                return format;
            }
        }

        [[nodiscard]] bool IsBc7(
            const engine::graphics::Format format) noexcept
        {
            return
                format ==
                    engine::graphics::Format::BC7UNorm ||
                format ==
                    engine::graphics::Format::BC7UNormSrgb;
        }

        [[nodiscard]] bool TryMapDxgiFormat(
            const std::uint32_t dxgiFormat,
            engine::graphics::Format& outFormat,
            bool& outForceOpaqueAlpha) noexcept
        {
            using engine::graphics::Format;

            outFormat = Format::Unknown;
            outForceOpaqueAlpha = false;

            switch (dxgiFormat)
            {
            case 2U:
                outFormat = Format::R32G32B32A32Float;
                return true;

            case 6U:
                outFormat = Format::R32G32B32Float;
                return true;

            case 10U:
                outFormat = Format::R16G16B16A16Float;
                return true;

            case 16U:
                outFormat = Format::R32G32Float;
                return true;

            case 28U:
                outFormat = Format::R8G8B8A8UNorm;
                return true;

            case 29U:
                outFormat = Format::R8G8B8A8UNormSrgb;
                return true;

            case 34U:
                outFormat = Format::R16G16Float;
                return true;

            case 41U:
                outFormat = Format::R32Float;
                return true;

            case 49U:
                outFormat = Format::R8G8UNorm;
                return true;

            case 54U:
                outFormat = Format::R16Float;
                return true;

            case 56U:
                outFormat = Format::R16UNorm;
                return true;

            case 61U:
                outFormat = Format::R8UNorm;
                return true;

            case 71U:
                outFormat = Format::BC1UNorm;
                return true;

            case 72U:
                outFormat = Format::BC1UNormSrgb;
                return true;

            case 74U:
                outFormat = Format::BC2UNorm;
                return true;

            case 75U:
                outFormat = Format::BC2UNormSrgb;
                return true;

            case 77U:
                outFormat = Format::BC3UNorm;
                return true;

            case 78U:
                outFormat = Format::BC3UNormSrgb;
                return true;

            case 80U:
                outFormat = Format::BC4UNorm;
                return true;

            case 83U:
                outFormat = Format::BC5UNorm;
                return true;

            case 87U:
                outFormat = Format::B8G8R8A8UNorm;
                return true;

            case 88U:
                outFormat = Format::B8G8R8A8UNorm;
                outForceOpaqueAlpha = true;
                return true;

            case 91U:
                outFormat = Format::B8G8R8A8UNormSrgb;
                return true;

            case 93U:
                outFormat = Format::B8G8R8A8UNormSrgb;
                outForceOpaqueAlpha = true;
                return true;

            case 98U:
                outFormat = Format::BC7UNorm;
                return true;

            case 99U:
                outFormat = Format::BC7UNormSrgb;
                return true;

            default:
                return false;
            }
        }

        [[nodiscard]] bool TryMapLegacyFormat(
            const DdsPixelFormat& pixelFormat,
            engine::graphics::Format& outFormat,
            bool& outForceOpaqueAlpha) noexcept
        {
            using engine::graphics::Format;

            outFormat = Format::Unknown;
            outForceOpaqueAlpha = false;

            if (
                (pixelFormat.flags & DdpfFourCc) != 0U
            )
            {
                switch (pixelFormat.fourCc)
                {
                case FourCcDxt1:
                    outFormat = Format::BC1UNorm;
                    return true;

                case FourCcDxt2:
                case FourCcDxt3:
                    outFormat = Format::BC2UNorm;
                    return true;

                case FourCcDxt4:
                case FourCcDxt5:
                    outFormat = Format::BC3UNorm;
                    return true;

                case FourCcAti1:
                case FourCcBc4U:
                    outFormat = Format::BC4UNorm;
                    return true;

                case FourCcAti2:
                case FourCcBc5U:
                    outFormat = Format::BC5UNorm;
                    return true;

                default:
                    return false;
                }
            }

            if (
                (pixelFormat.flags & DdpfRgb) != 0U &&
                pixelFormat.rgbBitCount == 32U
            )
            {
                const bool hasAlpha =
                    (pixelFormat.flags &
                     DdpfAlphaPixels) != 0U &&
                    pixelFormat.alphaMask ==
                        0xFF000000U;

                const bool noAlpha =
                    pixelFormat.alphaMask == 0U;

                if (!hasAlpha && !noAlpha)
                {
                    return false;
                }

                if (
                    pixelFormat.redMask ==
                        0x000000FFU &&
                    pixelFormat.greenMask ==
                        0x0000FF00U &&
                    pixelFormat.blueMask ==
                        0x00FF0000U
                )
                {
                    outFormat =
                        Format::R8G8B8A8UNorm;

                    outForceOpaqueAlpha =
                        noAlpha;

                    return true;
                }

                if (
                    pixelFormat.redMask ==
                        0x00FF0000U &&
                    pixelFormat.greenMask ==
                        0x0000FF00U &&
                    pixelFormat.blueMask ==
                        0x000000FFU
                )
                {
                    outFormat =
                        Format::B8G8R8A8UNorm;

                    outForceOpaqueAlpha =
                        noAlpha;

                    return true;
                }

                return false;
            }

            if (
                (pixelFormat.flags & DdpfLuminance) != 0U
            )
            {
                if (
                    pixelFormat.rgbBitCount == 8U &&
                    pixelFormat.redMask == 0xFFU
                )
                {
                    outFormat = Format::R8UNorm;
                    return true;
                }

                if (
                    pixelFormat.rgbBitCount == 16U &&
                    pixelFormat.redMask == 0xFFFFU
                )
                {
                    outFormat = Format::R16UNorm;
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] AssetResult BuildLayouts(
            const std::byte* sourceData,
            const std::size_t sourceSize,
            const std::size_t pixelDataOffset,
            const engine::graphics::TextureDesc& desc,
            std::vector<TextureSubresourceLayout>&
                outLayouts,
            std::size_t& outConsumedSize) noexcept
        {
            outLayouts.clear();
            outConsumedSize = 0U;

            std::size_t cursor = pixelDataOffset;

            const std::uint32_t layerCount =
                desc.dimension ==
                    engine::graphics::
                        TextureDimension::Texture3D
                ? 1U
                : desc.arrayLayers;

            try
            {
                std::size_t reserveCount = 0U;

                if (
                    !TryMultiply(
                        static_cast<std::size_t>(
                            layerCount),
                        static_cast<std::size_t>(
                            desc.mipLevels),
                        reserveCount)
                )
                {
                    return AssetResult::CorruptData;
                }

                outLayouts.reserve(reserveCount);

                for (
                    std::uint32_t layer = 0U;
                    layer < layerCount;
                    ++layer
                )
                {
                    std::uint32_t width = desc.width;
                    std::uint32_t height = desc.height;
                    std::uint32_t depth = desc.depth;

                    for (
                        std::uint32_t mip = 0U;
                        mip < desc.mipLevels;
                        ++mip
                    )
                    {
                        const std::size_t rowPitch =
                            engine::graphics::
                                CalculateRowPitch(
                                    desc.format,
                                    width);

                        const std::size_t slicePitch =
                            engine::graphics::
                                CalculateSlicePitch(
                                    desc.format,
                                    width,
                                    height);

                        if (
                            rowPitch == 0U ||
                            slicePitch == 0U
                        )
                        {
                            return
                                AssetResult::UnsupportedFormat;
                        }

                        const std::size_t mipDepth =
                            desc.dimension ==
                                engine::graphics::
                                    TextureDimension::Texture3D
                            ? static_cast<std::size_t>(
                                depth)
                            : 1U;

                        std::size_t dataSize = 0U;

                        if (
                            !TryMultiply(
                                slicePitch,
                                mipDepth,
                                dataSize)
                        )
                        {
                            return AssetResult::CorruptData;
                        }

                        std::size_t endOffset = 0U;

                        if (
                            !TryAdd(
                                cursor,
                                dataSize,
                                endOffset) ||
                            endOffset > sourceSize
                        )
                        {
                            return AssetResult::CorruptData;
                        }

                        TextureSubresourceLayout layout;
                        layout.mipLevel = mip;
                        layout.arrayLayer = layer;
                        layout.offset =
                            cursor - pixelDataOffset;
                        layout.dataSize = dataSize;
                        layout.rowPitch = rowPitch;
                        layout.slicePitch = slicePitch;

                        outLayouts.push_back(layout);

                        cursor = endOffset;

                        width =
                            NextMipDimension(width);

                        height =
                            NextMipDimension(height);

                        depth =
                            NextMipDimension(depth);
                    }
                }

                outConsumedSize =
                    cursor - pixelDataOffset;

                return AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                outLayouts.clear();
                return AssetResult::OutOfMemory;
            }
            catch (...)
            {
                outLayouts.clear();
                return AssetResult::InternalError;
            }
        }

        void ForceOpaqueAlpha(
            std::vector<std::byte>& bytes,
            const std::vector<
                TextureSubresourceLayout>& layouts) noexcept
        {
            constexpr std::byte opaqueAlpha{
                static_cast<unsigned char>(0xFFU)
            };

            for (
                const TextureSubresourceLayout& layout :
                layouts
            )
            {
                if (
                    layout.offset > bytes.size() ||
                    layout.dataSize >
                        bytes.size() - layout.offset
                )
                {
                    continue;
                }

                std::byte* subresource =
                    bytes.data() + layout.offset;

                for (
                    std::size_t offset = 3U;
                    offset < layout.dataSize;
                    offset += 4U
                )
                {
                    subresource[offset] = opaqueAlpha;
                }
            }
        }
    }

    bool DdsTextureDecoder::IsDds(
        const AssetData& source) noexcept
    {
        if (
            source.GetData() == nullptr ||
            source.GetSize() < sizeof(std::uint32_t)
        )
        {
            return false;
        }

        std::uint32_t magic = 0U;

        std::memcpy(
            &magic,
            source.GetData(),
            sizeof(magic));

        return magic == DdsMagic;
    }

    AssetResult DdsTextureDecoder::Decode(
        const AssetData& source,
        TextureAsset& outTexture) noexcept
    {
        return Decode(
            source,
            DdsTextureDecodeOptions{},
            outTexture);
    }

    AssetResult DdsTextureDecoder::Decode(
        const AssetData& source,
        const DdsTextureDecodeOptions& options,
        TextureAsset& outTexture) noexcept
    {
        outTexture.Clear();

        if (!IsDds(source))
        {
            return AssetResult::UnsupportedFormat;
        }

        const std::byte* bytes =
            source.GetData();

        const std::size_t byteCount =
            source.GetSize();

        std::size_t offset = 0U;

        std::uint32_t magic = 0U;
        DdsHeader header{};

        if (
            !ReadStructure(
                bytes,
                byteCount,
                offset,
                magic) ||
            !ReadStructure(
                bytes,
                byteCount,
                offset,
                header)
        )
        {
            return AssetResult::CorruptData;
        }

        if (
            magic != DdsMagic ||
            header.size != sizeof(DdsHeader) ||
            header.pixelFormat.size !=
                sizeof(DdsPixelFormat) ||
            header.width == 0U ||
            header.height == 0U
        )
        {
            return AssetResult::CorruptData;
        }

        const bool hasDx10Header =
            (header.pixelFormat.flags &
             DdpfFourCc) != 0U &&
            header.pixelFormat.fourCc ==
                FourCcDx10;

        DdsHeaderDx10 dx10Header{};

        if (
            hasDx10Header &&
            !ReadStructure(
                bytes,
                byteCount,
                offset,
                dx10Header)
        )
        {
            return AssetResult::CorruptData;
        }

        engine::graphics::Format format =
            engine::graphics::Format::Unknown;

        bool forceOpaqueAlpha = false;

        engine::graphics::TextureDimension dimension =
            engine::graphics::TextureDimension::Texture2D;

        std::uint32_t arrayLayers = 1U;
        std::uint32_t depth = 1U;

        if (hasDx10Header)
        {
            if (
                !TryMapDxgiFormat(
                    dx10Header.dxgiFormat,
                    format,
                    forceOpaqueAlpha)
            )
            {
                return AssetResult::UnsupportedFormat;
            }

            if (dx10Header.arraySize == 0U)
            {
                return AssetResult::CorruptData;
            }

            switch (dx10Header.resourceDimension)
            {
            case D3D10ResourceDimensionTexture1D:
                if (
                    (dx10Header.miscFlag &
                     D3D10ResourceMiscTextureCube) != 0U
                )
                {
                    return AssetResult::CorruptData;
                }

                dimension =
                    engine::graphics::
                        TextureDimension::Texture1D;

                arrayLayers =
                    dx10Header.arraySize;

                depth = 1U;
                break;

            case D3D10ResourceDimensionTexture2D:
                if (
                    (dx10Header.miscFlag &
                     D3D10ResourceMiscTextureCube) != 0U
                )
                {
                    std::size_t cubeLayerCount = 0U;

                    if (
                        !TryMultiply(
                            static_cast<std::size_t>(
                                dx10Header.arraySize),
                            6U,
                            cubeLayerCount) ||
                        cubeLayerCount >
                            (std::numeric_limits<
                                std::uint32_t>::max)()
                    )
                    {
                        return AssetResult::CorruptData;
                    }

                    dimension =
                        engine::graphics::
                            TextureDimension::TextureCube;

                    arrayLayers =
                        static_cast<std::uint32_t>(
                            cubeLayerCount);
                }
                else
                {
                    dimension =
                        engine::graphics::
                            TextureDimension::Texture2D;

                    arrayLayers =
                        dx10Header.arraySize;
                }

                depth = 1U;
                break;

            case D3D10ResourceDimensionTexture3D:
                if (
                    dx10Header.arraySize != 1U ||
                    (dx10Header.miscFlag &
                     D3D10ResourceMiscTextureCube) != 0U ||
                    header.depth == 0U
                )
                {
                    return AssetResult::CorruptData;
                }

                dimension =
                    engine::graphics::
                        TextureDimension::Texture3D;

                arrayLayers = 1U;
                depth = header.depth;
                break;

            default:
                return AssetResult::UnsupportedFormat;
            }
        }
        else
        {
            if (
                !TryMapLegacyFormat(
                    header.pixelFormat,
                    format,
                    forceOpaqueAlpha)
            )
            {
                return AssetResult::UnsupportedFormat;
            }

            if (
                (header.caps2 &
                 DdsCaps2CubeMap) != 0U
            )
            {
                if (
                    (header.caps2 &
                     DdsCaps2CubeMapAllFaces) !=
                    DdsCaps2CubeMapAllFaces
                )
                {
                    return AssetResult::CorruptData;
                }

                dimension =
                    engine::graphics::
                        TextureDimension::TextureCube;

                arrayLayers = 6U;
                depth = 1U;
            }
            else if (
                (header.caps2 &
                 DdsCaps2Volume) != 0U
            )
            {
                if (header.depth == 0U)
                {
                    return AssetResult::CorruptData;
                }

                dimension =
                    engine::graphics::
                        TextureDimension::Texture3D;

                arrayLayers = 1U;
                depth = header.depth;
            }
        }

        if (options.forceSrgb)
        {
            format = ToSrgb(format);
        }

        if (
            IsBc7(format) &&
            !options.allowBc7
        )
        {
            return AssetResult::UnsupportedFormat;
        }

        engine::graphics::TextureDesc desc;
        desc.dimension = dimension;

        desc.width = header.width;

        desc.height =
            dimension ==
                engine::graphics::
                    TextureDimension::Texture1D
            ? 1U
            : header.height;

        desc.depth = depth;
        desc.arrayLayers = arrayLayers;

        desc.mipLevels =
            header.mipMapCount == 0U
            ? 1U
            : header.mipMapCount;

        desc.sampleCount = 1U;
        desc.format = format;

        desc.usage =
            engine::graphics::ResourceUsage::Immutable;

        desc.bindFlags =
            engine::graphics::
                TextureBindFlags::ShaderResource;

        desc.cpuAccess =
            engine::graphics::CpuAccessFlags::None;

        desc.generateMipmaps = false;

        if (!desc.IsValid())
        {
            return AssetResult::CorruptData;
        }

        std::vector<TextureSubresourceLayout> layouts;

        std::size_t consumedDataSize = 0U;

        AssetResult result =
            BuildLayouts(
                bytes,
                byteCount,
                offset,
                desc,
                layouts,
                consumedDataSize);

        if (Failed(result))
        {
            return result;
        }

        if (
            consumedDataSize == 0U ||
            offset > byteCount ||
            consumedDataSize >
                byteCount - offset
        )
        {
            return AssetResult::CorruptData;
        }

        try
        {
            outTexture.bytes_.resize(
                consumedDataSize);

            std::memcpy(
                outTexture.bytes_.data(),
                bytes + offset,
                consumedDataSize);

            outTexture.desc_ = desc;
            outTexture.subresources_ =
                std::move(layouts);

            if (forceOpaqueAlpha)
            {
                ForceOpaqueAlpha(
                    outTexture.bytes_,
                    outTexture.subresources_);
            }

            if (!outTexture.IsValid())
            {
                outTexture.Clear();
                return AssetResult::CorruptData;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            outTexture.Clear();
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            outTexture.Clear();
            return AssetResult::InternalError;
        }
    }
}