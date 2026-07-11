#pragma once

#include "Math/Scalar.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include <cstddef>
#include <cmath>

namespace engine::math
{
    struct Quaternion;

    // Соглашения:
    //
    // 1. Хранение row-major.
    // 2. Векторы являются row vectors: result = vector * matrix.
    // 3. Translation находится в m[3][0..2].
    // 4. A * B сначала применяет A, затем B.
    // 5. Углы передаются в радианах.
    struct Matrix4 final
    {
        float m[4][4] =
        {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        };

        constexpr Matrix4() noexcept = default;

        constexpr Matrix4(
            const float m00,
            const float m01,
            const float m02,
            const float m03,
            const float m10,
            const float m11,
            const float m12,
            const float m13,
            const float m20,
            const float m21,
            const float m22,
            const float m23,
            const float m30,
            const float m31,
            const float m32,
            const float m33) noexcept
            : m
            {
                {m00, m01, m02, m03},
                {m10, m11, m12, m13},
                {m20, m21, m22, m23},
                {m30, m31, m32, m33}
            }
        {
        }

        [[nodiscard]]
        static constexpr Matrix4 Identity() noexcept
        {
            return {};
        }

        [[nodiscard]]
        static constexpr Matrix4 Zero() noexcept
        {
            return
            {
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f
            };
        }

        [[nodiscard]]
        static constexpr Matrix4 CreateTranslation(
            const Vector3& translation) noexcept
        {
            return
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                translation.x,
                translation.y,
                translation.z,
                1.0f
            };
        }

        [[nodiscard]]
        static constexpr Matrix4 CreateScale(
            const Vector3& scale) noexcept
        {
            return
            {
                scale.x, 0.0f,    0.0f,    0.0f,
                0.0f,    scale.y, 0.0f,    0.0f,
                0.0f,    0.0f,    scale.z, 0.0f,
                0.0f,    0.0f,    0.0f,    1.0f
            };
        }

        [[nodiscard]]
        static constexpr Matrix4 CreateUniformScale(
            const float scale) noexcept
        {
            return CreateScale(
                {scale, scale, scale}
            );
        }

        [[nodiscard]]
        static Matrix4 CreateRotationX(
            const float radians) noexcept
        {
            const float sine =
                std::sin(radians);

            const float cosine =
                std::cos(radians);

            return
            {
                1.0f, 0.0f,    0.0f,   0.0f,
                0.0f, cosine,  sine,    0.0f,
                0.0f, -sine,   cosine,  0.0f,
                0.0f, 0.0f,    0.0f,   1.0f
            };
        }

        [[nodiscard]]
        static Matrix4 CreateRotationY(
            const float radians) noexcept
        {
            const float sine =
                std::sin(radians);

            const float cosine =
                std::cos(radians);

            return
            {
                cosine, 0.0f, -sine,  0.0f,
                0.0f,   1.0f, 0.0f,  0.0f,
                sine,   0.0f, cosine, 0.0f,
                0.0f,   0.0f, 0.0f,  1.0f
            };
        }

        [[nodiscard]]
        static Matrix4 CreateRotationZ(
            const float radians) noexcept
        {
            const float sine =
                std::sin(radians);

            const float cosine =
                std::cos(radians);

            return
            {
                cosine, sine,   0.0f, 0.0f,
                -sine,  cosine, 0.0f, 0.0f,
                0.0f,   0.0f,   1.0f, 0.0f,
                0.0f,   0.0f,   0.0f, 1.0f
            };
        }

        [[nodiscard]]
        static Matrix4 CreateFromQuaternion(
            const Quaternion& rotation) noexcept;

        [[nodiscard]]
        static Matrix4 CreateTRS(
            const Vector3& translation,
            const Quaternion& rotation,
            const Vector3& scale) noexcept;

        [[nodiscard]]
        constexpr float* Data() noexcept
        {
            return &m[0][0];
        }

        [[nodiscard]]
        constexpr const float* Data() const noexcept
        {
            return &m[0][0];
        }

        [[nodiscard]]
        constexpr float& At(
            const std::size_t row,
            const std::size_t column) noexcept
        {
            return m[row][column];
        }

        [[nodiscard]]
        constexpr const float& At(
            const std::size_t row,
            const std::size_t column) const noexcept
        {
            return m[row][column];
        }

        [[nodiscard]]
        constexpr Vector4 Row(
            const std::size_t row) const noexcept
        {
            return
            {
                m[row][0],
                m[row][1],
                m[row][2],
                m[row][3]
            };
        }

        [[nodiscard]]
        constexpr Vector4 Column(
            const std::size_t column) const noexcept
        {
            return
            {
                m[0][column],
                m[1][column],
                m[2][column],
                m[3][column]
            };
        }

