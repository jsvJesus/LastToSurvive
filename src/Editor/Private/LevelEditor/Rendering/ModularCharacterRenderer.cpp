#include "Editor/LevelEditor/Rendering/ModularCharacterRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/GpuSkeletalMesh.h>
#include <Assets/SkeletalMeshAsset.h>
#include <Assets/SkeletalMeshAssetLoader.h>

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/Shader.h>

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cmath>
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

namespace lts::editor
{
    namespace
    {
        constexpr std::uintmax_t
            MaximumSkeletalMeshFileSize =
                512U * 1024U * 1024U;

        struct alignas(16)
            ObjectConstants final
        {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4X4 viewProjection;

            DirectX::XMFLOAT4 baseColor;
            DirectX::XMFLOAT4 materialParameters;

            DirectX::XMFLOAT4
                sunDirectionIntensity;

            DirectX::XMFLOAT4 sunColor;
            DirectX::XMFLOAT4 ambientColor;
        };

        static_assert(
            sizeof(ObjectConstants) % 16U == 0U);

        struct ResolvedLighting final
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
        DirectX::XMMATRIX BuildWorldMatrix(
            const EditorTransform& transform) noexcept
        {
            const DirectX::XMMATRIX scale =
                DirectX::XMMatrixScaling(
                    transform.scale[0],
                    transform.scale[1],
                    transform.scale[2]);

            const DirectX::XMMATRIX rotation =
                DirectX::
                    XMMatrixRotationRollPitchYaw(
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

        [[nodiscard]]
        ResolvedLighting ResolveLighting(
            const SceneDocument& document) noexcept
        {
            ResolvedLighting result;

            for (
                const EditorSceneEntity& entity :
                document.GetEntities())
            {
                if (
                    !entity.environment.has_value() ||
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

            for (
                const EditorSceneEntity& entity :
                document.GetEntities())
            {
                if (
                    !entity.directionalLight.
                        has_value())
                {
                    continue;
                }

                const auto& light =
                    *entity.directionalLight;

                const float pitch =
                    DirectX::XMConvertToRadians(
                        entity.transform.
                            rotationDegrees[0]);

                const float yaw =
                    DirectX::XMConvertToRadians(
                        entity.transform.
                            rotationDegrees[1]);

                const float cosinePitch =
                    std::cos(pitch);

                DirectX::XMFLOAT3 direction
                {
                    -cosinePitch * std::sin(yaw),
                    -std::sin(pitch),
                    -cosinePitch * std::cos(yaw)
                };

                DirectX::XMStoreFloat3(
                    &result.direction,
                    DirectX::XMVector3Normalize(
                        DirectX::XMLoadFloat3(
                            &direction)));

                result.color =
                {
                    (std::max)(
                        light.color[0],
                        0.0F),

                    (std::max)(
                        light.color[1],
                        0.0F),

                    (std::max)(
                        light.color[2],
                        0.0F)
                };

                result.intensity =
                    (std::max)(
                        light.intensity,
                        0.0F) *
                    0.25F;

                break;
            }

            return result;
        }

        [[nodiscard]]
        DirectX::XMFLOAT4 GetSlotColor(
            const engine::scene::
                CharacterMeshSlot slot) noexcept
        {
            switch (slot)
            {
                case engine::scene::
                    CharacterMeshSlot::Hair:
                    return
                    {
                        0.22F,
                        0.14F,
                        0.08F,
                        1.0F
                    };

                case engine::scene::
                    CharacterMeshSlot::Head:
                    return
                    {
                        0.68F,
                        0.50F,
                        0.38F,
                        1.0F
                    };

                case engine::scene::
                    CharacterMeshSlot::Body:
                    return
                    {
                        0.22F,
                        0.37F,
                        0.45F,
                        1.0F
                    };

                case engine::scene::
                    CharacterMeshSlot::Legs:
                    return
                    {
                        0.16F,
                        0.23F,
                        0.29F,
                        1.0F
                    };

                case engine::scene::
                    CharacterMeshSlot::Shoes:
                    return
                    {
                        0.08F,
                        0.08F,
                        0.085F,
                        1.0F
                    };

                case engine::scene::
                    CharacterMeshSlot::
                        FirstPersonBody:

                case engine::scene::
                    CharacterMeshSlot::Count:

                default:
                    return
                    {
                        0.55F,
                        0.58F,
                        0.62F,
                        1.0F
                    };
            }
        }

        [[nodiscard]]
        std::wstring LowercasePath(
            std::wstring value)
        {
            for (wchar_t& character : value)
            {
                character =
                    static_cast<wchar_t>(
                        std::towlower(character));
            }

            return value;
        }

        [[nodiscard]]
        std::filesystem::path ResolveAssetFile(
            const std::wstring& assetPath) noexcept
        {
            try
            {
                std::filesystem::path requested(
                    assetPath);

                if (requested.empty())
                {
                    return {};
                }

                std::error_code error;

                if (requested.is_absolute())
                {
                    if (std::filesystem::is_regular_file(
                            requested,
                            error) &&
                        !error)
                    {
                        return
                            requested.lexically_normal();
                    }

                    return {};
                }

                std::filesystem::path current =
                    std::filesystem::current_path(
                        error);

                if (error)
                {
                    return {};
                }

                while (!current.empty())
                {
                    error.clear();

                    const std::filesystem::path
                        directCandidate =
                            current /
                            requested;

                    if (std::filesystem::is_regular_file(
                            directCandidate,
                            error) &&
                        !error)
                    {
                        return
                            directCandidate.
                                lexically_normal();
                    }

                    error.clear();

                    const std::filesystem::path
                        gameCandidate =
                            current /
                            L"game" /
                            requested;

                    if (std::filesystem::is_regular_file(
                            gameCandidate,
                            error) &&
                        !error)
                    {
                        return
                            gameCandidate.
                                lexically_normal();
                    }

                    const std::filesystem::path parent =
                        current.parent_path();

                    if (
                        parent.empty() ||
                        parent == current)
                    {
                        break;
                    }

                    current = parent;
                }
            }
            catch (...)
            {
            }

            return {};
        }

        [[nodiscard]]
        engine::assets::AssetResult ReadAssetData(
            const std::filesystem::path& path,
            engine::assets::AssetData&
                output) noexcept
        {
            output.Clear();

            try
            {
                std::error_code error;

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        path,
                        error);

                if (error)
                {
                    return engine::assets::
                        AssetResult::IoError;
                }

                if (
                    fileSize == 0U ||
                    fileSize >
                        MaximumSkeletalMeshFileSize ||
                    fileSize >
                        static_cast<std::uintmax_t>(
                            (std::numeric_limits<
                                std::streamsize>::
                                    max)()))
                {
                    return engine::assets::
                        AssetResult::FileTooLarge;
                }

                const engine::assets::AssetResult
                    resizeResult =
                        output.Resize(
                            static_cast<std::size_t>(
                                fileSize));

                if (engine::assets::Failed(
                        resizeResult))
                {
                    return resizeResult;
                }

                std::ifstream stream(
                    path,
                    std::ios::binary);

                if (!stream)
                {
                    output.Clear();

                    return engine::assets::
                        AssetResult::IoError;
                }

                stream.read(
                    reinterpret_cast<char*>(
                        output.GetData()),
                    static_cast<std::streamsize>(
                        fileSize));

                if (!stream)
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
            CreateMetadata(
                const std::filesystem::path&
                    logicalPath,
                const std::size_t sourceSize,
                engine::assets::AssetMetadata&
                    metadata) noexcept
        {
            engine::assets::AssetPath path;

            const engine::assets::AssetResult
                result =
                    engine::assets::
                        AssetPath::TryCreate(
                            logicalPath.
                                generic_u8string(),
                            path);

            if (engine::assets::Failed(result))
            {
                return result;
            }

            metadata = {};

            metadata.path = std::move(path);
            metadata.id = metadata.path.GetId();

            metadata.type =
                engine::assets::
                    AssetType::SkeletalMesh;

            metadata.schemaVersion = 1U;
            metadata.sourceSize = sourceSize;

            return engine::assets::
                AssetResult::Success;
        }

        void LogGraphicsFailure(
            const char* const operation,
            const engine::graphics::
                GraphicsResult result) noexcept
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Modular character graphics operation";

            message += " failed: ";

            message +=
                engine::graphics::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.ModularCharacter",
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
                    : "Modular character asset operation";

            message += " failed for '";
            message += path.generic_u8string();
            message += "': ";

            message +=
                engine::assets::
                    ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                "LTS.Editor.ModularCharacter",
                message);
        }
    }

    class ModularCharacterRenderer::Impl final
    {
        struct CachedMesh final
        {
            std::unique_ptr<
                engine::assets::GpuSkeletalMesh>
                gpu;
        };

    public:
        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice&
                device) noexcept
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

            /*
             * Bind-pose renderer временно использует
             * тот же shader, что и StaticMeshRenderer.
             */
            if (!CompileEditorShaderFile(
                    L"StaticMesh.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.ModularCharacter",
                    vertexBytecode))
            {
                device_ = nullptr;
                return false;
            }

            if (!CompileEditorShaderFile(
                    L"StaticMesh.hlsl",
                    "PSMain",
                    "ps_5_0",
                    "LTS.Editor.ModularCharacter",
                    pixelBytecode))
            {
                device_ = nullptr;
                return false;
            }

            engine::graphics::ShaderDesc
                vertexDescription;

            vertexDescription.stage =
                engine::graphics::
                    ShaderStage::Vertex;

            vertexDescription.bytecode.data =
                vertexBytecode->
                    GetBufferPointer();

            vertexDescription.bytecode.size =
                vertexBytecode->
                    GetBufferSize();

            vertexDescription.debugName =
                "EditorModularCharacter.VertexShader";

            engine::graphics::GraphicsResult result =
                device.CreateShader(
                    vertexDescription,
                    vertexShader_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character vertex shader",
                    result);

                Shutdown(device);
                return false;
            }

            engine::graphics::ShaderDesc
                pixelDescription;

            pixelDescription.stage =
                engine::graphics::
                    ShaderStage::Pixel;

            pixelDescription.bytecode.data =
                pixelBytecode->
                    GetBufferPointer();

            pixelDescription.bytecode.size =
                pixelBytecode->
                    GetBufferSize();

            pixelDescription.debugName =
                "EditorModularCharacter.PixelShader";

            result =
                device.CreateShader(
                    pixelDescription,
                    pixelShader_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character pixel shader",
                    result);

                Shutdown(device);
                return false;
            }

