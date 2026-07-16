#include "Editor/EditorStaticMeshRenderer.h"

#include <Assets/MeshAsset.h>

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/Shader.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr const char*
            StaticMeshShaderSource = R"(
cbuffer ObjectBuffer : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 ViewProjection;
    float4 Tint;
};

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPosition =
        mul(float4(input.position, 1.0f), World);

    output.position =
        mul(worldPosition, ViewProjection);

    output.normal =
        normalize(
            mul(
                float4(input.normal, 0.0f),
                World).xyz);

    output.texcoord = input.texcoord;

    return output;
}

float4 PSMain(VertexOutput input) : SV_TARGET
{
    const float3 lightDirection =
        normalize(float3(-0.35f, 0.85f, -0.40f));

    const float diffuse =
        saturate(
            dot(
                normalize(input.normal),
                lightDirection));

    const float lighting =
        0.22f +
        diffuse * 0.78f;

    const float uvVariation =
        0.94f +
        0.06f *
        saturate(
            frac(
                abs(input.texcoord.x) * 4.0f +
                abs(input.texcoord.y) * 4.0f));

    return float4(
        Tint.rgb *
        lighting *
        uvVariation,
        Tint.a);
}
)";

        struct alignas(16)
            ObjectConstants final
        {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4X4 viewProjection;
            DirectX::XMFLOAT4 tint;
        };

        static_assert(
            sizeof(ObjectConstants) % 16U == 0U);

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

            return
                scale *
                rotation *
                translation;
        }

        [[nodiscard]]
        bool CompileShader(
            const char* const entryPoint,
            const char* const target,
            ComPtr<ID3DBlob>& bytecode) noexcept
        {
            bytecode.Reset();

            ComPtr<ID3DBlob> errors;

            constexpr UINT flags =
                D3DCOMPILE_ENABLE_STRICTNESS |
                D3DCOMPILE_WARNINGS_ARE_ERRORS |
                D3DCOMPILE_OPTIMIZATION_LEVEL3;

            const HRESULT result =
                D3DCompile(
                    StaticMeshShaderSource,
                    std::char_traits<char>::length(
                        StaticMeshShaderSource),
                    "EditorStaticMesh.hlsl",
                    nullptr,
                    nullptr,
                    entryPoint,
                    target,
                    flags,
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
                    "LTS.Editor.StaticMesh",
                    static_cast<const char*>(
                        errors->GetBufferPointer()));
            }

            return false;
        }
    }

    class EditorStaticMeshRenderer::Impl final
    {
    public:
        struct GpuMesh final
        {
            engine::graphics::BufferHandle
                vertexBuffer;

            engine::graphics::BufferHandle
                indexBuffer;

            std::uint32_t indexCount = 0U;
        };

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept
        {
            device_ = &device;

            ComPtr<ID3DBlob> vertexBytecode;
            ComPtr<ID3DBlob> pixelBytecode;

            if (
                !CompileShader(
                    "VSMain",
                    "vs_5_0",
                    vertexBytecode) ||
                !CompileShader(
                    "PSMain",
                    "ps_5_0",
                    pixelBytecode))
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
                "EditorStaticMesh.VertexShader";

            auto result =
                device.CreateShader(
                    vertexShaderDescription,
                    vertexShader_);

            if (engine::graphics::Failed(result))
            {
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
                "EditorStaticMesh.PixelShader";

            result =
                device.CreateShader(
                    pixelShaderDescription,
                    pixelShader_);

            if (engine::graphics::Failed(result))
            {
                Shutdown(device);
                return false;
            }

            const std::array<
                engine::graphics::
                    VertexElementDesc,
                3U> elements
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
                    "NORMAL",
                    0U,
                    engine::graphics::
                        Format::R32G32B32Float,
                    0U,
                    12U,
                    engine::graphics::
                        VertexInputRate::PerVertex,
                    0U
                },
                {
                    "TEXCOORD",
                    0U,
                    engine::graphics::
                        Format::R32G32Float,
                    0U,
                    24U,
                    engine::graphics::
                        VertexInputRate::PerVertex,
                    0U
                }
            }};

            engine::graphics::
                InputLayoutDesc
                    inputLayoutDescription;

            inputLayoutDescription.vertexShader =
                vertexShader_;

            inputLayoutDescription.elements =
                elements.data();

            inputLayoutDescription.elementCount =
                elements.size();

            inputLayoutDescription.debugName =
                "EditorStaticMesh.InputLayout";

            result =
                device.CreateInputLayout(
                    inputLayoutDescription,
                    inputLayout_);

            if (engine::graphics::Failed(result))
            {
                Shutdown(device);
                return false;
            }

            engine::graphics::BufferDesc
                constantBufferDescription;

            constantBufferDescription.byteSize =
                sizeof(ObjectConstants);

            constantBufferDescription.stride = 0U;

            constantBufferDescription.usage =
                engine::graphics::
                    ResourceUsage::Default;

            constantBufferDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Constant;

            constantBufferDescription.miscFlags =
                engine::graphics::
                    BufferMiscFlags::None;

            constantBufferDescription.cpuAccess =
                engine::graphics::
                    CpuAccessFlags::None;

            constantBufferDescription.indexFormat =
                engine::graphics::
                    IndexFormat::None;

            result =
                device.CreateBuffer(
                    constantBufferDescription,
                    nullptr,
                    objectBuffer_);

            if (engine::graphics::Failed(result))
            {
                Shutdown(device);
                return false;
            }

            engine::graphics::
                GraphicsPipelineDesc
                    pipelineDescription;

            pipelineDescription.vertexShader =
                vertexShader_;

            pipelineDescription.pixelShader =
                pixelShader_;

            pipelineDescription.inputLayout =
                inputLayout_;

            pipelineDescription.topology =
                engine::graphics::
                    PrimitiveTopology::
                        TriangleList;

            pipelineDescription.rasterizer.fillMode =
                engine::graphics::
                    FillMode::Solid;

            /*
             * Старые SCO/SCB встречаются с разным winding.
             * На первом этапе отключаем culling.
             */
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::
                    CullMode::None;

            pipelineDescription.rasterizer.
                depthClipEnable = true;

            pipelineDescription.blend.
                renderTargets[0].
                    blendEnable = false;

            pipelineDescription.depthStencil.
                depthEnable = true;

            pipelineDescription.depthStencil.
                depthWriteEnable = true;

            pipelineDescription.depthStencil.
                depthFunction =
                    engine::graphics::
                        ComparisonFunction::
                            LessEqual;

            pipelineDescription.debugName =
                "EditorStaticMesh.Pipeline";

            result =
                device.CreateGraphicsPipeline(
                    pipelineDescription,
                    pipeline_);

            if (engine::graphics::Failed(result))
            {
                Shutdown(device);
                return false;
            }

            initialized_ = true;
            return true;
        }

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept
        {
            initialized_ = false;

            for (auto& entry : meshes_)
            {
                DestroyGpuMesh(
                    device,
                    entry.second);
            }

            meshes_.clear();
            failedMeshes_.clear();

            if (pipeline_.IsValid())
            {
                static_cast<void>(
                    device.
                        DestroyGraphicsPipeline(
                            pipeline_));

                pipeline_ = {};
            }

            if (inputLayout_.IsValid())
            {
                static_cast<void>(
                    device.
                        DestroyInputLayout(
                            inputLayout_));

                inputLayout_ = {};
            }

            if (pixelShader_.IsValid())
            {
                static_cast<void>(
                    device.
                        DestroyShader(
                            pixelShader_));

                pixelShader_ = {};
            }

            if (vertexShader_.IsValid())
            {
                static_cast<void>(
                    device.
                        DestroyShader(
                            vertexShader_));

                vertexShader_ = {};
            }

            if (objectBuffer_.IsValid())
            {
                static_cast<void>(
                    device.
                        DestroyBuffer(
                            objectBuffer_));

                objectBuffer_ = {};
            }

            device_ = nullptr;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const EditorSceneDocument& document,
            const DirectX::XMFLOAT4X4&
                viewProjection) noexcept
        {
            if (
                !initialized_ ||
                device_ == nullptr)
            {
                return engine::graphics::
                    GraphicsResult::InvalidState;
            }

            auto result =
                context.SetGraphicsPipeline(
                    pipeline_);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            result =
                context.SetConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    &objectBuffer_,
                    1U);

            if (engine::graphics::Failed(result))
            {
                context.UnbindGraphicsPipeline();
                return result;
            }

            const auto& entities =
                document.GetEntities();

            const std::size_t selectedIndex =
                document.GetSelectedIndex();

            for (
                std::size_t entityIndex = 0U;
                entityIndex < entities.size();
                ++entityIndex)
            {
                const EditorSceneEntity& entity =
                    entities[entityIndex];

                if (
                    !entity.staticMesh.has_value() ||
                    !entity.staticMesh->visible ||
                    entity.staticMesh->
                        assetPath.empty())
                {
                    continue;
                }

                GpuMesh* const mesh =
                    GetOrLoadMesh(
                        entity.staticMesh->
                            assetPath);

                if (mesh == nullptr)
                {
                    continue;
                }

                ObjectConstants constants{};

                DirectX::XMStoreFloat4x4(
                    &constants.world,
                    BuildWorldMatrix(
                        entity.transform));

                constants.viewProjection =
                    viewProjection;

                constants.tint =
                    entityIndex == selectedIndex
                        ? DirectX::XMFLOAT4
                        {
                            1.0F,
                            0.45F,
                            0.10F,
                            1.0F
                        }
                        : DirectX::XMFLOAT4
                        {
                            0.58F,
                            0.63F,
                            0.66F,
                            1.0F
                        };

                result =
                    context.UpdateBuffer(
                        objectBuffer_,
                        &constants,
                        sizeof(constants));

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                engine::graphics::
                    VertexBufferBinding
                        vertexBinding;

                vertexBinding.buffer =
                    mesh->vertexBuffer;

                vertexBinding.stride =
                    static_cast<std::uint32_t>(
                        sizeof(
                            engine::assets::
                                MeshVertex));

                vertexBinding.offset = 0U;

                result =
                    context.SetVertexBuffers(
                        0U,
                        &vertexBinding,
                        1U);

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                engine::graphics::
                    IndexBufferBinding
                        indexBinding;

                indexBinding.buffer =
                    mesh->indexBuffer;

                indexBinding.offset = 0U;

                result =
                    context.SetIndexBuffer(
                        indexBinding);

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                result =
                    context.DrawIndexed(
                        mesh->indexCount,
                        0U,
                        0);

                if (engine::graphics::Failed(result))
                {
                    break;
                }
            }

            context.UnbindIndexBuffer();

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    1U));

            context.UnbindGraphicsPipeline();

            return result;
        }

    private:
        [[nodiscard]]
        GpuMesh* GetOrLoadMesh(
            const std::wstring& assetPath) noexcept
        {
            std::error_code error;

            std::filesystem::path path(
                assetPath);

            if (!path.is_absolute())
            {
                path =
                    std::filesystem::
                        current_path(error) /
                    path;
            }

            if (error)
            {
                return nullptr;
            }

            path =
                path.lexically_normal();

            const std::wstring key =
                path.wstring();

            const auto existing =
                meshes_.find(key);

            if (existing != meshes_.end())
            {
                return &existing->second;
            }

            if (
                failedMeshes_.find(key) !=
                failedMeshes_.end())
            {
                return nullptr;
            }

            GpuMesh mesh;

            if (!LoadGpuMesh(
                    path,
                    mesh))
            {
                failedMeshes_.insert(key);
                return nullptr;
            }

            const auto result =
                meshes_.emplace(
                    key,
                    mesh);

            return &result.first->second;
        }

        [[nodiscard]]
        bool LoadGpuMesh(
            const std::filesystem::path& path,
            GpuMesh& gpuMesh) noexcept
        {
            engine::assets::MeshAsset asset;
            std::wstring error;

            if (
                !engine::assets::
                    MeshAssetIO::Load(
                        path,
                        asset,
                        error))
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.StaticMesh",
                    "Failed to load a mesh asset.");

                return false;
            }

            engine::graphics::BufferDesc
                vertexDescription;

            vertexDescription.byteSize =
                asset.vertices.size() *
                sizeof(
                    engine::assets::
                        MeshVertex);

            vertexDescription.stride =
                static_cast<std::uint32_t>(
                    sizeof(
                        engine::assets::
                            MeshVertex));

            vertexDescription.usage =
                engine::graphics::
                    ResourceUsage::Immutable;

            vertexDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Vertex;

            vertexDescription.miscFlags =
                engine::graphics::
                    BufferMiscFlags::None;

            vertexDescription.cpuAccess =
                engine::graphics::
                    CpuAccessFlags::None;

            vertexDescription.indexFormat =
                engine::graphics::
                    IndexFormat::None;

            engine::graphics::
                BufferInitialData
                    vertexInitialData;

            vertexInitialData.data =
                reinterpret_cast<
                    const std::byte*>(
                        asset.vertices.data());

            vertexInitialData.dataSize =
                vertexDescription.byteSize;

            auto result =
                device_->CreateBuffer(
                    vertexDescription,
                    &vertexInitialData,
                    gpuMesh.vertexBuffer);

            if (engine::graphics::Failed(result))
            {
                return false;
            }

            engine::graphics::BufferDesc
                indexDescription;

            indexDescription.byteSize =
                asset.indices.size() *
                sizeof(std::uint32_t);

            indexDescription.stride =
                sizeof(std::uint32_t);

            indexDescription.usage =
                engine::graphics::
                    ResourceUsage::Immutable;

            indexDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Index;

            indexDescription.miscFlags =
                engine::graphics::
                    BufferMiscFlags::None;

            indexDescription.cpuAccess =
                engine::graphics::
                    CpuAccessFlags::None;

            indexDescription.indexFormat =
                engine::graphics::
                    IndexFormat::UInt32;

            engine::graphics::
                BufferInitialData
                    indexInitialData;

            indexInitialData.data =
                reinterpret_cast<
                    const std::byte*>(
                        asset.indices.data());

            indexInitialData.dataSize =
                indexDescription.byteSize;

            result =
                device_->CreateBuffer(
                    indexDescription,
                    &indexInitialData,
                    gpuMesh.indexBuffer);

            if (engine::graphics::Failed(result))
            {
                static_cast<void>(
                    device_->DestroyBuffer(
                        gpuMesh.vertexBuffer));

                gpuMesh.vertexBuffer = {};
                return false;
            }

            gpuMesh.indexCount =
                static_cast<std::uint32_t>(
                    asset.indices.size());

            return true;
        }

        static void DestroyGpuMesh(
            engine::graphics::RenderDevice& device,
            GpuMesh& mesh) noexcept
        {
            if (mesh.indexBuffer.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        mesh.indexBuffer));

                mesh.indexBuffer = {};
            }

            if (mesh.vertexBuffer.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        mesh.vertexBuffer));

                mesh.vertexBuffer = {};
            }

            mesh.indexCount = 0U;
        }

        engine::graphics::RenderDevice*
            device_ = nullptr;

        engine::graphics::BufferHandle
            objectBuffer_;

        engine::graphics::ShaderHandle
            vertexShader_;

        engine::graphics::ShaderHandle
            pixelShader_;

        engine::graphics::InputLayoutHandle
            inputLayout_;

        engine::graphics::PipelineStateHandle
            pipeline_;

        std::unordered_map<
            std::wstring,
            GpuMesh> meshes_;

        std::unordered_set<
            std::wstring> failedMeshes_;

        bool initialized_ = false;
    };

    EditorStaticMeshRenderer::
        EditorStaticMeshRenderer() noexcept =
            default;

    EditorStaticMeshRenderer::
        ~EditorStaticMeshRenderer() noexcept =
            default;

    bool EditorStaticMeshRenderer::Initialize(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ != nullptr)
        {
            return true;
        }

        try
        {
            impl_ =
                std::make_unique<Impl>();
        }
        catch (...)
        {
            return false;
        }

        if (!impl_->Initialize(device))
        {
            impl_.reset();
            return false;
        }

        return true;
    }

    void EditorStaticMeshRenderer::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ == nullptr)
        {
            return;
        }

        impl_->Shutdown(device);
        impl_.reset();
    }

    engine::graphics::GraphicsResult
        EditorStaticMeshRenderer::Render(
            engine::graphics::CommandContext& context,
            const EditorSceneDocument& document,
            const DirectX::XMFLOAT4X4&
                viewProjection) noexcept
    {
        if (impl_ == nullptr)
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        return impl_->Render(
            context,
            document,
            viewProjection);
    }
}