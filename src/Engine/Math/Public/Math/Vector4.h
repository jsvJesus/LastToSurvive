#pragma once

#include "Math/Scalar.h"
#include "Math/Vector3.h"

#include <cmath>

namespace engine::math
{
    struct Vector4 final
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;

        constexpr Vector4() noexcept = default;

        constexpr Vector4(
            const float xValue,
            const float yValue,
            const float zValue,
            const float wValue) noexcept
            : x(xValue)
            , y(yValue)
            , z(zValue)
            , w(wValue)
        {
        }

        constexpr Vector4(
            const Vector3& vector,
            const float wValue) noexcept
            : x(vector.x)
            , y(vector.y)
            , z(vector.z)
            , w(wValue)
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
                    : index == 2U
                        ? z
                        : w;
        }

        [[nodiscard]]
        constexpr const float& operator[](
            const unsigned index) const noexcept
        {
            return index == 0U
                ? x
                : index == 1U
                    ? y
                    : index == 2U
                        ? z
                        : w;
        }

        [[nodiscard]]
        constexpr Vector3 XYZ() const noexcept
        {
            return
            {
                x,
                y,
                z
            };
        }

        [[nodiscard]]
        constexpr float LengthSquared() const noexcept
        {
            return
                (x * x) +
                (y * y) +
                (z * z) +
                (w * w);
        }

        [[nodiscard]]
        float Length() const noexcept
        {
            return std::sqrt(LengthSquared());
        }

        [[nodiscard]]
        constexpr float Dot(
            const Vector4& other) const noexcept
        {
            return
                (x * other.x) +
                (y * other.y) +
                (z * other.z) +
                (w * other.w);
        }

        [[nodiscard]]
        Vector4 Normalized(
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
                w = 0.0f;
                return false;
            }

            x /= length;
            y /= length;
            z /= length;
            w /= length;
            return true;
        }

        [[nodiscard]]
        constexpr bool IsNearlyZero(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                Abs(x) <= epsilon &&
                Abs(y) <= epsilon &&
                Abs(z) <= epsilon &&
                Abs(w) <= epsilon;
        }

        [[nodiscard]]
        constexpr bool IsNearlyEqual(
            const Vector4& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                NearlyEqual(x, other.x, epsilon) &&
                NearlyEqual(y, other.y, epsilon) &&
                NearlyEqual(z, other.z, epsilon) &&
                NearlyEqual(w, other.w, epsilon);
        }

        constexpr Vector4& operator+=(
            const Vector4& other) noexcept
        {
            x += other.x;
            y += other.y;
            z += other.z;
            w += other.w;
            return *this;
        }

        constexpr Vector4& operator-=(
            const Vector4& other) noexcept
        {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            w -= other.w;
            return *this;
        }

        constexpr Vector4& operator*=(
            const float scalar) noexcept
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            w *= scalar;
            return *this;
        }

        constexpr Vector4& operator/=(
            const float scalar) noexcept
        {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            w /= scalar;
            return *this;
        }

        [[nodiscard]]
        constexpr Vector4 operator-() const noexcept
        {
            return
            {
                -x,
                -y,
                -z,
                -w
            };
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Vector4& left,
            const Vector4& right) noexcept
        {
            return
                left.x == right.x &&
                left.y == right.y &&
                left.z == right.z &&
                left.w == right.w;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Vector4& left,
            const Vector4& right) noexcept
        {
            return !(left == right);
        }

        [[nodiscard]]
        friend constexpr Vector4 operator+(
            Vector4 left,
            const Vector4& right) noexcept
        {
            left += right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Vector4 operator-(
            Vector4 left,
            const Vector4& right) noexcept
        {
            left -= right;
            return left;
        }

        [[nodiscard]]
        friend constexpr Vector4 operator*(
            Vector4 vector,
            const float scalar) noexcept
        {
            vector *= scalar;
            return vector;
        }

        [[nodiscard]]
        friend constexpr Vector4 operator*(
            const float scalar,
            Vector4 vector) noexcept
        {
            vector *= scalar;
            return vector;
        }

        [[nodiscard]]
        friend constexpr Vector4 operator/(
            Vector4 vector,
            const float scalar) noexcept
        {
            vector /= scalar;
            return vector;
        }
    };

    inline constexpr Vector4 Vector4Zero
    {
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    inline constexpr Vector4 Vector4One
    {
        1.0f,
        1.0f,
        1.0f,
        1.0f
    };

    inline constexpr Vector4 Vector4UnitX
    {
        1.0f,
        0.0f,
        0.0f,
        0.0f
    };

    inline constexpr Vector4 Vector4UnitY
    {
        0.0f,
        1.0f,
        0.0f,
        0.0f
    };

    inline constexpr Vector4 Vector4UnitZ
    {
        0.0f,
        0.0f,
        1.0f,
        0.0f
    };

    inline constexpr Vector4 Vector4UnitW
    {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };
}