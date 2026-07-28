#include "Editor/LevelEditor/Rendering/ModularCharacterRenderer.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/GpuSkeletalMesh.h>
#include <Assets/SkeletalMeshAsset.h>
#include <Assets/SkeletalMeshAssetLoader.h>
#include <Assets/SkeletonAsset.h>
#include <Assets/SkeletonAssetLoader.h>

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
#include <cstring>
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

        struct alignas(16)
            SkinningConstants final
        {
            /*
             * x = skinning enabled
             * y = bone count
             */
            DirectX::XMFLOAT4 parameters{};

            std::array<
                DirectX::XMFLOAT4X4,
                engine::assets::
                    MaximumSkeletonBones>
                boneMatrices{};
        };

        static_assert(
            sizeof(SkinningConstants) % 16U == 0U);

        static_assert(
            sizeof(SkinningConstants) <= 65536U);

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
        std::filesystem::path FindDataRoot(
            std::filesystem::path current) noexcept
        {
            try
            {
                current =
                    current.lexically_normal();

                while (!current.empty())
                {
                    if (
                        LowercasePath(
                            current.filename().
                                wstring()) ==
                        L"data")
                    {
                        return current;
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
        std::filesystem::path ResolveSkeletonFile(
            const std::filesystem::path&
                skeletalMeshFile,
            const std::string&
                skeletonAssetPath) noexcept
        {
            try
            {
                if (skeletonAssetPath.empty())
                {
                    return {};
                }

                const std::filesystem::path requested =
                    std::filesystem::u8path(
                        skeletonAssetPath);

                std::error_code error;

                if (requested.is_absolute())
                {
                    return
                        std::filesystem::is_regular_file(
                            requested,
                            error) &&
                        !error
                            ? requested.
                                lexically_normal()
                            : std::filesystem::path{};
                }

                const std::filesystem::path dataRoot =
                    FindDataRoot(
                        skeletalMeshFile.
                            parent_path());

                if (dataRoot.empty())
                {
                    return {};
                }

                /*
                 * Обычно .skm хранит:
                 *
                 * Skeletons/Characters/CH_Skeletal.sk
                 */
                std::filesystem::path candidate =
                    dataRoot /
                    requested;

                if (std::filesystem::is_regular_file(
                        candidate,
                        error) &&
                    !error)
                {
                    return
                        candidate.lexically_normal();
                }

                error.clear();

                /*
                 * Поддержка пути с префиксом Data/.
                 */
                candidate =
                    dataRoot.parent_path() /
                    requested;

                if (std::filesystem::is_regular_file(
                        candidate,
                        error) &&
                    !error)
                {
                    return
                        candidate.lexically_normal();
                }
            }
            catch (...)
            {
            }

            return {};
        }

        [[nodiscard]]
        DirectX::XMMATRIX LoadMatrix(
            const std::array<float, 16U>&
                source) noexcept
        {
            DirectX::XMFLOAT4X4 stored;

            std::memcpy(
                &stored,
                source.data(),
                sizeof(stored));

            return DirectX::XMLoadFloat4x4(
                &stored);
        }

        void StoreMatrix(
            const DirectX::XMMATRIX& matrix,
            DirectX::XMFLOAT4X4&
                destination) noexcept
        {
            DirectX::XMStoreFloat4x4(
                &destination,
                matrix);
        }

        [[nodiscard]]
        bool InvertMatrix(
            const DirectX::XMMATRIX& source,
            DirectX::XMMATRIX& inverse) noexcept
        {
            DirectX::XMVECTOR determinant;

            inverse =
                DirectX::XMMatrixInverse(
                    &determinant,
                    source);

            const float value =
                DirectX::XMVectorGetX(
                    determinant);

            return
                std::isfinite(value) &&
                std::fabs(value) >
                    0.0000001F;
        }

        [[nodiscard]]
        bool BuildBindPoseSkinningConstants(
            const engine::assets::SkeletonAsset&
                skeleton,
            const std::array<float, 3U>& pivot,
            SkinningConstants& output) noexcept
        {
            if (!skeleton.IsValid())
            {
                return false;
            }

            output = {};

            output.parameters =
            {
                1.0F,
                static_cast<float>(
                    skeleton.GetBoneCount()),
                0.0F,
                0.0F
            };

            for (
                DirectX::XMFLOAT4X4& matrix :
                output.boneMatrices)
            {
                DirectX::XMStoreFloat4x4(
                    &matrix,
                    DirectX::XMMatrixIdentity());
            }

            const DirectX::XMMATRIX pivotTransform =
                DirectX::XMMatrixTranslation(
                    -pivot[0],
                    -pivot[1],
                    -pivot[2]);

            for (
                std::size_t boneIndex = 0U;
                boneIndex <
                    skeleton.GetBoneCount();
                ++boneIndex)
            {
                const engine::assets::SkeletonBone*
                    bone =
                        skeleton.GetBone(
                            boneIndex);

                if (bone == nullptr)
                {
                    return false;
                }

                const DirectX::XMMATRIX bindMatrix =
                    LoadMatrix(
                        bone->
                            absoluteBindMatrix);

                /*
                 * То же преобразование pivot,
                 * которое используется старым preview.
                 */
                const DirectX::XMMATRIX shiftedBind =
                    bindMatrix *
                    pivotTransform;

                /*
                 * Пока current pose равна bind pose.
                 *
                 * На следующем этапе shiftedCurrent
                 * будет получаться из .anim.
                 */
                const DirectX::XMMATRIX shiftedCurrent =
                    bindMatrix *
                    pivotTransform;

                DirectX::XMMATRIX inverseBind;

                if (!InvertMatrix(
                        shiftedBind,
                        inverseBind))
                {
                    return false;
                }

                StoreMatrix(
                    inverseBind *
                        shiftedCurrent,
                    output.boneMatrices[
                        boneIndex]);
            }

            return true;
        }

        [[nodiscard]]
        bool ValidateMeshSkeleton(
            const engine::assets::
                SkeletalMeshAsset& mesh,
            const engine::assets::
                SkeletonAsset& skeleton) noexcept
        {
            if (
                !mesh.IsValid() ||
                !skeleton.IsValid())
            {
                return false;
            }

            const engine::assets::
                SkeletalMeshVertex* vertices =
                    mesh.GetVertexData();

            if (vertices == nullptr)
            {
                return false;
            }

            for (
                std::size_t vertexIndex = 0U;
                vertexIndex <
                    mesh.GetVertexCount();
                ++vertexIndex)
            {
                const auto& vertex =
                    vertices[vertexIndex];

                for (
                    std::size_t influence = 0U;
                    influence < 4U;
                    ++influence)
                {
                    if (
                        vertex.boneWeights[
                            influence] >
                            0.000001F &&
                        static_cast<std::size_t>(
                            vertex.boneIndices[
                                influence]) >=
                            skeleton.GetBoneCount())
                    {
                        return false;
                    }
                }
            }

            return true;
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
                const engine::assets::AssetType type,
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

            metadata.type = type;

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
        struct CachedSkeleton final
        {
            engine::assets::SkeletonAsset asset;
        };

        struct CachedMesh final
        {
            std::unique_ptr<
                engine::assets::GpuSkeletalMesh>
                gpu;

            std::shared_ptr<CachedSkeleton>
                skeleton;

            SkinningConstants bindPose;
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
                    L"ModularCharacter.hlsl",
                    "VSMain",
                    "vs_5_0",
                    "LTS.Editor.ModularCharacter",
                    vertexBytecode))
            {
                device_ = nullptr;
                return false;
            }

            if (!CompileEditorShaderFile(
                    L"ModularCharacter.hlsl",
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
                6U>
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
                    },
                    {
                        "BLENDINDICES",
                        0U,
                        engine::graphics::
                            Format::R8G8B8A8UInt,
                        0U,
                        48U,
                        engine::graphics::
                            VertexInputRate::PerVertex,
                        0U
                    },
                    {
                        "BLENDWEIGHT",
                        0U,
                        engine::graphics::
                            Format::R32G32B32A32Float,
                        0U,
                        52U,
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

            constantDescription.byteSize =
                sizeof(SkinningConstants);

            result =
                device.CreateBuffer(
                    constantDescription,
                    nullptr,
                    skinningBuffer_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character skinning buffer",
                    result);

                Shutdown(device);
                return false;
            }

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
            skeletons_.clear();
            failedSkeletons_.clear();

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

            if (skinningBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        skinningBuffer_));

                skinningBuffer_ = {};
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

            const std::array<
                engine::graphics::BufferHandle,
                2U>
                vertexConstantBuffers
                {{
                    objectBuffer_,
                    skinningBuffer_
                }};

            result =
                context.SetConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    vertexConstantBuffers.data(),
                    vertexConstantBuffers.size());

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
                        vertexConstantBuffers.size()));

                context.UnbindGraphicsPipeline();

                return result;
            }

            if (engine::graphics::Failed(result))
            {
                context.UnbindGraphicsPipeline();

                return result;
            }

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

                    result =
                        context.UpdateBuffer(
                            skinningBuffer_,
                            &cached->bindPose,
                            sizeof(
                                cached->bindPose));

                    if (engine::graphics::Failed(
                            result))
                    {
                        break;
                    }

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

                CachedMesh cached;

                if (!LoadMesh(
                        resolvedPath,
                        std::filesystem::path(
                            assetPath),
                        cached))
                {
                    failedMeshes_.insert(key);

                    return nullptr;
                }

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
        std::shared_ptr<CachedSkeleton>
            GetOrLoadSkeleton(
                const std::filesystem::path&
                    filePath,
                const std::filesystem::path&
                    logicalPath) noexcept
        {
            try
            {
                const std::wstring key =
                    LowercasePath(
                        filePath.
                            lexically_normal().
                            wstring());

                const auto existing =
                    skeletons_.find(key);

                if (existing != skeletons_.end())
                {
                    return existing->second;
                }

                if (
                    failedSkeletons_.find(key) !=
                    failedSkeletons_.end())
                {
                    return nullptr;
                }

                engine::assets::AssetData source;

                engine::assets::AssetResult result =
                    ReadAssetData(
                        filePath,
                        source);

                if (engine::assets::Failed(result))
                {
                    LogAssetFailure(
                        filePath,
                        "Read skeleton",
                        result);

                    failedSkeletons_.insert(key);
                    return nullptr;
                }

                engine::assets::AssetMetadata metadata;

                result =
                    CreateMetadata(
                        logicalPath,
                        source.GetSize(),
                        engine::assets::
                            AssetType::Skeleton,
                        metadata);

                if (engine::assets::Failed(result))
                {
                    LogAssetFailure(
                        filePath,
                        "Create skeleton metadata",
                        result);

                    failedSkeletons_.insert(key);
                    return nullptr;
                }

                engine::assets::
                    SkeletonAssetLoader loader;

                std::unique_ptr<
                    engine::assets::LoadedAsset>
                    loadedAsset;

                result =
                    loader.Load(
                        metadata,
                        source,
                        loadedAsset);

                if (
                    engine::assets::Failed(result) ||
                    loadedAsset == nullptr ||
                    loadedAsset->GetType() !=
                        engine::assets::
                            AssetType::Skeleton)
                {
                    if (
                        engine::assets::Succeeded(
                            result))
                    {
                        result =
                            engine::assets::
                                AssetResult::
                                    TypeMismatch;
                    }

                    LogAssetFailure(
                        filePath,
                        "Load skeleton",
                        result);

                    failedSkeletons_.insert(key);
                    return nullptr;
                }

                auto* const loadedSkeleton =
                    static_cast<
                        engine::assets::
                            SkeletonLoadedAsset*>(
                                loadedAsset.get());

                auto cached =
                    std::make_shared<
                        CachedSkeleton>();

                cached->asset =
                    loadedSkeleton->
                        ReleaseSkeleton();

                skeletons_.emplace(
                    key,
                    cached);

                return cached;
            }
            catch (const std::bad_alloc&)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Not enough memory to load skeleton.");

                return nullptr;
            }
            catch (...)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Unexpected skeleton loading failure.");

                return nullptr;
            }
        }

        [[nodiscard]]
        bool LoadMesh(
            const std::filesystem::path& filePath,
            const std::filesystem::path& logicalPath,
            CachedMesh& output) noexcept
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

                    return false;
                }

                engine::assets::AssetMetadata metadata;

                assetResult =
                    CreateMetadata(
                        logicalPath,
                        source.GetSize(),
                        engine::assets::
                            AssetType::SkeletalMesh,
                        metadata);

                if (engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Create skeletal mesh metadata",
                        assetResult);

                    return false;
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

                    return false;
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

                const std::filesystem::path
                    skeletonFile =
                        ResolveSkeletonFile(
                            filePath,
                            cpuMesh.
                                GetSkeletonAssetPath());

                if (skeletonFile.empty())
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.ModularCharacter",
                        "Skeleton referenced by .skm was not found.");

                    return false;
                }

                const std::filesystem::path
                    logicalSkeletonPath =
                        std::filesystem::u8path(
                            cpuMesh.
                                GetSkeletonAssetPath());

                std::shared_ptr<CachedSkeleton>
                    skeleton =
                        GetOrLoadSkeleton(
                            skeletonFile,
                            logicalSkeletonPath);

                if (skeleton == nullptr)
                {
                    return false;
                }

                if (!ValidateMeshSkeleton(
                        cpuMesh,
                        skeleton->asset))
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.ModularCharacter",
                        "Skeletal mesh contains a bone index "
                        "outside the loaded skeleton.");

                    return false;
                }

                SkinningConstants bindPose;

                if (!BuildBindPoseSkinningConstants(
                        skeleton->asset,
                        cpuMesh.GetPivot(),
                        bindPose))
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.ModularCharacter",
                        "Failed to build skeleton bind matrices.");

                    return false;
                }

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

                    return false;
                }

                output.gpu =
                    std::move(gpuMesh);

                output.skeleton =
                    std::move(skeleton);

                output.bindPose =
                    bindPose;

                return true;
            }
            catch (const std::bad_alloc&)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Not enough memory to load a skeletal mesh.");

                return false;
            }
            catch (...)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Unexpected skeletal mesh loading failure.");

                return false;
            }
        }

        engine::graphics::RenderDevice*
            device_ = nullptr;

        engine::graphics::BufferHandle
            objectBuffer_;

        engine::graphics::BufferHandle
            skinningBuffer_;

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

        std::unordered_map<
            std::wstring,
            std::shared_ptr<CachedSkeleton>>
            skeletons_;

        std::unordered_set<std::wstring>
            failedMeshes_;

        std::unordered_set<std::wstring>
            failedSkeletons_;

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