            /*
             * Первые 48 байт SkeletalMeshVertex совместимы
             * со входом StaticMesh.hlsl.
             *
             * Bone indices начинаются с offset 48,
             * weights — с offset 52.
             */
            const std::array<
                engine::graphics::
                    VertexElementDesc,
                4U>
                elements
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

            engine::graphics::InputLayoutDesc
                inputDescription;

            inputDescription.vertexShader =
                vertexShader_;

            inputDescription.elements =
                elements.data();

            inputDescription.elementCount =
                elements.size();

            inputDescription.debugName =
                "EditorModularCharacter.InputLayout";

            result =
                device.CreateInputLayout(
                    inputDescription,
                    inputLayout_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character input layout",
                    result);

                Shutdown(device);
                return false;
            }

            engine::graphics::BufferDesc
                constantDescription;

            constantDescription.byteSize =
                sizeof(ObjectConstants);

            constantDescription.stride = 0U;

            constantDescription.usage =
                engine::graphics::
                    ResourceUsage::Default;

            constantDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Constant;

            constantDescription.miscFlags =
                engine::graphics::
                    BufferMiscFlags::None;

            constantDescription.cpuAccess =
                engine::graphics::
                    CpuAccessFlags::None;

            constantDescription.indexFormat =
                engine::graphics::
                    IndexFormat::None;

