#pragma once

#include <cstdint>

namespace engine::graphics
{
    enum class ResourceUsage : std::uint8_t
    {
        Default = 0,
        Immutable,
        Dynamic,
        Staging
    };

    enum class CpuAccessFlags : std::uint8_t
    {
        None = 0,
        Read = 1U << 0U,
        Write = 1U << 1U
    };

    [[nodiscard]] constexpr CpuAccessFlags operator|(
        const CpuAccessFlags left,
        const CpuAccessFlags right) noexcept
    {
        return static_cast<CpuAccessFlags>(
            static_cast<std::uint8_t>(left) |
            static_cast<std::uint8_t>(right));
    }

    [[nodiscard]] constexpr CpuAccessFlags operator&(
        const CpuAccessFlags left,
        const CpuAccessFlags right) noexcept
    {
        return static_cast<CpuAccessFlags>(
            static_cast<std::uint8_t>(left) &
            static_cast<std::uint8_t>(right));
    }

    constexpr CpuAccessFlags& operator|=(
        CpuAccessFlags& left,
        const CpuAccessFlags right) noexcept
    {
        left = left | right;
        return left;
    }

    [[nodiscard]] constexpr bool HasAnyFlag(
        const CpuAccessFlags value,
        const CpuAccessFlags flags) noexcept
    {
        return (value & flags) != CpuAccessFlags::None;
    }
}
