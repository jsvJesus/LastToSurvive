#pragma once

#include "Math/Scalar.h"
#include "Math/Vector3.h"

#include <cmath>

namespace engine::math
{
    struct Quaternion final
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;

        constexpr Quaternion() noexcept = default;

        constexpr Quaternion(
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

        [[nodiscard]]
        static Quaternion FromAxisAngle(
            const Vector3& axis,
            const float radians) noexcept
        {
            const Vector3 normalizedAxis =
                axis.Normalized();

            if (normalizedAxis.IsNearlyZero())
            {
                return {};
            }

            const float halfAngle =
                radians * 0.5f;

            const float sine =
                std::sin(halfAngle);

            const float cosine =
                std::cos(halfAngle);

            return
            {
                normalizedAxis.x * sine,
                normalizedAxis.y * sine,
                normalizedAxis.z * sine,
                cosine
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
            return std::sqrt(
                LengthSquared()
            );
        }

        [[nodiscard]]
        Quaternion Normalized(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            const float length = Length();

            if (length <= epsilon)
            {
                return {};
            }

            const float inverseLength =
                1.0f / length;

            return
            {
                x * inverseLength,
                y * inverseLength,
                z * inverseLength,
                w * inverseLength
            };
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
                w = 1.0f;
                return false;
            }

            const float inverseLength =
                1.0f / length;

            x *= inverseLength;
            y *= inverseLength;
            z *= inverseLength;
            w *= inverseLength;

            return true;
        }

        [[nodiscard]]
        constexpr Quaternion Conjugated() const noexcept
        {
            return
            {
                -x,
                -y,
                -z,
                w
            };
        }

        [[nodiscard]]
        Quaternion Inversed(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            const float lengthSquared =
                LengthSquared();

            if (lengthSquared <= epsilon)
            {
                return {};
            }

            const float inverseLengthSquared =
                1.0f / lengthSquared;

            return
            {
                -x * inverseLengthSquared,
                -y * inverseLengthSquared,
                -z * inverseLengthSquared,
                w * inverseLengthSquared
            };
        }

        [[nodiscard]]
        constexpr float Dot(
            const Quaternion& other) const noexcept
        {
            return
                (x * other.x) +
                (y * other.y) +
                (z * other.z) +
                (w * other.w);
        }

        [[nodiscard]]
        Vector3 Rotate(
            const Vector3& vector) const noexcept
        {
            const Quaternion unit =
                Normalized();

            const Vector3 imaginary
            {
                unit.x,
                unit.y,
                unit.z
            };

            const Vector3 twiceCross =
                2.0f * imaginary.Cross(vector);

            return
                vector +
                (unit.w * twiceCross) +
                imaginary.Cross(twiceCross);
        }

        [[nodiscard]]
        constexpr bool IsNearlyEqual(
            const Quaternion& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                NearlyEqual(x, other.x, epsilon) &&
                NearlyEqual(y, other.y, epsilon) &&
                NearlyEqual(z, other.z, epsilon) &&
                NearlyEqual(w, other.w, epsilon);
        }

        [[nodiscard]]
        bool RepresentsSameRotation(
            const Quaternion& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            const Quaternion left =
                Normalized();

            const Quaternion right =
                other.Normalized();

            return NearlyEqual(
                Abs(left.Dot(right)),
                1.0f,
                epsilon
            );
        }

        // Hamilton product without engine composition semantics.
        [[nodiscard]]
        static constexpr Quaternion HamiltonProduct(
            const Quaternion& left,
            const Quaternion& right) noexcept
        {
            return
            {
                (left.w * right.x) +
                (left.x * right.w) +
                (left.y * right.z) -
                (left.z * right.y),

                (left.w * right.y) -
                (left.x * right.z) +
                (left.y * right.w) +
                (left.z * right.x),

                (left.w * right.z) +
                (left.x * right.y) -
                (left.y * right.x) +
                (left.z * right.w),

                (left.w * right.w) -
                (left.x * right.x) -
                (left.y * right.y) -
                (left.z * right.z)
            };
        }

        // Применяет first, затем second.
        // Соответствует row-vector матрицам:
        // Matrix(first) * Matrix(second).
        [[nodiscard]]
        static constexpr Quaternion Concatenate(
            const Quaternion& first,
            const Quaternion& second) noexcept
        {
            return HamiltonProduct(
                second,
                first
            );
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Quaternion& left,
            const Quaternion& right) noexcept
        {
            return
                left.x == right.x &&
                left.y == right.y &&
                left.z == right.z &&
                left.w == right.w;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Quaternion& left,
            const Quaternion& right) noexcept
        {
            return !(left == right);
        }
    };

    inline constexpr Quaternion QuaternionIdentity
    {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };
}