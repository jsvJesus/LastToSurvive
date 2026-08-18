#include "Math/Transform.h"

namespace engine::math
{
    Matrix4 Transform::ToMatrix() const noexcept
    {
        return Matrix4::CreateTRS(
            translation,
            rotation,
            scale
        );
    }

    Vector3 Transform::TransformPoint(
        const Vector3& point) const noexcept
    {
        return
            TransformVector(point) +
            translation;
    }

    Vector3 Transform::TransformVector(
        const Vector3& vector) const noexcept
    {
        const Vector3 scaled
        {
            vector.x * scale.x,
            vector.y * scale.y,
            vector.z * scale.z
        };

        return rotation.Rotate(scaled);
    }

    bool Transform::IsNearlyEqual(
        const Transform& other,
        const float epsilon) const noexcept
    {
        return
            translation.IsNearlyEqual(
                other.translation,
                epsilon
            ) &&
            rotation.RepresentsSameRotation(
                other.rotation,
                epsilon
            ) &&
            scale.IsNearlyEqual(
                other.scale,
                epsilon
            );
    }

    bool Transform::TryFromMatrix(
        const Matrix4& matrix,
        Transform& transform,
        const float epsilon) noexcept
    {
        Vector3 translation;
        Quaternion rotation;
        Vector3 scale;

        if (
            !matrix.Decompose(
                translation,
                rotation,
                scale,
                epsilon
            )
        )
        {
            return false;
        }

        transform =
        {
            translation,
            rotation,
            scale
        };

        return true;
    }
}