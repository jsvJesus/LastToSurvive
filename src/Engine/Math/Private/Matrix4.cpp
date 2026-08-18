#include "Math/Matrix4.h"

#include "Math/Quaternion.h"

#include <cstddef>
#include <cmath>

namespace
{
    [[nodiscard]]
    engine::math::Quaternion QuaternionFromRotationMatrix(
        const engine::math::Matrix4& matrix) noexcept
    {
        using engine::math::Quaternion;

        const float trace =
            matrix.m[0][0] +
            matrix.m[1][1] +
            matrix.m[2][2];

        Quaternion result;

        if (trace > 0.0f)
        {
            const float root =
                std::sqrt(trace + 1.0f);

            const float scale =
                2.0f * root;

            result.w = 0.25f * scale;

            result.x =
                (matrix.m[1][2] - matrix.m[2][1]) /
                scale;

            result.y =
                (matrix.m[2][0] - matrix.m[0][2]) /
                scale;

            result.z =
                (matrix.m[0][1] - matrix.m[1][0]) /
                scale;
        }
        else if (
            matrix.m[0][0] > matrix.m[1][1] &&
            matrix.m[0][0] > matrix.m[2][2]
        )
        {
            const float scale =
                2.0f *
                std::sqrt(
                    1.0f +
                    matrix.m[0][0] -
                    matrix.m[1][1] -
                    matrix.m[2][2]
                );

            result.w =
                (matrix.m[1][2] - matrix.m[2][1]) /
                scale;

            result.x =
                0.25f * scale;

            result.y =
                (matrix.m[0][1] + matrix.m[1][0]) /
                scale;

            result.z =
                (matrix.m[0][2] + matrix.m[2][0]) /
                scale;
        }
        else if (matrix.m[1][1] > matrix.m[2][2])
        {
            const float scale =
                2.0f *
                std::sqrt(
                    1.0f +
                    matrix.m[1][1] -
                    matrix.m[0][0] -
                    matrix.m[2][2]
                );

            result.w =
                (matrix.m[2][0] - matrix.m[0][2]) /
                scale;

            result.x =
                (matrix.m[0][1] + matrix.m[1][0]) /
                scale;

            result.y =
                0.25f * scale;

            result.z =
                (matrix.m[1][2] + matrix.m[2][1]) /
                scale;
        }
        else
        {
            const float scale =
                2.0f *
                std::sqrt(
                    1.0f +
                    matrix.m[2][2] -
                    matrix.m[0][0] -
                    matrix.m[1][1]
                );

            result.w =
                (matrix.m[0][1] - matrix.m[1][0]) /
                scale;

            result.x =
                (matrix.m[0][2] + matrix.m[2][0]) /
                scale;

            result.y =
                (matrix.m[1][2] + matrix.m[2][1]) /
                scale;

            result.z =
                0.25f * scale;
        }

        return result.Normalized();
    }
}

namespace engine::math
{
    Matrix4 Matrix4::CreateFromQuaternion(
        const Quaternion& rotation) noexcept
    {
        const Quaternion unit =
            rotation.Normalized();

        const float xx = unit.x * unit.x;
        const float yy = unit.y * unit.y;
        const float zz = unit.z * unit.z;

        const float xy = unit.x * unit.y;
        const float xz = unit.x * unit.z;
        const float yz = unit.y * unit.z;

        const float xw = unit.x * unit.w;
        const float yw = unit.y * unit.w;
        const float zw = unit.z * unit.w;

        return
        {
            1.0f - (2.0f * (yy + zz)),
            2.0f * (xy + zw),
            2.0f * (xz - yw),
            0.0f,

            2.0f * (xy - zw),
            1.0f - (2.0f * (xx + zz)),
            2.0f * (yz + xw),
            0.0f,

            2.0f * (xz + yw),
            2.0f * (yz - xw),
            1.0f - (2.0f * (xx + yy)),
            0.0f,

            0.0f,
            0.0f,
            0.0f,
            1.0f
        };
    }

    Matrix4 Matrix4::CreateTRS(
        const Vector3& translation,
        const Quaternion& rotation,
        const Vector3& scale) noexcept
    {
        return
            CreateScale(scale) *
            CreateFromQuaternion(rotation) *
            CreateTranslation(translation);
    }

    bool Matrix4::TryInverse(
        Matrix4& inverse,
        const float epsilon) const noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        float augmented[4][8] = {};

        for (std::size_t row = 0U; row < 4U; ++row)
        {
            for (
                std::size_t column = 0U;
                column < 4U;
                ++column
            )
            {
                const float value =
                    m[row][column];

                if (!IsFinite(value))
                {
                    return false;
                }

                augmented[row][column] =
                    value;

                augmented[row][column + 4U] =
                    row == column
                        ? 1.0f
                        : 0.0f;
            }
        }

