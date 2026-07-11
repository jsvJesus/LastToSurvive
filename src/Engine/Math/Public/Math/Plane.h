#pragma once

#include "Math/Vector3.h"

#include <cstdint>

namespace engine::math
{
    enum class PlaneIntersectionType : std::uint8_t
    {
        Back,
        Intersecting,
        Front
    };

    // Уравнение плоскости:
    // normal.Dot(point) + distance = 0.
    struct Plane final
    {
        Vector3 normal =
            Vector3UnitY;

        float distance =
            0.0f;

        constexpr Plane() noexcept = default;

        constexpr Plane(
            const Vector3& normalValue,
            const float distanceValue) noexcept
            : normal(normalValue)
            , distance(distanceValue)
        {
        }

        [[nodiscard]]
        static bool TryFromPointNormal(
            const Vector3& point,
            const Vector3& normal,
            Plane& result,
            const float epsilon = DefaultEpsilon) noexcept
        {
            Vector3 normalizedNormal =
                normal;

            if (!normalizedNormal.Normalize(epsilon))
            {
                return false;
            }

            result =
            {
                normalizedNormal,
                -normalizedNormal.Dot(point)
            };

            return true;
        }

        [[nodiscard]]
        static bool TryFromPoints(
            const Vector3& point0,
            const Vector3& point1,
            const Vector3& point2,
            Plane& result,
            const float epsilon = DefaultEpsilon) noexcept
        {
            Vector3 calculatedNormal =
                (point1 - point0).Cross(
                    point2 - point0
                );

            if (!calculatedNormal.Normalize(epsilon))
            {
                return false;
            }

            result =
            {
                calculatedNormal,
                -calculatedNormal.Dot(point0)
            };

            return true;
        }

        [[nodiscard]]
        constexpr float SignedDistanceTo(
            const Vector3& point) const noexcept
        {
            return
                normal.Dot(point) +
                distance;
        }

        [[nodiscard]]
        constexpr Vector3 ProjectPoint(
            const Vector3& point) const noexcept
        {
            return
                point -
                (
                    normal *
                    SignedDistanceTo(point)
                );
        }

        [[nodiscard]]
        constexpr PlaneIntersectionType ClassifyPoint(
            const Vector3& point,
            const float epsilon = DefaultEpsilon) const noexcept
        {
            const float signedDistance =
                SignedDistanceTo(point);

            if (signedDistance > epsilon)
            {
                return PlaneIntersectionType::Front;
            }

            if (signedDistance < -epsilon)
            {
                return PlaneIntersectionType::Back;
            }

            return PlaneIntersectionType::Intersecting;
        }

        bool Normalize(
            const float epsilon = DefaultEpsilon) noexcept
        {
            const float normalLength =
                normal.Length();

            if (normalLength <= epsilon)
            {
                return false;
            }

            normal /= normalLength;
            distance /= normalLength;

            return true;
        }

        [[nodiscard]]
        bool IsValid(
            const float epsilon = DefaultEpsilon) const noexcept
        {
            return
                IsFinite(normal.x) &&
                IsFinite(normal.y) &&
                IsFinite(normal.z) &&
                IsFinite(distance) &&
                normal.LengthSquared() >
                    (epsilon * epsilon);
        }
    };
}