#include "Math/Frustum.h"

#include "Math/Intersection.h"

namespace
{
    [[nodiscard]]
    constexpr std::size_t PlaneIndex(
        const engine::math::FrustumPlane plane) noexcept
    {
        return static_cast<std::size_t>(plane);
    }

    [[nodiscard]]
    constexpr engine::math::Plane MakePlane(
        const float a,
        const float b,
        const float c,
        const float d) noexcept
    {
        return
        {
            {a, b, c},
            d
        };
    }
}

namespace engine::math
{
    bool Frustum::TryFromViewProjection(
        const Matrix4& viewProjection,
        Frustum& result,
        const FrustumDepthMode depthMode,
        const float epsilon) noexcept
    {
        // Row-vector + Direct3D clip space:
        //
        // -w <= x <= w
        // -w <= y <= w
        //  0 <= z <= w
        //
        // Поэтому плоскости извлекаются
        // из столбцов матрицы.

        const Plane depthMinimum =
            MakePlane(
                viewProjection.m[0][2],
                viewProjection.m[1][2],
                viewProjection.m[2][2],
                viewProjection.m[3][2]
            );

        const Plane depthMaximum =
            MakePlane(
                viewProjection.m[0][3] -
                    viewProjection.m[0][2],

                viewProjection.m[1][3] -
                    viewProjection.m[1][2],

                viewProjection.m[2][3] -
                    viewProjection.m[2][2],

                viewProjection.m[3][3] -
                    viewProjection.m[3][2]
            );

        std::array<Plane, FrustumPlaneCount>
            extractedPlanes;

        if (
            depthMode ==
            FrustumDepthMode::Forward
        )
        {
            extractedPlanes[
                PlaneIndex(FrustumPlane::Near)
            ] = depthMinimum;

            extractedPlanes[
                PlaneIndex(FrustumPlane::Far)
            ] = depthMaximum;
        }
        else
        {
            extractedPlanes[
                PlaneIndex(FrustumPlane::Near)
            ] = depthMaximum;

            extractedPlanes[
                PlaneIndex(FrustumPlane::Far)
            ] = depthMinimum;
        }

        extractedPlanes[
            PlaneIndex(FrustumPlane::Left)
        ] =
            MakePlane(
                viewProjection.m[0][3] +
                    viewProjection.m[0][0],

                viewProjection.m[1][3] +
                    viewProjection.m[1][0],

                viewProjection.m[2][3] +
                    viewProjection.m[2][0],

                viewProjection.m[3][3] +
                    viewProjection.m[3][0]
            );

        extractedPlanes[
            PlaneIndex(FrustumPlane::Right)
        ] =
            MakePlane(
                viewProjection.m[0][3] -
                    viewProjection.m[0][0],

                viewProjection.m[1][3] -
                    viewProjection.m[1][0],

                viewProjection.m[2][3] -
                    viewProjection.m[2][0],

                viewProjection.m[3][3] -
                    viewProjection.m[3][0]
            );

        extractedPlanes[
            PlaneIndex(FrustumPlane::Top)
        ] =
            MakePlane(
                viewProjection.m[0][3] -
                    viewProjection.m[0][1],

                viewProjection.m[1][3] -
                    viewProjection.m[1][1],

                viewProjection.m[2][3] -
                    viewProjection.m[2][1],

                viewProjection.m[3][3] -
                    viewProjection.m[3][1]
            );

        extractedPlanes[
            PlaneIndex(FrustumPlane::Bottom)
        ] =
            MakePlane(
                viewProjection.m[0][3] +
                    viewProjection.m[0][1],

                viewProjection.m[1][3] +
                    viewProjection.m[1][1],

                viewProjection.m[2][3] +
                    viewProjection.m[2][1],

                viewProjection.m[3][3] +
                    viewProjection.m[3][1]
            );

        return TryFromPlanes(
            extractedPlanes,
            result,
            epsilon
        );
    }

    bool Frustum::TryFromPlanes(
        const std::array<
            Plane,
            FrustumPlaneCount
        >& sourcePlanes,
        Frustum& result,
        const float epsilon) noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        Frustum candidate;
        candidate.planes =
            sourcePlanes;

        for (Plane& plane : candidate.planes)
        {
            if (!plane.IsValid(safeEpsilon))
            {
                return false;
            }

            if (!plane.Normalize(safeEpsilon))
            {
                return false;
            }

            if (!plane.IsValid(safeEpsilon))
            {
                return false;
            }
        }

        result = candidate;
        return true;
    }

    bool Frustum::ContainsPoint(
        const Vector3& point,
        const float epsilon) const noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        if (!IsValid(safeEpsilon))
        {
            return false;
        }

        for (const Plane& plane : planes)
        {
            if (
                plane.SignedDistanceTo(point) <
                -safeEpsilon
            )
            {
                return false;
            }
        }

        return true;
    }

    ContainmentType Frustum::Classify(
        const Sphere& sphere,
        const float epsilon) const noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        if (
            !IsValid(safeEpsilon) ||
            !sphere.IsValid()
        )
        {
            // Fail-open: повреждённый frustum
            // не должен скрывать объект.
            return ContainmentType::Intersecting;
        }

        bool intersects =
            false;

        for (const Plane& plane : planes)
        {
            const PlaneIntersectionType relation =
                engine::math::Classify(
                    plane,
                    sphere,
                    safeEpsilon
                );

            if (
                relation ==
                PlaneIntersectionType::Back
            )
            {
                return ContainmentType::Outside;
            }

            if (
                relation ==
                PlaneIntersectionType::Intersecting
            )
            {
                intersects = true;
            }
        }

        return intersects
            ? ContainmentType::Intersecting
            : ContainmentType::Inside;
    }

    ContainmentType Frustum::Classify(
        const BoundingBox& box,
        const float epsilon) const noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        if (
            !IsValid(safeEpsilon) ||
            !box.IsValid()
        )
        {
            return ContainmentType::Intersecting;
        }

        bool intersects =
            false;

        for (const Plane& plane : planes)
        {
            const PlaneIntersectionType relation =
                engine::math::Classify(
                    plane,
                    box,
                    safeEpsilon
                );

            if (
                relation ==
                PlaneIntersectionType::Back
            )
            {
                return ContainmentType::Outside;
            }

            if (
                relation ==
                PlaneIntersectionType::Intersecting
            )
            {
                intersects = true;
            }
        }

        return intersects
            ? ContainmentType::Intersecting
            : ContainmentType::Inside;
    }

    bool Frustum::IsValid(
        const float epsilon) const noexcept
    {
        const float safeEpsilon =
            Max(
                Abs(epsilon),
                DefaultEpsilon
            );

        for (const Plane& plane : planes)
        {
            if (!plane.IsValid(safeEpsilon))
            {
                return false;
            }
        }

        return true;
    }
}