            result =
                device.CreateBuffer(
                    constantDescription,
                    nullptr,
                    objectBuffer_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character constant buffer",
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
                "EditorModularCharacter.Pipeline";

            result =
                device.CreateGraphicsPipeline(
                    pipelineDescription,
                    pipeline_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character pipeline",
                    result);

                Shutdown(device);
                return false;
            }

            initialized_ = true;

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Information,
                "LTS.Editor.ModularCharacter",
                "Modular character renderer initialized.");

            return true;
        }

        void Shutdown(
            engine::graphics::RenderDevice&
                device) noexcept
        {
            initialized_ = false;

            for (auto& pair : meshes_)
            {
                if (pair.second.gpu != nullptr)
                {
                    static_cast<void>(
                        pair.second.gpu->
                            Release(device));
                }
            }

            meshes_.clear();
            failedMeshes_.clear();

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

            result =
                context.SetConstantBuffers(
                    engine::graphics::
                        ShaderStage::Pixel,
                    0U,
                    &objectBuffer_,
                    1U);

            if (engine::graphics::Failed(result))
            {
                static_cast<void>(
                    context.UnbindConstantBuffers(
                        engine::graphics::
                            ShaderStage::Vertex,
                        0U,
                        1U));

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

            const ResolvedLighting lighting =
                ResolveLighting(document);

            constexpr std::array<
                engine::scene::CharacterMeshSlot,
                5U>
                visibleSlots
                {{
                    engine::scene::
                        CharacterMeshSlot::Hair,

                    engine::scene::
                        CharacterMeshSlot::Head,

                    engine::scene::
                        CharacterMeshSlot::Body,

                    engine::scene::
                        CharacterMeshSlot::Legs,

                    engine::scene::
                        CharacterMeshSlot::Shoes
                }};

            for (
                std::size_t entityIndex = 0U;
                entityIndex < entities.size();
                ++entityIndex)
            {
                const EditorSceneEntity& entity =
                    entities[entityIndex];

                if (
                    !entity.skeletalMesh.has_value() ||
                    !entity.skeletalMesh->visible)
                {
                    continue;
                }

                const auto& component =
                    *entity.skeletalMesh;

                ObjectConstants constants{};

                DirectX::XMStoreFloat4x4(
                    &constants.world,
                    BuildWorldMatrix(
                        entity.transform));

                constants.viewProjection =
                    viewProjection;

                constants.materialParameters =
                {
                    entityIndex == selectedIndex
                        ? 1.0F
                        : 0.0F,

                    0.0F,
                    0.0F,
                    0.0F
                };

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

                for (
                    const engine::scene::
                        CharacterMeshSlot slot :
                    visibleSlots)
                {
                    const auto& part =
                        component.GetPart(slot);

                    if (
                        !part.visible ||
                        part.assetPath.empty())
                    {
                        continue;
                    }

                    CachedMesh* const cached =
                        GetOrLoadMesh(
                            part.assetPath);

                    if (
                        cached == nullptr ||
                        cached->gpu == nullptr)
                    {
                        continue;
                    }

                    engine::assets::
                        GpuSkeletalMesh* const mesh =
                            cached->gpu.get();

                    constants.baseColor =
                        GetSlotColor(slot);

                    result =
                        context.UpdateBuffer(
                            objectBuffer_,
                            &constants,
                            sizeof(constants));

                    if (engine::graphics::Failed(
                            result))
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

                    if (engine::graphics::Failed(
                            result))
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

                    if (engine::graphics::Failed(
                            result))
                    {
                        break;
                    }

                    for (
                        std::size_t sectionIndex = 0U;
                        sectionIndex <
                            mesh->GetSectionCount();
                        ++sectionIndex)
                    {
                        const engine::assets::
                            SkeletalMeshSection*
                                section =
                                    mesh->GetSection(
                                        sectionIndex);

                        if (section == nullptr)
                        {
                            continue;
                        }

                        result =
                            context.DrawIndexed(
                                section->indexCount,
                                section->firstIndex,
                                0);

                        if (engine::graphics::Failed(
                                result))
                        {
                            break;
                        }
                    }

                    if (engine::graphics::Failed(
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

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    1U));

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Pixel,
                    0U,
                    1U));

            context.UnbindGraphicsPipeline();

            return result;
        }

    private:
        [[nodiscard]]
        CachedMesh* GetOrLoadMesh(
            const std::wstring& assetPath) noexcept
        {
            try
            {
                const std::filesystem::path
                    resolvedPath =
                        ResolveAssetFile(
                            assetPath);

                if (resolvedPath.empty())
                {
                    const std::wstring key =
                        LowercasePath(assetPath);

                    if (
                        failedMeshes_.insert(key).
                            second)
                    {
                        engine::core::GetLogger().Write(
                            engine::core::
                                LogLevel::Error,
                            "LTS.Editor.ModularCharacter",
                            "Skeletal mesh file was not found.");
                    }

                    return nullptr;
                }

                const std::wstring key =
                    LowercasePath(
                        resolvedPath.wstring());

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

                std::unique_ptr<
                    engine::assets::GpuSkeletalMesh>
                    gpuMesh =
                        LoadGpuMesh(
                            resolvedPath,
                            std::filesystem::path(
                                assetPath));

                if (gpuMesh == nullptr)
                {
                    failedMeshes_.insert(key);

                    return nullptr;
                }

                CachedMesh cached;

                cached.gpu =
                    std::move(gpuMesh);

                auto insertion =
                    meshes_.emplace(
                        key,
                        std::move(cached));

                return &insertion.first->second;
            }
            catch (...)
            {
                return nullptr;
            }
        }

        [[nodiscard]]
        std::unique_ptr<
            engine::assets::GpuSkeletalMesh>
                LoadGpuMesh(
                    const std::filesystem::path&
                        filePath,
                    const std::filesystem::path&
                        logicalPath) noexcept
        {
            try
            {
                engine::assets::AssetData source;

                engine::assets::AssetResult
                    assetResult =
                        ReadAssetData(
                            filePath,
                            source);

                if (engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Read skeletal mesh",
                        assetResult);

                    return nullptr;
                }

                engine::assets::AssetMetadata
                    metadata;

                assetResult =
                    CreateMetadata(
                        logicalPath,
                        source.GetSize(),
                        metadata);

                if (engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Create skeletal mesh metadata",
                        assetResult);

                    return nullptr;
                }

                engine::assets::
                    SkeletalMeshAssetLoader loader;

                std::unique_ptr<
                    engine::assets::LoadedAsset>
                    loadedAsset;

                assetResult =
                    loader.Load(
                        metadata,
                        source,
                        loadedAsset);

                if (
                    engine::assets::Failed(
                        assetResult) ||
                    loadedAsset == nullptr ||
                    loadedAsset->GetType() !=
                        engine::assets::
                            AssetType::SkeletalMesh)
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
                        "Load skeletal mesh",
                        assetResult);

                    return nullptr;
                }

                auto* const loadedMesh =
                    static_cast<
                        engine::assets::
                            SkeletalMeshLoadedAsset*>(
                                loadedAsset.get());

                engine::assets::SkeletalMeshAsset
                    cpuMesh =
                        loadedMesh->
                            ReleaseSkeletalMesh();

                auto gpuMesh =
                    std::make_unique<
                        engine::assets::
                            GpuSkeletalMesh>();

                const engine::graphics::
                    GraphicsResult uploadResult =
                        gpuMesh->Upload(
                            *device_,
                            cpuMesh);

                if (engine::graphics::Failed(
                        uploadResult))
                {
                    LogGraphicsFailure(
                        "Upload skeletal mesh",
                        uploadResult);

                    return nullptr;
                }

                return gpuMesh;
            }
            catch (const std::bad_alloc&)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Not enough memory to load a skeletal mesh.");

                return nullptr;
            }
            catch (...)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Unexpected skeletal mesh loading failure.");

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

        std::unordered_map<
            std::wstring,
            CachedMesh>
            meshes_;

        std::unordered_set<std::wstring>
            failedMeshes_;

        bool initialized_ = false;
    };

    ModularCharacterRenderer::
        ModularCharacterRenderer() noexcept =
            default;

    ModularCharacterRenderer::
        ~ModularCharacterRenderer() noexcept =
            default;

    bool ModularCharacterRenderer::Initialize(
        engine::graphics::RenderDevice&
            device) noexcept
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

    void ModularCharacterRenderer::Shutdown(
        engine::graphics::RenderDevice&
            device) noexcept
    {
        if (impl_ == nullptr)
        {
            return;
        }

        impl_->Shutdown(device);
        impl_.reset();
    }

    engine::graphics::GraphicsResult
        ModularCharacterRenderer::Render(
            engine::graphics::CommandContext& context,
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
}