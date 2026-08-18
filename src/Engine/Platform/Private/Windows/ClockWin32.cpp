#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Clock.h"

namespace engine::platform
{
    namespace
    {
        [[nodiscard]] Clock::Tick QueryClockFrequency() noexcept
        {
            LARGE_INTEGER frequency{};

            if (::QueryPerformanceFrequency(
                    &frequency) == FALSE)
            {
                return 0;
            }

            if (frequency.QuadPart <= 0)
            {
                return 0;
            }

            return static_cast<Clock::Tick>(
                frequency.QuadPart);
        }
    }

    Clock::Tick Clock::Now() noexcept
    {
        LARGE_INTEGER timestamp{};

        if (::QueryPerformanceCounter(
                &timestamp) == FALSE)
        {
            return 0;
        }

        if (timestamp.QuadPart < 0)
        {
            return 0;
        }

        return static_cast<Tick>(
            timestamp.QuadPart);
    }

    Clock::Tick Clock::Frequency() noexcept
    {
        static const Tick frequency =
            QueryClockFrequency();

        return frequency;
    }

    double Clock::ToSeconds(
        const Tick ticks) noexcept
    {
        const Tick frequency =
            Frequency();

        if (frequency == 0)
        {
            return 0.0;
        }

        return static_cast<double>(ticks) /
            static_cast<double>(frequency);
    }

    double Clock::ToMilliseconds(
        const Tick ticks) noexcept
    {
        return ToSeconds(ticks) * 1000.0;
    }

    double Clock::ElapsedSeconds(
        const Tick start,
        const Tick end) noexcept
    {
        if (end < start)
        {
            return 0.0;
        }

        return ToSeconds(
            end - start);
    }

    double Clock::ElapsedMilliseconds(
        const Tick start,
        const Tick end) noexcept
    {
        if (end < start)
        {
            return 0.0;
        }

        return ToMilliseconds(
            end - start);
    }
}