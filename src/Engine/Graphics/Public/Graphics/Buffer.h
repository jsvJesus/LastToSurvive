#pragma once

#include "Graphics/ResourceUsage.h"

#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    enum class BufferBindFlags : std::uint16_t
    {
        None = 0,
        Vertex = 1U << 0U,
        Index = 1U << 1U,
        Constant = 1U << 2U,
        ShaderResource = 1U << 3U,
        UnorderedAccess = 1U << 4U,
        IndirectArguments = 1U << 5U
    };

    enum class BufferMiscFlags : std::uint8_t
    {
        None = 0,
        Structured = 1U << 0U,
        Raw = 1U << 1U
    };

    enum class IndexFormat : std::uint8_t
    {
        None = 0,
        UInt16,
        UInt32
    };

    [[nodiscard]] constexpr BufferBindFlags operator|(
        const BufferBindFlags left,
        const BufferBindFlags right) noexcept
    {
        return static_cast<BufferBindFlags>(
            static_cast<std::uint16_t>(left) |
            static_cast<std::uint16_t>(right));
    }

    [[nodiscard]] constexpr BufferBindFlags operator&(
        const BufferBindFlags left,
        const BufferBindFlags right) noexcept
    {
        return static_cast<BufferBindFlags>(
            static_cast<std::uint16_t>(left) &
            static_cast<std::uint16_t>(right));
    }

    constexpr BufferBindFlags& operator|=(
        BufferBindFlags& left,
        const BufferBindFlags right) noexcept
    {
        left = left | right;
        return left;
    }

    [[nodiscard]] constexpr bool HasAnyFlag(
        const BufferBindFlags value,
        const BufferBindFlags flags) noexcept
    {
        return (value & flags) != BufferBindFlags::None;
    }

    [[nodiscard]] constexpr BufferMiscFlags operator|(
        const BufferMiscFlags left,
        const BufferMiscFlags right) noexcept
    {
        return static_cast<BufferMiscFlags>(
            static_cast<std::uint8_t>(left) |
            static_cast<std::uint8_t>(right));
    }

    [[nodiscard]] constexpr BufferMiscFlags operator&(
        const BufferMiscFlags left,
        const BufferMiscFlags right) noexcept
    {
        return static_cast<BufferMiscFlags>(
            static_cast<std::uint8_t>(left) &
            static_cast<std::uint8_t>(right));
    }

    constexpr BufferMiscFlags& operator|=(
        BufferMiscFlags& left,
        const BufferMiscFlags right) noexcept
    {
        left = left | right;
        return left;
    }

    [[nodiscard]] constexpr bool HasAnyFlag(
        const BufferMiscFlags value,
        const BufferMiscFlags flags) noexcept
    {
        return (value & flags) != BufferMiscFlags::None;
    }

    struct BufferDesc final
    {
        std::size_t byteSize = 0;
        std::uint32_t stride = 0;

        ResourceUsage usage = ResourceUsage::Default;
        BufferBindFlags bindFlags = BufferBindFlags::None;
        BufferMiscFlags miscFlags = BufferMiscFlags::None;
        CpuAccessFlags cpuAccess = CpuAccessFlags::None;
        IndexFormat indexFormat = IndexFormat::None;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct BufferInitialData final
    {
        const std::byte* data = nullptr;
        std::size_t dataSize = 0;

        [[nodiscard]] bool IsValid() const noexcept;
    };
}
