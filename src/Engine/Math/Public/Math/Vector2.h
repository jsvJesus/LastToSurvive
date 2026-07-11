#pragma once

#include "Math/Scalar.h"

#include <cmath>

namespace engine::math
{
    struct Vector2 final
    {
        float x = 0.0f;
        float y = 0.0f;

        constexpr Vector2() noexcept = default;

        constexpr Vector2(
            const float xValue,
            const float yValue) noexcept
            : x(xValue)
            , y(yValue)
        {
        }

        [[nodiscard]]
        constexpr float* Data() noexcept
        {
            return &x;
        }

        [[nodiscard]]
        constexpr const float* Data() const noexcept
        {
            return &x;
        }

        [[nodiscard]]
        constexpr float& operator[](
            const unsigned index) noexcept
        {
            return index == 0U ? x : y;
        }

        [[nodiscard]]
        constexpr const float& operator[](
            const unsigned index) const noexcept
        {
            return index == 0U ? x : y;
        }

        [[nodiscard]]
        constexpr float LengthSquared() const noexcept
        {
            return (x * x) + (y * y);
        }

        [[nodiscard]]
        float Length() const noexcept
        {
            return std::sqrt(LengthSquared());
        }

        [[nodiscard]]
        constexpr float Dot(
            const Vector2& other) const noexcept
        {
            return (x * other.x) + (y * other.y);
        }

        [[nodiscard]]
        Vector2 Normalized(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            const float length = Length();

            if (length <= epsilon)
            {
                return {};
            }

            return *this / length;
        }

        bool Normalize(
            const float epsilon = DefaultEpsilon) noexcept
        {
            const float length = Length();

            if (length <= epsilon)
            {
                x = 0.0f;
                y = 0.0f;
                return false;
            }

            x /= length;
            y /= length;
            return true;
        }

        [[nodiscard]]
        constexpr bool IsNearlyZero(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                Abs(x) <= epsilon &&
                Abs(y) <= epsilon;
        }

        [[nodiscard]]
        constexpr bool IsNearlyEqual(
            const Vector2& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                NearlyEqual(x, other.x, epsilon) &&
                NearlyEqual(y, other.y, epsilon);
        }

        constexpr Vector2& operator+=(
            const Vector2& other) noexcept
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        constexpr Vector2& operator-=(
            const Vector2& other) noexcept
        {
            x -= other.x;
            y -= other.y;
            return *this;
        }

        constexpr Vector2& operator*=(
            const float scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            return *this;
        }

        constexpr Vector2& operator/=(
            const float scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            return *this;
        }

        [[nodiscard]]
        constexpr Vector2 operator-() const noexcept
        {
            return {-x, -y};
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Vector2& left,
            const Vector2& right) noexcept
        {
            return
                left.x == right.x &&
                left.y == right.y;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Vector2& left,
            const Vector2& right) noexcept
        {
            return !(left == right);
        }

        [[nodiscard]]
        friend constexpr Vector2 operator+(
            Vector2 left,
            const Vector2& right) noexcept
        {
            left += right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Vector2 operator-(
            Vector2 left,
            const Vector2& right) noexcept
        {
            left -= right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Vector2 operator*(
            Vector2 vector,
            const float scalar) noexcept
        {
            vector *= scalar;
            return vector;
        }

        [[nodiscard]]
        friend constexpr Vector2 operator*(
            const float scalar,
            Vector2 vector) noexcept
        {
            vector *= scalar;
            return vector;
        }

        [[nodiscard]]
        friend constexpr Vector2 operator/(
            Vector2 vector,
            const float scalar) noexcept
        {
            vector /= scalar;
            return vector;
        }
    };

    inline constexpr Vector2 Vector2Zero{0.0f, 0.0f};
    inline constexpr Vector2 Vector2One{1.0f, 1.0f};
    inline constexpr Vector2 Vector2UnitX{1.0f, 0.0f};
    inline constexpr Vector2 Vector2UnitY{0.0f, 1.0f};
}