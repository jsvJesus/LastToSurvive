#include "Editor/LevelEditor/Rendering/ModularCharacterRenderer.h"
#include "Editor/LevelEditor/Rendering/CharacterAnimationEvaluator.h"
#include "Editor/LevelEditor/Rendering/ShaderCompiler.h"

#include <Assets/AssetData.h>
#include <Assets/AssetMetadata.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/AnimationAsset.h>
#include <Assets/AnimationAssetLoader.h>
#include <Assets/DdsTextureDecoder.h>
#include <Assets/GpuTexture.h>
#include <Assets/MaterialAsset.h>
#include <Assets/MaterialAssetLoader.h>
#include <Assets/TextureAsset.h>
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
#include <Graphics/Sampler.h>
#include <Graphics/Texture.h>

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uintmax_t
            MaximumSkeletalMeshFileSize =
                512U * 1024U * 1024U;

        /*
         * BodyFPS находится в camera-space.
         *
         * X: небольшое смещение вправо.
         * Y: руки опущены ниже центра камеры.
         * Z: модель отодвинута от near plane,
         *    чтобы руки не занимали весь экран.
         */
        constexpr float
            FirstPersonPresentationRightOffset =
                0.025F;

        constexpr float
            FirstPersonPresentationHeadOffset =
                -0.05F;

        constexpr float
            FirstPersonPresentationForwardOffset =
                0.38F;

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

        struct SkinningConstants final
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
        bool EqualAsciiInsensitive(
            const std::string_view first,
            const std::string_view second) noexcept
        {
            if (first.size() != second.size())
            {
                return false;
            }

            for (
                std::size_t index = 0U;
                index < first.size();
                ++index)
            {
                const unsigned char firstCharacter =
                    static_cast<unsigned char>(
                        first[index]);

                const unsigned char secondCharacter =
                    static_cast<unsigned char>(
                        second[index]);

                if (
                    std::tolower(firstCharacter) !=
                    std::tolower(secondCharacter))
                {
                    return false;
                }
            }

            return true;
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
        std::filesystem::path ResolveDataAssetFile(
            const std::filesystem::path&
                sourceFile,
            const std::string&
                assetPath) noexcept
        {
            try
            {
                if (assetPath.empty())
                {
                    return {};
                }

                const std::filesystem::path requested =
                    std::filesystem::u8path(
                        assetPath);

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

                const std::filesystem::path dataRoot =
                    FindDataRoot(
                        sourceFile.parent_path());

                if (dataRoot.empty())
                {
                    return {};
                }

                /*
                 * Путь без Data/:
                 *
                 * Materials/Characters/test.material
                 * Textures/Characters/test.dds
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
                 * Путь с Data/:
                 *
                 * Data/Materials/...
                 * Data/Textures/...
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
        bool TryResolveBoneModelPosition(
            const engine::assets::SkeletonAsset&
                skeleton,
            const DirectX::XMFLOAT4X4* const
                modelBoneMatrices,
            const std::size_t matrixCount,
            const std::string_view boneName,
            DirectX::XMFLOAT3& position) noexcept
        {
            if (
                modelBoneMatrices == nullptr ||
                boneName.empty())
            {
                return false;
            }

            const std::size_t boneCount =
                (std::min)(
                    matrixCount,
                    skeleton.GetBoneCount());

            for (
                std::size_t boneIndex = 0U;
                boneIndex < boneCount;
                ++boneIndex)
            {
                const engine::assets::SkeletonBone*
                    bone =
                        skeleton.GetBone(
                            boneIndex);

                if (
                    bone == nullptr ||
                    !EqualAsciiInsensitive(
                        bone->name,
                        boneName))
                {
                    continue;
                }

                const DirectX::XMVECTOR
                    modelPosition =
                        DirectX::XMVector3TransformCoord(
                            DirectX::XMVectorZero(),
                            DirectX::XMLoadFloat4x4(
                                &modelBoneMatrices[
                                    boneIndex]));

                DirectX::XMFLOAT3 resolved;

                DirectX::XMStoreFloat3(
                    &resolved,
                    modelPosition);

                if (
                    !std::isfinite(resolved.x) ||
                    !std::isfinite(resolved.y) ||
                    !std::isfinite(resolved.z))
                {
                    return false;
                }

                position = resolved;
                return true;
            }

            return false;
        }

        [[nodiscard]]
        bool TryResolveBindBoneModelPosition(
            const engine::assets::SkeletonAsset&
                skeleton,
            const std::array<float, 3U>& pivot,
            const std::string_view boneName,
            DirectX::XMFLOAT3& position) noexcept
        {
            if (boneName.empty())
            {
                return false;
            }

            const DirectX::XMMATRIX pivotTransform =
                DirectX::XMMatrixTranslation(
                    -pivot[0],
                    -pivot[1],
                    -pivot[2]);

            for (
                std::size_t boneIndex = 0U;
                boneIndex < skeleton.GetBoneCount();
                ++boneIndex)
            {
                const engine::assets::SkeletonBone*
                    bone =
                        skeleton.GetBone(
                            boneIndex);

                if (
                    bone == nullptr ||
                    !EqualAsciiInsensitive(
                        bone->name,
                        boneName))
                {
                    continue;
                }

                const DirectX::XMVECTOR
                    modelPosition =
                        DirectX::XMVector3TransformCoord(
                            DirectX::XMVectorZero(),
                            LoadMatrix(
                                bone->absoluteBindMatrix) *
                                pivotTransform);

                DirectX::XMFLOAT3 resolved;

                DirectX::XMStoreFloat3(
                    &resolved,
                    modelPosition);

                if (
                    !std::isfinite(resolved.x) ||
                    !std::isfinite(resolved.y) ||
                    !std::isfinite(resolved.z))
                {
                    return false;
                }

                position = resolved;
                return true;
            }

            return false;
        }

        [[nodiscard]]
        DirectX::XMMATRIX
            BuildAnimationTrackMatrix(
                const engine::assets::
                    AnimationTrack& track,
                const std::uint32_t firstFrame,
                const std::uint32_t secondFrame,
                const float interpolation) noexcept
        {
            const engine::assets::AnimationKey&
                first =
                    track.keys[firstFrame];

            const engine::assets::AnimationKey&
                second =
                    track.keys[secondFrame];

            DirectX::XMVECTOR firstRotation =
                DirectX::XMVectorSet(
                    first.rotation[0],
                    first.rotation[1],
                    first.rotation[2],
                    first.rotation[3]);

            DirectX::XMVECTOR secondRotation =
                DirectX::XMVectorSet(
                    second.rotation[0],
                    second.rotation[1],
                    second.rotation[2],
                    second.rotation[3]);

            firstRotation =
                DirectX::XMQuaternionNormalize(
                    firstRotation);

            secondRotation =
                DirectX::XMQuaternionNormalize(
                    secondRotation);

            const DirectX::XMVECTOR rotation =
                DirectX::XMQuaternionSlerp(
                    firstRotation,
                    secondRotation,
                    interpolation);

            const float translationX =
                first.translation[0] +
                (
                    second.translation[0] -
                    first.translation[0]
                ) *
                interpolation;

            const float translationY =
                first.translation[1] +
                (
                    second.translation[1] -
                    first.translation[1]
                ) *
                interpolation;

            const float translationZ =
                first.translation[2] +
                (
                    second.translation[2] -
                    first.translation[2]
                ) *
                interpolation;

            DirectX::XMFLOAT4X4 stored;

            DirectX::XMStoreFloat4x4(
                &stored,
                DirectX::XMMatrixRotationQuaternion(
                    rotation));

            stored._41 = translationX;
            stored._42 = translationY;
            stored._43 = translationZ;

            return DirectX::XMLoadFloat4x4(
                &stored);
        }

        [[nodiscard]]
        bool BuildSkinningConstants(
            const engine::assets::SkeletonAsset&
                skeleton,
            const engine::assets::AnimationAsset*
                animation,
            const std::array<float, 3U>& pivot,
            const double elapsedSeconds,
            SkinningConstants& output) noexcept
        {
            if (!skeleton.IsValid())
            {
                return false;
            }

            const bool useAnimation =
                animation != nullptr &&
                animation->IsCompatibleWith(
                    skeleton);

            std::uint32_t firstFrame = 0U;
            std::uint32_t secondFrame = 0U;

            float interpolation = 0.0F;

            if (useAnimation)
            {
                const double frameCount =
                    static_cast<double>(
                        animation->
                            GetFrameCount());

                const double framePosition =
                    std::fmod(
                        (std::max)(
                            elapsedSeconds,
                            0.0) *
                        static_cast<double>(
                            animation->
                                GetFrameRate()),
                        frameCount);

                firstFrame =
                    static_cast<std::uint32_t>(
                        std::floor(
                            framePosition));

                secondFrame =
                    firstFrame + 1U;

                if (
                    secondFrame >=
                    animation->GetFrameCount())
                {
                    secondFrame = 0U;
                }

                interpolation =
                    static_cast<float>(
                        framePosition -
                        static_cast<double>(
                            firstFrame));
            }

            output = {};

            output.parameters =
            {
                1.0F,
                static_cast<float>(
                    skeleton.GetBoneCount()),
                useAnimation ? 1.0F : 0.0F,
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

            std::array<
                DirectX::XMFLOAT4X4,
                engine::assets::
                    MaximumSkeletonBones>
                currentAbsolute{};

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

                DirectX::XMMATRIX bindLocalMatrix =
                    bindMatrix;

                if (bone->parentIndex >= 0)
                {
                    const std::size_t parentIndex =
                        static_cast<std::size_t>(
                            bone->parentIndex);

                    if (
                        parentIndex >= boneIndex ||
                        parentIndex >=
                            skeleton.GetBoneCount())
                    {
                        return false;
                    }

                    const engine::assets::
                        SkeletonBone* parentBone =
                            skeleton.GetBone(
                                parentIndex);

                    if (parentBone == nullptr)
                    {
                        return false;
                    }

                    DirectX::XMMATRIX
                        inverseParentBind;

                    if (!InvertMatrix(
                            LoadMatrix(
                                parentBone->
                                    absoluteBindMatrix),
                            inverseParentBind))
                    {
                        return false;
                    }

                    bindLocalMatrix =
                        bindMatrix *
                        inverseParentBind;
                }

                DirectX::XMMATRIX localMatrix =
                    bindLocalMatrix;

                if (useAnimation)
                {
                    const engine::assets::
                        AnimationTrack* track =
                            animation->
                                GetTrackForBone(
                                    boneIndex);

                    if (track != nullptr)
                    {
                        localMatrix =
                            BuildAnimationTrackMatrix(
                                *track,
                                firstFrame,
                                secondFrame,
                                interpolation);

                        /*
                         * Пока root motion не подключён
                         * к CharacterController, блокируем
                         * горизонтальное перемещение root.
                         */
                        if (bone->parentIndex < 0)
                        {
                            DirectX::XMFLOAT4X4
                                animatedLocal;

                            DirectX::XMFLOAT4X4
                                bindLocal;

                            DirectX::XMStoreFloat4x4(
                                &animatedLocal,
                                localMatrix);

                            DirectX::XMStoreFloat4x4(
                                &bindLocal,
                                bindLocalMatrix);

                            animatedLocal._41 =
                                bindLocal._41;

                            animatedLocal._43 =
                                bindLocal._43;

                            localMatrix =
                                DirectX::
                                    XMLoadFloat4x4(
                                        &animatedLocal);
                        }
                    }
                }

                DirectX::XMMATRIX absoluteMatrix =
                    localMatrix;

                if (bone->parentIndex >= 0)
                {
                    absoluteMatrix *=
                        DirectX::XMLoadFloat4x4(
                            &currentAbsolute[
                                static_cast<
                                    std::size_t>(
                                        bone->
                                            parentIndex)]);
                }

                DirectX::XMStoreFloat4x4(
                    &currentAbsolute[boneIndex],
                    absoluteMatrix);

                const DirectX::XMMATRIX shiftedBind =
                    bindMatrix *
                    pivotTransform;

                const DirectX::XMMATRIX
                    shiftedCurrent =
                        absoluteMatrix *
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
        struct CachedTexture final
        {
            std::unique_ptr<
                engine::assets::GpuTexture>
                gpu;
        };

        struct CachedMaterial final
        {
            engine::assets::MaterialAssetDesc
                desc;

            std::shared_ptr<CachedTexture>
                baseColorTexture;

            engine::graphics::SamplerHandle
                sampler;
        };
        
        struct CachedSkeleton final
        {
            engine::assets::SkeletonAsset asset;
        };

        struct CachedAnimation final
        {
            engine::assets::AnimationAsset asset;

            std::chrono::steady_clock::time_point
                startedAt;
        };

        struct CachedMesh final
        {
            std::unique_ptr<
                engine::assets::GpuSkeletalMesh>
                gpu;

            std::shared_ptr<CachedSkeleton>
                skeleton;

            std::vector<
                std::shared_ptr<CachedMaterial>>
                materials;

            std::array<float, 3U> pivot{};
        };

        struct CachedCharacterPose final
        {
            std::shared_ptr<CachedSkeleton>
                skeleton;

            std::array<
                DirectX::XMFLOAT4X4,
                engine::assets::MaximumSkeletonBones>
                modelBoneMatrices{};

            DirectX::XMFLOAT4X4 world{};

            std::uint32_t boneCount = 0U;
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

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create modular character object buffer",
                    result);

                Shutdown(device);
                return false;
            }

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

            pipelineDescription.blend.
                renderTargets[0].
                    blendEnable = true;

            pipelineDescription.blend.
                renderTargets[0].
                    sourceColor =
                        engine::graphics::
                            BlendFactor::SourceAlpha;

            pipelineDescription.blend.
                renderTargets[0].
                    destinationColor =
                        engine::graphics::
                            BlendFactor::
                                InverseSourceAlpha;

            pipelineDescription.blend.
                renderTargets[0].
                    sourceAlpha =
                        engine::graphics::
                            BlendFactor::One;

            pipelineDescription.blend.
                renderTargets[0].
                    destinationAlpha =
                        engine::graphics::
                            BlendFactor::
                                InverseSourceAlpha;

            pipelineDescription.depthStencil.
                depthWriteEnable = false;

            pipelineDescription.debugName =
                "EditorModularCharacter."
                "TransparentPipeline";

            result =
                device.CreateGraphicsPipeline(
                    pipelineDescription,
                    transparentPipeline_);

            if (engine::graphics::Failed(result))
            {
                LogGraphicsFailure(
                    "Create transparent modular "
                    "character pipeline",
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

            for (auto& pair : materials_)
            {
                if (
                    pair.second != nullptr &&
                    pair.second->sampler.IsValid())
                {
                    static_cast<void>(
                        device.DestroySampler(
                            pair.second->sampler));

                    pair.second->sampler = {};
                }
            }

            for (auto& pair : textures_)
            {
                if (
                    pair.second != nullptr &&
                    pair.second->gpu != nullptr)
                {
                    static_cast<void>(
                        pair.second->gpu->
                            Release(device));
                }
            }

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
            animations_.clear();
            failedAnimations_.clear();
            materials_.clear();
            failedMaterials_.clear();
            textures_.clear();
            failedTextures_.clear();
            characterPoses_.clear();

            if (transparentPipeline_.IsValid())
            {
                static_cast<void>(
                    device.DestroyGraphicsPipeline(
                        transparentPipeline_));

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
                viewProjection,
            const DirectX::XMFLOAT4X4* const
                firstPersonInverseView,
            const DirectX::XMFLOAT4X4* const
                firstPersonViewProjection) noexcept
        {
            if (
                !initialized_ ||
                device_ == nullptr)
            {
                return engine::graphics::
                    GraphicsResult::InvalidState;
            }

            /*
             * Update() читает позу предыдущего кадра.
             * Перед сборкой текущего кадра очищаем cache.
             */
            characterPoses_.clear();

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
                static_cast<void>(
                    context.UnbindConstantBuffers(
                        engine::graphics::
                            ShaderStage::Vertex,
                        0U,
                        vertexConstantBuffers.size()));

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

                std::array<
    engine::scene::CharacterMeshSlot,
    5U>
    renderSlots =
        visibleSlots;

                std::size_t renderSlotCount =
                    renderSlots.size();

                const bool firstPersonView =
                    entity.characterAnimation.has_value() &&
                    entity.characterAnimation->
                        runtime.viewMode ==
                            engine::scene::
                                CharacterViewMode::
                                    FirstPerson;

                if (firstPersonView)
                {
                    /*
                     * FirstPersonBody — внутреннее имя
                     * уже назначенного BodyFPS.
                     */
                    renderSlots[0U] =
                        engine::scene::
                            CharacterMeshSlot::
                                FirstPersonBody;

                    renderSlotCount = 1U;
                }

                /*
                 * Старый однослойный путь остаётся
                 * безопасным fallback, пока новая
                 * state machine ещё не активировала
                 * runtime-слои.
                 */
                std::shared_ptr<CachedAnimation>
                    legacyAnimation;

                double legacyAnimationSeconds = 0.0;

                if (!component.idleAnimation.empty())
                {
                    legacyAnimation =
                        GetOrLoadAnimation(
                            component.idleAnimation);

                    if (legacyAnimation != nullptr)
                    {
                        legacyAnimationSeconds =
                            std::chrono::duration<
                                double>(
                                    std::chrono::
                                        steady_clock::
                                            now() -
                                    legacyAnimation->
                                        startedAt)
                                .count();
                    }
                }

                CharacterAnimationLayerSample
                    lowerBodySample;

                CharacterAnimationLayerSample
                    turnInPlaceSample;

                CharacterAnimationLayerSample
                    upperBodySample;

                CharacterAnimationLayerSample
                    actionSample;

                const engine::scene::
                    CharacterAnimationComponent*
                        animationComponent =
                            nullptr;

                bool useLayeredAnimation = false;

                if (
                    entity.characterAnimation.
                        has_value() &&
                    entity.characterAnimation->
                        enabled)
                {
                    animationComponent =
                        &*entity.characterAnimation;

                    const auto resolveLayer =
                        [this](
                            const engine::scene::
                                CharacterAnimationLayerRuntime&
                                    runtime) noexcept
                        {
                            CharacterAnimationLayerSample
                                sample;

                            sample.active =
                                runtime.active;

                            sample.currentTimeSeconds =
                                runtime.currentTimeSeconds;

                            sample.previousTimeSeconds =
                                runtime.previousTimeSeconds;

                            sample.weight =
                                std::clamp(
                                    runtime.weight,
                                    0.0F,
                                    1.0F);

                            sample.loopMode =
                                runtime.loopMode;

                            sample.previousLoopMode =
                                runtime.previousLoopMode;

                            if (
                                !runtime.currentClip.
                                    empty())
                            {
                                const std::shared_ptr<
                                    CachedAnimation>
                                    current =
                                        GetOrLoadAnimation(
                                            runtime.
                                                currentClip);

                                if (current != nullptr)
                                {
                                    sample.
                                        currentAnimation =
                                            &current->asset;
                                }
                            }

                            if (
                                !runtime.previousClip.
                                    empty())
                            {
                                const std::shared_ptr<
                                    CachedAnimation>
                                    previous =
                                        GetOrLoadAnimation(
                                            runtime.
                                                previousClip);

                                if (previous != nullptr)
                                {
                                    sample.
                                        previousAnimation =
                                            &previous->asset;
                                }
                            }

                            const bool transitionActive =
                                runtime.
                                    transitionDurationSeconds >
                                        0.0F &&
                                runtime.
                                    transitionElapsedSeconds <
                                runtime.
                                    transitionDurationSeconds;

                            sample.transitionAlpha =
                                transitionActive
                                    ? std::clamp(
                                        runtime.
                                            transitionElapsedSeconds /
                                        runtime.
                                            transitionDurationSeconds,
                                        0.0F,
                                        1.0F)
                                    : 1.0F;

                            /*
                             * Слой считается активным только
                             * при наличии хотя бы одного
                             * успешно загруженного клипа.
                             */
                            sample.active =
                                sample.active &&
                                (
                                    sample.currentAnimation !=
                                        nullptr ||
                                    sample.previousAnimation !=
                                        nullptr
                                );

                            return sample;
                        };

                    lowerBodySample =
                        resolveLayer(
                            animationComponent->
                                runtime.lowerBody);

                    turnInPlaceSample =
                        resolveLayer(
                            animationComponent->
                                runtime.turnInPlace);

                    upperBodySample =
                        resolveLayer(
                            animationComponent->
                                runtime.upperBody);

                    actionSample =
                        resolveLayer(
                            animationComponent->
                                runtime.action);

                    useLayeredAnimation =
                        lowerBodySample.active ||
                        turnInPlaceSample.active ||
                        upperBodySample.active ||
                        actionSample.active;
                }

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
                    std::size_t renderSlotIndex = 0U;
                    renderSlotIndex < renderSlotCount;
                    ++renderSlotIndex)
                {
                    const engine::scene::
                        CharacterMeshSlot slot =
                            renderSlots[
                                renderSlotIndex];
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

                    DirectX::XMFLOAT3
                        firstPersonHeadModelPosition{};

                    bool firstPersonHeadResolved =
                        false;

                    SkinningConstants skinning{};

                    bool skinningReady = false;

                    /*
                     * Новый Lower/Upper/Action evaluator.
                     */
                    if (
                        useLayeredAnimation &&
                        animationComponent != nullptr)
                    {
                        CharacterAnimationEvaluationInput
                            evaluationInput;

                        evaluationInput.skeleton =
                            &cached->skeleton->asset;

                        evaluationInput.pivot =
                            cached->pivot;

                        evaluationInput.lowerBody =
                            lowerBodySample;

                        evaluationInput.turnInPlace =
                            turnInPlaceSample;

                        evaluationInput.upperBody =
                            upperBodySample;

                        evaluationInput.action =
                            actionSample;

                        evaluationInput.
                            upperBodyRootBone =
                                animationComponent->
                                    animationSet.
                                    upperBodyRootBone;

                        evaluationInput.
                            actionRootBone =
                                animationComponent->
                                    animationSet.
                                    actionRootBone;

                        evaluationInput.
                            lookRootBone =
                                animationComponent->
                                    animationSet.
                                    lookRootBone;

                        evaluationInput.
                            lookYawOffsetDegrees =
                                animationComponent->
                                    runtime.
                                    upperBodyYawOffsetDegrees;

                        evaluationInput.
                            lookPitchOffsetDegrees =
                                animationComponent->
                                    runtime.
                                    upperBodyPitchOffsetDegrees;

                        evaluationInput.
                            blockControllerOwnedRootTransform =
                                entity.
                                    characterController.
                                    has_value() &&
                                !entity.
                                    characterController->
                                    useRootMotion;

                        CharacterAnimationPose pose;

                        if (animationEvaluator_.Evaluate(
                                evaluationInput,
                                pose))
                        {
                            skinning.parameters =
                            {
                                1.0F,

                                static_cast<float>(
                                    pose.boneCount),

                                pose.animated
                                    ? 1.0F
                                    : 0.0F,

                                0.0F
                            };

                            skinning.boneMatrices =
                                pose.boneMatrices;

                            const bool preferredPoseSlot =
                                firstPersonView
                                    ? slot ==
                                        engine::scene::
                                            CharacterMeshSlot::
                                                FirstPersonBody
                                    : slot ==
                                        engine::scene::
                                            CharacterMeshSlot::Body;

                            auto cachedPoseIterator =
                                characterPoses_.find(
                                    entity.id);

                            if (
                                pose.boneCount > 0U &&
                                (
                                    preferredPoseSlot ||
                                    cachedPoseIterator ==
                                        characterPoses_.end()
                                ))
                            {
                                CachedCharacterPose cachedPose;

                                cachedPose.skeleton =
                                    cached->skeleton;

                                cachedPose.modelBoneMatrices =
                                    pose.modelBoneMatrices;

                                cachedPose.world =
                                    constants.world;

                                cachedPose.boneCount =
                                    pose.boneCount;

                                if (
                                    cachedPoseIterator ==
                                        characterPoses_.end())
                                {
                                    characterPoses_.emplace(
                                        entity.id,
                                        std::move(cachedPose));
                                }
                                else
                                {
                                    cachedPoseIterator->second =
                                        std::move(cachedPose);
                                }
                            }

                            skinningReady = true;
                        }
                    }

                    /*
                     * Пока runtime-слои не заполнены,
                     * сохраняем старый рабочий preview.
                     *
                     * Также сюда попадём при ошибке
                     * layered evaluator.
                     */
                    if (!skinningReady)
                    {
                        const engine::assets::
                            AnimationAsset*
                                legacyAnimationAsset =
                                    legacyAnimation !=
                                        nullptr
                                        ? &legacyAnimation->
                                            asset
                                        : nullptr;

                        if (!BuildSkinningConstants(
                                cached->skeleton->asset,
                                legacyAnimationAsset,
                                cached->pivot,
                                legacyAnimationSeconds,
                                skinning))
                        {
                            /*
                             * Ошибка анимации не должна
                             * скрыть персонажа.
                             */
                            if (!BuildSkinningConstants(
                                    cached->
                                        skeleton->asset,
                                    nullptr,
                                    cached->pivot,
                                    0.0,
                                    skinning))
                            {
                                continue;
                            }
                        }
                    }

                    const bool firstPersonPresentation =
                        firstPersonView &&
                        slot ==
                            engine::scene::
                                CharacterMeshSlot::
                                    FirstPersonBody &&
                        firstPersonInverseView != nullptr &&
                        firstPersonViewProjection !=
                            nullptr;

                    if (
                        firstPersonPresentation &&
                        !firstPersonHeadResolved)
                    {
                        firstPersonHeadResolved =
                            TryResolveBindBoneModelPosition(
                                cached->skeleton->asset,
                                cached->pivot,
                                "Bip01_Head",
                                firstPersonHeadModelPosition);
                    }

                    if (
                        firstPersonPresentation &&
                        firstPersonHeadResolved)
                    {
                        /*
                         * Скелет BodyFPS сначала приводится
                         * к bind-положению Bip01_Head.
                         *
                         * Затем исправляется направление
                         * старого WarZ skeleton.
                         *
                         * После этого руки размещаются
                         * относительно FPS-камеры.
                         */
                        const DirectX::XMMATRIX
                            headToOrigin =
                                DirectX::XMMatrixTranslation(
                                    -firstPersonHeadModelPosition.x,
                                    -firstPersonHeadModelPosition.y,
                                    -firstPersonHeadModelPosition.z);

                        const DirectX::XMMATRIX
                            facingCorrection =
                                DirectX::XMMatrixRotationY(
                                    DirectX::XM_PI);

                        const DirectX::XMMATRIX
                            cameraOffset =
                                DirectX::XMMatrixTranslation(
                                    FirstPersonPresentationRightOffset,
                                    FirstPersonPresentationHeadOffset,
                                    FirstPersonPresentationForwardOffset);

                        const DirectX::XMMATRIX
                            cameraSpacePresentation =
                                headToOrigin *
                                facingCorrection *
                                cameraOffset;

                        DirectX::XMStoreFloat4x4(
                            &constants.world,
                            cameraSpacePresentation *
                                DirectX::XMLoadFloat4x4(
                                    firstPersonInverseView));

                        constants.viewProjection =
                            *firstPersonViewProjection;
                    }

                    result =
                        context.UpdateBuffer(
                            skinningBuffer_,
                            &skinning,
                            sizeof(skinning));

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

                        std::shared_ptr<
                            CachedMaterial>
                                material;

                        if (
                            section->materialSlot <
                            cached->
                                materials.size())
                        {
                            material =
                                cached->materials[
                                    section->
                                        materialSlot];
                        }

                        engine::graphics::
                            TextureHandle
                                baseColorTexture;

                        engine::graphics::
                            SamplerHandle
                                materialSampler;

                        bool transparent = false;

                        constants.baseColor =
                            GetSlotColor(slot);

                        constants.
                            materialParameters.y =
                                0.0F;

                        constants.
                            materialParameters.z =
                                0.5F;

                        constants.
                            materialParameters.w =
                                0.0F;

                        if (material != nullptr)
                        {
                            constants.baseColor =
                            {
                                material->desc.
                                    baseColorFactor[0],

                                material->desc.
                                    baseColorFactor[1],

                                material->desc.
                                    baseColorFactor[2],

                                material->desc.
                                    baseColorFactor[3]
                            };

                            constants.
                                materialParameters.z =
                                    material->desc.
                                        alphaCutoff;

                            constants.
                                materialParameters.w =
                                    static_cast<float>(
                                        static_cast<
                                            std::uint32_t>(
                                                material->
                                                    desc.
                                                    alphaMode));

                            transparent =
                                material->desc.
                                    alphaMode ==
                                engine::assets::
                                    MaterialAlphaMode::
                                        Blend;

                            if (
                                material->
                                    baseColorTexture !=
                                        nullptr &&
                                material->
                                    baseColorTexture->
                                    gpu != nullptr &&
                                material->
                                    baseColorTexture->
                                    gpu->IsValid() &&
                                material->sampler.
                                    IsValid())
                            {
                                baseColorTexture =
                                    material->
                                        baseColorTexture->
                                        gpu->
                                        GetHandle();

                                materialSampler =
                                    material->sampler;

                                constants.
                                    materialParameters.y =
                                        1.0F;
                            }
                        }

                        result =
                            context.SetGraphicsPipeline(
                                transparent
                                    ? transparentPipeline_
                                    : pipeline_);

                        if (engine::graphics::Failed(
                                result))
                        {
                            break;
                        }

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

                        if (
                            baseColorTexture.IsValid() &&
                            materialSampler.IsValid())
                        {
                            result =
                                context.SetShaderResources(
                                    engine::graphics::
                                        ShaderStage::Pixel,
                                    0U,
                                    &baseColorTexture,
                                    1U);

                            if (engine::graphics::Failed(
                                    result))
                            {
                                break;
                            }

                            result =
                                context.SetSamplers(
                                    engine::graphics::
                                        ShaderStage::Pixel,
                                    0U,
                                    &materialSampler,
                                    1U);

                            if (engine::graphics::Failed(
                                    result))
                            {
                                break;
                            }
                        }
                        else
                        {
                            static_cast<void>(
                                context.
                                    UnbindShaderResources(
                                        engine::graphics::
                                            ShaderStage::
                                                Pixel,
                                        0U,
                                        1U));

                            static_cast<void>(
                                context.UnbindSamplers(
                                    engine::graphics::
                                        ShaderStage::
                                            Pixel,
                                    0U,
                                    1U));
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
                context.UnbindShaderResources(
                    engine::graphics::
                        ShaderStage::Pixel,
                    0U,
                    1U));

            static_cast<void>(
                context.UnbindSamplers(
                    engine::graphics::
                        ShaderStage::Pixel,
                    0U,
                    1U));

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Vertex,
                    0U,
                    2U));

            static_cast<void>(
                context.UnbindConstantBuffers(
                    engine::graphics::
                        ShaderStage::Pixel,
                    0U,
                    1U));

            context.UnbindGraphicsPipeline();

            return result;
        }

        [[nodiscard]]
