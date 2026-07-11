#pragma once

#include <cmath>

namespace engine::math
{
    inline constexpr float Pi =
        3.14159265358979323846f;

    inline constexpr float TwoPi =
        Pi * 2.0f;

    inline constexpr float HalfPi =
        Pi * 0.5f;

    inline constexpr float DefaultEpsilon =
        0.000001f;

    [[nodiscard]]
    constexpr float Abs(const float value) noexcept
    {
        return value < 0.0f ? -value : value;
    }

    [[nodiscard]]
    constexpr float Min(
        const float left,
        const float right) noexcept
    {
        return left < right ? left : right;
    }

    [[nodiscard]]
    constexpr float Max(
        const float left,
        const float right) noexcept
    {
        return left > right ? left : right;
    }

    [[nodiscard]]
    constexpr float Clamp(
        const float value,
        const float minimum,
        const float maximum) noexcept
    {
        return value < minimum
            ? minimum
            : value > maximum
                ? maximum
                : value;
    }

    [[nodiscard]]
    constexpr float Saturate(const float value) noexcept
    {
        return Clamp(value, 0.0f, 1.0f);
    }

    [[nodiscard]]
    constexpr float Lerp(
        const float start,
        const float end,
        const float amount) noexcept
    {
        return start + ((end - start) * amount);
    }

    [[nodiscard]]
    constexpr float DegreesToRadians(
        const float degrees) noexcept
    {
        return degrees * (Pi / 180.0f);
    }

    [[nodiscard]]
    constexpr float RadiansToDegrees(
        const float radians) noexcept
    {
        return radians * (180.0f / Pi);
    }

    [[nodiscard]]
    constexpr bool NearlyEqual(
        const float left,
        const float right,
        const float epsilon = DefaultEpsilon) noexcept
    {
        return Abs(left - right) <= epsilon;
    }

    [[nodiscard]]
    inline bool IsFinite(const float value) noexcept
    {
        return std::isfinite(value);
    }
}