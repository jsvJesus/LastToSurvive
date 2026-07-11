#include "Math/Matrix4.h"

#include "Math/Quaternion.h"

namespace engine::math
{
    Matrix4 Matrix4::CreateFromQuaternion(
        const Quaternion& rotation) noexcept
    {
        const Quaternion unit =
            rotation.Normalized();

        const float xx = unit.x * unit.x;
        const float yy = unit.y * unit.y;
        const float zz = unit.z * unit.z;

        const float xy = unit.x * unit.y;
        const float xz = unit.x * unit.z;
        const float yz = unit.y * unit.z;

        const float xw = unit.x * unit.w;
        const float yw = unit.y * unit.w;
        const float zw = unit.z * unit.w;

        return
        {
            1.0f - (2.0f * (yy + zz)),
            2.0f * (xy + zw),
            2.0f * (xz - yw),
            0.0f,

            2.0f * (xy - zw),
            1.0f - (2.0f * (xx + zz)),
            2.0f * (yz + xw),
            0.0f,

            2.0f * (xz + yw),
            2.0f * (yz - xw),
            1.0f - (2.0f * (xx + yy)),
            0.0f,

            0.0f,
            0.0f,
            0.0f,
            1.0f
        };
    }

    Matrix4 Matrix4::CreateTRS(
        const Vector3& translation,
        const Quaternion& rotation,
        const Vector3& scale) noexcept
    {
        return
            CreateScale(scale) *
            CreateFromQuaternion(rotation) *
            CreateTranslation(translation);
    }
}