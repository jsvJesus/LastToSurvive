#include "Legacy/CullingConversion.h"

#include <d3dx9math.h>

#include <array>
#include <cstddef>

static_assert(
    sizeof(D3DXPLANE) ==
    sizeof(float) * 4U
);

namespace engine::legacy
{
    bool ToFrustum(
        const ::D3DXPLANE* planes,
        const std::size_t planeCount,
        math::Frustum& result,
        const float epsilon) noexcept
    {
        if (
            planes == nullptr ||
            planeCount <
                math::FrustumPlaneCount
        )
        {
            return false;
        }

        std::array<
            math::Plane,
            math::FrustumPlaneCount
        > convertedPlanes;

        for (
            std::size_t index = 0U;
            index < math::FrustumPlaneCount;
            ++index
        )
        {
            convertedPlanes[index] =
            {
                {
                    planes[index].a,
                    planes[index].b,
                    planes[index].c
                },
                planes[index].d
            };
        }

        return math::Frustum::TryFromPlanes(
            convertedPlanes,
            result,
            epsilon
        );
    }

    bool ToD3DXPlanes(
        const math::Frustum& frustum,
        ::D3DXPLANE* planes,
        const std::size_t planeCount) noexcept
    {
        if (
            planes == nullptr ||
            planeCount <
                math::FrustumPlaneCount ||
            !frustum.IsValid()
        )
        {
            return false;
        }

        for (
            std::size_t index = 0U;
            index < math::FrustumPlaneCount;
            ++index
        )
        {
            const math::Plane& source =
                frustum.planes[index];

            planes[index].a =
                source.normal.x;

            planes[index].b =
                source.normal.y;

            planes[index].c =
                source.normal.z;

            planes[index].d =
                source.distance;
        }

        return true;
    }
}