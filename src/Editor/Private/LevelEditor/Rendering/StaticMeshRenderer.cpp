#include "Editor/LevelEditor/Rendering/StaticMeshRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/AssetType.h>
#include <Assets/GpuMesh.h>
#include <Assets/MaterialAssetLoader.h>
#include <Assets/MeshAsset.h>
#include <Assets/MeshAssetLoader.h>

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>

#include <DirectXMath.h>
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uintmax_t MaximumMeshFileSize =
            512U * 1024U * 1024U;

        struct alignas(16) ObjectConstants final
        {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4X4 viewProjection;

            DirectX::XMFLOAT4 baseColor;
            DirectX::XMFLOAT4 materialParameters;

            // xyz = направление от поверхности к солнцу.
            // w = нормализованная интенсивность.
            DirectX::XMFLOAT4 sunDirectionIntensity;

            DirectX::XMFLOAT4 sunColor;
            DirectX::XMFLOAT4 ambientColor;
        };

        static_assert(
            sizeof(ObjectConstants) % 16U == 0U);

        [[nodiscard]] bool DecodeWicRgba(
            const std::filesystem::path& path,
            std::vector<std::byte>& pixels,
            std::uint32_t& width,
            std::uint32_t& height) noexcept
        {
            pixels.clear(); width = 0U; height = 0U;
            try
            {
                Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
                HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                    CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
                if (FAILED(hr)) return false;
                Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
                hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr,
                    GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
                if (FAILED(hr)) return false;
                Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
                hr = decoder->GetFrame(0U, frame.GetAddressOf());
                if (FAILED(hr) || FAILED(frame->GetSize(&width, &height)) ||
                    width == 0U || height == 0U || width > 16384U || height > 16384U)
                    return false;
                Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
                hr = factory->CreateFormatConverter(converter.GetAddressOf());
                if (FAILED(hr)) return false;
                hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                    WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
                if (FAILED(hr)) return false;
                const std::size_t rowPitch = static_cast<std::size_t>(width) * 4U;
                const std::size_t byteCount = rowPitch * static_cast<std::size_t>(height);
                if (byteCount > static_cast<std::size_t>((std::numeric_limits<UINT>::max)()))
                    return false;
                pixels.resize(byteCount);
                hr = converter->CopyPixels(nullptr, static_cast<UINT>(rowPitch),
                    static_cast<UINT>(byteCount), reinterpret_cast<BYTE*>(pixels.data()));
                return SUCCEEDED(hr);
            }
            catch (...) { pixels.clear(); width = 0U; height = 0U; return false; }
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
                        transform.
                            rotationDegrees[0]),
                    DirectX::XMConvertToRadians(
                        transform.
                            rotationDegrees[1]),
                    DirectX::XMConvertToRadians(
                        transform.
                            rotationDegrees[2]));

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

        struct ResolvedDirectionalLight final
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
                1.0F,
                1.0F
            };

            float intensity = 1.0F;

            DirectX::XMFLOAT3 ambientColor
            {
                0.28F,
                0.31F,
                0.36F
            };

            float ambientIntensity = 1.0F;
        };

        [[nodiscard]]
        ResolvedDirectionalLight ResolveDirectionalLight(
            const SceneDocument& document) noexcept
        {
            ResolvedDirectionalLight result;

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (!entity.environment.has_value() ||
                    !entity.environment->visible)
                {
                    continue;
                }

                const auto& environment =
                    *entity.environment;

                result.ambientColor =
                {
                    (std::max)(
                        environment.ambientColor[0],
                        0.0F),

                    (std::max)(
                        environment.ambientColor[1],
                        0.0F),

                    (std::max)(
                        environment.ambientColor[2],
                        0.0F)
                };

                result.ambientIntensity =
                    (std::max)(
                        environment.ambientIntensity,
                        0.0F);

                break;
            }

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (!entity.directionalLight.has_value())
                {
                    continue;
                }

                const auto& light = *entity.directionalLight;

                const float pitch = DirectX::XMConvertToRadians(
                    entity.transform.rotationDegrees[0]);

                const float yaw = DirectX::XMConvertToRadians(
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

                /*
                 * Старое значение по умолчанию равно 4.
                 * Для shader нормализуем его к 1.
                 */
                result.intensity =
                    (std::max)(light.intensity, 0.0F) *
                    0.25F;

                break;
            }

            return result;
        }

        [[nodiscard]]
        std::wstring LowercasePath(
            std::wstring value)
        {
            for (wchar_t& character : value)
            {
                character =
                    static_cast<wchar_t>(
                        std::towlower(
                            character));
            }

            return value;
        }

        void LogGraphicsFailure(
            const char* const operation,
            const engine::graphics::
                GraphicsResult result) noexcept
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Static mesh graphics operation";

            message += " failed: ";

            message +=
                engine::graphics::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.StaticMesh",
                message);
        }

        void LogAssetFailure(
            const std::filesystem::path& path,
            const char* const operation,
            const engine::assets::
                AssetResult result)
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Static mesh asset operation";

            message += " failed for '";
            message += path.generic_u8string();
            message += "': ";

            message +=
                engine::assets::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.StaticMesh",
                message);
        }

        [[nodiscard]]
        engine::assets::AssetResult ReadAssetData(
            const std::filesystem::path& path,
            engine::assets::AssetData& output) noexcept
        {
            output.Clear();

            try
            {
                std::error_code filesystemError;

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        path,
                        filesystemError);

                if (filesystemError)
                {
                    return engine::assets::
                        AssetResult::IoError;
                }

                if (
                    fileSize == 0U ||
                    fileSize >
                        MaximumMeshFileSize ||
                    fileSize >
                        static_cast<std::uintmax_t>(
                            std::numeric_limits<
                                std::streamsize>::
                                    max()))
                {
                    return engine::assets::
                        AssetResult::FileTooLarge;
                }

                const engine::assets::AssetResult
                    resizeResult =
                        output.Resize(
                            static_cast<std::size_t>(
                                fileSize));

                if (
                    engine::assets::Failed(
                        resizeResult))
                {
                    return resizeResult;
                }

                std::ifstream input(
                    path,
                    std::ios::binary);

                if (!input)
                {
                    output.Clear();

                    return engine::assets::
                        AssetResult::IoError;
                }

                input.read(
                    reinterpret_cast<char*>(
                        output.GetData()),
                    static_cast<std::streamsize>(
                        fileSize));

                if (!input)
                {
                    output.Clear();

                    return engine::assets::
                        AssetResult::IoError;
                }

                return engine::assets::
                    AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                output.Clear();

                return engine::assets::
                    AssetResult::OutOfMemory;
            }
            catch (...)
            {
                output.Clear();

                return engine::assets::
                    AssetResult::InternalError;
            }
        }

        [[nodiscard]]
        engine::assets::AssetResult
            CreateMeshMetadata(
                const std::filesystem::path& requestedPath,
                const std::size_t sourceSize,
                engine::assets::AssetMetadata& metadata) noexcept
        {
            try
            {
                std::filesystem::path logicalPath =
                    requestedPath;

                if (logicalPath.is_absolute())
                {
                    std::error_code currentPathError;

                    const std::filesystem::path gameRoot =
                        std::filesystem::current_path(
                            currentPathError);

                    if (!currentPathError)
                    {
                        std::error_code relativeError;

                        const std::filesystem::path relative =
                            std::filesystem::relative(
                                logicalPath,
                                gameRoot,
                                relativeError);

                        if (
                            !relativeError &&
                            !relative.empty())
                        {
                            logicalPath = relative;
                        }
                    }

                    if (logicalPath.is_absolute())
                    {
                        logicalPath =
                            logicalPath.filename();
                    }
                }

                const std::string logicalName =
                    logicalPath.
                        lexically_normal().
                        generic_u8string();

                engine::assets::AssetPath assetPath;

                const engine::assets::AssetResult
                    pathResult =
                        engine::assets::
                            AssetPath::TryCreate(
                                logicalName,
                                assetPath);

                if (
                    engine::assets::Failed(
                        pathResult))
                {
                    return pathResult;
                }

                metadata = {};

                metadata.path =
                    std::move(assetPath);

                metadata.id =
                    metadata.path.GetId();

                metadata.type =
                    engine::assets::
                        AssetType::Mesh;

                metadata.schemaVersion = 1U;

                metadata.sourceSize =
                    static_cast<std::uint64_t>(
                        sourceSize);

                return engine::assets::
                    AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                return engine::assets::
                    AssetResult::OutOfMemory;
            }
            catch (...)
            {
                return engine::assets::
                    AssetResult::InternalError;
            }
        }
    }

    class StaticMeshRenderer::Impl final
    {
        struct CachedMaterial final
        {
            engine::assets::MaterialAssetDesc desc;
            engine::graphics::TextureHandle baseColorTexture;
            engine::graphics::SamplerHandle sampler;
        };

        struct CachedMesh final
        {
            std::unique_ptr<engine::assets::GpuMesh> gpu;
            std::vector<CachedMaterial> materials;
        };

    public:
        [[nodiscard]]
        bool Initialize(
            engine::graphics::
                RenderDevice& device) noexcept
        {
            if (initialized_)
            {
                return true;
            }

            device_ = &device;

            Microsoft::WRL::ComPtr<ID3DBlob>
                vertexBytecode;

            Microsoft::WRL::ComPtr<ID3DBlob>
                pixelBytecode;

            if (!CompileEditorShaderFile(
                    L"StaticMesh.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.StaticMesh",
                    vertexBytecode))
            {
                device_ = nullptr;
                return false;
            }

            if (!CompileEditorShaderFile(
                    L"StaticMesh.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.StaticMesh",
                    pixelBytecode))
            {
                device_ = nullptr;
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
                LogGraphicsFailure(
                    "Create static mesh vertex shader",
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
                "EditorStaticMesh.PixelShader";

            result =
                device.CreateShader(
                    pixelShaderDescription,
                    pixelShader_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create static mesh pixel shader",
                    result);

                Shutdown(device);
                return false;
            }

            const std::array<
                engine::graphics::
                    VertexElementDesc,
                4U> elements
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
                    "TANGENT",
                    0U,
                    engine::graphics::
                        Format::R32G32B32A32Float,
                    0U,
                    24U,
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
                    40U,
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
                LogGraphicsFailure(
                    "Create static mesh input layout",
                    result);

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
                LogGraphicsFailure(
                    "Create static mesh object buffer",
                    result);

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
                LogGraphicsFailure(
                    "Create static mesh pipeline",
                    result);

                Shutdown(device);
                return false;
            }

            pipelineDescription.blend.renderTargets[0].blendEnable = true;
            pipelineDescription.blend.renderTargets[0].sourceColor =
                engine::graphics::BlendFactor::SourceAlpha;
            pipelineDescription.blend.renderTargets[0].destinationColor =
                engine::graphics::BlendFactor::InverseSourceAlpha;
            pipelineDescription.blend.renderTargets[0].sourceAlpha =
                engine::graphics::BlendFactor::One;
            pipelineDescription.blend.renderTargets[0].destinationAlpha =
                engine::graphics::BlendFactor::InverseSourceAlpha;
            pipelineDescription.depthStencil.depthWriteEnable = false;
            pipelineDescription.debugName = "EditorStaticMesh.TransparentPipeline";
            result = device.CreateGraphicsPipeline(
                pipelineDescription, transparentPipeline_);
            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure("Create transparent static mesh pipeline", result);
                Shutdown(device);
                return false;
            }

            initialized_ = true;

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Information,
                "LTS.Editor.StaticMesh",
                "Editor static mesh renderer initialized.");

            return true;
        }

        void Shutdown(
            engine::graphics::
                RenderDevice& device) noexcept
        {
            initialized_ = false;

            for (auto& entry : meshes_)
            {
                for (CachedMaterial& material : entry.second.materials)
                {
                    if (material.baseColorTexture.IsValid())
                        static_cast<void>(device.DestroyTexture(material.baseColorTexture));
                    if (material.sampler.IsValid())
                        static_cast<void>(device.DestroySampler(material.sampler));
                }
                if (entry.second.gpu != nullptr)
                {
                    static_cast<void>(
                        entry.second.gpu->Release(
                            device));
                }
            }

            meshes_.clear();
            failedMeshes_.clear();

            if (transparentPipeline_.IsValid())
            {
                static_cast<void>(device.DestroyGraphicsPipeline(transparentPipeline_));
                transparentPipeline_ = {};
            }

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

            if (objectBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        objectBuffer_));

                objectBuffer_ = {};
            }

            device_ = nullptr;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const SceneDocument& document,
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

            engine::graphics::GraphicsResult result =
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

            result = context.SetConstantBuffers(
                engine::graphics::ShaderStage::Pixel, 0U, &objectBuffer_, 1U);
            if (engine::graphics::Failed(result))
            {
                static_cast<void>(context.UnbindConstantBuffers(
                    engine::graphics::ShaderStage::Vertex, 0U, 1U));
                context.UnbindGraphicsPipeline();
                return result;
            }

            result =
                engine::graphics::
                    GraphicsResult::Success;

            const auto& entities =
                document.GetEntities();

            const std::size_t selectedIndex =
                document.GetSelectedIndex();
            const ResolvedDirectionalLight lighting =
                ResolveDirectionalLight(document);

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

                CachedMesh* const cachedMesh =
                    GetOrLoadMesh(
                        entity.staticMesh->
                            assetPath);

                if (cachedMesh == nullptr || cachedMesh->gpu == nullptr)
                {
                    continue;
                }
                engine::assets::GpuMesh* const mesh = cachedMesh->gpu.get();

                ObjectConstants constants{};

                DirectX::XMStoreFloat4x4(
                    &constants.world,
                    BuildWorldMatrix(
                        entity.transform));

                constants.viewProjection =
                    viewProjection;

                constants.baseColor = { 0.58F, 0.63F, 0.66F, 1.0F };
                constants.materialParameters = {
                    entityIndex == selectedIndex ? 1.0F : 0.0F, 0.0F, 0.0F, 0.0F };
                
                constants.sunDirectionIntensity =
                {
                    lighting.direction.x,
                    lighting.direction.y,
                    lighting.direction.z,
                    lighting.intensity
                };

                constants.sunColor =
                {
                    lighting.color.x,
                    lighting.color.y,
                    lighting.color.z,
                    1.0F
                };

                constants.ambientColor =
                {
                    lighting.ambientColor.x *
                        lighting.ambientIntensity,

                    lighting.ambientColor.y *
                        lighting.ambientIntensity,

                    lighting.ambientColor.z *
                        lighting.ambientIntensity,

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
                    mesh->GetVertexBuffer();

                vertexBinding.stride =
                    mesh->GetVertexStride();

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
                    mesh->GetIndexBuffer();

                indexBinding.offset = 0U;

                result =
                    context.SetIndexBuffer(
                        indexBinding);

                if (engine::graphics::Failed(result))
                {
                    break;
                }

                for (
                    std::size_t submeshIndex = 0U;
                    submeshIndex <
                        mesh->GetSubmeshCount();
                    ++submeshIndex)
                {
                    const engine::assets::
                        MeshSubmesh* const submesh =
                            mesh->GetSubmesh(
                                submeshIndex);

                    if (submesh == nullptr)
                    {
                        continue;
                    }

                    engine::graphics::TextureHandle texture;
                    engine::graphics::SamplerHandle sampler;
                    bool transparent = false;
                    if (submesh->materialSlot < cachedMesh->materials.size())
                    {
                        const CachedMaterial& material =
                            cachedMesh->materials[submesh->materialSlot];
                        constants.baseColor = {
                            material.desc.baseColorFactor[0], material.desc.baseColorFactor[1],
                            material.desc.baseColorFactor[2], material.desc.baseColorFactor[3] };
                        texture = material.baseColorTexture;
                        sampler = material.sampler;
                        transparent = material.desc.alphaMode ==
                            engine::assets::MaterialAlphaMode::Blend;
                        constants.materialParameters.y = texture.IsValid() ? 1.0F : 0.0F;
                    }
                    else
                    {
                        constants.baseColor = { 0.58F, 0.63F, 0.66F, 1.0F };
                        constants.materialParameters.y = 0.0F;
                    }
                    result = context.SetGraphicsPipeline(
                        transparent ? transparentPipeline_ : pipeline_);
                    if (engine::graphics::Failed(result)) break;
                    result = context.UpdateBuffer(objectBuffer_, &constants, sizeof(constants));
                    if (engine::graphics::Failed(result)) break;
                    if (texture.IsValid())
                    {
                        result = context.SetShaderResources(
                            engine::graphics::ShaderStage::Pixel, 0U, &texture, 1U);
                        if (engine::graphics::Failed(result)) break;
                        result = context.SetSamplers(
                            engine::graphics::ShaderStage::Pixel, 0U, &sampler, 1U);
                        if (engine::graphics::Failed(result)) break;
                    }
                    else
                    {
                        static_cast<void>(context.UnbindShaderResources(
                            engine::graphics::ShaderStage::Pixel, 0U, 1U));
                        static_cast<void>(context.UnbindSamplers(
                            engine::graphics::ShaderStage::Pixel, 0U, 1U));
                    }

                    result =
                        context.DrawIndexed(
                            submesh->indexCount,
                            submesh->firstIndex,
                            submesh->baseVertex);

                    if (
                        engine::graphics::Failed(
                            result))
                    {
                        break;
                    }
                }

                if (engine::graphics::Failed(result))
                {
                    break;
                }
            }

            context.UnbindIndexBuffer();
            static_cast<void>(context.UnbindShaderResources(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));
            static_cast<void>(context.UnbindSamplers(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    1U));
            static_cast<void>(context.UnbindConstantBuffers(
                engine::graphics::ShaderStage::Pixel, 0U, 1U));

            context.UnbindGraphicsPipeline();

            return result;
        }

        [[nodiscard]] bool TryGetMeshBounds(
            const std::wstring& assetPath,
            DirectX::XMFLOAT3& minimum,
            DirectX::XMFLOAT3& maximum) const noexcept
        {
            try
            {
                std::filesystem::path path(assetPath);
                if (!path.is_absolute())
                {
                    path = std::filesystem::current_path() / path;
                }
                const auto found = meshes_.find(
                    LowercasePath(path.lexically_normal().wstring()));
                if (found == meshes_.end() || found->second.gpu == nullptr)
                {
                    return false;
                }
                const engine::assets::MeshBounds& bounds =
                    found->second.gpu->GetBounds();
                minimum = {bounds.minimum[0], bounds.minimum[1], bounds.minimum[2]};
                maximum = {bounds.maximum[0], bounds.maximum[1], bounds.maximum[2]};
                return bounds.IsValid();
            }
            catch (...)
            {
                return false;
            }
        }

    private:
        [[nodiscard]]
        CachedMesh* GetOrLoadMesh(
            const std::wstring& assetPath) noexcept
        {
            try
            {
                std::filesystem::path path(
                    assetPath);

                std::error_code filesystemError;

                if (!path.is_absolute())
                {
                    const std::filesystem::path gameRoot =
                        std::filesystem::current_path(
                            filesystemError);

                    if (filesystemError)
                    {
                        return nullptr;
                    }

                    path =
                        gameRoot /
                        path;
                }

                path =
                    path.lexically_normal();

                const std::wstring key =
                    LowercasePath(
                        path.wstring());

                const auto existing =
                    meshes_.find(key);

                if (existing != meshes_.end())
                {
                    return
                        &existing->second;
                }

                if (
                    failedMeshes_.find(key) !=
                    failedMeshes_.end())
                {
                    return nullptr;
                }

                std::unique_ptr<engine::assets::GpuMesh> mesh =
                        LoadGpuMesh(
                            path,
                            std::filesystem::path(
                                assetPath));

                if (mesh == nullptr)
                {
                    failedMeshes_.insert(key);
                    return nullptr;
                }

                CachedMesh cached;
                cached.gpu = std::move(mesh);
                LoadMaterials(path, cached.materials);
                auto inserted = meshes_.emplace(key, std::move(cached));
                return &inserted.first->second;
            }
            catch (...)
            {
                return nullptr;
            }
        }

        void LoadMaterials(
            const std::filesystem::path& meshPath,
            std::vector<CachedMaterial>& output) noexcept
        {
            output.clear();
            try
            {
                std::filesystem::path meshesRoot;
                std::filesystem::path cursor = meshPath.parent_path();
                while (!cursor.empty())
                {
                    if (LowercasePath(cursor.filename().wstring()) == L"meshes")
                    {
                        meshesRoot = cursor;
                        break;
                    }
                    const auto parent = cursor.parent_path();
                    if (parent == cursor) break;
                    cursor = parent;
                }
                if (meshesRoot.empty()) return;
                std::error_code filesystemError;
                const auto package = std::filesystem::relative(
                    meshPath.parent_path(), meshesRoot, filesystemError);
                if (filesystemError) return;
                const auto directory = meshesRoot.parent_path() / L"Materials" / package;
                if (!std::filesystem::is_directory(directory, filesystemError) || filesystemError)
                    return;
                std::vector<std::filesystem::path> files;
                for (std::filesystem::directory_iterator iterator(directory, filesystemError), end;
                     !filesystemError && iterator != end; iterator.increment(filesystemError))
                {
                    if (iterator->is_regular_file() &&
                        LowercasePath(iterator->path().extension().wstring()) == L".material")
                        files.push_back(iterator->path());
                }
                std::sort(files.begin(), files.end(), [](const auto& left, const auto& right)
                { return LowercasePath(left.filename().wstring()) < LowercasePath(right.filename().wstring()); });
                for (const auto& file : files)
                {
                    engine::assets::AssetData data;
                    if (engine::assets::Failed(ReadAssetData(file, data))) continue;
                    const auto gameRoot = meshesRoot.parent_path().parent_path();
                    const auto logical = std::filesystem::relative(file, gameRoot, filesystemError);
                    if (filesystemError) continue;
                    engine::assets::AssetPath assetPath;
                    if (engine::assets::Failed(engine::assets::AssetPath::TryCreate(
                            logical.generic_u8string(), assetPath))) continue;
                    engine::assets::AssetMetadata metadata;
                    metadata.path = std::move(assetPath);
                    metadata.id = metadata.path.GetId();
                    metadata.type = engine::assets::AssetType::Material;
                    metadata.schemaVersion = 2U;
                    metadata.sourceSize = data.GetSize();
                    engine::assets::MaterialAssetLoader loader;
                    std::unique_ptr<engine::assets::LoadedAsset> loaded;
                    if (engine::assets::Failed(loader.Load(metadata, data, loaded)) || !loaded)
                        continue;
                    const auto* loadedMaterial = static_cast<engine::assets::MaterialLoadedAsset*>(loaded.get());
                    CachedMaterial material;
                    material.desc = loadedMaterial->GetMaterial().GetDesc();
                    if (material.desc.baseColorTexture)
                    {
                        const auto texturePath = gameRoot /
                            std::filesystem::u8path(material.desc.baseColorTexture->String());
                        std::vector<std::byte> pixels;
                        std::uint32_t width = 0U, height = 0U;
                        if (DecodeWicRgba(texturePath, pixels, width, height))
                        {
                            engine::graphics::TextureDesc desc;
                            desc.width = width; desc.height = height;
                            desc.format = engine::graphics::Format::R8G8B8A8UNormSrgb;
                            engine::graphics::TextureSubresourceData initial;
                            initial.data = pixels.data(); initial.dataSize = pixels.size();
                            initial.rowPitch = static_cast<std::size_t>(width) * 4U;
                            initial.slicePitch = pixels.size();
                            if (engine::graphics::Failed(device_->CreateTexture(
                                    desc, &initial, 1U, material.baseColorTexture)))
                                material.baseColorTexture = {};
                        }
                    }
                    if (material.baseColorTexture.IsValid())
                    {
                        engine::graphics::SamplerDesc samplerDesc = material.desc.sampler;
                        samplerDesc.addressU = engine::graphics::TextureAddressMode::Wrap;
                        samplerDesc.addressV = engine::graphics::TextureAddressMode::Wrap;
                        if (engine::graphics::Failed(device_->CreateSampler(
                                samplerDesc, material.sampler)))
                        {
                            static_cast<void>(device_->DestroyTexture(material.baseColorTexture));
                            material.baseColorTexture = {};
                        }
                    }
                    output.push_back(std::move(material));
                }
            }
            catch (...) { output.clear(); }
        }

        [[nodiscard]]
        std::unique_ptr<
            engine::assets::GpuMesh>
                LoadGpuMesh(
                    const std::filesystem::path& filePath,
                    const std::filesystem::path& logicalPath) noexcept
        {
            try
            {
                engine::assets::AssetData sourceData;

                engine::assets::AssetResult assetResult =
                    ReadAssetData(
                        filePath,
                        sourceData);

                if (
                    engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Read LTS mesh",
                        assetResult);

                    return nullptr;
                }

                engine::assets::AssetMetadata metadata;

                assetResult =
                    CreateMeshMetadata(
                        logicalPath,
                        sourceData.GetSize(),
                        metadata);

                if (
                    engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Create mesh metadata",
                        assetResult);

                    return nullptr;
                }

                engine::assets::
                    MeshAssetLoader loader;

                std::unique_ptr<
                    engine::assets::LoadedAsset>
                        loadedAsset;

                assetResult =
                    loader.Load(
                        metadata,
                        sourceData,
                        loadedAsset);

                if (
                    engine::assets::Failed(
                        assetResult) ||
                    loadedAsset == nullptr ||
                    loadedAsset->GetType() !=
                        engine::assets::
                            AssetType::Mesh)
                {
                    if (
                        engine::assets::Succeeded(
                            assetResult))
                    {
                        assetResult =
                            engine::assets::
                                AssetResult::
                                    TypeMismatch;
                    }

                    LogAssetFailure(
                        filePath,
                        "Load LTS mesh",
                        assetResult);

                    return nullptr;
                }

                auto* const loadedMesh =
                    static_cast<
                        engine::assets::
                            MeshLoadedAsset*>(
                                loadedAsset.get());

                engine::assets::MeshAsset cpuMesh =
                    loadedMesh->ReleaseMesh();

                auto gpuMesh =
                    std::make_unique<
                        engine::assets::GpuMesh>();

                const engine::graphics::
                    GraphicsResult uploadResult =
                        gpuMesh->Upload(
                            *device_,
                            cpuMesh);

                if (
                    engine::graphics::Failed(
                        uploadResult))
                {
                    LogGraphicsFailure(
                        "Upload static mesh",
                        uploadResult);

                    return nullptr;
                }

                return gpuMesh;
            }
            catch (const std::bad_alloc&)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.StaticMesh",
                    "Not enough memory to load a static mesh.");

                return nullptr;
            }
            catch (...)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.StaticMesh",
                    "Unexpected static mesh loading failure.");

                return nullptr;
            }
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

        engine::graphics::PipelineStateHandle
            transparentPipeline_;

        std::unordered_map<
            std::wstring,
            CachedMesh> meshes_;

        std::unordered_set<
            std::wstring> failedMeshes_;

        bool initialized_ = false;
    };

    StaticMeshRenderer::
        StaticMeshRenderer() noexcept =
            default;

    StaticMeshRenderer::
        ~StaticMeshRenderer() noexcept =
            default;

    bool StaticMeshRenderer::Initialize(
        engine::graphics::
            RenderDevice& device) noexcept
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

    void StaticMeshRenderer::Shutdown(
        engine::graphics::
            RenderDevice& device) noexcept
    {
        if (impl_ == nullptr)
        {
            return;
        }

        impl_->Shutdown(device);
        impl_.reset();
    }

    engine::graphics::GraphicsResult
        StaticMeshRenderer::Render(
            engine::graphics::
                CommandContext& context,
            const SceneDocument& document,
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

    bool StaticMeshRenderer::TryGetMeshBounds(
        const std::wstring& assetPath,
        DirectX::XMFLOAT3& minimum,
        DirectX::XMFLOAT3& maximum) const noexcept
    {
        return impl_ != nullptr &&
            impl_->TryGetMeshBounds(assetPath, minimum, maximum);
    }
}
