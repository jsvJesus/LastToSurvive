#pragma once

#include <cstdint>

namespace engine::platform
{
    class Clock final
    {
    public:
        using Tick = std::uint64_t;

        [[nodiscard]] static Tick Now() noexcept;

        [[nodiscard]] static Tick Frequency() noexcept;

        [[nodiscard]] static double ToSeconds(
            Tick ticks) noexcept;

        [[nodiscard]] static double ToMilliseconds(
            Tick ticks) noexcept;

        [[nodiscard]] static double ElapsedSeconds(
            Tick start,
            Tick end) noexcept;

        [[nodiscard]] static double ElapsedMilliseconds(
            Tick start,
            Tick end) noexcept;
    };
}