bool TryGetAnimationDuration(
    const std::wstring& animationPath,
    double& durationSeconds) const noexcept
        {
            durationSeconds = 0.0;

            if (animationPath.empty())
            {
                return false;
            }

            const std::shared_ptr<CachedAnimation>
                animation =
                    const_cast<Impl*>(this)->
                        GetOrLoadAnimation(
                            animationPath);

            if (animation == nullptr)
            {
                return false;
            }

            const std::uint32_t frameCount =
                animation->asset.
                    GetFrameCount();

            const double frameRate =
                static_cast<double>(
                    animation->asset.
                        GetFrameRate());

            if (
                frameCount <= 1U ||
                !std::isfinite(frameRate) ||
                frameRate <= 0.0)
            {
                return false;
            }

            /*
             * One-shot заканчивается на кадре N - 1.
             * Это соответствует WarZ PauseOnEnd.
             */
            durationSeconds =
                static_cast<double>(
                    frameCount - 1U) /
                frameRate;

            return
                std::isfinite(durationSeconds) &&
                durationSeconds > 0.0;
        }

        [[nodiscard]]
        bool TryGetBoneWorldPosition(
            const EditorEntityId entityId,
            const std::string_view boneName,
            DirectX::XMFLOAT3& position) const noexcept
        {
            if (boneName.empty())
            {
                return false;
            }

            const auto poseIterator =
                characterPoses_.find(
                    entityId);

            if (
                poseIterator ==
                    characterPoses_.end() ||
                poseIterator->second.skeleton ==
                    nullptr)
            {
                return false;
            }

            const CachedCharacterPose& pose =
                poseIterator->second;

            const engine::assets::SkeletonAsset&
                skeleton =
                    pose.skeleton->asset;

            const std::size_t boneCount =
                (std::min)(
                    static_cast<std::size_t>(
                        pose.boneCount),
                    skeleton.GetBoneCount());

            std::size_t foundBoneIndex =
                boneCount;

            for (
                std::size_t boneIndex = 0U;
                boneIndex < boneCount;
                ++boneIndex)
            {
                const engine::assets::SkeletonBone*
                    bone =
                        skeleton.GetBone(
                            boneIndex);

                if (
                    bone != nullptr &&
                    EqualAsciiInsensitive(
                        bone->name,
                        boneName))
                {
                    foundBoneIndex =
                        boneIndex;

                    break;
                }
            }

            if (foundBoneIndex >= boneCount)
            {
                return false;
            }

            const DirectX::XMMATRIX boneModel =
                DirectX::XMLoadFloat4x4(
                    &pose.modelBoneMatrices[
                        foundBoneIndex]);

            const DirectX::XMMATRIX world =
                DirectX::XMLoadFloat4x4(
                    &pose.world);

            const DirectX::XMVECTOR worldPosition =
                DirectX::XMVector3TransformCoord(
                    DirectX::XMVectorZero(),
                    boneModel * world);

            DirectX::XMFLOAT3 resolved;

            DirectX::XMStoreFloat3(
                &resolved,
                worldPosition);

            if (
                !std::isfinite(resolved.x) ||
                !std::isfinite(resolved.y) ||
                !std::isfinite(resolved.z))
            {
                return false;
            }

            position = resolved;
            return true;
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
        std::shared_ptr<CachedAnimation>
            GetOrLoadAnimation(
                const std::wstring&
                    assetPath) noexcept
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
                        failedAnimations_.
                            insert(key).second)
                    {
                        std::string message = "Animation file was not found: ";

                        message +=
                            std::filesystem::path(
                                assetPath).
                                generic_u8string();

                        engine::core::GetLogger().Write(
                            engine::core::LogLevel::Error,
                            "LTS.Editor.ModularCharacter",
                            message);
                    }

                    return nullptr;
                }

                const std::wstring key =
                    LowercasePath(
                        resolvedPath.
                            lexically_normal().
                            wstring());

                const auto existing =
                    animations_.find(key);

                if (existing != animations_.end())
                {
                    return existing->second;
                }

                if (
                    failedAnimations_.find(key) !=
                    failedAnimations_.end())
                {
                    return nullptr;
                }

                engine::assets::AssetData source;

                engine::assets::AssetResult result =
                    ReadAssetData(
                        resolvedPath,
                        source);

                if (engine::assets::Failed(result))
                {
                    LogAssetFailure(
                        resolvedPath,
                        "Read animation",
                        result);

                    failedAnimations_.insert(key);
                    return nullptr;
                }

                engine::assets::AssetMetadata metadata;

                result =
                    CreateMetadata(
                        std::filesystem::path(
                            assetPath),
                        source.GetSize(),
                        engine::assets::
                            AssetType::Animation,
                        metadata);

                if (engine::assets::Failed(result))
                {
                    LogAssetFailure(
                        resolvedPath,
                        "Create animation metadata",
                        result);

                    failedAnimations_.insert(key);
                    return nullptr;
                }

                engine::assets::
                    AnimationAssetLoader loader;

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
                            AssetType::Animation)
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
                        resolvedPath,
                        "Load animation",
                        result);

                    failedAnimations_.insert(key);
                    return nullptr;
                }

                auto* const loadedAnimation =
                    static_cast<
                        engine::assets::
                            AnimationLoadedAsset*>(
                                loadedAsset.get());

                auto cached =
                    std::make_shared<
                        CachedAnimation>();

                cached->asset =
                    loadedAnimation->
                        ReleaseAnimation();

                cached->startedAt =
                    std::chrono::
                        steady_clock::now();

                animations_.emplace(
                    key,
                    cached);

                return cached;
            }
            catch (const std::bad_alloc&)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Not enough memory to load animation.");

                return nullptr;
            }
            catch (...)
            {
                engine::core::GetLogger().Write(
                    engine::core::LogLevel::Error,
                    "LTS.Editor.ModularCharacter",
                    "Unexpected animation loading failure.");

                return nullptr;
            }
        }

        [[nodiscard]]
        std::shared_ptr<CachedTexture>
            GetOrLoadTexture(
                const std::filesystem::path&
                    filePath) noexcept
        {
            try
            {
                const std::wstring key =
                    LowercasePath(
                        filePath.
                            lexically_normal().
                            wstring());

                const auto existing =
                    textures_.find(key);

                if (existing != textures_.end())
                {
                    return existing->second;
                }

                if (
                    failedTextures_.find(key) !=
                    failedTextures_.end())
                {
                    return nullptr;
                }

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
                        "Read character texture",
                        assetResult);

                    failedTextures_.insert(key);
                    return nullptr;
                }

                if (!engine::assets::
                        DdsTextureDecoder::IsDds(
                            source))
                {
                    engine::core::GetLogger().Write(
                        engine::core::LogLevel::Error,
                        "LTS.Editor.ModularCharacter",
                        "Character base-color texture "
                        "is not a DDS file.");

                    failedTextures_.insert(key);
                    return nullptr;
                }

                engine::assets::
                    DdsTextureDecodeOptions
                        decodeOptions;

                decodeOptions.forceSrgb = true;
                decodeOptions.allowBc7 = true;

                engine::assets::TextureAsset
                    cpuTexture;

                assetResult =
                    engine::assets::
                        DdsTextureDecoder::Decode(
                            source,
                            decodeOptions,
                            cpuTexture);

                if (engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        filePath,
                        "Decode character DDS texture",
                        assetResult);

                    failedTextures_.insert(key);
                    return nullptr;
                }

                auto cached =
                    std::make_shared<
                        CachedTexture>();

                cached->gpu =
                    std::make_unique<
                        engine::assets::GpuTexture>();

                engine::assets::
                    GpuTextureUploadOptions
                        uploadOptions;

                uploadOptions.requestedColorSpace =
                    engine::assets::
                        RequestedColorSpace::Srgb;

                const engine::graphics::
                    GraphicsResult graphicsResult =
                        cached->gpu->Upload(
                            *device_,
                            cpuTexture,
                            uploadOptions);

                if (engine::graphics::Failed(
                        graphicsResult))
                {
                    LogGraphicsFailure(
                        "Upload character base-color texture",
                        graphicsResult);

                    failedTextures_.insert(key);
                    return nullptr;
                }

                textures_.emplace(
                    key,
                    cached);

                return cached;
            }
            catch (const std::bad_alloc&)
            {
                return nullptr;
            }
            catch (...)
            {
                return nullptr;
            }
        }

        [[nodiscard]]
        std::shared_ptr<CachedMaterial>
            GetOrLoadMaterial(
                const std::filesystem::path&
                    skeletalMeshFile,
                const std::string&
                    materialAssetPath) noexcept
        {
            try
            {
                const std::filesystem::path
                    materialFile =
                        ResolveDataAssetFile(
                            skeletalMeshFile,
                            materialAssetPath);

                if (materialFile.empty())
                {
                    const std::wstring failedKey =
                        LowercasePath(
                            std::filesystem::u8path(
                                materialAssetPath).
                                wstring());

                    if (
                        failedMaterials_.
                            insert(failedKey).
                            second)
                    {
                        std::string message =
                            "Character material was "
                            "not found: ";

                        message += materialAssetPath;

                        engine::core::GetLogger().Write(
                            engine::core::
                                LogLevel::Error,
                            "LTS.Editor.ModularCharacter",
                            message);
                    }

                    return nullptr;
                }

                const std::wstring key =
                    LowercasePath(
                        materialFile.
                            lexically_normal().
                            wstring());

                const auto existing =
                    materials_.find(key);

                if (existing != materials_.end())
                {
                    return existing->second;
                }

                if (
                    failedMaterials_.find(key) !=
                    failedMaterials_.end())
                {
                    return nullptr;
                }

                engine::assets::AssetData source;

                engine::assets::AssetResult
                    assetResult =
                        ReadAssetData(
                            materialFile,
                            source);

                if (engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        materialFile,
                        "Read character material",
                        assetResult);

                    failedMaterials_.insert(key);
                    return nullptr;
                }

                engine::assets::AssetMetadata
                    metadata;

                assetResult =
                    CreateMetadata(
                        std::filesystem::u8path(
                            materialAssetPath),
                        source.GetSize(),
                        engine::assets::
                            AssetType::Material,
                        metadata);

                if (engine::assets::Failed(
                        assetResult))
                {
                    LogAssetFailure(
                        materialFile,
                        "Create character material metadata",
                        assetResult);

                    failedMaterials_.insert(key);
                    return nullptr;
                }

                engine::assets::
                    MaterialAssetLoader loader;

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
                            AssetType::Material)
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
                        materialFile,
                        "Load character material",
                        assetResult);

                    failedMaterials_.insert(key);
                    return nullptr;
                }

                const auto* const loadedMaterial =
                    static_cast<
                        engine::assets::
                            MaterialLoadedAsset*>(
                                loadedAsset.get());

                auto cached =
                    std::make_shared<
                        CachedMaterial>();

                cached->desc =
                    loadedMaterial->
                        GetMaterial().
                        GetDesc();

                if (
                    cached->desc.
                        baseColorTexture.
                        has_value())
                {
                    const std::filesystem::path
                        textureFile =
                            ResolveDataAssetFile(
                                materialFile,
                                cached->desc.
                                    baseColorTexture->
                                    String());

                    if (!textureFile.empty())
                    {
                        cached->baseColorTexture =
                            GetOrLoadTexture(
                                textureFile);
                    }
                    else
                    {
                        std::string message =
                            "Character base-color texture "
                            "was not found: ";

                        message +=
                            cached->desc.
                                baseColorTexture->
                                String();

                        engine::core::GetLogger().Write(
                            engine::core::
                                LogLevel::Error,
                            "LTS.Editor.ModularCharacter",
                            message);
                    }
                }

                if (
                    cached->baseColorTexture !=
                        nullptr &&
                    cached->baseColorTexture->gpu !=
                        nullptr &&
                    cached->baseColorTexture->gpu->
                        IsValid())
                {
                    engine::graphics::SamplerDesc
                        samplerDescription =
                            cached->desc.sampler;

                    const engine::graphics::
                        GraphicsResult samplerResult =
                            device_->CreateSampler(
                                samplerDescription,
                                cached->sampler);

                    if (engine::graphics::Failed(
                            samplerResult))
                    {
                        LogGraphicsFailure(
                            "Create character material sampler",
                            samplerResult);

                        cached->baseColorTexture.reset();
                        cached->sampler = {};
                    }
                }

                materials_.emplace(
                    key,
                    cached);

                return cached;
            }
            catch (const std::bad_alloc&)
            {
                return nullptr;
            }
            catch (...)
            {
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

                output.materials.clear();

                output.materials.resize(
                    cpuMesh.
                        GetMaterialSlotCount());

                for (
                    std::size_t sectionIndex = 0U;
                    sectionIndex <
                        cpuMesh.GetSectionCount();
                    ++sectionIndex)
                {
                    const engine::assets::
                        SkeletalMeshSection*
                            section =
                                cpuMesh.GetSection(
                                    sectionIndex);

                    if (
                        section == nullptr ||
                        section->materialSlot >=
                            output.materials.size() ||
                        section->
                            materialAssetPath.empty())
                    {
                        continue;
                    }

                    auto& material =
                        output.materials[
                            section->materialSlot];

                    if (material == nullptr)
                    {
                        material =
                            GetOrLoadMaterial(
                                filePath,
                                section->
                                    materialAssetPath);
                    }
                }

                output.gpu =
                    std::move(gpuMesh);

                output.skeleton =
                    std::move(skeleton);

                output.pivot =
                    cpuMesh.GetPivot();

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

        CharacterAnimationEvaluator
            animationEvaluator_;

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

        engine::graphics::PipelineStateHandle
            transparentPipeline_;

        std::unordered_map<
            std::wstring,
            CachedMesh>
            meshes_;

        std::unordered_map<
            std::wstring,
            std::shared_ptr<CachedSkeleton>>
            skeletons_;

        std::unordered_map<
            std::wstring,
            std::shared_ptr<CachedAnimation>>
            animations_;

        std::unordered_map<
            std::wstring,
            std::shared_ptr<CachedMaterial>>
            materials_;

        std::unordered_map<
            std::wstring,
            std::shared_ptr<CachedTexture>>
            textures_;

        std::unordered_map<
            EditorEntityId,
            CachedCharacterPose>
            characterPoses_;

        std::unordered_set<std::wstring>
            failedMeshes_;

        std::unordered_set<std::wstring>
            failedSkeletons_;

        std::unordered_set<std::wstring>
            failedAnimations_;

        std::unordered_set<std::wstring>
            failedMaterials_;

        std::unordered_set<std::wstring>
            failedTextures_;

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
                viewProjection,
            const DirectX::XMFLOAT4X4* const
                firstPersonInverseView,
            const DirectX::XMFLOAT4X4* const
                firstPersonViewProjection) noexcept
    {
        if (impl_ == nullptr)
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        return impl_->Render(
            context,
            document,
            viewProjection,
            firstPersonInverseView,
            firstPersonViewProjection);
    }

    bool ModularCharacterRenderer::
        TryGetAnimationDuration(
            const std::wstring& animationPath,
            double& durationSeconds) const noexcept
    {
        return
            impl_ != nullptr &&
            impl_->TryGetAnimationDuration(
                animationPath,
                durationSeconds);
    }

    bool ModularCharacterRenderer::
        TryGetBoneWorldPosition(
            const EditorEntityId entityId,
            const std::string_view boneName,
            DirectX::XMFLOAT3& position) const noexcept
    {
        return
            impl_ != nullptr &&
            impl_->TryGetBoneWorldPosition(
                entityId,
                boneName,
                position);
    }
}
