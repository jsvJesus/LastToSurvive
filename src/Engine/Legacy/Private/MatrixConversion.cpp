#include "Legacy/MatrixConversion.h"

#include <d3dx9math.h>

static_assert(
    sizeof(D3DXMATRIX) ==
    sizeof(engine::math::Matrix4)
);

namespace engine::legacy
{
    math::Matrix4 ToMatrix4(
        const ::D3DXMATRIX& matrix) noexcept
    {
        return
        {
            matrix._11,
            matrix._12,
            matrix._13,
            matrix._14,

            matrix._21,
            matrix._22,
            matrix._23,
            matrix._24,

            matrix._31,
            matrix._32,
            matrix._33,
            matrix._34,

            matrix._41,
            matrix._42,
            matrix._43,
            matrix._44
        };
    }

    ::D3DXMATRIX ToD3DXMatrix(
        const math::Matrix4& matrix) noexcept
    {
        ::D3DXMATRIX result;

        result._11 = matrix.m[0][0];
        result._12 = matrix.m[0][1];
        result._13 = matrix.m[0][2];
        result._14 = matrix.m[0][3];

        result._21 = matrix.m[1][0];
        result._22 = matrix.m[1][1];
        result._23 = matrix.m[1][2];
        result._24 = matrix.m[1][3];

        result._31 = matrix.m[2][0];
        result._32 = matrix.m[2][1];
        result._33 = matrix.m[2][2];
        result._34 = matrix.m[2][3];

        result._41 = matrix.m[3][0];
        result._42 = matrix.m[3][1];
        result._43 = matrix.m[3][2];
        result._44 = matrix.m[3][3];

        return result;
    }
}