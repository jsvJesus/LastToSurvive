#pragma once

#include "Math/Scalar.h"

#include <cstdint>

namespace engine::math
{
    struct Color32 final
    {
        std::uint8_t r = 0U;
        std::uint8_t g = 0U;
        std::uint8_t b = 0U;
        std::uint8_t a = 0U;

        constexpr Color32() noexcept = default;

        constexpr Color32(
            const std::uint8_t red,
            const std::uint8_t green,
            const std::uint8_t blue,
            const std::uint8_t alpha = 255U) noexcept
            : r(red)
            , g(green)
            , b(blue)
            , a(alpha)
        {
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Color32& left,
            const Color32& right) noexcept
        {
            return
                left.r == right.r &&
                left.g == right.g &&
                left.b == right.b &&
                left.a == right.a;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Color32& left,
            const Color32& right) noexcept
        {
            return !(left == right);
        }
    };

    struct Color final
    {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;

        constexpr Color() noexcept = default;

        constexpr Color(
            const float red,
            const float green,
            const float blue,
            const float alpha = 1.0f) noexcept
            : r(red)
            , g(green)
            , b(blue)
            , a(alpha)
        {
        }

        [[nodiscard]]
        static constexpr Color FromRGBA8(
            const Color32 value) noexcept
        {
            constexpr float inverseByte =
                1.0f / 255.0f;

            return
            {
                static_cast<float>(value.r) * inverseByte,
                static_cast<float>(value.g) * inverseByte,
                static_cast<float>(value.b) * inverseByte,
                static_cast<float>(value.a) * inverseByte
            };
        }

        [[nodiscard]]
        Color32 ToRGBA8() const noexcept
        {
            const Color clamped = Clamped();

            const auto toByte =
                [](const float value) noexcept
                -> std::uint8_t
                {
                    return static_cast<std::uint8_t>(
                        (value * 255.0f) + 0.5f
                    );
                };

            return
            {
                toByte(clamped.r),
                toByte(clamped.g),
                toByte(clamped.b),
                toByte(clamped.a)
            };
        }

        [[nodiscard]]
        constexpr Color Clamped() const noexcept
        {
            return
            {
                Saturate(r),
                Saturate(g),
                Saturate(b),
                Saturate(a)
            };
        }

        [[nodiscard]]
        constexpr Color WithAlpha(
            const float alpha) const noexcept
        {
            return
            {
                r,
                g,
                b,
                alpha
            };
        }

        [[nodiscard]]
        constexpr bool IsNearlyEqual(
            const Color& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                NearlyEqual(r, other.r, epsilon) &&
                NearlyEqual(g, other.g, epsilon) &&
                NearlyEqual(b, other.b, epsilon) &&
                NearlyEqual(a, other.a, epsilon);
        }

        [[nodiscard]]
        static constexpr Color Lerp(
            const Color& start,
            const Color& end,
            const float amount) noexcept
        {
            return
            {
                engine::math::Lerp(
                    start.r,
                    end.r,
                    amount
                ),
                engine::math::Lerp(
                    start.g,
                    end.g,
                    amount
                ),
                engine::math::Lerp(
                    start.b,
                    end.b,
                    amount
                ),
                engine::math::Lerp(
                    start.a,
                    end.a,
                    amount
                )
            };
        }

        constexpr Color& operator+=(
            const Color& other) noexcept
        {
            r += other.r;
            g += other.g;
            b += other.b;
            a += other.a;
            return *this;
        }

        constexpr Color& operator-=(
            const Color& other) noexcept
        {
            r -= other.r;
            g -= other.g;
            b -= other.b;
            a -= other.a;
            return *this;
        }

        constexpr Color& operator*=(
            const float scalar) noexcept
        {
            r *= scalar;
            g *= scalar;
            b *= scalar;
            a *= scalar;
            return *this;
        }

        constexpr Color& operator*=(
            const Color& other) noexcept
        {
            r *= other.r;
            g *= other.g;
            b *= other.b;
            a *= other.a;
            return *this;
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Color& left,
            const Color& right) noexcept
        {
            return
                left.r == right.r &&
                left.g == right.g &&
                left.b == right.b &&
                left.a == right.a;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Color& left,
            const Color& right) noexcept
        {
            return !(left == right);
        }

        [[nodiscard]]
        friend constexpr Color operator+(
            Color left,
            const Color& right) noexcept
        {
            left += right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Color operator-(
            Color left,
            const Color& right) noexcept
        {
            left -= right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Color operator*(
            Color color,
            const float scalar) noexcept
        {
            color *= scalar;
            return color;
        }

        [[nodiscard]]
        friend constexpr Color operator*(
            const float scalar,
            Color color) noexcept
        {
            color *= scalar;
            return color;
        }

        [[nodiscard]]
        friend constexpr Color operator*(
            Color left,
            const Color& right) noexcept
        {
            left *= right;
            return left;
        }
    };

    inline constexpr Color ColorTransparent
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    inline constexpr Color ColorBlack
    {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    inline constexpr Color ColorWhite
    {
        1.0f,
        1.0f,
        1.0f,
        1.0f
    };

    inline constexpr Color ColorRed
    {
        1.0f,
        0.0f,
        0.0f,
        1.0f
    };

    inline constexpr Color ColorGreen
    {
        0.0f,
        1.0f,
        0.0f,
        1.0f
    };

    inline constexpr Color ColorBlue
    {
        0.0f,
        0.0f,
        1.0f,
        1.0f
    };
}