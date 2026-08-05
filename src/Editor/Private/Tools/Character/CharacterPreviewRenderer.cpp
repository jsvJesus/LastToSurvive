#include "Editor/Tools/Character/CharacterPreviewRenderer.h"

#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"
#include "Editor/Tools/Character/CharacterEditor.h"
#include "Editor/Tools/Character/CharacterPose.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/AssetType.h>
#include <Assets/GpuSkeletalMesh.h>
#include <Assets/SkeletalMeshAsset.h>
#include <Assets/SkeletalMeshAssetLoader.h>
#include <Assets/SkeletonAsset.h>
#include <Assets/SkeletonAssetLoader.h>

#include <Core/Log.h>

#include <Graphics/Buffer.h>
#include <Graphics/CommandContext.h>
#include <Graphics/Format.h>
#include <Graphics/GraphicsBackend.h>
#include <Graphics/GraphicsResult.h>
#include <Graphics/InputLayout.h>
#include <Graphics/PipelineState.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/ResourceHandle.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>

#include <GraphicsDX11/D3D11Device.h>

#include <DirectXMath.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uintmax_t MaximumPreviewAssetSize =
            512U * 1024U * 1024U;

        constexpr std::uint32_t MinimumPreviewSize = 64U;
        constexpr std::uint32_t MaximumPreviewSize = 4096U;

        constexpr const char* PreviewLogCategory =
            "LTS.Editor.CharacterPreview";

        struct alignas(16) PreviewConstants final
        {
            DirectX::XMFLOAT4X4 world;
            DirectX::XMFLOAT4X4 viewProjection;

            DirectX::XMFLOAT4 baseColor;
            DirectX::XMFLOAT4 lightDirection;
            DirectX::XMFLOAT4 ambientColor;

            DirectX::XMFLOAT4 renderParameters;
        };

        static_assert(
            sizeof(PreviewConstants) % 16U == 0U);

        [[nodiscard]]
        std::filesystem::path ResolveAssetPath(
            const std::filesystem::path& requestedPath) noexcept
        {
            if (requestedPath.empty())
            {
                return {};
            }

            try
            {
                if (requestedPath.is_absolute())
                {
                    return requestedPath.lexically_normal();
                }

                std::error_code filesystemError;

                const std::filesystem::path workingDirectory =
                    std::filesystem::current_path(filesystemError);

                if (filesystemError)
                {
                    return requestedPath.lexically_normal();
                }

                const std::filesystem::path directPath =
                    workingDirectory / requestedPath;

                filesystemError.clear();

                if (std::filesystem::is_regular_file(
                        directPath,
                        filesystemError) &&
                    !filesystemError)
                {
                    return directPath.lexically_normal();
                }

                const std::filesystem::path gamePath =
                    workingDirectory / L"game" / requestedPath;

                filesystemError.clear();

                if (std::filesystem::is_regular_file(
                        gamePath,
                        filesystemError) &&
                    !filesystemError)
                {
                    return gamePath.lexically_normal();
                }

                return directPath.lexically_normal();
            }
            catch (...)
            {
                return requestedPath;
            }
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
                    return engine::assets::AssetResult::IoError;
                }

                if (fileSize == 0U ||
                    fileSize > MaximumPreviewAssetSize ||
                    fileSize >
                        static_cast<std::uintmax_t>(
                            (std::numeric_limits<std::streamsize>::max)()))
                {
                    return engine::assets::AssetResult::FileTooLarge;
                }

                const auto resizeResult =
                    output.Resize(
                        static_cast<std::size_t>(fileSize));

                if (engine::assets::Failed(resizeResult))
                {
                    return resizeResult;
                }

                std::ifstream input(
                    path,
                    std::ios::binary);

                if (!input)
                {
                    output.Clear();

                    return engine::assets::AssetResult::IoError;
                }

                input.read(
                    reinterpret_cast<char*>(
                        output.GetData()),
                    static_cast<std::streamsize>(
                        fileSize));

                if (!input)
                {
                    output.Clear();

                    return engine::assets::AssetResult::IoError;
                }

                return engine::assets::AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                output.Clear();

                return engine::assets::AssetResult::OutOfMemory;
            }
            catch (...)
            {
                output.Clear();

                return engine::assets::AssetResult::InternalError;
            }
        }

        [[nodiscard]]
        engine::assets::AssetResult CreateAssetMetadata(
            const std::filesystem::path& requestedPath,
            const std::size_t sourceSize,
            const engine::assets::AssetType assetType,
            engine::assets::AssetMetadata& metadata) noexcept
        {
            try
            {
                std::filesystem::path logicalPath =
                    requestedPath;

                if (logicalPath.is_absolute())
                {
                    std::error_code currentPathError;

                    const std::filesystem::path workingDirectory =
                        std::filesystem::current_path(
                            currentPathError);

                    if (!currentPathError)
                    {
                        std::error_code relativeError;

                        const std::filesystem::path relativePath =
                            std::filesystem::relative(
                                logicalPath,
                                workingDirectory,
                                relativeError);

                        if (!relativeError &&
                            !relativePath.empty())
                        {
                            logicalPath = relativePath;
                        }
                    }

                    if (logicalPath.is_absolute())
                    {
                        logicalPath =
                            logicalPath.filename();
                    }
                }

                const std::string logicalName =
                    logicalPath
                        .lexically_normal()
                        .generic_u8string();

                engine::assets::AssetPath assetPath;

                const auto pathResult =
                    engine::assets::AssetPath::TryCreate(
                        logicalName,
                        assetPath);

                if (engine::assets::Failed(pathResult))
                {
                    return pathResult;
                }

                metadata = {};
                metadata.path = std::move(assetPath);
                metadata.id = metadata.path.GetId();
                metadata.type = assetType;
                metadata.schemaVersion = 1U;
                metadata.sourceSize =
                    static_cast<std::uint64_t>(
                        sourceSize);

                return engine::assets::AssetResult::Success;
            }
            catch (const std::bad_alloc&)
            {
                return engine::assets::AssetResult::OutOfMemory;
            }
            catch (...)
            {
                return engine::assets::AssetResult::InternalError;
            }
        }

        [[nodiscard]]
        bool LoadSkeletonFile(
            const std::filesystem::path& requestedPath,
            engine::assets::SkeletonAsset& output,
            std::string& errorMessage) noexcept
        {
            output.Clear();

            const std::filesystem::path path =
                ResolveAssetPath(requestedPath);

            engine::assets::AssetData source;

            const auto readResult =
                ReadAssetData(path, source);

            if (engine::assets::Failed(readResult))
            {
                errorMessage =
                    "Failed to read skeleton file: ";

                errorMessage +=
                    path.generic_u8string();

                errorMessage += " (";
                errorMessage +=
                    engine::assets::ToString(readResult);
                errorMessage += ')';

                return false;
            }

            engine::assets::AssetMetadata metadata;

            const auto metadataResult =
                CreateAssetMetadata(
                    path,
                    source.GetSize(),
                    engine::assets::AssetType::Skeleton,
                    metadata);

            if (engine::assets::Failed(metadataResult))
            {
                errorMessage =
                    "Failed to create skeleton metadata: ";

                errorMessage +=
                    engine::assets::ToString(
                        metadataResult);

                return false;
            }

            engine::assets::SkeletonAssetLoader loader;

            std::unique_ptr<engine::assets::LoadedAsset>
                loadedAsset;

            const auto loadResult =
                loader.Load(
                    metadata,
                    source,
                    loadedAsset);

            if (engine::assets::Failed(loadResult) ||
                loadedAsset == nullptr ||
                loadedAsset->GetType() !=
                    engine::assets::AssetType::Skeleton)
            {
                errorMessage =
                    "Failed to parse skeleton file: ";

                errorMessage +=
                    path.generic_u8string();

                errorMessage += " (";
                errorMessage +=
                    engine::assets::ToString(loadResult);
                errorMessage += ')';

                return false;
            }

            auto* const skeletonAsset =
                static_cast<
                    engine::assets::SkeletonLoadedAsset*>(
                        loadedAsset.get());

            output =
                skeletonAsset->ReleaseSkeleton();

            if (!output.IsValid())
            {
                errorMessage =
                    "Skeleton file contains invalid data: ";

                errorMessage +=
                    path.generic_u8string();

                output.Clear();
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool LoadSkeletalMeshFile(
            const std::filesystem::path& requestedPath,
            engine::assets::SkeletalMeshAsset& output,
            std::string& errorMessage) noexcept
        {
            output.Clear();

            const std::filesystem::path path =
                ResolveAssetPath(requestedPath);

            engine::assets::AssetData source;

            const auto readResult =
                ReadAssetData(path, source);

            if (engine::assets::Failed(readResult))
            {
                errorMessage =
                    "Failed to read skeletal mesh: ";

                errorMessage +=
                    path.generic_u8string();

                errorMessage += " (";
                errorMessage +=
                    engine::assets::ToString(readResult);
                errorMessage += ')';

                return false;
            }

            engine::assets::AssetMetadata metadata;

            const auto metadataResult =
                CreateAssetMetadata(
                    path,
                    source.GetSize(),
                    engine::assets::AssetType::SkeletalMesh,
                    metadata);

            if (engine::assets::Failed(metadataResult))
            {
                errorMessage =
                    "Failed to create skeletal mesh metadata: ";

                errorMessage +=
                    engine::assets::ToString(
                        metadataResult);

                return false;
            }

            engine::assets::SkeletalMeshAssetLoader loader;

            std::unique_ptr<engine::assets::LoadedAsset>
                loadedAsset;

            const auto loadResult =
                loader.Load(
                    metadata,
                    source,
                    loadedAsset);

            if (engine::assets::Failed(loadResult) ||
                loadedAsset == nullptr ||
                loadedAsset->GetType() !=
                    engine::assets::AssetType::SkeletalMesh)
            {
                errorMessage =
                    "Failed to parse skeletal mesh: ";

                errorMessage +=
                    path.generic_u8string();

                errorMessage += " (";
                errorMessage +=
                    engine::assets::ToString(loadResult);
                errorMessage += ')';

                return false;
            }

            auto* const skeletalMeshAsset =
                static_cast<
                    engine::assets::SkeletalMeshLoadedAsset*>(
                        loadedAsset.get());

            output =
                skeletalMeshAsset->ReleaseSkeletalMesh();

            if (!output.IsValid())
            {
                errorMessage =
                    "Skeletal mesh contains invalid data: ";

                errorMessage +=
                    path.generic_u8string();

                output.Clear();
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool ValidateBoneIndices(
            const engine::assets::SkeletalMeshAsset& mesh,
            const engine::assets::SkeletonAsset& skeleton,
            std::string& errorMessage)
        {
            const auto* const vertices =
                mesh.GetVertexData();

            const std::size_t vertexCount =
                mesh.GetVertexCount();

            const std::size_t boneCount =
                skeleton.GetBoneCount();

            if (vertices == nullptr ||
                vertexCount == 0U)
            {
                errorMessage =
                    "Skeletal mesh has no vertex data.";

                return false;
            }

            for (std::size_t vertexIndex = 0U;
                 vertexIndex < vertexCount;
                 ++vertexIndex)
            {
                const auto& vertex =
                    vertices[vertexIndex];

                for (std::size_t influenceIndex = 0U;
                     influenceIndex <
                         vertex.boneIndices.size();
                     ++influenceIndex)
                {
                    if (vertex.boneWeights[influenceIndex] <=
                        0.0F)
                    {
                        continue;
                    }

                    const std::size_t boneIndex =
                        vertex.boneIndices[influenceIndex];

                    if (boneIndex >= boneCount)
                    {
                        std::ostringstream message;

                        message
                            << "Skeletal mesh vertex "
                            << vertexIndex
                            << " references bone "
                            << boneIndex
                            << ", but skeleton contains only "
                            << boneCount
                            << " bones.";

                        errorMessage =
                            message.str();

                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]]
        DirectX::XMMATRIX BuildTransformMatrix(
            const CharacterTransform& transform) noexcept
        {
            const DirectX::XMMATRIX scale =
                DirectX::XMMatrixScaling(
                    transform.scale.x,
                    transform.scale.y,
                    transform.scale.z);

            const DirectX::XMMATRIX rotation =
                DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(
                        transform.rotation.x),
                    DirectX::XMConvertToRadians(
                        transform.rotation.y),
                    DirectX::XMConvertToRadians(
                        transform.rotation.z));

            const DirectX::XMMATRIX translation =
                DirectX::XMMatrixTranslation(
                    transform.position.x,
                    transform.position.y,
                    transform.position.z);

            return scale * rotation * translation;
        }

        void LogGraphicsFailure(
            const char* operation,
            const engine::graphics::GraphicsResult result) noexcept
        {
            std::string message =
                operation != nullptr
                    ? operation
                    : "Character preview graphics operation";

            message += " failed: ";
            message +=
                engine::graphics::ToString(result);

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Error,
                PreviewLogCategory,
                message);
        }
    }

    class CharacterPreviewRenderer::Impl final
    {
        struct PreviewMesh final
        {
            std::filesystem::path sourcePath;

            std::unique_ptr<
                engine::assets::GpuSkeletalMesh>
                gpuMesh;

            DirectX::XMFLOAT4X4 world;

            DirectX::XMFLOAT4 color
            {
                0.65F,
                0.67F,
                0.70F,
                1.0F
            };

            bool skinned = true;
        };

    public:
        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept
        {
            if (initialized_)
            {
                return device_ == &device;
            }

            if (device.GetBackend() !=
                engine::graphics::GraphicsBackend::D3D11)
            {
                status_ =
                    "Character Preview requires D3D11.";

                return false;
            }

            device_ = &device;

            Microsoft::WRL::ComPtr<ID3DBlob>
                vertexBytecode;

            Microsoft::WRL::ComPtr<ID3DBlob>
                pixelBytecode;

            if (!CompileEditorShaderFile(
                    L"CharacterPreview.hlsl",
                    "VSMain",
                    "vs_5_0",
                    PreviewLogCategory,
                    vertexBytecode))
            {
                status_ =
                    "Failed to compile Character Preview vertex shader.";

                device_ = nullptr;
                return false;
            }

            if (!CompileEditorShaderFile(
                    L"CharacterPreview.hlsl",
                    "PSMain",
                    "ps_5_0",
                    PreviewLogCategory,
                    pixelBytecode))
            {
                status_ =
                    "Failed to compile Character Preview pixel shader.";

                device_ = nullptr;
                return false;
            }

            engine::graphics::ShaderDesc
                vertexShaderDescription;

            vertexShaderDescription.stage =
                engine::graphics::ShaderStage::Vertex;

            vertexShaderDescription.bytecode.data =
                vertexBytecode->GetBufferPointer();

            vertexShaderDescription.bytecode.size =
                vertexBytecode->GetBufferSize();

            vertexShaderDescription.debugName =
                "CharacterPreview.VertexShader";

            auto graphicsResult =
                device.CreateShader(
                    vertexShaderDescription,
                    vertexShader_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview vertex shader",
                    graphicsResult);

                status_ =
                    "Failed to create Character Preview vertex shader.";

                Shutdown(device);
                return false;
            }

            engine::graphics::ShaderDesc
                pixelShaderDescription;

            pixelShaderDescription.stage =
                engine::graphics::ShaderStage::Pixel;

            pixelShaderDescription.bytecode.data =
                pixelBytecode->GetBufferPointer();

            pixelShaderDescription.bytecode.size =
                pixelBytecode->GetBufferSize();

            pixelShaderDescription.debugName =
                "CharacterPreview.PixelShader";

            graphicsResult =
                device.CreateShader(
                    pixelShaderDescription,
                    pixelShader_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview pixel shader",
                    graphicsResult);

                status_ =
                    "Failed to create Character Preview pixel shader.";

                Shutdown(device);
                return false;
            }

            const std::array<
                engine::graphics::VertexElementDesc,
                4U> inputElements
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
                    "NORMAL",
                    0U,
                    engine::graphics::Format::R32G32B32Float,
                    0U,
                    12U,
                    engine::graphics::VertexInputRate::PerVertex,
                    0U
                },
                {
                    "BLENDINDICES",
                    0U,
                    engine::graphics::Format::R8G8B8A8UInt,
                    0U,
                    48U,
                    engine::graphics::VertexInputRate::PerVertex,
                    0U
                },
                {
                    "BLENDWEIGHT",
                    0U,
                    engine::graphics::Format::R32G32B32A32Float,
                    0U,
                    52U,
                    engine::graphics::VertexInputRate::PerVertex,
                    0U
                }
            }};

            engine::graphics::InputLayoutDesc
                inputLayoutDescription;

            inputLayoutDescription.vertexShader =
                vertexShader_;

            inputLayoutDescription.elements =
                inputElements.data();

            inputLayoutDescription.elementCount =
                inputElements.size();

            inputLayoutDescription.debugName =
                "CharacterPreview.InputLayout";

            graphicsResult =
                device.CreateInputLayout(
                    inputLayoutDescription,
                    inputLayout_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview input layout",
                    graphicsResult);

                status_ =
                    "Failed to create Character Preview input layout.";

                Shutdown(device);
                return false;
            }

            engine::graphics::BufferDesc
                constantBufferDescription;

            constantBufferDescription.byteSize =
                sizeof(PreviewConstants);

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

            graphicsResult =
                device.CreateBuffer(
                    constantBufferDescription,
                    nullptr,
                    constantBuffer_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview constant buffer",
                    graphicsResult);

                status_ =
                    "Failed to create Character Preview constant buffer.";

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
                engine::graphics::PrimitiveTopology::TriangleList;

            pipelineDescription.rasterizer.fillMode =
                engine::graphics::FillMode::Solid;

            /*
             * Пока отключаем culling, потому что старые и новые
             * экспортированные SKM могут иметь разный winding.
             */
            pipelineDescription.rasterizer.cullMode =
                engine::graphics::CullMode::None;

            pipelineDescription.rasterizer.depthClipEnable =
                true;

            pipelineDescription.blend.renderTargets[0]
                .blendEnable = false;

            pipelineDescription.depthStencil.depthEnable =
                true;

            pipelineDescription.depthStencil.depthWriteEnable =
                true;

            pipelineDescription.depthStencil.depthFunction =
                engine::graphics::ComparisonFunction::LessEqual;

            pipelineDescription.debugName =
                "CharacterPreview.Pipeline";

            graphicsResult =
                device.CreateGraphicsPipeline(
                    pipelineDescription,
                    pipeline_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview pipeline",
                    graphicsResult);

                status_ =
                    "Failed to create Character Preview pipeline.";

                Shutdown(device);
                return false;
            }

            initialized_ = true;
            status_ =
                "Character Preview renderer initialized.";

            engine::core::GetLogger().Write(
                engine::core::LogLevel::Information,
                PreviewLogCategory,
                status_);

            return true;
        }

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept
        {
            ReleaseCharacter(device);
            DestroyRenderTargets(device);

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

            if (constantBuffer_.IsValid())
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        constantBuffer_));

                constantBuffer_ = {};
            }

            initialized_ = false;
            characterLoaded_ = false;
            device_ = nullptr;
            status_ =
                "Character Preview renderer stopped.";
        }

        [[nodiscard]]
        bool LoadCharacter(
            engine::graphics::RenderDevice& device,
            const CharacterDefinition& character) noexcept
        {
            if (!initialized_ ||
                device_ != &device)
            {
                status_ =
                    "Character Preview renderer is not initialized.";

                return false;
            }

            ReleaseCharacter(device);

            if (character.bodySkeletonFile.empty())
            {
                status_ =
                    "Select body.skeleton before loading preview.";

                return false;
            }

            std::string loadError;

            if (!LoadSkeletonFile(
                    character.bodySkeletonFile,
                    skeleton_,
                    loadError))
            {
                status_ =
                    std::move(loadError);

                return false;
            }

            if (!pose_.Initialize(skeleton_))
            {
                status_ = "Failed to build bind pose from skeleton.";

                ReleaseCharacter(device);
                return false;
            }

            focusCenter_ =
            {
                0.0F,
                0.0F,
                0.0F
            };

            focusRadius_ = 1.0F;
            focusResolved_ = false;

            static constexpr std::array<
                DirectX::XMFLOAT4,
                4U> moduleColors
            {{
                {0.76F, 0.62F, 0.52F, 1.0F},
                {0.38F, 0.43F, 0.46F, 1.0F},
                {0.24F, 0.28F, 0.31F, 1.0F},
                {0.16F, 0.17F, 0.18F, 1.0F}
            }};

            for (std::size_t index = 0U;
                 index < character.modules.size();
                 ++index)
            {
                const CharacterMeshSlot& slot =
                    character.modules[index];

                const bool preferForFocus =
                    index ==
                    static_cast<std::size_t>(
                        CharacterModuleType::Body);

                if (!AddMesh(
                        device,
                        slot.meshFile,
                        slot.visible,
                        DirectX::XMMatrixIdentity(),
                        moduleColors[index],
                        preferForFocus))
                {
                    ReleaseCharacter(device);
                    return false;
                }
            }

            static constexpr std::array<
                DirectX::XMFLOAT4,
                3U> armorColors
            {{
                {0.25F, 0.29F, 0.24F, 1.0F},
                {0.20F, 0.24F, 0.22F, 1.0F},
                {0.31F, 0.34F, 0.27F, 1.0F}
            }};

            for (std::size_t index = 0U;
                 index < character.armor.size();
                 ++index)
            {
                const CharacterArmorSlot& slot =
                    character.armor[index];

                if (!slot.visible ||
                    slot.meshFile.empty())
                {
                    continue;
                }

                DirectX::XMMATRIX world;

                if (!BuildAttachmentWorld(
                        slot.attachmentBone,
                        slot.localTransform,
                        world))
                {
                    ReleaseCharacter(device);
                    return false;
                }

                if (!AddMesh(
                        device,
                        slot.meshFile,
                        true,
                        world,
                        armorColors[index],
                        false))
                {
                    ReleaseCharacter(device);
                    return false;
                }
            }

            static constexpr std::array<
                DirectX::XMFLOAT4,
                2U> weaponColors
            {{
                {0.30F, 0.31F, 0.32F, 1.0F},
                {0.24F, 0.25F, 0.27F, 1.0F}
            }};

            for (std::size_t index = 0U;
                 index < character.weapons.size();
                 ++index)
            {
                const CharacterWeapon& weapon =
                    character.weapons[index];

                if (!weapon.visible ||
                    weapon.meshFile.empty())
                {
                    continue;
                }

                DirectX::XMMATRIX world;

                if (!BuildAttachmentWorld(
                        weapon.ik.attachmentBone,
                        weapon.ik.weaponTransform,
                        world))
                {
                    ReleaseCharacter(device);
                    return false;
                }

                if (!AddMesh(
                        device,
                        weapon.meshFile,
                        true,
                        world,
                        weaponColors[index],
                        false))
                {
                    ReleaseCharacter(device);
                    return false;
                }
            }

            characterLoaded_ = true;

            std::ostringstream message;

            message
                << "Preview loaded: "
                << meshes_.size()
                << " mesh(es), "
                << skeleton_.GetBoneCount()
                << " bones.";

            status_ =
                message.str();

            return true;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Resize(
            engine::graphics::RenderDevice& device,
            std::uint32_t width,
            std::uint32_t height) noexcept
        {
            if (!initialized_ ||
                device_ != &device)
            {
                return engine::graphics::GraphicsResult::InvalidState;
            }

            width =
                std::clamp(
                    width,
                    MinimumPreviewSize,
                    MaximumPreviewSize);

            height =
                std::clamp(
                    height,
                    MinimumPreviewSize,
                    MaximumPreviewSize);

            if (colorTarget_.IsValid() &&
                depthTarget_.IsValid() &&
                width_ == width &&
                height_ == height)
            {
                return engine::graphics::GraphicsResult::Success;
            }

            DestroyRenderTargets(device);

            engine::graphics::TextureDesc
                colorDescription;

            colorDescription.dimension =
                engine::graphics::TextureDimension::Texture2D;

            colorDescription.width = width;
            colorDescription.height = height;
            colorDescription.depth = 1U;
            colorDescription.arrayLayers = 1U;
            colorDescription.mipLevels = 1U;
            colorDescription.sampleCount = 1U;

            colorDescription.format =
                engine::graphics::Format::R8G8B8A8UNorm;

            colorDescription.usage =
                engine::graphics::ResourceUsage::Default;

            colorDescription.bindFlags =
                engine::graphics::TextureBindFlags::RenderTarget |
                engine::graphics::TextureBindFlags::ShaderResource;

            colorDescription.cpuAccess =
                engine::graphics::CpuAccessFlags::None;

            auto graphicsResult =
                device.CreateTexture(
                    colorDescription,
                    nullptr,
                    0U,
                    colorTarget_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview color target",
                    graphicsResult);

                colorTarget_ = {};
                return graphicsResult;
            }

            engine::graphics::TextureDesc
                depthDescription;

            depthDescription.dimension =
                engine::graphics::TextureDimension::Texture2D;

            depthDescription.width = width;
            depthDescription.height = height;
            depthDescription.depth = 1U;
            depthDescription.arrayLayers = 1U;
            depthDescription.mipLevels = 1U;
            depthDescription.sampleCount = 1U;

            depthDescription.format =
                engine::graphics::Format::D32Float;

            depthDescription.usage =
                engine::graphics::ResourceUsage::Default;

            depthDescription.bindFlags =
                engine::graphics::TextureBindFlags::DepthStencil;

            depthDescription.cpuAccess =
                engine::graphics::CpuAccessFlags::None;

            graphicsResult =
                device.CreateTexture(
                    depthDescription,
                    nullptr,
                    0U,
                    depthTarget_);

            if (engine::graphics::Failed(graphicsResult))
            {
                LogGraphicsFailure(
                    "Create Character Preview depth target",
                    graphicsResult);

                static_cast<void>(
                    device.DestroyTexture(
                        colorTarget_));

                colorTarget_ = {};
                depthTarget_ = {};

                return graphicsResult;
            }

            width_ = width;
            height_ = height;

            return engine::graphics::GraphicsResult::Success;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const float yawDegrees,
            const float pitchDegrees,
            const float distanceMultiplier) noexcept
        {
            if (!initialized_ ||
                !colorTarget_.IsValid() ||
                !depthTarget_.IsValid() ||
                width_ == 0U ||
                height_ == 0U)
            {
                return engine::graphics::GraphicsResult::InvalidState;
            }

            if (!animationPlayer_.Evaluate(pose_))
            {
                status_ =
                    "Failed to evaluate Character Pose.";

                return engine::graphics::GraphicsResult::InvalidState;
            }

            auto graphicsResult =
                context.SetRenderTargets(
                    &colorTarget_,
                    1U,
                    depthTarget_);

            if (engine::graphics::Failed(graphicsResult))
            {
                return graphicsResult;
            }

            const auto finishRendering =
                [&context]() noexcept
                {
                    context.UnbindIndexBuffer();

                    static_cast<void>(
                        context.UnbindConstantBuffers(
                            engine::graphics::ShaderStage::Vertex,
                            0U,
                            1U));

                    static_cast<void>(
                        context.UnbindConstantBuffers(
                            engine::graphics::ShaderStage::Pixel,
                            0U,
                            1U));

                    context.UnbindGraphicsPipeline();
                    context.UnbindRenderTargets();
                };

            engine::graphics::Viewport viewport;

            viewport.x = 0.0F;
            viewport.y = 0.0F;
            viewport.width =
                static_cast<float>(width_);

            viewport.height =
                static_cast<float>(height_);

            viewport.minDepth = 0.0F;
            viewport.maxDepth = 1.0F;

            graphicsResult =
                context.SetViewport(viewport);

            if (engine::graphics::Failed(graphicsResult))
            {
                finishRendering();
                return graphicsResult;
            }

            engine::graphics::ClearColor clearColor;

            clearColor.red = 0.045F;
            clearColor.green = 0.052F;
            clearColor.blue = 0.060F;
            clearColor.alpha = 1.0F;

            graphicsResult =
                context.ClearColorTarget(
                    colorTarget_,
                    clearColor);

            if (engine::graphics::Failed(graphicsResult))
            {
                finishRendering();
                return graphicsResult;
            }

            graphicsResult =
                context.ClearDepthStencilTarget(
                    depthTarget_,
                    engine::graphics::ClearDepthStencilFlags::Depth,
                    1.0F,
                    0U);

            if (engine::graphics::Failed(graphicsResult))
            {
                finishRendering();
                return graphicsResult;
            }

            if (!characterLoaded_ ||
                meshes_.empty())
            {
                finishRendering();

                return engine::graphics::GraphicsResult::Success;
            }

            const float safeRadius =
                std::max(
                    focusRadius_,
                    0.01F);

            const float safeDistanceMultiplier =
                std::clamp(
                    distanceMultiplier,
                    0.5F,
                    20.0F);

            const float cameraDistance =
                std::max(
                    safeRadius *
                        safeDistanceMultiplier,
                    safeRadius * 1.25F);

            const float yaw =
                DirectX::XMConvertToRadians(
                    yawDegrees);

            const float pitch =
                DirectX::XMConvertToRadians(
                    std::clamp(
                        pitchDegrees,
                        -80.0F,
                        80.0F));

            const float cosPitch =
                std::cos(pitch);

            const DirectX::XMVECTOR target =
                DirectX::XMVectorSet(
                    focusCenter_.x,
                    focusCenter_.y,
                    focusCenter_.z,
                    1.0F);

            const DirectX::XMVECTOR offset =
                DirectX::XMVectorSet(
                    std::sin(yaw) *
                        cosPitch *
                        cameraDistance,

                    std::sin(pitch) *
                        cameraDistance,

                    std::cos(yaw) *
                        cosPitch *
                        cameraDistance,

                    0.0F);

            const DirectX::XMVECTOR eye =
                DirectX::XMVectorAdd(
                    target,
                    offset);

            const DirectX::XMMATRIX view =
                DirectX::XMMatrixLookAtLH(
                    eye,
                    target,
                    DirectX::XMVectorSet(
                        0.0F,
                        1.0F,
                        0.0F,
                        0.0F));

            const float aspectRatio =
                static_cast<float>(width_) /
                static_cast<float>(height_);

            const float nearPlane =
                std::max(
                    safeRadius * 0.001F,
                    0.01F);

            const float farPlane =
                std::max(
                    cameraDistance +
                        safeRadius * 20.0F,
                    1000.0F);

            const DirectX::XMMATRIX projection =
                DirectX::XMMatrixPerspectiveFovLH(
                    DirectX::XMConvertToRadians(
                        45.0F),
                    aspectRatio,
                    nearPlane,
                    farPlane);

            DirectX::XMFLOAT4X4
                viewProjection;

            DirectX::XMStoreFloat4x4(
                &viewProjection,
                view * projection);

            graphicsResult =
                context.SetGraphicsPipeline(
                    pipeline_);

            if (engine::graphics::Failed(graphicsResult))
            {
                finishRendering();
                return graphicsResult;
            }

            graphicsResult =
                context.SetConstantBuffers(
                    engine::graphics::ShaderStage::Vertex,
                    0U,
                    &constantBuffer_,
                    1U);

            if (engine::graphics::Failed(graphicsResult))
            {
                finishRendering();
                return graphicsResult;
            }

            graphicsResult =
                context.SetConstantBuffers(
                    engine::graphics::ShaderStage::Pixel,
                    0U,
                    &constantBuffer_,
                    1U);

            if (engine::graphics::Failed(graphicsResult))
            {
                finishRendering();
                return graphicsResult;
            }

            for (const PreviewMesh& mesh : meshes_)
            {
                if (mesh.gpuMesh == nullptr ||
                    !mesh.gpuMesh->IsValid())
                {
                    continue;
                }

                PreviewConstants constants;

                constants.world =
                    mesh.world;

                constants.viewProjection =
                    viewProjection;

                constants.baseColor =
                    mesh.color;

                constants.lightDirection =
                {
                    -0.35F,
                    0.80F,
                    -0.45F,
                    1.0F
                };

                constants.ambientColor =
                {
                    0.20F,
                    0.22F,
                    0.25F,
                    1.0F
                };

                constants.renderParameters =
{
                    mesh.skinned
                        ? 1.0F
                        : 0.0F,

                    0.0F,
                    0.0F,
                    0.0F
                };

                graphicsResult =
                    context.UpdateBuffer(
                        constantBuffer_,
                        &constants,
                        sizeof(constants));

                if (engine::graphics::Failed(graphicsResult))
                {
                    break;
                }

                engine::graphics::VertexBufferBinding
                    vertexBinding;

                vertexBinding.buffer =
                    mesh.gpuMesh->GetVertexBuffer();

                vertexBinding.stride =
                    mesh.gpuMesh->GetVertexStride();

                vertexBinding.offset = 0U;

                graphicsResult =
                    context.SetVertexBuffers(
                        0U,
                        &vertexBinding,
                        1U);

                if (engine::graphics::Failed(graphicsResult))
                {
                    break;
                }

                engine::graphics::IndexBufferBinding
                    indexBinding;

                indexBinding.buffer =
                    mesh.gpuMesh->GetIndexBuffer();

                indexBinding.offset = 0U;

                graphicsResult =
                    context.SetIndexBuffer(
                        indexBinding);

                if (engine::graphics::Failed(graphicsResult))
                {
                    break;
                }

                for (std::size_t sectionIndex = 0U;
                     sectionIndex <
                         mesh.gpuMesh->GetSectionCount();
                     ++sectionIndex)
                {
                    const auto* const section =
                        mesh.gpuMesh->GetSection(
                            sectionIndex);

                    if (section == nullptr ||
                        section->indexCount == 0U)
                    {
                        continue;
                    }

                    graphicsResult =
                        context.DrawIndexed(
                            section->indexCount,
                            section->firstIndex,
                            0);

                    if (engine::graphics::Failed(
                            graphicsResult))
                    {
                        break;
                    }
                }

                if (engine::graphics::Failed(
                        graphicsResult))
                {
                    break;
                }
            }

            finishRendering();

            return graphicsResult;
        }

        [[nodiscard]]
        void* GetImGuiTextureId(
            const engine::graphics::RenderDevice& device) const noexcept
        {
            if (!colorTarget_.IsValid() ||
                device.GetBackend() !=
                    engine::graphics::GraphicsBackend::D3D11)
            {
                return nullptr;
            }

            const auto& d3d11Device =
                static_cast<
                    const engine::graphics::d3d11::D3D11Device&>(
                        device);

            return reinterpret_cast<void*>(
                d3d11Device.GetTextureShaderResourceView(
                    colorTarget_));
        }

        [[nodiscard]]
        bool IsInitialized() const noexcept
        {
            return initialized_;
        }

        [[nodiscard]]
        bool HasCharacter() const noexcept
        {
            return characterLoaded_;
        }

        [[nodiscard]]
        const std::string& GetStatus() const noexcept
        {
            return status_;
        }

    private:
        [[nodiscard]]
        bool BuildAttachmentWorld(
            const std::string& attachmentBone,
            const CharacterTransform& localTransform,
            DirectX::XMMATRIX& output) noexcept
        {
            if (attachmentBone.empty())
            {
                status_ =
                    "Attachment bone name is empty.";

                return false;
            }

            const engine::assets::SkeletonBone*
                selectedBone = nullptr;

            for (std::size_t index = 0U;
                 index < skeleton_.GetBoneCount();
                 ++index)
            {
                const auto* const bone =
                    skeleton_.GetBone(index);

                if (bone != nullptr &&
                    bone->name == attachmentBone)
                {
                    selectedBone = bone;
                    break;
                }
            }

            if (selectedBone == nullptr)
            {
                status_ =
                    "Attachment bone not found in skeleton: ";

                status_ +=
                    attachmentBone;

                return false;
            }

            DirectX::XMFLOAT4X4
                bindMatrix;

            static_assert(
                sizeof(bindMatrix) ==
                sizeof(selectedBone->absoluteBindMatrix));

            std::memcpy(
                &bindMatrix,
                selectedBone
                    ->absoluteBindMatrix
                    .data(),
                sizeof(bindMatrix));

            const DirectX::XMMATRIX boneWorld =
                DirectX::XMLoadFloat4x4(
                    &bindMatrix);

            output =
                BuildTransformMatrix(
                    localTransform) *
                boneWorld;

            return true;
        }

        [[nodiscard]]
        bool AddMesh(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& requestedPath,
            const bool visible,
            DirectX::FXMMATRIX world,
            const DirectX::XMFLOAT4& color,
            const bool preferForFocus) noexcept
        {
            if (!visible ||
                requestedPath.empty())
            {
                return true;
            }

            engine::assets::SkeletalMeshAsset
                skeletalMesh;

            std::string loadError;

            if (!LoadSkeletalMeshFile(
                    requestedPath,
                    skeletalMesh,
                    loadError))
            {
                status_ =
                    std::move(loadError);

                return false;
            }

            if (!ValidateBoneIndices(
                    skeletalMesh,
                    skeleton_,
                    loadError))
            {
                status_ =
                    requestedPath.generic_u8string();

                status_ += ": ";
                status_ +=
                    loadError;

                return false;
            }

            auto gpuMesh =
                std::make_unique<
                    engine::assets::GpuSkeletalMesh>();

            const auto uploadResult =
                gpuMesh->Upload(
                    device,
                    skeletalMesh);

            if (engine::graphics::Failed(
                    uploadResult))
            {
                status_ =
                    "Failed to upload skeletal mesh to GPU: ";

                status_ +=
                    requestedPath.generic_u8string();

                status_ += " (";
                status_ +=
                    engine::graphics::ToString(
                        uploadResult);

                status_ += ')';

                return false;
            }

            const auto& bounds =
                skeletalMesh.GetBounds();

            if (bounds.IsValid() &&
                (preferForFocus ||
                 !focusResolved_))
            {
                focusCenter_ =
                {
                    bounds.sphereCenter[0],
                    bounds.sphereCenter[1],
                    bounds.sphereCenter[2]
                };

                focusRadius_ =
                    std::max(
                        bounds.sphereRadius,
                        0.01F);

                focusResolved_ = true;
            }

            PreviewMesh previewMesh;

            previewMesh.sourcePath =
                ResolveAssetPath(
                    requestedPath);

            previewMesh.gpuMesh =
                std::move(gpuMesh);

            previewMesh.color =
                color;

            DirectX::XMStoreFloat4x4(
                &previewMesh.world,
                world);

            meshes_.push_back(
                std::move(previewMesh));

            return true;
        }

        void ReleaseCharacter(
            engine::graphics::RenderDevice& device) noexcept
        {
            for (PreviewMesh& mesh : meshes_)
            {
                if (mesh.gpuMesh != nullptr &&
                    mesh.gpuMesh->IsValid())
                {
                    static_cast<void>(
                        mesh.gpuMesh->Release(
                            device));
                }
            }

            meshes_.clear();
            skeleton_.Clear();

            characterLoaded_ = false;
            focusResolved_ = false;

            focusCenter_ =
            {
                0.0F,
                0.0F,
                0.0F
            };

            focusRadius_ = 1.0F;
        }

        void DestroyRenderTargets(
            engine::graphics::RenderDevice& device) noexcept
        {
            if (depthTarget_.IsValid())
            {
                static_cast<void>(
                    device.DestroyTexture(
                        depthTarget_));

                depthTarget_ = {};
            }

            if (colorTarget_.IsValid())
            {
                static_cast<void>(
                    device.DestroyTexture(
                        colorTarget_));

                colorTarget_ = {};
            }

            width_ = 0U;
            height_ = 0U;
        }

        engine::graphics::RenderDevice*
            device_ = nullptr;

        engine::graphics::ShaderHandle
            vertexShader_;

        engine::graphics::ShaderHandle
            pixelShader_;

        engine::graphics::InputLayoutHandle
            inputLayout_;

        engine::graphics::PipelineStateHandle
            pipeline_;

        engine::graphics::BufferHandle
            constantBuffer_;

        engine::graphics::TextureHandle
            colorTarget_;

        engine::graphics::TextureHandle
            depthTarget_;

        engine::assets::SkeletonAsset
            skeleton_;

        std::vector<PreviewMesh>
            meshes_;

        engine::graphics::BufferHandle
            bonePaletteBuffer_;

        DirectX::XMFLOAT3 focusCenter_
        {
            0.0F,
            0.0F,
            0.0F
        };

        float focusRadius_ = 1.0F;

        std::uint32_t width_ = 0U;
        std::uint32_t height_ = 0U;

        bool initialized_ = false;
        bool characterLoaded_ = false;
        bool focusResolved_ = false;

        std::string status_ =
            "Character Preview is not initialized.";
    };

    CharacterPreviewRenderer::
        CharacterPreviewRenderer() noexcept = default;

    CharacterPreviewRenderer::
        ~CharacterPreviewRenderer() noexcept = default;

    bool CharacterPreviewRenderer::Initialize(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ == nullptr)
        {
            try
            {
                impl_ =
                    std::make_unique<Impl>();
            }
            catch (...)
            {
                return false;
            }
        }

        return impl_->Initialize(device);
    }

    void CharacterPreviewRenderer::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ == nullptr)
        {
            return;
        }

        impl_->Shutdown(device);
        impl_.reset();
    }

    bool CharacterPreviewRenderer::LoadCharacter(
        engine::graphics::RenderDevice& device,
        const CharacterDefinition& character) noexcept
    {
        if (impl_ == nullptr)
        {
            return false;
        }

        return impl_->LoadCharacter(
            device,
            character);
    }

    void CharacterPreviewRenderer::Update(float deltaSeconds) noexcept
    {
        animationPlayer_.Update(deltaSeconds);
    }

    [[nodiscard]]
    bool CharacterPreviewRenderer::SetAnimationClip(std::shared_ptr<const CharacterAnimationClip> clip) noexcept
    {
        return animationPlayer_.SetClip(
        std::move(clip));
    }

    void CharacterPreviewRenderer::ClearAnimationClip() noexcept
    {
        animationPlayer_.ClearClip();
    }

    void CharacterPreviewRenderer::PlayAnimation() noexcept
    {
        animationPlayer_.Play();
    }

    void CharacterPreviewRenderer::PauseAnimation() noexcept
    {
        animationPlayer_.Pause();
    }

    void CharacterPreviewRenderer::StopAnimation() noexcept
    {
        animationPlayer_.Stop();
    }

    void CharacterPreviewRenderer::SetAnimationLooping(bool looping) noexcept
    {
        animationPlayer_.SetLooping(looping);
    }

    void CharacterPreviewRenderer::SetAnimationSpeed(float speed) noexcept
    {
        animationPlayer_.SetPlaybackSpeed(speed);
    }

    engine::graphics::GraphicsResult
    CharacterPreviewRenderer::Resize(
        engine::graphics::RenderDevice& device,
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (impl_ == nullptr)
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        return impl_->Resize(
            device,
            width,
            height);
    }

    engine::graphics::GraphicsResult
    CharacterPreviewRenderer::Render(
        engine::graphics::CommandContext& context,
        const float yawDegrees,
        const float pitchDegrees,
        const float distanceMultiplier) noexcept
    {
        if (impl_ == nullptr)
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        return impl_->Render(
            context,
            yawDegrees,
            pitchDegrees,
            distanceMultiplier);
    }

    void* CharacterPreviewRenderer::GetImGuiTextureId(
        const engine::graphics::RenderDevice& device) const noexcept
    {
        if (impl_ == nullptr)
        {
            return nullptr;
        }

        return impl_->GetImGuiTextureId(
            device);
    }

    bool CharacterPreviewRenderer::IsInitialized() const noexcept
    {
        return impl_ != nullptr &&
            impl_->IsInitialized();
    }

    bool CharacterPreviewRenderer::HasCharacter() const noexcept
    {
        return impl_ != nullptr &&
            impl_->HasCharacter();
    }

    const std::string&
    CharacterPreviewRenderer::GetStatus() const noexcept
    {
        static const std::string emptyStatus;

        return impl_ != nullptr
            ? impl_->GetStatus()
            : emptyStatus;
    }
}