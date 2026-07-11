#include "Math/Intersection.h"

#include <cmath>
#include <cstddef>
#include <limits>

namespace engine::math
{
    bool Intersects(
        const Ray& ray,
        const Plane& plane,
        float& parameter,
        const float epsilon) noexcept
    {
        parameter = 0.0f;

        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        if (
            !ray.IsValid(safeEpsilon) ||
            !plane.IsValid(safeEpsilon)
        )
        {
            return false;
        }

        const float denominator =
            plane.normal.Dot(
                ray.direction
            );

        if (Abs(denominator) <= safeEpsilon)
        {
            return false;
        }

        const float candidate =
            -plane.SignedDistanceTo(
                ray.origin
            ) /
            denominator;

        if (candidate < 0.0f)
        {
            return false;
        }

        parameter = candidate;
        return true;
    }

    bool Intersects(
        const Ray& ray,
        const Sphere& sphere,
        float& parameter,
        const float epsilon) noexcept
    {
        parameter = 0.0f;

        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        if (
            !ray.IsValid(safeEpsilon) ||
            !sphere.IsValid()
        )
        {
            return false;
        }

        const Vector3 offset =
            ray.origin -
            sphere.center;

        const float directionLengthSquared =
            ray.direction.Dot(
                ray.direction
            );

        if (
            directionLengthSquared <=
            (safeEpsilon * safeEpsilon)
        )
        {
            return false;
        }

        const float sphereTerm =
            offset.Dot(offset) -
            (sphere.radius * sphere.radius);

        if (sphereTerm <= 0.0f)
        {
            // Начало луча уже находится внутри сферы.
            parameter = 0.0f;
            return true;
        }

        const float projected =
            offset.Dot(
                ray.direction
            );

        if (projected > 0.0f)
        {
            return false;
        }

        const float discriminant =
            (projected * projected) -
            (
                directionLengthSquared *
                sphereTerm
            );

        if (discriminant < 0.0f)
        {
            return false;
        }

        const float candidate =
            (
                -projected -
                std::sqrt(discriminant)
            ) /
            directionLengthSquared;

        if (candidate < 0.0f)
        {
            return false;
        }

        parameter = candidate;
        return true;
    }

    bool Intersects(
        const Ray& ray,
        const BoundingBox& box,
        float& parameter,
        const float epsilon) noexcept
    {
        parameter = 0.0f;

        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        if (
            !ray.IsValid(safeEpsilon) ||
            !box.IsValid()
        )
        {
            return false;
        }

        float minimumParameter =
            0.0f;

        float maximumParameter =
            std::numeric_limits<float>::max();

        for (std::size_t axis = 0U; axis < 3U; ++axis)
        {
            const float origin =
                ray.origin[
                    static_cast<unsigned>(axis)
                ];

            const float direction =
                ray.direction[
                    static_cast<unsigned>(axis)
                ];

            const float minimum =
                box.minimum[
                    static_cast<unsigned>(axis)
                ];

            const float maximum =
                box.maximum[
                    static_cast<unsigned>(axis)
                ];

            if (Abs(direction) <= safeEpsilon)
            {
                if (
                    origin < minimum ||
                    origin > maximum
                )
                {
                    return false;
                }

                continue;
            }

            const float inverseDirection =
                1.0f / direction;

            float nearParameter =
                (minimum - origin) *
                inverseDirection;

            float farParameter =
                (maximum - origin) *
                inverseDirection;

            if (nearParameter > farParameter)
            {
                const float temporary =
                    nearParameter;

                nearParameter =
                    farParameter;

                farParameter =
                    temporary;
            }

            minimumParameter =
                Max(
                    minimumParameter,
                    nearParameter
                );

            maximumParameter =
                Min(
                    maximumParameter,
                    farParameter
                );

            if (
                minimumParameter >
                maximumParameter
            )
            {
                return false;
            }
        }

        parameter = minimumParameter;
        return true;
    }

    bool Intersects(
        const Sphere& left,
        const Sphere& right) noexcept
    {
        if (
            !left.IsValid() ||
            !right.IsValid()
        )
        {
            return false;
        }

        const Vector3 difference =
            left.center -
            right.center;

        const float combinedRadius =
            left.radius +
            right.radius;

        return
            difference.LengthSquared() <=
            (combinedRadius * combinedRadius);
    }

    bool Intersects(
        const Sphere& sphere,
        const BoundingBox& box) noexcept
    {
        if (
            !sphere.IsValid() ||
            !box.IsValid()
        )
        {
            return false;
        }

        const Vector3 closestPoint =
            box.ClosestPoint(
                sphere.center
            );

        const Vector3 difference =
            sphere.center -
            closestPoint;

        return
            difference.LengthSquared() <=
            (sphere.radius * sphere.radius);
    }

    PlaneIntersectionType Classify(
        const Plane& plane,
        const Sphere& sphere,
        const float epsilon) noexcept
    {
        if (
            !plane.IsValid() ||
            !sphere.IsValid()
        )
        {
            return PlaneIntersectionType::Intersecting;
        }

        const float signedDistance =
            plane.SignedDistanceTo(
                sphere.center
            );

        const float projectedRadius =
            sphere.radius *
            plane.normal.Length();

        if (
            signedDistance >
            projectedRadius + epsilon
        )
        {
            return PlaneIntersectionType::Front;
        }

        if (
            signedDistance <
            -projectedRadius - epsilon
        )
        {
            return PlaneIntersectionType::Back;
        }

        return PlaneIntersectionType::Intersecting;
    }

    PlaneIntersectionType Classify(
        const Plane& plane,
        const BoundingBox& box,
        const float epsilon) noexcept
    {
        if (
            !plane.IsValid() ||
            !box.IsValid()
        )
        {
            return PlaneIntersectionType::Intersecting;
        }

        const Vector3 center =
            box.Center();

        const Vector3 extents =
            box.Extents();

        const float projectedRadius =
            (Abs(plane.normal.x) * extents.x) +
            (Abs(plane.normal.y) * extents.y) +
            (Abs(plane.normal.z) * extents.z);

        const float signedDistance =
            plane.SignedDistanceTo(
                center
            );

        if (
            signedDistance >
            projectedRadius + epsilon
        )
        {
            return PlaneIntersectionType::Front;
        }

        if (
            signedDistance <
            -projectedRadius - epsilon
        )
        {
            return PlaneIntersectionType::Back;
        }

        return PlaneIntersectionType::Intersecting;
    }
}