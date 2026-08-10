#include "Editor/LevelEditor/Rendering/SkyRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Shader.h>

#include <DirectXMath.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <string>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr wchar_t SkyShaderFile[] = L"Sky.hlsl";

        struct SkyVertex final
        {
            float position[2];
        };

        struct alignas(16) SkyConstants final
        {
            DirectX::XMFLOAT4X4 inverseViewProjection{};

            DirectX::XMFLOAT4 cameraPosition{};
            DirectX::XMFLOAT4 topColorIntensity{};
            DirectX::XMFLOAT4 horizonColorExponent{};
            DirectX::XMFLOAT4 groundColor{};
            DirectX::XMFLOAT4 sunDirectionSize{};
            DirectX::XMFLOAT4 sunColorIntensity{};
            DirectX::XMFLOAT4 fogColorEnabled{};
            DirectX::XMFLOAT4 cloudColorCoverage{};
            DirectX::XMFLOAT4 cloudParameters{};
            DirectX::XMFLOAT4 cloudMotion{};
        };

        static_assert(sizeof(SkyConstants) % 16U == 0U);

        struct ResolvedSun final
        {
            DirectX::XMFLOAT3 direction
            {
                -0.35F,
                0.85F,
                -0.40F
            };

            DirectX::XMFLOAT3 color
            {
                1.0F,
                0.94F,
                0.82F
            };

            float intensity = 1.0F;
        };

        [[nodiscard]]
        ResolvedSun ResolveSun(
            const SceneDocument& document) noexcept
        {
            ResolvedSun result;

            for (const EditorSceneEntity& entity :
                 document.GetEntities())
            {
                if (!entity.directionalLight.has_value())
                {
                    continue;
                }

                const auto& light = *entity.directionalLight;

                const float pitch =
                    DirectX::XMConvertToRadians(
                        entity.transform.rotationDegrees[0]);

                const float yaw =
                    DirectX::XMConvertToRadians(
                        entity.transform.rotationDegrees[1]);

                const float cosinePitch = std::cos(pitch);

                DirectX::XMFLOAT3 direction
                {
                    -cosinePitch * std::sin(yaw),
                    -std::sin(pitch),
                    -cosinePitch * std::cos(yaw)
                };

                DirectX::XMStoreFloat3(
                    &result.direction,
                    DirectX::XMVector3Normalize(
                        DirectX::XMLoadFloat3(&direction)));

                result.color =
                {
                    (std::max)(light.color[0], 0.0F),
                    (std::max)(light.color[1], 0.0F),
                    (std::max)(light.color[2], 0.0F)
                };

                result.intensity =
                    (std::max)(light.intensity, 0.0F) *
                    0.25F;

                break;
            }

            return result;
        }

        [[nodiscard]]
        bool CompileSkyShader(
            const char* entryPoint,
            const char* target,
            ComPtr<ID3DBlob>& bytecode) noexcept
        {
            return CompileEditorShaderFile(
                SkyShaderFile,
                entryPoint,
                target,
                "LTS.Editor.Sky",
                bytecode);
        }

        void LogGraphicsFailure(
            const char* operation,
            const engine::graphics::GraphicsResult result)
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Sky graphics operation";

            message += " failed: ";
            message += engine::graphics::ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.Sky",
                message);
        }
    }

    bool SkyRenderer::Initialize(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (initialized_)
        {
            return true;
        }

        ComPtr<ID3DBlob> vertexBytecode;
        ComPtr<ID3DBlob> pixelBytecode;

        if (!CompileSkyShader(
                "VSMain",
                "vs_5_0",
                vertexBytecode) ||
            !CompileSkyShader(
                "PSMain",
                "ps_5_0",
                pixelBytecode))
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
            "EditorSky.VertexShader";

        auto result = device.CreateShader(
            vertexShaderDescription,
            vertexShader_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create sky vertex shader",
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
            "EditorSky.PixelShader";

        result = device.CreateShader(
            pixelShaderDescription,
            pixelShader_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create sky pixel shader",
                result);

            Shutdown(device);
            return false;
        }

        const std::array<
            engine::graphics::VertexElementDesc,
            1U> elements
        {{
            {
                "POSITION",
                0U,
                engine::graphics::Format::R32G32Float,
                0U,
                0U,
                engine::graphics::VertexInputRate::PerVertex,
                0U
            }
        }};

        engine::graphics::InputLayoutDesc inputLayoutDescription;
        inputLayoutDescription.vertexShader = vertexShader_;
        inputLayoutDescription.elements = elements.data();
        inputLayoutDescription.elementCount = elements.size();
        inputLayoutDescription.debugName =
            "EditorSky.InputLayout";

        result = device.CreateInputLayout(
            inputLayoutDescription,
            inputLayout_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create sky input layout",
                result);

            Shutdown(device);
            return false;
        }

        constexpr std::array<SkyVertex, 6U> vertices
        {{
            {{-1.0F, -1.0F}},
            {{-1.0F,  1.0F}},
            {{ 1.0F,  1.0F}},

            {{-1.0F, -1.0F}},
            {{ 1.0F,  1.0F}},
            {{ 1.0F, -1.0F}}
        }};

        engine::graphics::BufferDesc vertexBufferDescription;
        vertexBufferDescription.byteSize =
            sizeof(vertices);
        vertexBufferDescription.stride =
            sizeof(SkyVertex);
        vertexBufferDescription.usage =
            engine::graphics::ResourceUsage::Immutable;
        vertexBufferDescription.bindFlags =
            engine::graphics::BufferBindFlags::Vertex;
        vertexBufferDescription.miscFlags =
            engine::graphics::BufferMiscFlags::None;
        vertexBufferDescription.cpuAccess =
            engine::graphics::CpuAccessFlags::None;
        vertexBufferDescription.indexFormat =
            engine::graphics::IndexFormat::None;

        engine::graphics::BufferInitialData vertexData;
        vertexData.data =
            reinterpret_cast<const std::byte*>(
                vertices.data());
        vertexData.dataSize = sizeof(vertices);

        result = device.CreateBuffer(
            vertexBufferDescription,
            &vertexData,
            vertexBuffer_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create sky vertex buffer",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::BufferDesc constantBufferDescription;
        constantBufferDescription.byteSize =
            sizeof(SkyConstants);
        constantBufferDescription.stride = 0U;
        constantBufferDescription.usage =
            engine::graphics::ResourceUsage::Default;
        constantBufferDescription.bindFlags =
            engine::graphics::BufferBindFlags::Constant;
        constantBufferDescription.miscFlags =
            engine::graphics::BufferMiscFlags::None;
        constantBufferDescription.cpuAccess =
            engine::graphics::CpuAccessFlags::None;
        constantBufferDescription.indexFormat =
            engine::graphics::IndexFormat::None;

        result = device.CreateBuffer(
            constantBufferDescription,
            nullptr,
            constantBuffer_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create sky constant buffer",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::GraphicsPipelineDesc pipelineDescription;
        pipelineDescription.vertexShader = vertexShader_;
        pipelineDescription.pixelShader = pixelShader_;
        pipelineDescription.inputLayout = inputLayout_;
        pipelineDescription.topology =
            engine::graphics::PrimitiveTopology::TriangleList;

        pipelineDescription.rasterizer.fillMode =
            engine::graphics::FillMode::Solid;
        pipelineDescription.rasterizer.cullMode =
            engine::graphics::CullMode::None;
        pipelineDescription.rasterizer.depthClipEnable = true;

        pipelineDescription.blend.renderTargets[0].
            blendEnable = false;

        pipelineDescription.depthStencil.depthEnable = false;
        pipelineDescription.depthStencil.depthWriteEnable = false;
        pipelineDescription.depthStencil.depthFunction =
            engine::graphics::ComparisonFunction::Always;

        pipelineDescription.debugName =
            "EditorSky.Pipeline";

        result = device.CreateGraphicsPipeline(
            pipelineDescription,
            pipeline_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create sky pipeline",
                result);

            Shutdown(device);
            return false;
        }

        vertexCount_ =
            static_cast<std::uint32_t>(
                vertices.size());

        initialized_ = true;

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.Sky",
            "Editor sky renderer initialized.");

        return true;
    }

    void SkyRenderer::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        initialized_ = false;
        vertexCount_ = 0U;

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

        if (constantBuffer_.IsValid())
        {
            static_cast<void>(
                device.DestroyBuffer(constantBuffer_));
            constantBuffer_ = {};
        }

        if (vertexBuffer_.IsValid())
        {
            static_cast<void>(
                device.DestroyBuffer(vertexBuffer_));
            vertexBuffer_ = {};
        }
    }

    engine::graphics::GraphicsResult SkyRenderer::Render(
        engine::graphics::CommandContext& context,
        const SceneDocument& document,
        const DirectX::XMFLOAT4X4& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition) noexcept
    {
        if (!initialized_)
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        const engine::scene::EnvironmentComponent*
            environment = nullptr;

        for (const EditorSceneEntity& entity :
             document.GetEntities())
        {
            if (entity.environment.has_value() &&
                entity.environment->visible)
            {
                environment =
                    &*entity.environment;

                break;
            }
        }

        if (environment == nullptr)
        {
            return engine::graphics::
                GraphicsResult::Success;
        }

        const DirectX::XMMATRIX viewProjectionMatrix =
            DirectX::XMLoadFloat4x4(&viewProjection);

        DirectX::XMVECTOR determinant;

        const DirectX::XMMATRIX inverseViewProjection =
            DirectX::XMMatrixInverse(
                &determinant,
                viewProjectionMatrix);

        const float determinantValue =
            DirectX::XMVectorGetX(determinant);

        if (!std::isfinite(determinantValue) ||
            std::abs(determinantValue) < 0.000001F)
        {
            return engine::graphics::
                GraphicsResult::InvalidArgument;
        }

        const ResolvedSun sun = ResolveSun(document);

        SkyConstants constants;

        DirectX::XMStoreFloat4x4(
            &constants.inverseViewProjection,
            inverseViewProjection);

        constants.cameraPosition =
        {
            cameraPosition.x,
            cameraPosition.y,
            cameraPosition.z,
            1.0F
        };

        constants.topColorIntensity =
        {
            environment->topColor[0],
            environment->topColor[1],
            environment->topColor[2],
            (std::max)(
                environment->skyIntensity,
                0.0F)
        };

        constants.horizonColorExponent =
        {
            environment->horizonColor[0],
            environment->horizonColor[1],
            environment->horizonColor[2],
            std::clamp(
                environment->horizonExponent,
                0.05F,
                8.0F)
        };

        constants.groundColor =
        {
            environment->groundColor[0],
            environment->groundColor[1],
            environment->groundColor[2],
            1.0F
        };

        constants.sunDirectionSize =
        {
            sun.direction.x,
            sun.direction.y,
            sun.direction.z,
            std::clamp(
                environment->sunDiskSizeDegrees,
                0.01F,
                20.0F)
        };

        constants.sunColorIntensity =
        {
            sun.color.x,
            sun.color.y,
            sun.color.z,
            environment->sunEnabled
                ? sun.intensity
                : 0.0F
        };

        constants.fogColorEnabled =
        {
            environment->fogColor[0],
            environment->fogColor[1],
            environment->fogColor[2],
            environment->fogEnabled ? 1.0F : 0.0F
        };

        constants.cloudColorCoverage =
        {
            environment->cloudColor[0],
            environment->cloudColor[1],
            environment->cloudColor[2],
            std::clamp(environment->cloudCoverage, 0.0F, 1.0F)
        };

        constants.cloudParameters =
        {
            std::clamp(environment->cloudDensity, 0.0F, 1.0F),
            (std::max)(environment->cloudScale, 0.000001F),
            (std::max)(environment->cloudHeight, 1.0F),
            environment->cloudPlaneEnabled ? 1.0F : 0.0F
        };

        const auto now = std::chrono::steady_clock::now().time_since_epoch();
        const float elapsedSeconds = std::chrono::duration<float>(now).count();
        constants.cloudMotion =
        {
            environment->cloudSpeed[0],
            environment->cloudSpeed[1],
            elapsedSeconds,
            environment->timeOfDay
        };

        auto result = context.UpdateBuffer(
            constantBuffer_,
            &constants,
            sizeof(constants));

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        result = context.SetGraphicsPipeline(pipeline_);

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        engine::graphics::VertexBufferBinding binding;
        binding.buffer = vertexBuffer_;
        binding.stride = sizeof(SkyVertex);
        binding.offset = 0U;

        result = context.SetVertexBuffers(
            0U,
            &binding,
            1U);

        if (!engine::graphics::Failed(result))
        {
            result = context.SetConstantBuffers(
                engine::graphics::ShaderStage::Pixel,
                0U,
                &constantBuffer_,
                1U);
        }

        if (!engine::graphics::Failed(result))
        {
            result = context.Draw(
                vertexCount_,
                0U);
        }

        static_cast<void>(
            context.UnbindConstantBuffers(
                engine::graphics::ShaderStage::Pixel,
                0U,
                1U));

        context.UnbindGraphicsPipeline();
        return result;
    }

    bool SkyRenderer::IsInitialized() const noexcept
    {
        return initialized_;
    }
}