        for (
            std::size_t pivotColumn = 0U;
            pivotColumn < 4U;
            ++pivotColumn
        )
        {
            std::size_t pivotRow =
                pivotColumn;

            float largestPivot =
                Abs(
                    augmented[pivotRow][pivotColumn]
                );

            for (
                std::size_t row = pivotColumn + 1U;
                row < 4U;
                ++row
            )
            {
                const float candidate =
                    Abs(
                        augmented[row][pivotColumn]
                    );

                if (candidate > largestPivot)
                {
                    largestPivot = candidate;
                    pivotRow = row;
                }
            }

            if (largestPivot <= safeEpsilon)
            {
                return false;
            }

            if (pivotRow != pivotColumn)
            {
                for (
                    std::size_t column = 0U;
                    column < 8U;
                    ++column
                )
                {
                    const float temporary =
                        augmented[pivotColumn][column];

                    augmented[pivotColumn][column] =
                        augmented[pivotRow][column];

                    augmented[pivotRow][column] =
                        temporary;
                }
            }

            const float pivot =
                augmented[pivotColumn][pivotColumn];

            for (
                std::size_t column = 0U;
                column < 8U;
                ++column
            )
            {
                augmented[pivotColumn][column] /=
                    pivot;
            }

            for (std::size_t row = 0U; row < 4U; ++row)
            {
                if (row == pivotColumn)
                {
                    continue;
                }

                const float factor =
                    augmented[row][pivotColumn];

                if (factor == 0.0f)
                {
                    continue;
                }

                for (
                    std::size_t column = 0U;
                    column < 8U;
                    ++column
                )
                {
                    augmented[row][column] -=
                        factor *
                        augmented[pivotColumn][column];
                }
            }
        }

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
                result.m[row][column] =
                    augmented[row][column + 4U];
            }
        }

        inverse = result;
        return true;
    }

    bool Matrix4::Decompose(
        Vector3& translation,
        Quaternion& rotation,
        Vector3& scale,
        const float epsilon) const noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        for (std::size_t row = 0U; row < 4U; ++row)
        {
            for (
                std::size_t column = 0U;
                column < 4U;
                ++column
            )
            {
                if (!IsFinite(m[row][column]))
                {
                    return false;
                }
            }
        }

        if (!IsAffine(safeEpsilon))
        {
            return false;
        }

        Vector3 right
        {
            m[0][0],
            m[0][1],
            m[0][2]
        };

        Vector3 up
        {
            m[1][0],
            m[1][1],
            m[1][2]
        };

        Vector3 forward
        {
            m[2][0],
            m[2][1],
            m[2][2]
        };

        Vector3 candidateScale
        {
            right.Length(),
            up.Length(),
            forward.Length()
        };

        if (
            candidateScale.x <= safeEpsilon ||
            candidateScale.y <= safeEpsilon ||
            candidateScale.z <= safeEpsilon
        )
        {
            return false;
        }

        right /= candidateScale.x;
        up /= candidateScale.y;
        forward /= candidateScale.z;

        const float orthogonalityEpsilon =
            Max(
                safeEpsilon * 16.0f,
                0.0001f
            );

        if (
            Abs(right.Dot(up)) >
                orthogonalityEpsilon ||
            Abs(right.Dot(forward)) >
                orthogonalityEpsilon ||
            Abs(up.Dot(forward)) >
                orthogonalityEpsilon
        )
        {
            // Матрица содержит shear.
            return false;
        }

        float handedness =
            right.Cross(up).Dot(forward);

        if (Abs(handedness) <= safeEpsilon)
        {
            return false;
        }

        if (handedness < 0.0f)
        {
            if (
                candidateScale.x >= candidateScale.y &&
                candidateScale.x >= candidateScale.z
            )
            {
                candidateScale.x =
                    -candidateScale.x;

                right = -right;
            }
            else if (
                candidateScale.y >= candidateScale.z
            )
            {
                candidateScale.y =
                    -candidateScale.y;

                up = -up;
            }
            else
            {
                candidateScale.z =
                    -candidateScale.z;

                forward = -forward;
            }

            handedness =
                right.Cross(up).Dot(forward);
        }

        if (
            !NearlyEqual(
                handedness,
                1.0f,
                orthogonalityEpsilon * 4.0f
            )
        )
        {
            return false;
        }

        const Matrix4 rotationMatrix
        {
            right.x,   right.y,   right.z,   0.0f,
            up.x,      up.y,      up.z,      0.0f,
            forward.x, forward.y, forward.z, 0.0f,
            0.0f,      0.0f,      0.0f,      1.0f
        };

        const Quaternion candidateRotation =
            QuaternionFromRotationMatrix(
                rotationMatrix
            );

        translation =
        {
            m[3][0],
            m[3][1],
            m[3][2]
        };

        rotation =
            candidateRotation;

        scale =
            candidateScale;

        return true;
    }
}