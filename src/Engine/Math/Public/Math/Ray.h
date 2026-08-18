#pragma once

#include "Math/Vector3.h"

namespace engine::math
{
    struct Ray final
    {
        Vector3 origin =
            Vector3Zero;

        Vector3 direction =
            Vector3UnitZ;

        constexpr Ray() noexcept = default;

        constexpr Ray(
            const Vector3& originValue,
            const Vector3& directionValue) noexcept
            : origin(originValue)
            , direction(directionValue)
        {
        }

        [[nodiscard]]
        constexpr Vector3 PointAt(
            const float parameter) const noexcept
        {
            return
                origin +
                (direction * parameter);
        }

        [[nodiscard]]
        bool IsValid(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                IsFinite(origin.x) &&
                IsFinite(origin.y) &&
                IsFinite(origin.z) &&
                IsFinite(direction.x) &&
                IsFinite(direction.y) &&
                IsFinite(direction.z) &&
                direction.LengthSquared() >
                    (epsilon * epsilon);
        }
    };
}