#include "Editor/LevelEditor/Scene/ScenePicker.h"
#include "Editor/LevelEditor/Rendering/StaticMeshRenderer.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace lts::editor
{
    namespace
    {
        struct PickBounds final
        {
            DirectX::XMFLOAT3 minimum;
            DirectX::XMFLOAT3 maximum;
        };

        [[nodiscard]]
        PickBounds GetEntityBounds(const EditorEntityKind kind) noexcept
        {
            switch (kind)
            {
                case EditorEntityKind::Environment:
                    return
                    {
                        {-0.90F, -0.10F, -0.90F},
                        {0.90F, 1.70F, 0.90F}
                    };

                case EditorEntityKind::DirectionalLight:
                    return
                    {
                        {-0.75F, -2.20F, -0.75F},
                        {0.75F, 0.75F, 1.75F}
                    };

                case EditorEntityKind::SpawnPoint:
                    return
                    {
                        {-1.00F, -0.10F, -1.00F},
                        {1.00F, 2.10F, 1.00F}
                    };

                case EditorEntityKind::Anomaly:
                    return
                    {
                        {-1.10F, -0.05F, -1.10F},
                        {1.10F, 0.85F, 1.10F}
                    };

                case EditorEntityKind::LootContainer:
                    return
                    {
                        {-0.90F, -0.10F, -0.65F},
                        {0.90F, 1.00F, 0.65F}
                    };

                case EditorEntityKind::Empty:
                default:
                    return
                    {
                        {-0.50F, -0.50F, -0.50F},
                        {0.50F, 0.50F, 0.50F}
                    };
            }
        }

        [[nodiscard]]
        DirectX::XMMATRIX BuildWorldMatrix(
            const EditorTransform& transform) noexcept
        {
            const DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(
                transform.scale[0],
                transform.scale[1],
                transform.scale[2]);

            const DirectX::XMMATRIX rotation =
                DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(
                        transform.rotationDegrees[0]),
                    DirectX::XMConvertToRadians(
                        transform.rotationDegrees[1]),
                    DirectX::XMConvertToRadians(
                        transform.rotationDegrees[2]));

            const DirectX::XMMATRIX translation =
                DirectX::XMMatrixTranslation(
                    transform.position[0],
                    transform.position[1],
                    transform.position[2]);

            return scale * rotation * translation;
        }

        [[nodiscard]]
        bool IntersectAabb(
            const DirectX::XMFLOAT3& rayOrigin,
            const DirectX::XMFLOAT3& rayDirection,
            const PickBounds& bounds,
            float& outDistance) noexcept
        {
            constexpr float DirectionEpsilon = 0.000001F;

            const std::array<float, 3U> origin
            {
                rayOrigin.x,
                rayOrigin.y,
                rayOrigin.z
            };

            const std::array<float, 3U> direction
            {
                rayDirection.x,
                rayDirection.y,
                rayDirection.z
            };

            const std::array<float, 3U> minimum
            {
                bounds.minimum.x,
                bounds.minimum.y,
                bounds.minimum.z
            };

            const std::array<float, 3U> maximum
            {
                bounds.maximum.x,
                bounds.maximum.y,
                bounds.maximum.z
            };

            float minimumDistance = 0.0F;
            float maximumDistance =
                std::numeric_limits<float>::max();

            for (std::size_t axis = 0U; axis < 3U; ++axis)
            {
                if (std::abs(direction[axis]) <= DirectionEpsilon)
                {
                    if (
                        origin[axis] < minimum[axis] ||
                        origin[axis] > maximum[axis])
                    {
                        return false;
                    }

                    continue;
                }

                const float inverseDirection =
                    1.0F / direction[axis];

                float firstDistance =
                    (minimum[axis] - origin[axis]) *
                    inverseDirection;

                float secondDistance =
                    (maximum[axis] - origin[axis]) *
                    inverseDirection;

                if (firstDistance > secondDistance)
                {
                    std::swap(
                        firstDistance,
                        secondDistance);
                }

                minimumDistance = std::max(
                    minimumDistance,
                    firstDistance);

                maximumDistance = std::min(
                    maximumDistance,
                    secondDistance);

                if (minimumDistance > maximumDistance)
                {
                    return false;
                }
            }

            if (
                maximumDistance < 0.0F ||
                !std::isfinite(minimumDistance))
            {
                return false;
            }

            outDistance = minimumDistance;
            return true;
        }

        [[nodiscard]]
        bool IntersectEntity(
            const EditorSceneEntity& entity,
            const EditorPickRay& ray,
            float& outDistance,
            const StaticMeshRenderer* const meshRenderer) noexcept
        {
            const DirectX::XMMATRIX world =
                BuildWorldMatrix(entity.transform);

            DirectX::XMVECTOR determinant;

            const DirectX::XMMATRIX inverseWorld =
                DirectX::XMMatrixInverse(
                    &determinant,
                    world);

            const float determinantValue =
                DirectX::XMVectorGetX(determinant);

            if (
                !std::isfinite(determinantValue) ||
                std::abs(determinantValue) <= 0.000001F)
            {
                return false;
            }

            const DirectX::XMVECTOR worldOrigin =
                DirectX::XMLoadFloat3(&ray.origin);

            const DirectX::XMVECTOR worldDirection =
                DirectX::XMLoadFloat3(&ray.direction);

            const DirectX::XMVECTOR localOriginVector =
                DirectX::XMVector3TransformCoord(
                    worldOrigin,
                    inverseWorld);

            /*
             * Direction не нормализуем после inverse transform.
             * Тогда distance остаётся параметром исходного world-ray
             * и расстояния разных объектов можно корректно сравнивать.
             */
            const DirectX::XMVECTOR localDirectionVector =
                DirectX::XMVector3TransformNormal(
                    worldDirection,
                    inverseWorld);

            DirectX::XMFLOAT3 localOrigin;
            DirectX::XMFLOAT3 localDirection;

            DirectX::XMStoreFloat3(
                &localOrigin,
                localOriginVector);

            DirectX::XMStoreFloat3(
                &localDirection,
                localDirectionVector);

            PickBounds bounds = GetEntityBounds(entity.kind);
            if (entity.staticMesh.has_value() && meshRenderer != nullptr)
            {
                static_cast<void>(meshRenderer->TryGetMeshBounds(
                    entity.staticMesh->assetPath,
                    bounds.minimum,
                    bounds.maximum));
            }

            return IntersectAabb(
                localOrigin,
                localDirection,
                bounds,
                outDistance);
        }
    }

    bool ScenePicker::Pick(
        const SceneDocument& document,
        const EditorPickRay& ray,
        std::size_t& outEntityIndex,
        float& outDistance,
        const StaticMeshRenderer* const meshRenderer) noexcept
    {
        outEntityIndex = InvalidEditorEntityIndex;
        outDistance = std::numeric_limits<float>::max();

        const DirectX::XMVECTOR direction =
            DirectX::XMLoadFloat3(&ray.direction);

        const float directionLengthSquared =
            DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(direction));

        if (
            !std::isfinite(directionLengthSquared) ||
            directionLengthSquared <= 0.000001F)
        {
            return false;
        }

        const auto& entities = document.GetEntities();

        for (std::size_t index = 0U; index < entities.size(); ++index)
        {
            float distance = 0.0F;

            if (!IntersectEntity(
                    entities[index],
                    ray,
                    distance,
                    meshRenderer))
            {
                continue;
            }

            if (distance < outDistance)
            {
                outDistance = distance;
                outEntityIndex = index;
            }
        }

        return outEntityIndex != InvalidEditorEntityIndex;
    }
}
