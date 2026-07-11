#pragma once

#include <cstdint>

namespace engine::core
{
    struct Version final
    {
        std::uint16_t major = 0;
        std::uint16_t minor = 0;
        std::uint16_t patch = 0;
        std::uint16_t build = 0;

        [[nodiscard]]
        constexpr std::uint64_t Packed() const noexcept
        {
            return
                (static_cast<std::uint64_t>(major) << 48U) |
                (static_cast<std::uint64_t>(minor) << 32U) |
                (static_cast<std::uint64_t>(patch) << 16U) |
                static_cast<std::uint64_t>(build);
        }
    };

    [[nodiscard]]
    constexpr bool operator==(
        const Version& left,
        const Version& right) noexcept
    {
        return
            left.major == right.major &&
            left.minor == right.minor &&
            left.patch == right.patch &&
            left.build == right.build;
    }

    [[nodiscard]]
    constexpr bool operator!=(
        const Version& left,
        const Version& right) noexcept
    {
        return !(left == right);
    }

    [[nodiscard]]
    Version GetEngineVersion() noexcept;
}