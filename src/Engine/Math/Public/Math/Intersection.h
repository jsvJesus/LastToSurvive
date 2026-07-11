#pragma once

#include "Math/BoundingBox.h"
#include "Math/Plane.h"
#include "Math/Ray.h"
#include "Math/Sphere.h"

namespace engine::math
{
    [[nodiscard]]
    bool Intersects(
        const Ray& ray,
        const Plane& plane,
        float& parameter,
        float epsilon = DefaultEpsilon) noexcept;

    [[nodiscard]]
    bool Intersects(
        const Ray& ray,
        const Sphere& sphere,
        float& parameter,
        float epsilon = DefaultEpsilon) noexcept;

    [[nodiscard]]
    bool Intersects(
        const Ray& ray,
        const BoundingBox& box,
        float& parameter,
        float epsilon = DefaultEpsilon) noexcept;

    [[nodiscard]]
    bool Intersects(
        const Sphere& left,
        const Sphere& right) noexcept;

    [[nodiscard]]
    bool Intersects(
        const Sphere& sphere,
        const BoundingBox& box) noexcept;

    [[nodiscard]]
    PlaneIntersectionType Classify(
        const Plane& plane,
        const Sphere& sphere,
        float epsilon = DefaultEpsilon) noexcept;

    [[nodiscard]]
    PlaneIntersectionType Classify(
        const Plane& plane,
        const BoundingBox& box,
        float epsilon = DefaultEpsilon) noexcept;
}