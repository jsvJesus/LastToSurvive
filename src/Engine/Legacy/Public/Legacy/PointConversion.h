#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"

namespace engine::legacy
{
    template <typename LegacyPoint2>
    [[nodiscard]]
    constexpr math::Vector2 ToVector2(
        const LegacyPoint2& value) noexcept
    {
        return
        {
            static_cast<float>(value.x),
            static_cast<float>(value.y)
        };
    }

    template <typename LegacyPoint3>
    [[nodiscard]]
    constexpr math::Vector3 ToVector3(
        const LegacyPoint3& value) noexcept
    {
        return
        {
            static_cast<float>(value.x),
            static_cast<float>(value.y),
            static_cast<float>(value.z)
        };
    }

    template <typename LegacyPoint2>
    [[nodiscard]]
    constexpr LegacyPoint2 ToLegacyPoint2D(
        const math::Vector2& value) noexcept
    {
        return LegacyPoint2(
            value.x,
            value.y
        );
    }

    template <typename LegacyPoint3>
    [[nodiscard]]
    constexpr LegacyPoint3 ToLegacyPoint3D(
        const math::Vector3& value) noexcept
    {
        return LegacyPoint3(
            value.x,
            value.y,
            value.z
        );
    }
}