#pragma once

#include "Math/Scalar.h"

#include <cmath>

namespace engine::math
{
    struct Vector3 final
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        constexpr Vector3() noexcept = default;

        constexpr Vector3(
            const float xValue,
            const float yValue,
            const float zValue) noexcept
            : x(xValue)
            , y(yValue)
            , z(zValue)
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
            return index == 0U
                ? x
                : index == 1U
                    ? y
                    : z;
        }

        [[nodiscard]]
        constexpr const float& operator[](
            const unsigned index) const noexcept
        {
            return index == 0U
                ? x
                : index == 1U
                    ? y
                    : z;
        }

        [[nodiscard]]
        constexpr float LengthSquared() const noexcept
        {
            return
                (x * x) +
                (y * y) +
                (z * z);
        }

        [[nodiscard]]
        float Length() const noexcept
        {
            return std::sqrt(LengthSquared());
        }

        [[nodiscard]]
        constexpr float Dot(
            const Vector3& other) const noexcept
        {
            return
                (x * other.x) +
                (y * other.y) +
                (z * other.z);
        }

        [[nodiscard]]
        constexpr Vector3 Cross(
            const Vector3& other) const noexcept
        {
            return
            {
                (y * other.z) - (z * other.y),
                (z * other.x) - (x * other.z),
                (x * other.y) - (y * other.x)
            };
        }

        [[nodiscard]]
        Vector3 Normalized(
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
                z = 0.0f;
                return false;
            }

            x /= length;
            y /= length;
            z /= length;
            return true;
        }

        [[nodiscard]]
        constexpr bool IsNearlyZero(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                Abs(x) <= epsilon &&
                Abs(y) <= epsilon &&
                Abs(z) <= epsilon;
        }

        [[nodiscard]]
        constexpr bool IsNearlyEqual(
            const Vector3& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                NearlyEqual(x, other.x, epsilon) &&
                NearlyEqual(y, other.y, epsilon) &&
                NearlyEqual(z, other.z, epsilon);
        }

        constexpr Vector3& operator+=(
            const Vector3& other) noexcept
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        constexpr Vector3& operator-=(
            const Vector3& other) noexcept
        {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        constexpr Vector3& operator*=(
            const float scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }

        constexpr Vector3& operator/=(
            const float scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            return *this;
        }

        [[nodiscard]]
        constexpr Vector3 operator-() const noexcept
        {
            return {-x, -y, -z};
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Vector3& left,
            const Vector3& right) noexcept
        {
            return
                left.x == right.x &&
                left.y == right.y &&
                left.z == right.z;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Vector3& left,
            const Vector3& right) noexcept
        {
            return !(left == right);
        }

        [[nodiscard]]
        friend constexpr Vector3 operator+(
            Vector3 left,
            const Vector3& right) noexcept
        {
            left += right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Vector3 operator-(
            Vector3 left,
            const Vector3& right) noexcept
        {
            left -= right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Vector3 operator*(
            Vector3 vector,
            const float scalar) noexcept
        {
            vector *= scalar;
            return vector;
        }

        [[nodiscard]]
        friend constexpr Vector3 operator*(
            const float scalar,
            Vector3 vector) noexcept
        {
            vector *= scalar;
            return vector;
        }

        [[nodiscard]]
        friend constexpr Vector3 operator/(
            Vector3 vector,
            const float scalar) noexcept
        {
            vector /= scalar;
            return vector;
        }
    };

    inline constexpr Vector3 Vector3Zero{0.0f, 0.0f, 0.0f};
    inline constexpr Vector3 Vector3One{1.0f, 1.0f, 1.0f};

    inline constexpr Vector3 Vector3UnitX{1.0f, 0.0f, 0.0f};
    inline constexpr Vector3 Vector3UnitY{0.0f, 1.0f, 0.0f};
    inline constexpr Vector3 Vector3UnitZ{0.0f, 0.0f, 1.0f};
}