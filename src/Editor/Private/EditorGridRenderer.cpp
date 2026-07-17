#include "Editor/EditorGridRenderer.h"
#include "Editor/EditorShaderCompiler.h"

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

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr int GridExtent = 50;
        constexpr int MajorGridInterval = 10;

        constexpr wchar_t GridShaderFile[] = L"Grid.hlsl";

        struct GridVertex final
        {
            float position[3];
            float color[4];
        };

        struct alignas(16) GridCameraConstants final
        {
            DirectX::XMFLOAT4X4 viewProjection;
        };

        static_assert(
            sizeof(GridCameraConstants) == 64U);

        void AddLine(
            std::vector<GridVertex>& vertices,
            const float startX,
            const float startY,
            const float startZ,
            const float endX,
            const float endY,
            const float endZ,
            const std::array<float, 4U>& color)
        {
            GridVertex startVertex{};

            startVertex.position[0] = startX;
            startVertex.position[1] = startY;
            startVertex.position[2] = startZ;

            startVertex.color[0] = color[0];
            startVertex.color[1] = color[1];
            startVertex.color[2] = color[2];
            startVertex.color[3] = color[3];

            GridVertex endVertex{};

            endVertex.position[0] = endX;
            endVertex.position[1] = endY;
            endVertex.position[2] = endZ;

            endVertex.color[0] = color[0];
            endVertex.color[1] = color[1];
            endVertex.color[2] = color[2];
            endVertex.color[3] = color[3];

            vertices.push_back(startVertex);
            vertices.push_back(endVertex);
        }

        [[nodiscard]]
        std::vector<GridVertex> BuildGridVertices()
        {
            constexpr std::array<float, 4U> minorColor
            {
                0.16F,
                0.18F,
                0.20F,
                1.0F
            };

            constexpr std::array<float, 4U> majorColor
            {
                0.30F,
                0.33F,
                0.35F,
                1.0F
            };

            constexpr std::array<float, 4U> xAxisColor
            {
                0.82F,
                0.18F,
                0.12F,
                1.0F
            };

            constexpr std::array<float, 4U> zAxisColor
            {
                0.12F,
                0.38F,
                0.82F,
                1.0F
            };

            std::vector<GridVertex> vertices;

            const std::size_t lineCount =
                static_cast<std::size_t>(
                    GridExtent * 2 + 1) *
                2U;

            vertices.reserve(lineCount * 2U);

            const float extent =
                static_cast<float>(
                    GridExtent);

            for (
                int coordinate = -GridExtent;
                coordinate <= GridExtent;
                ++coordinate
            )
            {
                const float position =
                    static_cast<float>(
                        coordinate);

                const bool isMajor =
                    coordinate %
                    MajorGridInterval == 0;

                const std::array<float, 4U>&
                    normalColor =
                        isMajor
                            ? majorColor
                            : minorColor;

                const std::array<float, 4U>&
                    parallelToZColor =
                        coordinate == 0
                            ? zAxisColor
                            : normalColor;

                AddLine(
                    vertices,
                    position,
                    0.0F,
                    -extent,
                    position,
                    0.0F,
                    extent,
                    parallelToZColor);

                const std::array<float, 4U>&
                    parallelToXColor =
                        coordinate == 0
                            ? xAxisColor
                            : normalColor;

                AddLine(
                    vertices,
                    -extent,
                    0.0F,
                    position,
                    extent,
                    0.0F,
                    position,
                    parallelToXColor);
            }

            return vertices;
        }

        bool CompileShader(
        const char* const entryPoint,
        const char* const target,
        ComPtr<ID3DBlob>& bytecode) noexcept
        {
            return CompileEditorShaderFile(
                GridShaderFile,
                entryPoint,
                target,
                "LTS.Editor.Grid",
                bytecode);
        }

        void LogGraphicsFailure(
            const char* operation,
            const engine::graphics::
                GraphicsResult result) noexcept
        {
            std::string message;

            message.reserve(192U);

            message +=
                operation != nullptr
                    ? operation
                    : "Unknown grid operation";

            message += " failed: ";

            message +=
                engine::graphics::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.Grid",
                message);
        }
    }

    bool EditorGridRenderer::Initialize(
        engine::graphics::
            RenderDevice& device) noexcept
    {
        if (initialized_)
        {
            return true;
        }

        const std::vector<GridVertex> vertices =
            BuildGridVertices();

        if (vertices.empty())
        {
            return false;
        }

        ComPtr<ID3DBlob> vertexBytecode;
        ComPtr<ID3DBlob> pixelBytecode;

        if (
            !CompileShader(
                "VSMain",
                "vs_5_0",
                vertexBytecode)
        )
        {
            return false;
        }

        if (
            !CompileShader(
                "PSMain",
                "ps_5_0",
                pixelBytecode)
        )
        {
            return false;
        }

        engine::graphics::ShaderDesc
            vertexShaderDescription;

        vertexShaderDescription.stage =
            engine::graphics::
                ShaderStage::Vertex;

        vertexShaderDescription.bytecode.data =
            vertexBytecode->
                GetBufferPointer();

        vertexShaderDescription.bytecode.size =
            vertexBytecode->
                GetBufferSize();

        vertexShaderDescription.debugName =
            "EditorGrid.VertexShader";

        auto result =
            device.CreateShader(
                vertexShaderDescription,
                vertexShader_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create grid vertex shader",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::ShaderDesc
            pixelShaderDescription;

        pixelShaderDescription.stage =
            engine::graphics::
                ShaderStage::Pixel;

        pixelShaderDescription.bytecode.data =
            pixelBytecode->
                GetBufferPointer();

        pixelShaderDescription.bytecode.size =
            pixelBytecode->
                GetBufferSize();

        pixelShaderDescription.debugName =
            "EditorGrid.PixelShader";

        result =
            device.CreateShader(
                pixelShaderDescription,
                pixelShader_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create grid pixel shader",
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
                engine::graphics::
                    Format::R32G32B32Float,
                0U,
                0U,
                engine::graphics::
                    VertexInputRate::PerVertex,
                0U
            },
            {
                "COLOR",
                0U,
                engine::graphics::
                    Format::R32G32B32A32Float,
                0U,
                12U,
                engine::graphics::
                    VertexInputRate::PerVertex,
                0U
            }
        }};

        engine::graphics::InputLayoutDesc
            inputLayoutDescription;

        inputLayoutDescription.vertexShader =
            vertexShader_;

        inputLayoutDescription.elements =
            elements.data();

        inputLayoutDescription.elementCount =
            elements.size();

        inputLayoutDescription.debugName =
            "EditorGrid.InputLayout";

        result =
            device.CreateInputLayout(
                inputLayoutDescription,
                inputLayout_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create grid input layout",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::BufferDesc
            vertexBufferDescription;

        vertexBufferDescription.byteSize =
            vertices.size() *
            sizeof(GridVertex);

        vertexBufferDescription.stride =
            static_cast<std::uint32_t>(
                sizeof(GridVertex));

        vertexBufferDescription.usage =
            engine::graphics::
                ResourceUsage::Immutable;

        vertexBufferDescription.bindFlags =
            engine::graphics::
                BufferBindFlags::Vertex;

        vertexBufferDescription.miscFlags =
            engine::graphics::
                BufferMiscFlags::None;

        vertexBufferDescription.cpuAccess =
            engine::graphics::
                CpuAccessFlags::None;

        vertexBufferDescription.indexFormat =
            engine::graphics::
                IndexFormat::None;

        engine::graphics::BufferInitialData
            vertexInitialData;

        vertexInitialData.data =
            reinterpret_cast<const std::byte*>(
                vertices.data());

        vertexInitialData.dataSize =
            vertexBufferDescription.byteSize;

        result =
            device.CreateBuffer(
                vertexBufferDescription,
                &vertexInitialData,
                vertexBuffer_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create grid vertex buffer",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::BufferDesc
            cameraBufferDescription;

        cameraBufferDescription.byteSize =
            sizeof(GridCameraConstants);

        cameraBufferDescription.stride = 0U;

        cameraBufferDescription.usage =
            engine::graphics::
                ResourceUsage::Default;

        cameraBufferDescription.bindFlags =
            engine::graphics::
                BufferBindFlags::Constant;

        cameraBufferDescription.miscFlags =
            engine::graphics::
                BufferMiscFlags::None;

        cameraBufferDescription.cpuAccess =
            engine::graphics::
                CpuAccessFlags::None;

        cameraBufferDescription.indexFormat =
            engine::graphics::
                IndexFormat::None;

        result =
            device.CreateBuffer(
                cameraBufferDescription,
                nullptr,
                cameraBuffer_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create grid camera buffer",
                result);

            Shutdown(device);
            return false;
        }

        engine::graphics::GraphicsPipelineDesc
            pipelineDescription;

        pipelineDescription.vertexShader =
            vertexShader_;

        pipelineDescription.pixelShader =
            pixelShader_;

        pipelineDescription.inputLayout =
            inputLayout_;

        pipelineDescription.topology =
            engine::graphics::
                PrimitiveTopology::LineList;

        pipelineDescription.rasterizer.fillMode =
            engine::graphics::
                FillMode::Solid;

        pipelineDescription.rasterizer.cullMode =
            engine::graphics::
                CullMode::None;

        pipelineDescription.rasterizer.depthClipEnable =
            true;

        pipelineDescription.blend.
            renderTargets[0].blendEnable =
                false;

        pipelineDescription.depthStencil.depthEnable =
            true;

        pipelineDescription.depthStencil.
            depthWriteEnable =
                true;

        pipelineDescription.depthStencil.depthFunction =
            engine::graphics::
                ComparisonFunction::LessEqual;

        pipelineDescription.debugName =
            "EditorGrid.Pipeline";

        result =
            device.CreateGraphicsPipeline(
                pipelineDescription,
                pipeline_);

        if (engine::graphics::Failed(result))
        {
            LogGraphicsFailure(
                "Create grid pipeline",
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
            "LTS.Editor.Grid",
            "Editor world grid initialized.");

        return true;
    }

    void EditorGridRenderer::Shutdown(
        engine::graphics::
            RenderDevice& device) noexcept
    {
        initialized_ = false;
        vertexCount_ = 0U;

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

    engine::graphics::GraphicsResult
    EditorGridRenderer::Render(
        engine::graphics::
            CommandContext& context,
        const DirectX::XMFLOAT4X4&
            viewProjection) noexcept
    {
        if (!initialized_)
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        GridCameraConstants cameraConstants{};

        cameraConstants.viewProjection =
            viewProjection;

        auto result =
            context.UpdateBuffer(
                cameraBuffer_,
                &cameraConstants,
                sizeof(cameraConstants));

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        result =
            context.SetGraphicsPipeline(
                pipeline_);

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        engine::graphics::VertexBufferBinding
            vertexBinding;

        vertexBinding.buffer =
            vertexBuffer_;

        vertexBinding.stride =
            static_cast<std::uint32_t>(
                sizeof(GridVertex));

        vertexBinding.offset = 0U;

        result =
            context.SetVertexBuffers(
                0U,
                &vertexBinding,
                1U);

        if (engine::graphics::Failed(result))
        {
            context.UnbindGraphicsPipeline();
            return result;
        }

        result =
            context.SetConstantBuffers(
                engine::graphics::
                    ShaderStage::Vertex,
                0U,
                &cameraBuffer_,
                1U);

        if (engine::graphics::Failed(result))
        {
            context.UnbindGraphicsPipeline();
            return result;
        }

        result =
            context.Draw(
                vertexCount_,
                0U);

        static_cast<void>(
            context.UnbindConstantBuffers(
                engine::graphics::
                    ShaderStage::Vertex,
                0U,
                1U));

        context.UnbindGraphicsPipeline();

        return result;
    }

    bool EditorGridRenderer::
        IsInitialized() const noexcept
    {
        return initialized_;
    }
}