#pragma once

#include "Math/BoundingBox.h"
#include "Math/Frustum.h"
#include "Math/Sphere.h"

#include <cstddef>

struct D3DXPLANE;

namespace engine::legacy
{
    template <typename LegacyBox>
    [[nodiscard]]
    constexpr math::BoundingBox ToBoundingBox(
        const LegacyBox& box) noexcept
    {
        return math::BoundingBox::FromOriginSize(
            {
                static_cast<float>(box.Org.x),
                static_cast<float>(box.Org.y),
                static_cast<float>(box.Org.z)
            },
            {
                static_cast<float>(box.Size.x),
                static_cast<float>(box.Size.y),
                static_cast<float>(box.Size.z)
            }
        );
    }

    template <typename LegacyPoint3>
    [[nodiscard]]
    constexpr math::Sphere ToSphere(
        const LegacyPoint3& center,
        const float radius) noexcept
    {
        return
        {
                {
                    static_cast<float>(center.x),
                    static_cast<float>(center.y),
                    static_cast<float>(center.z)
                },
                radius
            };
    }

    [[nodiscard]]
    bool ToFrustum(
        const ::D3DXPLANE* planes,
        std::size_t planeCount,
        math::Frustum& result,
        float epsilon =
            math::DefaultEpsilon) noexcept;

    [[nodiscard]]
    bool ToD3DXPlanes(
        const math::Frustum& frustum,
        ::D3DXPLANE* planes,
        std::size_t planeCount) noexcept;

    // Не включает тяжёлый r3dRender.h.
    // Результат соответствует VisibilityInfoEnum.
    [[nodiscard]]
    constexpr int ToLegacyVisibilityValue(
        const math::ContainmentType value) noexcept
    {
        switch (value)
        {
        case math::ContainmentType::Outside:
            return 0;

        case math::ContainmentType::Inside:
            return 1;

        case math::ContainmentType::Intersecting:
            return 2;
        }

        return 2;
    }
}