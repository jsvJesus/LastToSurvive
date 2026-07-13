#pragma once

#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    enum class Format : std::uint16_t
    {
        Unknown = 0,

        R8UNorm,
        R8G8UNorm,
        R8G8B8A8UNorm,
        R8G8B8A8UNormSrgb,
        B8G8R8A8UNorm,
        B8G8R8A8UNormSrgb,

        R16UNorm,
        R16Float,
        R16G16Float,
        R16G16B16A16Float,

        R32Float,
        R32G32Float,
        R32G32B32Float,
        R32G32B32A32Float,

        D16UNorm,
        D24UNormS8UInt,
        D32Float,

        BC1UNorm,
        BC1UNormSrgb,
        BC2UNorm,
        BC2UNormSrgb,
        BC3UNorm,
        BC3UNormSrgb,
        BC4UNorm,
        BC5UNorm,
        BC7UNorm,
        BC7UNormSrgb,

        Count
    };

    struct FormatInfo final
    {
        std::uint8_t blockWidth = 0;
        std::uint8_t blockHeight = 0;
        std::uint8_t bytesPerBlock = 0;

        bool compressed = false;
        bool depth = false;
        bool stencil = false;
        bool srgb = false;
    };

    [[nodiscard]] FormatInfo GetFormatInfo(
        Format format) noexcept;

    [[nodiscard]] bool IsColorFormat(
        Format format) noexcept;

    [[nodiscard]] bool IsDepthFormat(
        Format format) noexcept;

    [[nodiscard]] bool IsStencilFormat(
        Format format) noexcept;

    [[nodiscard]] bool IsCompressedFormat(
        Format format) noexcept;

    [[nodiscard]] bool IsSrgbFormat(
        Format format) noexcept;

    [[nodiscard]] std::size_t CalculateRowPitch(
        Format format,
        std::uint32_t width) noexcept;

    [[nodiscard]] std::size_t CalculateSlicePitch(
        Format format,
        std::uint32_t width,
        std::uint32_t height) noexcept;

    [[nodiscard]] const char* ToString(
        Format format) noexcept;
}
