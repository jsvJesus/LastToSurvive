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
        constexpr std::size_t CircleSegments = 32U;

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

    output.position = mul(
        float4(input.position, 1.0f),
        ViewProjection);

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
            0.52F,
            0.58F,
            0.62F,
            1.0F
        };

        constexpr MarkerColor LightColor
        {
            1.0F,
            0.78F,
            0.18F,
            1.0F
        };

        constexpr MarkerColor SpawnColor
        {
            0.18F,
            0.85F,
            0.34F,
            1.0F
        };

        constexpr MarkerColor AnomalyColor
        {
            0.82F,
            0.22F,
            0.62F,
            1.0F
        };

        constexpr MarkerColor LootColor
        {
            0.18F,
            0.68F,
            0.88F,
            1.0F
        };

        constexpr MarkerColor EmptyColor
        {
            0.70F,
            0.70F,
            0.70F,
            1.0F
        };

        constexpr MarkerColor SelectionColor
{
    1.0F,
    0.58F,
    0.08F,
    1.0F
};

        constexpr MarkerColor GizmoXColor
        {
            0.95F,
            0.12F,
            0.10F,
            1.0F
        };

        constexpr MarkerColor GizmoYColor
        {
            0.20F,
            0.90F,
            0.22F,
            1.0F
        };

        constexpr MarkerColor GizmoZColor
        {
            0.12F,
            0.42F,
            1.0F,
            1.0F
        };

        [[nodiscard]]
        DirectX::XMFLOAT4 ToFloat4(const MarkerColor& color) noexcept
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

            return DirectX::XMMatrixMultiply(
                DirectX::XMMatrixMultiply(scale, rotation),
                translation);
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
                {{0U, 1U}},
                {{1U, 2U}},
                {{2U, 3U}},
                {{3U, 0U}},

                {{4U, 5U}},
                {{5U, 6U}},
                {{6U, 7U}},
                {{7U, 4U}},

                {{0U, 4U}},
                {{1U, 5U}},
                {{2U, 6U}},
                {{3U, 7U}}
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

        void AddCircleXZ(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const float radius,
            const float height,
            const MarkerColor& color) noexcept
        {
            for (std::size_t index = 0U; index < CircleSegments; ++index)
            {
                const float startAngle =
                    DirectX::XM_2PI *
                    static_cast<float>(index) /
                    static_cast<float>(CircleSegments);

                const float endAngle =
                    DirectX::XM_2PI *
                    static_cast<float>(index + 1U) /
                    static_cast<float>(CircleSegments);

                const DirectX::XMFLOAT3 start
                {
                    std::cos(startAngle) * radius,
                    height,
                    std::sin(startAngle) * radius
                };

                const DirectX::XMFLOAT3 end
                {
                    std::cos(endAngle) * radius,
                    height,
                    std::sin(endAngle) * radius
                };

                AddLocalLine(
                    vertices,
                    vertexCount,
                    world,
                    start,
                    end,
                    color);
            }
        }

        void AddCircleXY(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const float radius,
            const MarkerColor& color) noexcept
        {
            for (std::size_t index = 0U; index < CircleSegments; ++index)
            {
                const float startAngle =
                    DirectX::XM_2PI *
                    static_cast<float>(index) /
                    static_cast<float>(CircleSegments);

                const float endAngle =
                    DirectX::XM_2PI *
                    static_cast<float>(index + 1U) /
                    static_cast<float>(CircleSegments);

                const DirectX::XMFLOAT3 start
                {
                    std::cos(startAngle) * radius,
                    std::sin(startAngle) * radius,
                    0.0F
                };

                const DirectX::XMFLOAT3 end
                {
                    std::cos(endAngle) * radius,
                    std::sin(endAngle) * radius,
                    0.0F
                };

                AddLocalLine(
                    vertices,
                    vertexCount,
                    world,
                    start,
                    end,
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

        void AddEnvironmentMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const MarkerColor& color) noexcept
        {
            AddWireBox(
                vertices,
                vertexCount,
                world,
                {-0.75F, 0.0F, -0.75F},
                {0.75F, 1.5F, 0.75F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {-1.0F, 0.0F, 0.0F},
                {1.0F, 0.0F, 0.0F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, 0.0F, -1.0F},
                {0.0F, 0.0F, 1.0F},
                color);
        }

        void AddDirectionalLightMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const MarkerColor& color) noexcept
        {
            AddCircleXY(
                vertices,
                vertexCount,
                world,
                0.45F,
                color);

            AddCircleXZ(
                vertices,
                vertexCount,
                world,
                0.45F,
                0.0F,
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, 0.0F, 0.0F},
                {0.0F, -2.0F, 1.5F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, -2.0F, 1.5F},
                {-0.35F, -1.55F, 1.25F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, -2.0F, 1.5F},
                {0.35F, -1.55F, 1.25F},
                color);
        }

        void AddSpawnPointMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const MarkerColor& color) noexcept
        {
            AddCircleXZ(
                vertices,
                vertexCount,
                world,
                0.65F,
                0.03F,
                color);

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

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, 1.1F, 1.5F},
                {-0.30F, 1.1F, 1.10F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, 1.1F, 1.5F},
                {0.30F, 1.1F, 1.10F},
                color);
        }

        void AddAnomalyMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const MarkerColor& color) noexcept
        {
            AddCircleXZ(
                vertices,
                vertexCount,
                world,
                1.0F,
                0.04F,
                color);

            AddCircleXZ(
                vertices,
                vertexCount,
                world,
                0.60F,
                0.05F,
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {-1.0F, 0.05F, 0.0F},
                {1.0F, 0.05F, 0.0F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.0F, 0.05F, -1.0F},
                {0.0F, 0.05F, 1.0F},
                color);

            constexpr std::array<DirectX::XMFLOAT3, 4U> spikes
            {{
                {0.65F, 0.0F, 0.0F},
                {-0.65F, 0.0F, 0.0F},
                {0.0F, 0.0F, 0.65F},
                {0.0F, 0.0F, -0.65F}
            }};

            for (const DirectX::XMFLOAT3& spike : spikes)
            {
                AddLocalLine(
                    vertices,
                    vertexCount,
                    world,
                    spike,
                    {spike.x, 0.75F, spike.z},
                    color);
            }
        }

        void AddLootContainerMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const MarkerColor& color) noexcept
        {
            AddWireBox(
                vertices,
                vertexCount,
                world,
                {-0.75F, 0.0F, -0.50F},
                {0.75F, 0.80F, 0.50F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {-0.75F, 0.80F, -0.50F},
                {0.75F, 0.80F, 0.50F},
                color);

            AddLocalLine(
                vertices,
                vertexCount,
                world,
                {0.75F, 0.80F, -0.50F},
                {-0.75F, 0.80F, 0.50F},
                color);
        }

        void AddEmptyMarker(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const MarkerColor& color) noexcept
        {
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
        }

        void AddSelectionBounds(
            std::array<MarkerVertex, MaxMarkerVertices>& vertices,
            std::size_t& vertexCount,
            const DirectX::XMMATRIX& world,
            const EditorEntityKind kind) noexcept
        {
            DirectX::XMFLOAT3 minimum
            {
                -1.0F,
                -0.10F,
                -1.0F
            };

            DirectX::XMFLOAT3 maximum
            {
                1.0F,
                2.1F,
                1.0F
            };

            switch (kind)
            {
                case EditorEntityKind::Anomaly:
                    minimum = {-1.10F, -0.05F, -1.10F};
                    maximum = {1.10F, 0.85F, 1.10F};
                    break;

                case EditorEntityKind::LootContainer:
                    minimum = {-0.90F, -0.10F, -0.65F};
                    maximum = {0.90F, 1.0F, 0.65F};
                    break;

                case EditorEntityKind::DirectionalLight:
                    minimum = {-0.75F, -2.2F, -0.75F};
                    maximum = {0.75F, 0.75F, 1.75F};
                    break;

                case EditorEntityKind::Environment:
                    minimum = {-0.90F, -0.10F, -0.90F};
                    maximum = {0.90F, 1.70F, 0.90F};
                    break;

                case EditorEntityKind::SpawnPoint:
                case EditorEntityKind::Empty:
                default:
                    break;
            }

            AddWireBox(
                vertices,
                vertexCount,
                world,
                minimum,
                maximum,
                SelectionColor);
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
                    AddEnvironmentMarker(
                        vertices,
                        vertexCount,
                        world,
                        color);
                    break;

                case EditorEntityKind::DirectionalLight:
                    AddDirectionalLightMarker(
                        vertices,
                        vertexCount,
                        world,
                        color);
                    break;

                case EditorEntityKind::SpawnPoint:
                    AddSpawnPointMarker(
                        vertices,
                        vertexCount,
                        world,
                        color);
                    break;

                case EditorEntityKind::Anomaly:
                    AddAnomalyMarker(
                        vertices,
                        vertexCount,
                        world,
                        color);
                    break;

                case EditorEntityKind::LootContainer:
                    AddLootContainerMarker(
                        vertices,
                        vertexCount,
                        world,
                        color);
                    break;

                case EditorEntityKind::Empty:
                default:
                    AddEmptyMarker(
                        vertices,
                        vertexCount,
                        world,
                        color);
                    break;
            }

            if (selected)
            {
                AddSelectionBounds(
                    vertices,
                    vertexCount,
                    world,
                    entity.kind);
            }
        }

        void AddGizmoArrow(
        std::array<MarkerVertex, MaxMarkerVertices>& vertices,
        std::size_t& vertexCount,
        const DirectX::XMFLOAT3& origin,
        const DirectX::XMFLOAT3& axis,
        const DirectX::XMFLOAT3& arrowSideA,
        const DirectX::XMFLOAT3& arrowSideB,
        const MarkerColor& color) noexcept
    {
        constexpr float AxisLength = 2.5F;
        constexpr float ArrowLength = 0.35F;
        constexpr float ArrowWidth = 0.18F;

        const DirectX::XMFLOAT3 end
        {
            origin.x + axis.x * AxisLength,
            origin.y + axis.y * AxisLength,
            origin.z + axis.z * AxisLength
        };

        const DirectX::XMFLOAT3 arrowBase
        {
            end.x - axis.x * ArrowLength,
            end.y - axis.y * ArrowLength,
            end.z - axis.z * ArrowLength
        };

        AddWorldLine(
            vertices,
            vertexCount,
            origin,
            end,
            color);

        AddWorldLine(
            vertices,
            vertexCount,
            end,
            {
                arrowBase.x + arrowSideA.x * ArrowWidth,
                arrowBase.y + arrowSideA.y * ArrowWidth,
                arrowBase.z + arrowSideA.z * ArrowWidth
            },
            color);

        AddWorldLine(
            vertices,
            vertexCount,
            end,
            {
                arrowBase.x - arrowSideA.x * ArrowWidth,
                arrowBase.y - arrowSideA.y * ArrowWidth,
                arrowBase.z - arrowSideA.z * ArrowWidth
            },
            color);

        AddWorldLine(
            vertices,
            vertexCount,
            end,
            {
                arrowBase.x + arrowSideB.x * ArrowWidth,
                arrowBase.y + arrowSideB.y * ArrowWidth,
                arrowBase.z + arrowSideB.z * ArrowWidth
            },
            color);

        AddWorldLine(
            vertices,
            vertexCount,
            end,
            {
                arrowBase.x - arrowSideB.x * ArrowWidth,
                arrowBase.y - arrowSideB.y * ArrowWidth,
                arrowBase.z - arrowSideB.z * ArrowWidth
            },
            color);
    }

    void AddTransformGizmo(
        std::array<MarkerVertex, MaxMarkerVertices>& vertices,
        std::size_t& vertexCount,
        const EditorSceneEntity& entity) noexcept
    {
        DirectX::XMFLOAT3 origin
        {
            entity.transform.position[0],
            entity.transform.position[1] + 0.08F,
            entity.transform.position[2]
        };

        AddGizmoArrow(
            vertices,
            vertexCount,
            origin,
            {1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
            {0.0F, 0.0F, 1.0F},
            GizmoXColor);

        AddGizmoArrow(
            vertices,
            vertexCount,
            origin,
            {0.0F, 1.0F, 0.0F},
            {1.0F, 0.0F, 0.0F},
            {0.0F, 0.0F, 1.0F},
            GizmoYColor);

        AddGizmoArrow(
            vertices,
            vertexCount,
            origin,
            {0.0F, 0.0F, 1.0F},
            {1.0F, 0.0F, 0.0F},
            {0.0F, 1.0F, 0.0F},
            GizmoZColor);
    }

        [[nodiscard]]
        std::size_t BuildSceneVertices(
        const EditorSceneDocument& document,
        std::array<MarkerVertex, MaxMarkerVertices>& vertices) noexcept
        {
            std::size_t vertexCount = 0U;

            const auto& entities = document.GetEntities();
            const std::size_t selectedIndex = document.GetSelectedIndex();

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
                    entities[selectedIndex]);
            }

            return vertexCount;
        }

        [[nodiscard]]
        bool CompileShader(
            const char* entryPoint,
            const char* target,
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

            if (errors != nullptr && errors->GetBufferPointer() != nullptr)
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
            const char* operation,
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
        vertexShaderDescription.stage = engine::graphics::ShaderStage::Vertex;
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
        pixelShaderDescription.stage = engine::graphics::ShaderStage::Pixel;
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
            static_cast<std::uint32_t>(sizeof(MarkerVertex));
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
                device.DestroyGraphicsPipeline(
                    pipeline_));

            pipeline_ = {};
        }

        if (inputLayout_.IsValid())
        {
            static_cast<void>(
                device.DestroyInputLayout(
                    inputLayout_));

            inputLayout_ = {};
        }

        if (pixelShader_.IsValid())
        {
            static_cast<void>(
                device.DestroyShader(
                    pixelShader_));

            pixelShader_ = {};
        }

        if (vertexShader_.IsValid())
        {
            static_cast<void>(
                device.DestroyShader(
                    vertexShader_));

            vertexShader_ = {};
        }

        if (cameraBuffer_.IsValid())
        {
            static_cast<void>(
                device.DestroyBuffer(
                    cameraBuffer_));

            cameraBuffer_ = {};
        }

        if (vertexBuffer_.IsValid())
        {
            static_cast<void>(
                device.DestroyBuffer(
                    vertexBuffer_));

            vertexBuffer_ = {};
        }
    }

    engine::graphics::GraphicsResult EditorSceneRenderer::Render(
        engine::graphics::CommandContext& context,
        const EditorSceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection) noexcept
    {
        if (!initialized_)
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        std::array<MarkerVertex, MaxMarkerVertices> vertices{};

        const std::size_t vertexCount =
            BuildSceneVertices(
                document,
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

        result = context.SetGraphicsPipeline(
            pipeline_);

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
            static_cast<std::uint32_t>(
                vertexCount),
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