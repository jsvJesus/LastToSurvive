#pragma once

#include "Math/Matrix4.h"

struct D3DXMATRIX;

namespace engine::legacy
{
    [[nodiscard]]
    math::Matrix4 ToMatrix4(
        const ::D3DXMATRIX& matrix) noexcept;

    [[nodiscard]]
    ::D3DXMATRIX ToD3DXMatrix(
        const math::Matrix4& matrix) noexcept;
}