#pragma once

#include "Math/Vector3.h"

namespace engine::math
{
    // Axis-aligned bounding box.
    struct BoundingBox final
    {
        Vector3 minimum =
            Vector3Zero;

        Vector3 maximum =
            Vector3Zero;

        constexpr BoundingBox() noexcept = default;

        constexpr BoundingBox(
            const Vector3& minimumValue,
            const Vector3& maximumValue) noexcept
            : minimum(minimumValue)
            , maximum(maximumValue)
        {
        }

        [[nodiscard]]
        static constexpr BoundingBox FromMinMax(
            const Vector3& minimum,
            const Vector3& maximum) noexcept
        {
            return
            {
                {
                    Min(minimum.x, maximum.x),
                    Min(minimum.y, maximum.y),
                    Min(minimum.z, maximum.z)
                },
                {
                    Max(minimum.x, maximum.x),
                    Max(minimum.y, maximum.y),
                    Max(minimum.z, maximum.z)
                }
            };
        }

        [[nodiscard]]
        static constexpr BoundingBox FromCenterExtents(
            const Vector3& center,
            const Vector3& extents) noexcept
        {
            const Vector3 safeExtents
            {
                Abs(extents.x),
                Abs(extents.y),
                Abs(extents.z)
            };

            return
            {
                center - safeExtents,
                center + safeExtents
            };
        }

        // Адаптер для старого формата Org + Size.
        [[nodiscard]]
        static constexpr BoundingBox FromOriginSize(
            const Vector3& origin,
            const Vector3& size) noexcept
        {
            return FromMinMax(
                origin,
                origin + size
            );
        }

        [[nodiscard]]
        constexpr Vector3 Center() const noexcept
        {
            return
                (minimum + maximum) *
                0.5f;
        }

        [[nodiscard]]
        constexpr Vector3 Extents() const noexcept
        {
            return
                (maximum - minimum) *
                0.5f;
        }

        [[nodiscard]]
        constexpr Vector3 Size() const noexcept
        {
            return
                maximum - minimum;
        }

        [[nodiscard]]
        constexpr bool ContainsPoint(
            const Vector3& point) const noexcept
        {
            return
                point.x >= minimum.x &&
                point.y >= minimum.y &&
                point.z >= minimum.z &&
                point.x <= maximum.x &&
                point.y <= maximum.y &&
                point.z <= maximum.z;
        }

        [[nodiscard]]
        constexpr bool ContainsBox(
            const BoundingBox& box) const noexcept
        {
            return
                box.minimum.x >= minimum.x &&
                box.minimum.y >= minimum.y &&
                box.minimum.z >= minimum.z &&
                box.maximum.x <= maximum.x &&
                box.maximum.y <= maximum.y &&
                box.maximum.z <= maximum.z;
        }

        [[nodiscard]]
        constexpr bool Intersects(
            const BoundingBox& box) const noexcept
        {
            return
                minimum.x <= box.maximum.x &&
                maximum.x >= box.minimum.x &&
                minimum.y <= box.maximum.y &&
                maximum.y >= box.minimum.y &&
                minimum.z <= box.maximum.z &&
                maximum.z >= box.minimum.z;
        }

        [[nodiscard]]
        constexpr Vector3 ClosestPoint(
            const Vector3& point) const noexcept
        {
            return
            {
                Clamp(
                    point.x,
                    minimum.x,
                    maximum.x
                ),
                Clamp(
                    point.y,
                    minimum.y,
                    maximum.y
                ),
                Clamp(
                    point.z,
                    minimum.z,
                    maximum.z
                )
            };
        }

        constexpr void ExpandToInclude(
            const Vector3& point) noexcept
        {
            minimum.x =
                Min(minimum.x, point.x);

            minimum.y =
                Min(minimum.y, point.y);

            minimum.z =
                Min(minimum.z, point.z);

            maximum.x =
                Max(maximum.x, point.x);

            maximum.y =
                Max(maximum.y, point.y);

            maximum.z =
                Max(maximum.z, point.z);
        }

        constexpr void ExpandToInclude(
            const BoundingBox& box) noexcept
        {
            ExpandToInclude(box.minimum);
            ExpandToInclude(box.maximum);
        }

        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return
                IsFinite(minimum.x) &&
                IsFinite(minimum.y) &&
                IsFinite(minimum.z) &&
                IsFinite(maximum.x) &&
                IsFinite(maximum.y) &&
                IsFinite(maximum.z) &&
                minimum.x <= maximum.x &&
                minimum.y <= maximum.y &&
                minimum.z <= maximum.z;
        }
    };
}