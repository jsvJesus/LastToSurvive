#pragma once

#include "Math/Vector3.h"

namespace engine::math
{
    struct Sphere final
    {
        Vector3 center =
            Vector3Zero;

        float radius =
            0.0f;

        constexpr Sphere() noexcept = default;

        constexpr Sphere(
            const Vector3& centerValue,
            const float radiusValue) noexcept
            : center(centerValue)
            , radius(radiusValue)
        {
        }

        [[nodiscard]]
        constexpr bool ContainsPoint(
            const Vector3& point) const noexcept
        {
            const Vector3 difference =
                point - center;

            return
                difference.LengthSquared() <=
                (radius * radius);
        }

        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return
                IsFinite(center.x) &&
                IsFinite(center.y) &&
                IsFinite(center.z) &&
                IsFinite(radius) &&
                radius >= 0.0f;
        }
    };
}