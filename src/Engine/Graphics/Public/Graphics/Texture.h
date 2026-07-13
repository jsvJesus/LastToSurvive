#pragma once

#include "Graphics/Format.h"
#include "Graphics/ResourceUsage.h"

#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    enum class TextureDimension : std::uint8_t
    {
        Texture1D = 0,
        Texture2D,
        Texture3D,
        TextureCube
    };

    enum class TextureBindFlags : std::uint16_t
    {
        None = 0,
        ShaderResource = 1U << 0U,
        RenderTarget = 1U << 1U,
        DepthStencil = 1U << 2U,
        UnorderedAccess = 1U << 3U
    };

    [[nodiscard]] constexpr TextureBindFlags operator|(
        const TextureBindFlags left,
        const TextureBindFlags right) noexcept
    {
        return static_cast<TextureBindFlags>(
            static_cast<std::uint16_t>(left) |
            static_cast<std::uint16_t>(right));
    }

    [[nodiscard]] constexpr TextureBindFlags operator&(
        const TextureBindFlags left,
        const TextureBindFlags right) noexcept
    {
        return static_cast<TextureBindFlags>(
            static_cast<std::uint16_t>(left) &
            static_cast<std::uint16_t>(right));
    }

    constexpr TextureBindFlags& operator|=(
        TextureBindFlags& left,
        const TextureBindFlags right) noexcept
    {
        left = left | right;
        return left;
    }

    [[nodiscard]] constexpr bool HasAnyFlag(
        const TextureBindFlags value,
        const TextureBindFlags flags) noexcept
    {
        return (value & flags) != TextureBindFlags::None;
    }

    struct TextureDesc final
    {
        TextureDimension dimension =
            TextureDimension::Texture2D;

        std::uint32_t width = 1;
        std::uint32_t height = 1;
        std::uint32_t depth = 1;
        std::uint32_t arrayLayers = 1;
        std::uint32_t mipLevels = 1;
        std::uint32_t sampleCount = 1;

        Format format = Format::R8G8B8A8UNorm;
        ResourceUsage usage = ResourceUsage::Default;
        TextureBindFlags bindFlags =
            TextureBindFlags::ShaderResource;
        CpuAccessFlags cpuAccess = CpuAccessFlags::None;

        bool generateMipmaps = false;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct TextureSubresourceData final
    {
        const std::byte* data = nullptr;
        std::size_t dataSize = 0;
        std::size_t rowPitch = 0;
        std::size_t slicePitch = 0;

        [[nodiscard]] bool IsValid() const noexcept;
    };
}
