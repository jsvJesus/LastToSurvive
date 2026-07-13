#include "Graphics/Format.h"

#include <limits>

namespace engine::graphics
{
    namespace
    {
        [[nodiscard]] constexpr FormatInfo MakeLinear(
            const std::uint8_t bytesPerPixel,
            const bool depth = false,
            const bool stencil = false,
            const bool srgb = false) noexcept
        {
            return FormatInfo{
                1,
                1,
                bytesPerPixel,
                false,
                depth,
                stencil,
                srgb
            };
        }

        [[nodiscard]] constexpr FormatInfo MakeCompressed(
            const std::uint8_t bytesPerBlock,
            const bool srgb = false) noexcept
        {
            return FormatInfo{
                4,
                4,
                bytesPerBlock,
                true,
                false,
                false,
                srgb
            };
        }

        [[nodiscard]] bool TryMultiply(
            const std::size_t left,
            const std::size_t right,
            std::size_t& outValue) noexcept
        {
            if (left == 0 || right == 0)
            {
                outValue = 0;
                return true;
            }

            if (left >
                (std::numeric_limits<std::size_t>::max)() / right)
            {
                outValue = 0;
                return false;
            }

            outValue = left * right;
            return true;
        }
    }

    FormatInfo GetFormatInfo(
        const Format format) noexcept
    {
        switch (format)
        {
        case Format::R8UNorm:
            return MakeLinear(1);
        case Format::R8G8UNorm:
            return MakeLinear(2);
        case Format::R8G8B8A8UNorm:
            return MakeLinear(4);
        case Format::R8G8B8A8UNormSrgb:
            return MakeLinear(4, false, false, true);
        case Format::B8G8R8A8UNorm:
            return MakeLinear(4);
        case Format::B8G8R8A8UNormSrgb:
            return MakeLinear(4, false, false, true);
        case Format::R16UNorm:
        case Format::R16Float:
            return MakeLinear(2);
        case Format::R16G16Float:
            return MakeLinear(4);
        case Format::R16G16B16A16Float:
            return MakeLinear(8);
        case Format::R32Float:
            return MakeLinear(4);
        case Format::R32G32Float:
            return MakeLinear(8);
        case Format::R32G32B32Float:
            return MakeLinear(12);
        case Format::R32G32B32A32Float:
            return MakeLinear(16);
        case Format::D16UNorm:
            return MakeLinear(2, true);
        case Format::D24UNormS8UInt:
            return MakeLinear(4, true, true);
        case Format::D32Float:
            return MakeLinear(4, true);
        case Format::BC1UNorm:
            return MakeCompressed(8);
        case Format::BC1UNormSrgb:
            return MakeCompressed(8, true);
        case Format::BC2UNorm:
            return MakeCompressed(16);
        case Format::BC2UNormSrgb:
            return MakeCompressed(16, true);
        case Format::BC3UNorm:
            return MakeCompressed(16);
        case Format::BC3UNormSrgb:
            return MakeCompressed(16, true);
        case Format::BC4UNorm:
            return MakeCompressed(8);
        case Format::BC5UNorm:
            return MakeCompressed(16);
        case Format::BC7UNorm:
            return MakeCompressed(16);
        case Format::BC7UNormSrgb:
            return MakeCompressed(16, true);
        case Format::Unknown:
        case Format::Count:
        default:
            return FormatInfo{};
        }
    }

    bool IsColorFormat(
        const Format format) noexcept
    {
        const FormatInfo info = GetFormatInfo(format);
        return info.bytesPerBlock != 0 && !info.depth;
    }

    bool IsDepthFormat(
        const Format format) noexcept
    {
        return GetFormatInfo(format).depth;
    }

    bool IsStencilFormat(
        const Format format) noexcept
    {
        return GetFormatInfo(format).stencil;
    }

    bool IsCompressedFormat(
        const Format format) noexcept
    {
        return GetFormatInfo(format).compressed;
    }

    bool IsSrgbFormat(
        const Format format) noexcept
    {
        return GetFormatInfo(format).srgb;
    }

    std::size_t CalculateRowPitch(
        const Format format,
        const std::uint32_t width) noexcept
    {
        const FormatInfo info = GetFormatInfo(format);
        if (width == 0 || info.bytesPerBlock == 0)
        {
            return 0;
        }

        const std::size_t blockCount =
            (static_cast<std::size_t>(width) +
                static_cast<std::size_t>(info.blockWidth) - 1U) /
            static_cast<std::size_t>(info.blockWidth);

        std::size_t rowPitch = 0;
        if (!TryMultiply(
                blockCount,
                static_cast<std::size_t>(info.bytesPerBlock),
                rowPitch))
        {
            return 0;
        }

        return rowPitch;
    }

    std::size_t CalculateSlicePitch(
        const Format format,
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        const FormatInfo info = GetFormatInfo(format);
        const std::size_t rowPitch =
            CalculateRowPitch(format, width);

        if (rowPitch == 0 || height == 0 || info.blockHeight == 0)
        {
            return 0;
        }

        const std::size_t rowCount =
            (static_cast<std::size_t>(height) +
                static_cast<std::size_t>(info.blockHeight) - 1U) /
            static_cast<std::size_t>(info.blockHeight);

        std::size_t slicePitch = 0;
        if (!TryMultiply(rowPitch, rowCount, slicePitch))
        {
            return 0;
        }

        return slicePitch;
    }

    const char* ToString(
        const Format format) noexcept
    {
        switch (format)
        {
        case Format::Unknown:
            return "Unknown";
        case Format::R8UNorm:
            return "R8UNorm";
        case Format::R8G8UNorm:
            return "R8G8UNorm";
        case Format::R8G8B8A8UNorm:
            return "R8G8B8A8UNorm";
        case Format::R8G8B8A8UNormSrgb:
            return "R8G8B8A8UNormSrgb";
        case Format::B8G8R8A8UNorm:
            return "B8G8R8A8UNorm";
        case Format::B8G8R8A8UNormSrgb:
            return "B8G8R8A8UNormSrgb";
        case Format::R16UNorm:
            return "R16UNorm";
        case Format::R16Float:
            return "R16Float";
        case Format::R16G16Float:
            return "R16G16Float";
        case Format::R16G16B16A16Float:
            return "R16G16B16A16Float";
        case Format::R32Float:
            return "R32Float";
        case Format::R32G32Float:
            return "R32G32Float";
        case Format::R32G32B32Float:
            return "R32G32B32Float";
        case Format::R32G32B32A32Float:
            return "R32G32B32A32Float";
        case Format::D16UNorm:
            return "D16UNorm";
        case Format::D24UNormS8UInt:
            return "D24UNormS8UInt";
        case Format::D32Float:
            return "D32Float";
        case Format::BC1UNorm:
            return "BC1UNorm";
        case Format::BC1UNormSrgb:
            return "BC1UNormSrgb";
        case Format::BC2UNorm:
            return "BC2UNorm";
        case Format::BC2UNormSrgb:
            return "BC2UNormSrgb";
        case Format::BC3UNorm:
            return "BC3UNorm";
        case Format::BC3UNormSrgb:
            return "BC3UNormSrgb";
        case Format::BC4UNorm:
            return "BC4UNorm";
        case Format::BC5UNorm:
            return "BC5UNorm";
        case Format::BC7UNorm:
            return "BC7UNorm";
        case Format::BC7UNormSrgb:
            return "BC7UNormSrgb";
        case Format::Count:
        default:
            return "Invalid";
        }
    }
}
