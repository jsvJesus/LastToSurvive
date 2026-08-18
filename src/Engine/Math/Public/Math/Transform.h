#pragma once

#include "Math/Matrix4.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"

namespace engine::math
{
    struct Transform final
    {
        Vector3 translation =
            Vector3Zero;

        Quaternion rotation =
            QuaternionIdentity;

        Vector3 scale =
            Vector3One;

        constexpr Transform() noexcept = default;

        constexpr Transform(
            const Vector3& translationValue,
            const Quaternion& rotationValue,
            const Vector3& scaleValue) noexcept
            : translation(translationValue)
            , rotation(rotationValue)
            , scale(scaleValue)
        {
        }

        [[nodiscard]]
        Matrix4 ToMatrix() const noexcept;

        [[nodiscard]]
        Vector3 TransformPoint(
            const Vector3& point) const noexcept;

        [[nodiscard]]
        Vector3 TransformVector(
            const Vector3& vector) const noexcept;

        [[nodiscard]]
        bool IsNearlyEqual(
            const Transform& other,
            float epsilon = DefaultEpsilon) const noexcept;

        [[nodiscard]]
        static bool TryFromMatrix(
            const Matrix4& matrix,
            Transform& transform,
            float epsilon = DefaultEpsilon) noexcept;
    };

    inline constexpr Transform TransformIdentity{};
}