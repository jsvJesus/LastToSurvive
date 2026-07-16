#include "Editor/EditorSceneRenderer.h"

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Shader.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr std::size_t MaxMarkerVertices = 4096U;
        constexpr std::size_t CircleSegments = 48U;

        constexpr const char* SceneMarkerShaderSource = R"(
cbuffer CameraBuffer : register(b0)
{
    row_major float4x4 ViewProjection;
};

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0f), ViewProjection);
    output.color = input.color;
    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    return input.color;
}
)";

        using MarkerColor = std::array<float, 4U>;

        struct MarkerVertex final
        {
            DirectX::XMFLOAT3 position;
            DirectX::XMFLOAT4 color;
        };

        static_assert(offsetof(MarkerVertex, color) == 12U);
        static_assert(sizeof(MarkerVertex) == 28U);

        struct alignas(16) CameraConstants final
        {
            DirectX::XMFLOAT4X4 viewProjection;
        };

        static_assert(sizeof(CameraConstants) == 64U);

        constexpr MarkerColor EnvironmentColor
        {
            0.52F, 0.58F, 0.62F, 1.0F
        };

        constexpr MarkerColor LightColor
        {
            1.0F, 0.78F, 0.18F, 1.0F
        };

        constexpr MarkerColor SpawnColor
        {
            0.18F, 0.85F, 0.34F, 1.0F
        };

        constexpr MarkerColor AnomalyColor
        {
            0.82F, 0.22F, 0.62F, 1.0F
        };

        constexpr MarkerColor LootColor
        {
            0.18F, 0.68F, 0.88F, 1.0F
        };

        constexpr MarkerColor EmptyColor
        {
            0.70F, 0.70F, 0.70F, 1.0F
        };

        constexpr MarkerColor SelectionColor
        {
            1.0F, 0.58F, 0.08F, 1.0F
        };

        constexpr MarkerColor GizmoXColor
        {
            0.95F, 0.12F, 0.10F, 1.0F
        };

        constexpr MarkerColor GizmoYColor
        {
            0.20F, 0.90F, 0.22F, 1.0F
        };

        constexpr MarkerColor GizmoZColor
        {
            0.12F, 0.42F, 1.0F, 1.0F
        };

        [[nodiscard]]
        DirectX::XMFLOAT4 ToFloat4(
            const MarkerColor& color) noexcept
        {
            return
            {
                color[0],
                color[1],
                color[2],
                color[3]
            };
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 AddVector(
            const DirectX::XMFLOAT3& left,
            const DirectX::XMFLOAT3& right) noexcept
        {
            return
            {
                left.x + right.x,
                left.y + right.y,
                left.z + right.z
            };
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 MultiplyVector(
            const DirectX::XMFLOAT3& value,
            const float scalar) noexcept
        {
            return
            {
                value.x * scalar,
                value.y * scalar,
                value.z * scalar
            };
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 CrossVector(
            const DirectX::XMFLOAT3& left,
            const DirectX::XMFLOAT3& right) noexcept
        {
            return
            {
                left.y * right.z - left.z * right.y,
                left.z * right.x - left.x * right.z,
                left.x * right.y - left.y * right.x
            };
        }

        [[nodiscard]]
        float VectorLengthSquared(
            const DirectX::XMFLOAT3& value) noexcept
        {
            return
                value.x * value.x +
                value.y * value.y +
                value.z * value.z;
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 NormalizeVector(
            const DirectX::XMFLOAT3& value) noexcept
        {
            const float lengthSquared =
                VectorLengthSquared(value);

            if (lengthSquared <= 0.000001F)
            {
                return {};
            }

            return MultiplyVector(
                value,
                1.0F / std::sqrt(lengthSquared));
        }

        [[nodiscard]]
        DirectX::XMMATRIX BuildWorldMatrix(
            const EditorTransform& transform) noexcept
        {
            const DirectX::XMMATRIX scale =
                DirectX::XMMatrixScaling(
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
        DirectX::XMFLOAT3 TransformPoint(
            const DirectX::XMFLOAT3& point,
            const DirectX::XMMATRIX& world) noexcept
        {
            DirectX::XMFLOAT3 result;

            DirectX::XMStoreFloat3(
                &result,
                DirectX::XMVector3TransformCoord(
                    DirectX::XMLoadFloat3(&point),
                    world));

            return result;
        }

        void PushVertex(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMFLOAT3& position,
            const MarkerColor& color) noexcept
        {
            if (vertexCount >= vertices.size())
            {
                return;
            }

            vertices[vertexCount].position = position;
            vertices[vertexCount].color = ToFloat4(color);
            ++vertexCount;
        }

        void AddWorldLine(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMFLOAT3& start,
            const DirectX::XMFLOAT3& end,
            const MarkerColor& color) noexcept
        {
            if (vertexCount + 2U > vertices.size())
            {
                return;
            }

            PushVertex(vertices, vertexCount, start, color);
            PushVertex(vertices, vertexCount, end, color);
        }

        void AddLocalLine(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const DirectX::XMFLOAT3& start,
            const DirectX::XMFLOAT3& end,
            const MarkerColor& color) noexcept
        {
            AddWorldLine(
                vertices,
                vertexCount,
                TransformPoint(start, world),
                TransformPoint(end, world),
                color);
        }

        void AddWireBox(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const DirectX::XMFLOAT3& minimum,
            const DirectX::XMFLOAT3& maximum,
            const MarkerColor& color) noexcept
        {
            const std::array<DirectX::XMFLOAT3, 8U> points
            {{
                {minimum.x, minimum.y, minimum.z},
                {maximum.x, minimum.y, minimum.z},
                {maximum.x, minimum.y, maximum.z},
                {minimum.x, minimum.y, maximum.z},
                {minimum.x, maximum.y, minimum.z},
                {maximum.x, maximum.y, minimum.z},
                {maximum.x, maximum.y, maximum.z},
                {minimum.x, maximum.y, maximum.z}
            }};

            constexpr std::array<std::array<std::size_t, 2U>, 12U> edges
            {{
                {{0U, 1U}}, {{1U, 2U}}, {{2U, 3U}}, {{3U, 0U}},
                {{4U, 5U}}, {{5U, 6U}}, {{6U, 7U}}, {{7U, 4U}},
                {{0U, 4U}}, {{1U, 5U}}, {{2U, 6U}}, {{3U, 7U}}
            }};

            for (const auto& edge : edges)
            {
                AddLocalLine(
                    vertices,
                    vertexCount,
                    world,
                    points[edge[0]],
                    points[edge[1]],
                    color);
            }
        }

        void AddCircle(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& axisA,
            const DirectX::XMFLOAT3& axisB,
            const float radius,
            const MarkerColor& color) noexcept
        {
            for (std::size_t index = 0U; index < CircleSegments; ++index)
            {
                const float firstAngle =
                    DirectX::XM_2PI *
                    static_cast<float>(index) /
                    static_cast<float>(CircleSegments);

                const float secondAngle =
                    DirectX::XM_2PI *
                    static_cast<float>(index + 1U) /
                    static_cast<float>(CircleSegments);

                const DirectX::XMFLOAT3 first = AddVector(
                    origin,
                    AddVector(
                        MultiplyVector(axisA, std::cos(firstAngle) * radius),
                        MultiplyVector(axisB, std::sin(firstAngle) * radius)));

                const DirectX::XMFLOAT3 second = AddVector(
                    origin,
                    AddVector(
                        MultiplyVector(axisA, std::cos(secondAngle) * radius),
                        MultiplyVector(axisB, std::sin(secondAngle) * radius)));

                AddWorldLine(
                    vertices,
                    vertexCount,
                    first,
                    second,
                    color);
            }
        }

        [[nodiscard]]
        const MarkerColor& GetEntityColor(
            const EditorEntityKind kind,
            const bool selected) noexcept
        {
            if (selected)
            {
                return SelectionColor;
            }

            switch (kind)
            {
                case EditorEntityKind::Environment:
                    return EnvironmentColor;

                case EditorEntityKind::DirectionalLight:
                    return LightColor;

                case EditorEntityKind::SpawnPoint:
                    return SpawnColor;

                case EditorEntityKind::Anomaly:
                    return AnomalyColor;

                case EditorEntityKind::LootContainer:
                    return LootColor;

                case EditorEntityKind::Empty:
                default:
                    return EmptyColor;
            }
        }

        void AddEntityMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const EditorSceneEntity& entity,
            const bool selected) noexcept
        {
            const DirectX::XMMATRIX world =
                BuildWorldMatrix(entity.transform);

            const MarkerColor& color =
                GetEntityColor(entity.kind, selected);

            switch (entity.kind)
            {
                case EditorEntityKind::Environment:
                    AddWireBox(
                        vertices,
                        vertexCount,
                        world,
                        {-0.75F, 0.0F, -0.75F},
                        {0.75F, 1.5F, 0.75F},
                        color);
                    break;

                case EditorEntityKind::DirectionalLight:
                {
                    const DirectX::XMFLOAT3 origin =
                        TransformPoint({0.0F, 0.0F, 0.0F}, world);

                    const DirectX::XMFLOAT3 axisX =
                        NormalizeVector(
                            TransformPoint({1.0F, 0.0F, 0.0F}, world));

                    static_cast<void>(axisX);

                    AddCircle(
                        vertices,
                        vertexCount,
                        origin,
                        {1.0F, 0.0F, 0.0F},
                        {0.0F, 1.0F, 0.0F},
                        0.45F,
                        color);

                    AddLocalLine(
                        vertices,
                        vertexCount,
                        world,
                        {0.0F, 0.0F, 0.0F},
                        {0.0F, -2.0F, 1.5F},
                        color);
                    break;
                }

                case EditorEntityKind::SpawnPoint:
                    AddLocalLine(
                        vertices,
                        vertexCount,
                        world,
                        {0.0F, 0.0F, 0.0F},
                        {0.0F, 2.0F, 0.0F},
                        color);

                    AddLocalLine(
                        vertices,
                        vertexCount,
                        world,
                        {0.0F, 1.1F, 0.0F},
                        {0.0F, 1.1F, 1.5F},
                        color);
                    break;

                case EditorEntityKind::Anomaly:
                {
                    const DirectX::XMFLOAT3 origin =
                        TransformPoint({0.0F, 0.05F, 0.0F}, world);

                    AddCircle(
                        vertices,
                        vertexCount,
                        origin,
                        {1.0F, 0.0F, 0.0F},
                        {0.0F, 0.0F, 1.0F},
                        1.0F,
                        color);

                    AddCircle(
                        vertices,
                        vertexCount,
                        origin,
                        {1.0F, 0.0F, 0.0F},
                        {0.0F, 0.0F, 1.0F},
                        0.60F,
                        color);
                    break;
                }

                case EditorEntityKind::LootContainer:
                    AddWireBox(
                        vertices,
                        vertexCount,
                        world,
                        {-0.75F, 0.0F, -0.50F},
                        {0.75F, 0.80F, 0.50F},
                        color);
                    break;

                case EditorEntityKind::Empty:
                default:
                    AddLocalLine(
                        vertices,
                        vertexCount,
                        world,
                        {-0.50F, 0.0F, 0.0F},
                        {0.50F, 0.0F, 0.0F},
                        color);

                    AddLocalLine(
                        vertices,
                        vertexCount,
                        world,
                        {0.0F, -0.50F, 0.0F},
                        {0.0F, 0.50F, 0.0F},
                        color);

                    AddLocalLine(
                        vertices,
                        vertexCount,
                        world,
                        {0.0F, 0.0F, -0.50F},
                        {0.0F, 0.0F, 0.50F},
                        color);
                    break;
            }

            if (selected)
            {
                AddWireBox(
                    vertices,
                    vertexCount,
                    world,
                    {-1.0F, -0.10F, -1.0F},
                    {1.0F, 2.1F, 1.0F},
                    SelectionColor);
            }
        }

        [[nodiscard]]
        MarkerColor GetGizmoAxisColor(
            const EditorTransformAxis axis,
            const EditorTransformVisualState& state) noexcept
        {
            if (
                axis == state.activeAxis ||
                axis == state.hotAxis)
            {
                return SelectionColor;
            }

            switch (axis)
            {
                case EditorTransformAxis::X:
                    return GizmoXColor;

                case EditorTransformAxis::Y:
                    return GizmoYColor;

                case EditorTransformAxis::Z:
                    return GizmoZColor;

                case EditorTransformAxis::None:
                default:
                    return SelectionColor;
            }
        }

        void GetGizmoAxes(
            const EditorSceneEntity& entity,
            const EditorTransformVisualState& state,
            DirectX::XMFLOAT3& axisX,
            DirectX::XMFLOAT3& axisY,
            DirectX::XMFLOAT3& axisZ) noexcept
        {
            axisX = {1.0F, 0.0F, 0.0F};
            axisY = {0.0F, 1.0F, 0.0F};
            axisZ = {0.0F, 0.0F, 1.0F};

            if (state.space == EditorTransformSpace::World)
            {
                return;
            }

            const DirectX::XMMATRIX rotation =
                DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(
                        entity.transform.rotationDegrees[0]),
                    DirectX::XMConvertToRadians(
                        entity.transform.rotationDegrees[1]),
                    DirectX::XMConvertToRadians(
                        entity.transform.rotationDegrees[2]));

            DirectX::XMStoreFloat3(
                &axisX,
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3TransformNormal(
                        DirectX::XMLoadFloat3(&axisX),
                        rotation)));

            DirectX::XMStoreFloat3(
                &axisY,
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3TransformNormal(
                        DirectX::XMLoadFloat3(&axisY),
                        rotation)));

            DirectX::XMStoreFloat3(
                &axisZ,
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3TransformNormal(
                        DirectX::XMLoadFloat3(&axisZ),
                        rotation)));
        }

        void AddGizmoArrow(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& axis,
            const MarkerColor& color) noexcept
        {
            constexpr float AxisLength = 2.5F;
            constexpr float ArrowLength = 0.35F;
            constexpr float ArrowWidth = 0.18F;

            const DirectX::XMFLOAT3 end =
                AddVector(
                    origin,
                    MultiplyVector(axis, AxisLength));

            const DirectX::XMFLOAT3 arrowBase =
                AddVector(
                    end,
                    MultiplyVector(axis, -ArrowLength));

            const DirectX::XMFLOAT3 reference =
                std::abs(axis.y) < 0.9F
                    ? DirectX::XMFLOAT3{0.0F, 1.0F, 0.0F}
                    : DirectX::XMFLOAT3{1.0F, 0.0F, 0.0F};

            const DirectX::XMFLOAT3 sideA =
                NormalizeVector(
                    CrossVector(axis, reference));

            const DirectX::XMFLOAT3 sideB =
                NormalizeVector(
                    CrossVector(axis, sideA));

            AddWorldLine(vertices, vertexCount, origin, end, color);

            AddWorldLine(
                vertices,
                vertexCount,
                end,
                AddVector(
                    arrowBase,
                    MultiplyVector(sideA, ArrowWidth)),
                color);

            AddWorldLine(
                vertices,
                vertexCount,
                end,
                AddVector(
                    arrowBase,
                    MultiplyVector(sideA, -ArrowWidth)),
                color);

            AddWorldLine(
                vertices,
                vertexCount,
                end,
                AddVector(
                    arrowBase,
                    MultiplyVector(sideB, ArrowWidth)),
                color);

            AddWorldLine(
                vertices,
                vertexCount,
                end,
                AddVector(
                    arrowBase,
                    MultiplyVector(sideB, -ArrowWidth)),
                color);
        }

        void AddScaleGizmoAxis(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& axis,
            const MarkerColor& color) noexcept
        {
            constexpr float AxisLength = 2.5F;
            constexpr float CubeHalfSize = 0.16F;

            const DirectX::XMFLOAT3 end =
                AddVector(
                    origin,
                    MultiplyVector(axis, AxisLength));

            AddWorldLine(
                vertices,
                vertexCount,
                origin,
                end,
                color);

            const DirectX::XMMATRIX cubeWorld =
                DirectX::XMMatrixTranslation(
                    end.x,
                    end.y,
                    end.z);

            AddWireBox(
                vertices,
                vertexCount,
                cubeWorld,
                {-CubeHalfSize, -CubeHalfSize, -CubeHalfSize},
                {CubeHalfSize, CubeHalfSize, CubeHalfSize},
                color);
        }

        void AddRotationGizmoAxis(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMFLOAT3& origin,
            const DirectX::XMFLOAT3& axis,
            const MarkerColor& color) noexcept
        {
            const DirectX::XMFLOAT3 reference =
                std::abs(axis.y) < 0.9F
                    ? DirectX::XMFLOAT3{0.0F, 1.0F, 0.0F}
                    : DirectX::XMFLOAT3{1.0F, 0.0F, 0.0F};

            const DirectX::XMFLOAT3 axisA =
                NormalizeVector(
                    CrossVector(axis, reference));

            const DirectX::XMFLOAT3 axisB =
                NormalizeVector(
                    CrossVector(axis, axisA));

            AddCircle(
                vertices,
                vertexCount,
                origin,
                axisA,
                axisB,
                2.0F,
                color);
        }

        void AddTransformGizmo(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const EditorSceneEntity& entity,
            const EditorTransformVisualState& state) noexcept
        {
            const DirectX::XMFLOAT3 origin
            {
                entity.transform.position[0],
                entity.transform.position[1] + 0.08F,
                entity.transform.position[2]
            };

            DirectX::XMFLOAT3 axisX;
            DirectX::XMFLOAT3 axisY;
            DirectX::XMFLOAT3 axisZ;

            GetGizmoAxes(
                entity,
                state,
                axisX,
                axisY,
                axisZ);

            const MarkerColor colorX =
                GetGizmoAxisColor(EditorTransformAxis::X, state);

            const MarkerColor colorY =
                GetGizmoAxisColor(EditorTransformAxis::Y, state);

            const MarkerColor colorZ =
                GetGizmoAxisColor(EditorTransformAxis::Z, state);

            switch (state.operation)
            {
                case EditorTransformOperation::Move:
                    AddGizmoArrow(
                        vertices,
                        vertexCount,
                        origin,
                        axisX,
                        colorX);

                    AddGizmoArrow(
                        vertices,
                        vertexCount,
                        origin,
                        axisY,
                        colorY);

                    AddGizmoArrow(
                        vertices,
                        vertexCount,
                        origin,
                        axisZ,
                        colorZ);
                    break;

                case EditorTransformOperation::Rotate:
                    AddRotationGizmoAxis(
                        vertices,
                        vertexCount,
                        origin,
                        axisX,
                        colorX);

                    AddRotationGizmoAxis(
                        vertices,
                        vertexCount,
                        origin,
                        axisY,
                        colorY);

                    AddRotationGizmoAxis(
                        vertices,
                        vertexCount,
                        origin,
                        axisZ,
                        colorZ);
                    break;

                case EditorTransformOperation::Scale:
                    AddScaleGizmoAxis(
                        vertices,
                        vertexCount,
                        origin,
                        axisX,
                        colorX);

                    AddScaleGizmoAxis(
                        vertices,
                        vertexCount,
                        origin,
                        axisY,
                        colorY);

                    AddScaleGizmoAxis(
                        vertices,
                        vertexCount,
                        origin,
                        axisZ,
                        colorZ);
                    break;

                default:
                    break;
            }
        }

        [[nodiscard]]
        std::size_t BuildSceneVertices(
            const EditorSceneDocument& document,
            const EditorTransformVisualState& transformState,
            std::array<MarkerVertex, MaxMarkerVertices>& vertices) noexcept
        {
            std::size_t vertexCount = 0U;

            const auto& entities = document.GetEntities();
            const std::size_t selectedIndex =
                document.GetSelectedIndex();

            for (std::size_t index = 0U; index < entities.size(); ++index)
            {
                AddEntityMarker(
                    vertices,
                    vertexCount,
                    entities[index],
                    index == selectedIndex);

                if (vertexCount >= vertices.size())
                {
                    break;
                }
            }

            if (
                selectedIndex < entities.size() &&
                vertexCount < vertices.size())
            {
                AddTransformGizmo(
                    vertices,
                    vertexCount,
                    entities[selectedIndex],
                    transformState);
            }

            return vertexCount;
        }

        [[nodiscard]]
        bool CompileShader(
            const char* const entryPoint,
            const char* const target,
            ComPtr<ID3DBlob>& bytecode) noexcept
        {
            if (entryPoint == nullptr || target == nullptr)
            {
                return false;
            }

            bytecode.Reset();

            ComPtr<ID3DBlob> errors;

            constexpr UINT compileFlags =
                D3DCOMPILE_ENABLE_STRICTNESS |
                D3DCOMPILE_WARNINGS_ARE_ERRORS |
                D3DCOMPILE_OPTIMIZATION_LEVEL3;

            const HRESULT result = D3DCompile(
                SceneMarkerShaderSource,
                std::char_traits<char>::length(
                    SceneMarkerShaderSource),
                "EditorSceneMarkers.hlsl",
                nullptr,
                nullptr,
                entryPoint,
                target,
                compileFlags,
                0,
                bytecode.GetAddressOf(),
                errors.GetAddressOf());

            if (SUCCEEDED(result))
            {
                return true;
            }

            if (
                errors != nullptr &&
                errors->GetBufferPointer() != nullptr)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.SceneRenderer",
                    static_cast<const char*>(
                        errors->GetBufferPointer()));
            }
            else
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.SceneRenderer",
                    "Failed to compile scene marker shader.");
            }

            return false;
        }

        void LogGraphicsFailure(
            const char* const operation,
            const engine::graphics::GraphicsResult result) noexcept
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Unknown scene renderer operation";

            message += " failed: ";
            message += engine::graphics::ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.SceneRenderer",
                message);
        }
    }

    bool EditorSceneRenderer::Initialize(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (initialized_)
        {
            return true;
        }

        ComPtr<ID3DBlob> vertexBytecode;
        ComPtr<ID3DBlob> pixelBytecode;

        if (!CompileShader("VSMain", "vs_5_0", vertexBytecode))
        {
            return false;
        }

        if (!CompileShader("PSMain", "ps_5_0", pixelBytecode))
        {
            return false;
        }

        engine::graphics::ShaderDesc vertexShaderDescription;
        vertexShaderDescription.stage =
            engine::graphics::ShaderStage::Vertex;

        vertexShaderDescription.bytecode.data =
            vertexBytecode->GetBufferPointer();

        vertexShaderDescription.bytecode.size =
            vertexBytecode->GetBufferSize();

        vertexShaderDescription.debugName =
            "EditorSceneRenderer.VertexShader";

        auto result = device.CreateShader(
            vertexShaderDescription,
            vertexShader_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create scene marker vertex shader",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::ShaderDesc pixelShaderDescription;
        pixelShaderDescription.stage =
            engine::graphics::ShaderStage::Pixel;

        pixelShaderDescription.bytecode.data =
            pixelBytecode->GetBufferPointer();

        pixelShaderDescription.bytecode.size =
            pixelBytecode->GetBufferSize();

        pixelShaderDescription.debugName =
            "EditorSceneRenderer.PixelShader";

        result = device.CreateShader(
            pixelShaderDescription,
            pixelShader_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create scene marker pixel shader",
                result);

            Shutdown(device);
            return false;
        }

        const std::array<
            engine::graphics::VertexElementDesc,
            2U> elements
        {{
            {
                "POSITION",
                0U,
                engine::graphics::Format::R32G32B32Float,
                0U,
                0U,
                engine::graphics::VertexInputRate::PerVertex,
                0U
            },
            {
                "COLOR",
                0U,
                engine::graphics::Format::R32G32B32A32Float,
                0U,
                12U,
                engine::graphics::VertexInputRate::PerVertex,
                0U
            }
        }};

        engine::graphics::InputLayoutDesc inputLayoutDescription;
        inputLayoutDescription.vertexShader = vertexShader_;
        inputLayoutDescription.elements = elements.data();
        inputLayoutDescription.elementCount = elements.size();
        inputLayoutDescription.debugName =
            "EditorSceneRenderer.InputLayout";

        result = device.CreateInputLayout(
            inputLayoutDescription,
            inputLayout_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create scene marker input layout",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::BufferDesc vertexBufferDescription;
        vertexBufferDescription.byteSize =
            sizeof(MarkerVertex) * MaxMarkerVertices;

        vertexBufferDescription.stride =
            static_cast<std::uint32_t>(
                sizeof(MarkerVertex));

        vertexBufferDescription.usage =
            engine::graphics::ResourceUsage::Dynamic;

        vertexBufferDescription.bindFlags =
            engine::graphics::BufferBindFlags::Vertex;

        vertexBufferDescription.miscFlags =
            engine::graphics::BufferMiscFlags::None;

        vertexBufferDescription.cpuAccess =
            engine::graphics::CpuAccessFlags::Write;

        vertexBufferDescription.indexFormat =
            engine::graphics::IndexFormat::None;

        result = device.CreateBuffer(
            vertexBufferDescription,
            nullptr,
            vertexBuffer_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create scene marker vertex buffer",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::BufferDesc cameraBufferDescription;
        cameraBufferDescription.byteSize =
            sizeof(CameraConstants);

        cameraBufferDescription.stride = 0U;
        cameraBufferDescription.usage =
            engine::graphics::ResourceUsage::Default;

        cameraBufferDescription.bindFlags =
            engine::graphics::BufferBindFlags::Constant;

        cameraBufferDescription.miscFlags =
            engine::graphics::BufferMiscFlags::None;

        cameraBufferDescription.cpuAccess =
            engine::graphics::CpuAccessFlags::None;

        cameraBufferDescription.indexFormat =
            engine::graphics::IndexFormat::None;

        result = device.CreateBuffer(
            cameraBufferDescription,
            nullptr,
            cameraBuffer_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create scene marker camera buffer",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::GraphicsPipelineDesc pipelineDescription;
        pipelineDescription.vertexShader = vertexShader_;
        pipelineDescription.pixelShader = pixelShader_;
        pipelineDescription.inputLayout = inputLayout_;
        pipelineDescription.topology =
            engine::graphics::PrimitiveTopology::LineList;

        pipelineDescription.rasterizer.fillMode =
            engine::graphics::FillMode::Solid;

        pipelineDescription.rasterizer.cullMode =
            engine::graphics::CullMode::None;

        pipelineDescription.rasterizer.depthClipEnable = true;

        pipelineDescription.blend.renderTargets[0].blendEnable = false;

        pipelineDescription.depthStencil.depthEnable = true;
        pipelineDescription.depthStencil.depthWriteEnable = false;
        pipelineDescription.depthStencil.depthFunction =
            engine::graphics::ComparisonFunction::LessEqual;

        pipelineDescription.debugName =
            "EditorSceneRenderer.Pipeline";

        result = device.CreateGraphicsPipeline(
            pipelineDescription,
            pipeline_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create scene marker pipeline",
                result);

            Shutdown(device);
            return false;
        }

        initialized_ = true;

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.SceneRenderer",
            "Editor scene renderer initialized.");

        return true;
    }

    void EditorSceneRenderer::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        initialized_ = false;

        if (pipeline_.IsValid())
        {
            static_cast<void>(
                device.DestroyGraphicsPipeline(pipeline_));
            pipeline_ = {};
        }

        if (inputLayout_.IsValid())
        {
            static_cast<void>(
                device.DestroyInputLayout(inputLayout_));
            inputLayout_ = {};
        }

        if (pixelShader_.IsValid())
        {
            static_cast<void>(
                device.DestroyShader(pixelShader_));
            pixelShader_ = {};
        }

        if (vertexShader_.IsValid())
        {
            static_cast<void>(
                device.DestroyShader(vertexShader_));
            vertexShader_ = {};
        }

        if (cameraBuffer_.IsValid())
        {
            static_cast<void>(
                device.DestroyBuffer(cameraBuffer_));
            cameraBuffer_ = {};
        }

        if (vertexBuffer_.IsValid())
        {
            static_cast<void>(
                device.DestroyBuffer(vertexBuffer_));
            vertexBuffer_ = {};
        }
    }

    engine::graphics::GraphicsResult EditorSceneRenderer::Render(
        engine::graphics::CommandContext& context,
        const EditorSceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection,
        const EditorTransformVisualState& transformState) noexcept
    {
        if (!initialized_)
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        std::array<MarkerVertex, MaxMarkerVertices> vertices{};

        const std::size_t vertexCount =
            BuildSceneVertices(
                document,
                transformState,
                vertices);

        if (vertexCount == 0U)
        {
            return engine::graphics::GraphicsResult::Success;
        }

        auto result = context.UpdateBuffer(
            vertexBuffer_,
            vertices.data(),
            sizeof(vertices));

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        CameraConstants cameraConstants;
        cameraConstants.viewProjection = viewProjection;

        result = context.UpdateBuffer(
            cameraBuffer_,
            &cameraConstants,
            sizeof(cameraConstants));

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        result = context.SetGraphicsPipeline(pipeline_);

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        engine::graphics::VertexBufferBinding vertexBinding;
        vertexBinding.buffer = vertexBuffer_;
        vertexBinding.stride =
            static_cast<std::uint32_t>(
                sizeof(MarkerVertex));
        vertexBinding.offset = 0U;

        result = context.SetVertexBuffers(
            0U,
            &vertexBinding,
            1U);

        if (engine::graphics::Failed(result))
        {
            context.UnbindGraphicsPipeline();
            return result;
        }

        result = context.SetConstantBuffers(
            engine::graphics::ShaderStage::Vertex,
            0U,
            &cameraBuffer_,
            1U);

        if (engine::graphics::Failed(result))
        {
            context.UnbindGraphicsPipeline();
            return result;
        }

        result = context.Draw(
            static_cast<std::uint32_t>(vertexCount),
            0U);

        static_cast<void>(
            context.UnbindConstantBuffers(
                engine::graphics::ShaderStage::Vertex,
                0U,
                1U));

        context.UnbindGraphicsPipeline();
        return result;
    }

    bool EditorSceneRenderer::IsInitialized() const noexcept
    {
        return initialized_;
    }
}
