#pragma once

#include "Math/BoundingBox.h"
#include "Math/Matrix4.h"
#include "Math/Plane.h"
#include "Math/Sphere.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine::math
{
    enum class FrustumPlane : std::uint8_t
    {
        Near = 0,
        Far,
        Left,
        Right,
        Top,
        Bottom,
        Count
    };

    enum class FrustumDepthMode : std::uint8_t
    {
        Forward,
        Reversed
    };

    // Порядок значений намеренно совместим
    // со старым VisibilityInfoEnum.
    enum class ContainmentType : std::uint8_t
    {
        Outside = 0,
        Inside = 1,
        Intersecting = 2
    };

    inline constexpr std::size_t FrustumPlaneCount =
        static_cast<std::size_t>(
            FrustumPlane::Count
        );

    struct Frustum final
    {
        std::array<Plane, FrustumPlaneCount> planes =
        {
            Plane{Vector3Zero, 0.0f},
            Plane{Vector3Zero, 0.0f},
            Plane{Vector3Zero, 0.0f},
            Plane{Vector3Zero, 0.0f},
            Plane{Vector3Zero, 0.0f},
            Plane{Vector3Zero, 0.0f}
        };

        [[nodiscard]]
        static bool TryFromViewProjection(
            const Matrix4& viewProjection,
            Frustum& result,
            FrustumDepthMode depthMode =
                FrustumDepthMode::Forward,
            float epsilon =
                DefaultEpsilon) noexcept;

        // Плоскости должны быть направлены внутрь frustum.
        [[nodiscard]]
        static bool TryFromPlanes(
            const std::array<
                Plane,
                FrustumPlaneCount
            >& sourcePlanes,
            Frustum& result,
            float epsilon =
                DefaultEpsilon) noexcept;

        [[nodiscard]]
        constexpr Plane& GetPlane(
            const FrustumPlane plane) noexcept
        {
            return planes[
                static_cast<std::size_t>(plane)
            ];
        }

        [[nodiscard]]
        constexpr const Plane& GetPlane(
            const FrustumPlane plane) const noexcept
        {
            return planes[
                static_cast<std::size_t>(plane)
            ];
        }

        [[nodiscard]]
        bool ContainsPoint(
            const Vector3& point,
            float epsilon =
                DefaultEpsilon) const noexcept;

        [[nodiscard]]
        ContainmentType Classify(
            const Sphere& sphere,
            float epsilon =
                DefaultEpsilon) const noexcept;

        [[nodiscard]]
        ContainmentType Classify(
            const BoundingBox& box,
            float epsilon =
                DefaultEpsilon) const noexcept;

        [[nodiscard]]
        bool Intersects(
            const Sphere& sphere,
            float epsilon =
                DefaultEpsilon) const noexcept
        {
            return
                Classify(sphere, epsilon) !=
                ContainmentType::Outside;
        }

        [[nodiscard]]
        bool Intersects(
            const BoundingBox& box,
            float epsilon =
                DefaultEpsilon) const noexcept
        {
            return
                Classify(box, epsilon) !=
                ContainmentType::Outside;
        }

        [[nodiscard]]
        bool IsValid(
            float epsilon =
                DefaultEpsilon) const noexcept;
    };
}