        [[nodiscard]]
        constexpr Vector3 Translation() const noexcept
        {
            return
            {
                m[3][0],
                m[3][1],
                m[3][2]
            };
        }

        constexpr void SetTranslation(
            const Vector3& translation) noexcept
        {
            m[3][0] = translation.x;
            m[3][1] = translation.y;
            m[3][2] = translation.z;
        }

        // Affine point transform, w считается равным 1.
        // Perspective divide здесь не выполняется.
        [[nodiscard]]
        constexpr Vector3 TransformPoint(
            const Vector3& point) const noexcept
        {
            return
            {
                (point.x * m[0][0]) +
                (point.y * m[1][0]) +
                (point.z * m[2][0]) +
                m[3][0],

                (point.x * m[0][1]) +
                (point.y * m[1][1]) +
                (point.z * m[2][1]) +
                m[3][1],

                (point.x * m[0][2]) +
                (point.y * m[1][2]) +
                (point.z * m[2][2]) +
                m[3][2]
            };
        }

        // Direction transform, translation не применяется.
        [[nodiscard]]
        constexpr Vector3 TransformVector(
            const Vector3& vector) const noexcept
        {
            return
            {
                (vector.x * m[0][0]) +
                (vector.y * m[1][0]) +
                (vector.z * m[2][0]),

                (vector.x * m[0][1]) +
                (vector.y * m[1][1]) +
                (vector.z * m[2][1]),

                (vector.x * m[0][2]) +
                (vector.y * m[1][2]) +
                (vector.z * m[2][2])
            };
        }

        [[nodiscard]]
        constexpr Vector4 Transform(
            const Vector4& vector) const noexcept
        {
            return
            {
                (vector.x * m[0][0]) +
                (vector.y * m[1][0]) +
                (vector.z * m[2][0]) +
                (vector.w * m[3][0]),

                (vector.x * m[0][1]) +
                (vector.y * m[1][1]) +
                (vector.z * m[2][1]) +
                (vector.w * m[3][1]),

                (vector.x * m[0][2]) +
                (vector.y * m[1][2]) +
                (vector.z * m[2][2]) +
                (vector.w * m[3][2]),

                (vector.x * m[0][3]) +
                (vector.y * m[1][3]) +
                (vector.z * m[2][3]) +
                (vector.w * m[3][3])
            };
        }

        [[nodiscard]]
        constexpr Matrix4 Transposed() const noexcept
        {
            return
            {
                m[0][0], m[1][0], m[2][0], m[3][0],
                m[0][1], m[1][1], m[2][1], m[3][1],
                m[0][2], m[1][2], m[2][2], m[3][2],
                m[0][3], m[1][3], m[2][3], m[3][3]
            };
        }

        [[nodiscard]]
        constexpr bool IsNearlyEqual(
            const Matrix4& other,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            for (std::size_t row = 0U; row < 4U; ++row)
            {
                for (
                    std::size_t column = 0U;
                    column < 4U;
                    ++column
                )
                {
                    if (
                        !NearlyEqual(
                            m[row][column],
                            other.m[row][column],
                            epsilon
                        )
                    )
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        constexpr Matrix4& operator*=(
            const Matrix4& right) noexcept
        {
            *this = *this * right;
            return *this;
        }

        [[nodiscard]]
        friend constexpr Matrix4 operator*(
            const Matrix4& left,
            const Matrix4& right) noexcept
        {
            Matrix4 result =
                Matrix4::Zero();

            for (std::size_t row = 0U; row < 4U; ++row)
            {
                for (
                    std::size_t column = 0U;
                    column < 4U;
                    ++column
                )
                {
                    for (
                        std::size_t index = 0U;
                        index < 4U;
                        ++index
                    )
                    {
                        result.m[row][column] +=
                            left.m[row][index] *
                            right.m[index][column];
                    }
                }
            }

            return result;
        }

        [[nodiscard]]
        friend constexpr bool operator==(
            const Matrix4& left,
            const Matrix4& right) noexcept
        {
            for (std::size_t row = 0U; row < 4U; ++row)
            {
                for (
                    std::size_t column = 0U;
                    column < 4U;
                    ++column
                )
                {
                    if (
                        left.m[row][column] !=
                        right.m[row][column]
                    )
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]]
        friend constexpr bool operator!=(
            const Matrix4& left,
            const Matrix4& right) noexcept
        {
            return !(left == right);
        }
    };

    inline constexpr Matrix4 Matrix4Identity =
        Matrix4::Identity();

    inline constexpr Matrix4 Matrix4Zero =
        Matrix4::Zero();
}