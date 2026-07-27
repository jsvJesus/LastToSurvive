#include "Editor/Tools/Import/LegacyAnimationReader.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
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
        struct BinaryHeader final
        {
            std::uint32_t fileId = 0U;
            std::uint32_t assetId = 0U;
            std::uint32_t version = 0U;
        };

        static_assert(sizeof(BinaryHeader) == 12U);

        constexpr std::size_t TrackNameSize = 32U;
        constexpr std::size_t KeyDiskSize =
            sizeof(float) * 7U;

        constexpr std::uint32_t MaximumTrackCount = 4096U;
        constexpr std::uint32_t MaximumFrameCount = 1000000U;

        constexpr std::uint64_t MaximumKeyCount =
            50000000ULL;

        [[nodiscard]]
        constexpr std::uint32_t MakeLegacyId(
            const char first,
            const char second,
            const char third,
            const char fourth) noexcept
        {
            return
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(first)) << 24U) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(second)) << 16U) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(third)) << 8U) |
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(fourth));
        }

        constexpr std::uint32_t BinaryFileId =
            MakeLegacyId('2', 'd', '3', 'r');

        constexpr std::uint32_t AnimationAssetId =
            MakeLegacyId('d', 'm', 'n', 'a');

        constexpr std::uint32_t AnimationVersion = 3U;

        template<typename Value>
        [[nodiscard]]
        bool ReadValue(
            std::ifstream& stream,
            Value& value) noexcept
        {
            stream.read(
                reinterpret_cast<char*>(&value),
                static_cast<std::streamsize>(
                    sizeof(Value)));

            return
                stream.gcount() ==
                static_cast<std::streamsize>(
                    sizeof(Value));
        }

        [[nodiscard]]
        bool ReadBytes(
            std::ifstream& stream,
            void* destination,
            const std::size_t byteCount) noexcept
        {
            if (byteCount == 0U)
            {
                return true;
            }

            if (destination == nullptr ||
                byteCount >
                    static_cast<std::size_t>(
                        (std::numeric_limits<
                            std::streamsize>::max)()))
            {
                return false;
            }

            stream.read(
                static_cast<char*>(destination),
                static_cast<std::streamsize>(
                    byteCount));

            return
                stream.gcount() ==
                static_cast<std::streamsize>(
                    byteCount);
        }

        [[nodiscard]]
        bool GetFileSize(
            const std::filesystem::path& path,
            std::uintmax_t& output) noexcept
        {
            std::error_code error;

            output =
                std::filesystem::file_size(
                    path,
                    error);

            return !error;
        }

        [[nodiscard]]
        std::string ReadTrackName(
            const std::array<char, TrackNameSize>& buffer)
        {
            std::size_t length = 0U;

            while (length < buffer.size() &&
                   buffer[length] != '\0')
            {
                ++length;
            }

            return std::string(
                buffer.data(),
                length);
        }

        [[nodiscard]]
        std::string ToLowerAscii(
            std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        DirectX::XMMATRIX LoadMatrix(
            const std::array<float, 16U>& source) noexcept
        {
            DirectX::XMFLOAT4X4 matrix;

            std::memcpy(
                &matrix,
                source.data(),
                sizeof(matrix));

            return DirectX::XMLoadFloat4x4(
                &matrix);
        }

        void StoreMatrix(
            const DirectX::XMMATRIX& matrix,
            std::array<float, 16U>& destination) noexcept
        {
            DirectX::XMFLOAT4X4 stored;

            DirectX::XMStoreFloat4x4(
                &stored,
                matrix);

            std::memcpy(
                destination.data(),
                &stored,
                sizeof(stored));
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
                std::abs(value) > 0.0000001F;
        }

        [[nodiscard]]
        DirectX::XMMATRIX BuildTrackMatrix(
            const LegacyAnimationTrack& track,
            const std::uint32_t firstFrame,
            const std::uint32_t secondFrame,
            const float interpolation) noexcept
        {
            const LegacyAnimationKey& first =
                track.keys[firstFrame];

            const LegacyAnimationKey& second =
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
    }

    bool LegacyAnimationReader::Read(
        const std::filesystem::path& path,
        const LegacySkeletonData* const skeleton,
        LegacyAnimationData& output) noexcept
    {
        output = {};
        output.sourcePath = path;

        try
        {
            std::uintmax_t fileSize = 0U;

            if (!GetFileSize(path, fileSize))
            {
                output.error =
                    "Failed to query animation file size.";

                return false;
            }

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream)
            {
                output.error =
                    "Failed to open animation file.";

                return false;
            }

            BinaryHeader header;

            if (!ReadValue(stream, header))
            {
                output.error =
                    "Animation header is truncated.";

                return false;
            }

            if (header.fileId != BinaryFileId)
            {
                output.error =
                    "Invalid WarZ binary file ID.";

                return false;
            }

            if (header.assetId != AnimationAssetId)
            {
                output.error =
                    "Selected file is not a WarZ animation.";

                return false;
            }

            if (header.version != AnimationVersion)
            {
                output.error =
                    "Unsupported WarZ animation version: " +
                    std::to_string(header.version) +
                    '.';

                return false;
            }

            std::uint32_t trackCount = 0U;
            std::uint32_t frameRateInteger = 0U;

            if (!ReadValue(
                    stream,
                    output.skeletonId) ||
                !ReadValue(
                    stream,
                    trackCount) ||
                !ReadValue(
                    stream,
                    output.frameCount) ||
                !ReadValue(
                    stream,
                    frameRateInteger))
            {
                output.error =
                    "Animation information is truncated.";

                return false;
            }

            if (trackCount == 0U ||
                trackCount > MaximumTrackCount)
            {
                output.error =
                    "Invalid animation track count: " +
                    std::to_string(trackCount) +
                    '.';

                return false;
            }

            if (output.frameCount == 0U ||
                output.frameCount > MaximumFrameCount)
            {
                output.error =
                    "Invalid animation frame count: " +
                    std::to_string(output.frameCount) +
                    '.';

                return false;
            }

            if (frameRateInteger == 0U ||
                frameRateInteger > 1000U)
            {
                output.error =
                    "Invalid animation frame rate: " +
                    std::to_string(frameRateInteger) +
                    '.';

                return false;
            }

            const std::uint64_t keyCount =
                static_cast<std::uint64_t>(
                    trackCount) *
                static_cast<std::uint64_t>(
                    output.frameCount);

            if (keyCount > MaximumKeyCount)
            {
                output.error =
                    "Animation contains too many keys.";

                return false;
            }

            const std::uintmax_t trackByteCount =
                TrackNameSize +
                sizeof(std::uint32_t) +
                static_cast<std::uintmax_t>(
                    output.frameCount) *
                KeyDiskSize;

            const std::uintmax_t expectedSize =
                sizeof(BinaryHeader) +
                sizeof(std::uint32_t) * 4U +
                static_cast<std::uintmax_t>(
                    trackCount) *
                trackByteCount;

            if (fileSize < expectedSize)
            {
                output.error =
                    "Animation file is truncated.";

                return false;
            }

            output.trailingByteCount =
                static_cast<std::size_t>(
                    fileSize - expectedSize);

            output.frameRate =
                static_cast<float>(
                    frameRateInteger);

            output.durationSeconds =
                output.frameCount > 1U
                    ? static_cast<float>(
                        output.frameCount - 1U) /
                        output.frameRate
                    : 0.0F;

            std::unordered_map<
                std::string,
                std::int32_t> skeletonBones;

            if (skeleton != nullptr)
            {
                skeletonBones.reserve(
                    skeleton->bones.size());

                output.boneToTrack.assign(
                    skeleton->bones.size(),
                    -1);

                for (std::size_t boneIndex = 0U;
                     boneIndex < skeleton->bones.size();
                     ++boneIndex)
                {
                    skeletonBones.emplace(
                        ToLowerAscii(
                            skeleton->bones[
                                boneIndex].name),
                        static_cast<std::int32_t>(
                            boneIndex));
                }

                if (output.skeletonId != 0U &&
                    skeleton->skeletonId != 0U &&
                    output.skeletonId !=
                        skeleton->skeletonId)
                {
                    output.skeletonIdMismatch = true;
                }
            }

            std::unordered_set<std::string>
                trackNames;

            trackNames.reserve(trackCount);

            output.tracks.reserve(trackCount);

            for (std::uint32_t trackIndex = 0U;
                 trackIndex < trackCount;
                 ++trackIndex)
            {
                std::array<char, TrackNameSize>
                    nameBuffer{};

                LegacyAnimationTrack track;

                if (!ReadBytes(
                        stream,
                        nameBuffer.data(),
                        nameBuffer.size()) ||
                    !ReadValue(
                        stream,
                        track.flags))
                {
                    output.error =
                        "Failed to read animation track " +
                        std::to_string(trackIndex) +
                        '.';

                    output.tracks.clear();
                    return false;
                }

                track.boneName =
                    ReadTrackName(nameBuffer);

                const std::string normalizedName =
                    ToLowerAscii(
                        track.boneName);

                if (!trackNames.insert(
                        normalizedName).second)
                {
                    ++output.duplicateTrackNameCount;
                }

                if (track.IsRootTrack())
                {
                    ++output.rootTrackCount;
                }

                const auto mappedBone =
                    skeletonBones.find(
                        normalizedName);

                if (mappedBone !=
                    skeletonBones.end())
                {
                    track.skeletonBoneIndex =
                        mappedBone->second;

                    const std::size_t boneIndex =
                        static_cast<std::size_t>(
                            mappedBone->second);

                    if (output.boneToTrack[
                            boneIndex] == -1)
                    {
                        output.boneToTrack[
                            boneIndex] =
                                static_cast<std::int32_t>(
                                    trackIndex);

                        ++output.mappedTrackCount;
                    }
                }
                else if (skeleton != nullptr)
                {
                    ++output.missingBoneTrackCount;
                }

                track.keys.resize(
                    output.frameCount);

                for (std::uint32_t frameIndex = 0U;
                     frameIndex < output.frameCount;
                     ++frameIndex)
                {
                    std::array<float, 7U> values{};

                    if (!ReadBytes(
                            stream,
                            values.data(),
                            sizeof(values)))
                    {
                        output.error =
                            "Animation key data is truncated.";

                        output.tracks.clear();
                        return false;
                    }

                    LegacyAnimationKey& key =
                        track.keys[frameIndex];

                    key.rotation =
                    {
                        values[0],
                        values[1],
                        values[2],
                        values[3]
                    };

                    key.translation =
                    {
                        values[4],
                        values[5],
                        values[6]
                    };

                    const bool rotationFinite =
                        std::isfinite(key.rotation[0]) &&
                        std::isfinite(key.rotation[1]) &&
                        std::isfinite(key.rotation[2]) &&
                        std::isfinite(key.rotation[3]);

                    const float rotationLengthSquared =
                        key.rotation[0] *
                            key.rotation[0] +
                        key.rotation[1] *
                            key.rotation[1] +
                        key.rotation[2] *
                            key.rotation[2] +
                        key.rotation[3] *
                            key.rotation[3];

                    if (!rotationFinite ||
                        !std::isfinite(
                            rotationLengthSquared) ||
                        rotationLengthSquared <
                            0.0000001F)
                    {
                        ++output.invalidQuaternionCount;

                        key.rotation =
                        {
                            0.0F,
                            0.0F,
                            0.0F,
                            1.0F
                        };
                    }
                    else
                    {
                        const float rotationLength =
                            std::sqrt(
                                rotationLengthSquared);

                        if (std::abs(
                                rotationLength -
                                1.0F) > 0.01F)
                        {
                            ++output.
                                nonNormalizedQuaternionCount;
                        }

                        const float inverseLength =
                            1.0F /
                            rotationLength;

                        for (float& component :
                             key.rotation)
                        {
                            component *=
                                inverseLength;
                        }
                    }

                    for (float& component :
                         key.translation)
                    {
                        if (!std::isfinite(component))
                        {
                            ++output.
                                invalidTranslationCount;

                            component = 0.0F;
                        }
                    }
                }

                output.tracks.push_back(
                    std::move(track));
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            output.error =
                "Animation read failed: " +
                std::string(
                    exception.what());

            output.tracks.clear();
            output.boneToTrack.clear();

            return false;
        }
        catch (...)
        {
            output.error =
                "Animation read failed with an unknown error.";

            output.tracks.clear();
            output.boneToTrack.clear();

            return false;
        }
    }

    bool LegacyAnimationReader::Sample(
        const LegacyAnimationData& animation,
        const LegacySkeletonData& skeleton,
        const std::array<float, 3U>& meshPivot,
        const float frame,
        const bool loopInterpolation,
        const bool lockRootHorizontalMovement,
        LegacyAnimationPose& output,
        std::string& error) noexcept
    {
        output = {};
        error.clear();

        try
        {
            const std::size_t boneCount =
                skeleton.bones.size();

            if (boneCount == 0U ||
                boneCount >
                    LegacyAnimationMaximumBones)
            {
                error =
                    "Unsupported preview bone count: " +
                    std::to_string(boneCount) +
                    '.';

                return false;
            }

            if (!animation.IsCompatible() ||
                animation.boneToTrack.size() !=
                    boneCount ||
                animation.frameCount == 0U)
            {
                error =
                    "Animation is not compatible with the selected skeleton.";

                return false;
            }

            const float lastFrame =
                static_cast<float>(
                    animation.frameCount - 1U);

            const float sampleFrame =
                std::clamp(
                    frame,
                    0.0F,
                    lastFrame);

            const std::uint32_t firstFrame =
                static_cast<std::uint32_t>(
                    std::floor(sampleFrame));

            std::uint32_t secondFrame =
                firstFrame + 1U;

            if (secondFrame >=
                animation.frameCount)
            {
                secondFrame =
                    loopInterpolation &&
                    animation.frameCount > 1U
                        ? 0U
                        : animation.frameCount - 1U;
            }

            const float interpolation =
                sampleFrame -
                static_cast<float>(
                    firstFrame);

            std::vector<DirectX::XMFLOAT4X4>
                bindAbsolute(boneCount);

            std::vector<DirectX::XMFLOAT4X4>
                currentAbsolute(boneCount);

            output.absoluteBoneMatrices.resize(
                boneCount);

            output.skinMatrices.resize(
                boneCount);

            const DirectX::XMMATRIX pivotTransform =
                DirectX::XMMatrixTranslation(
                    -meshPivot[0],
                    -meshPivot[1],
                    -meshPivot[2]);

            for (std::size_t boneIndex = 0U;
                 boneIndex < boneCount;
                 ++boneIndex)
            {
                const LegacyBone& bone =
                    skeleton.bones[boneIndex];

                std::memcpy(
                    &bindAbsolute[boneIndex],
                    bone.absoluteBindMatrix.data(),
                    sizeof(DirectX::XMFLOAT4X4));

                const DirectX::XMMATRIX bindMatrix =
                    DirectX::XMLoadFloat4x4(
                        &bindAbsolute[boneIndex]);

                DirectX::XMMATRIX localMatrix =
                    bindMatrix;

                if (bone.parentIndex >= 0)
                {
                    const std::size_t parentIndex =
                        static_cast<std::size_t>(
                            bone.parentIndex);

                    if (parentIndex >= boneIndex ||
                        parentIndex >= boneCount)
                    {
                        error =
                            "Invalid skeleton hierarchy while sampling animation.";

                        return false;
                    }

                    DirectX::XMMATRIX inverseParentBind;

                    if (!InvertMatrix(
                            DirectX::XMLoadFloat4x4(
                                &bindAbsolute[
                                    parentIndex]),
                            inverseParentBind))
                    {
                        error =
                            "Failed to invert parent bind matrix.";

                        return false;
                    }

                    localMatrix =
                        bindMatrix *
                        inverseParentBind;
                }

                const std::int32_t trackIndex =
                    animation.boneToTrack[
                        boneIndex];

                if (trackIndex >= 0 &&
                    trackIndex <
                        static_cast<std::int32_t>(
                            animation.tracks.size()))
                {
                    localMatrix =
                        BuildTrackMatrix(
                            animation.tracks[
                                static_cast<std::size_t>(
                                    trackIndex)],
                            firstFrame,
                            secondFrame,
                            interpolation);

                    if (lockRootHorizontalMovement &&
                        bone.parentIndex < 0)
                    {
                        DirectX::XMFLOAT4X4 animatedLocal;
                        DirectX::XMFLOAT4X4 bindLocal;

                        DirectX::XMStoreFloat4x4(
                            &animatedLocal,
                            localMatrix);

                        DirectX::XMStoreFloat4x4(
                            &bindLocal,
                            bone.parentIndex < 0
                                ? bindMatrix
                                : localMatrix);

                        animatedLocal._41 =
                            bindLocal._41;

                        animatedLocal._43 =
                            bindLocal._43;

                        localMatrix =
                            DirectX::XMLoadFloat4x4(
                                &animatedLocal);
                    }
                }

                DirectX::XMMATRIX absoluteMatrix =
                    localMatrix;

                if (bone.parentIndex >= 0)
                {
                    absoluteMatrix *=
                        DirectX::XMLoadFloat4x4(
                            &currentAbsolute[
                                static_cast<std::size_t>(
                                    bone.parentIndex)]);
                }

                DirectX::XMStoreFloat4x4(
                    &currentAbsolute[boneIndex],
                    absoluteMatrix);

                StoreMatrix(
                    absoluteMatrix,
                    output.absoluteBoneMatrices[
                        boneIndex]);

                /*
                 * Mesh vertices were converted to:
                 *
                 * position - meshPivot
                 *
                 * Therefore bind/current matrices need the same
                 * object-space translation before skinning.
                 */
                const DirectX::XMMATRIX shiftedBind =
                    bindMatrix *
                    pivotTransform;

                const DirectX::XMMATRIX shiftedCurrent =
                    absoluteMatrix *
                    pivotTransform;

                DirectX::XMMATRIX inverseBind;

                if (!InvertMatrix(
                        shiftedBind,
                        inverseBind))
                {
                    error =
                        "Failed to invert bind matrix for bone " +
                        std::to_string(boneIndex) +
                        '.';

                    return false;
                }

                StoreMatrix(
                    inverseBind *
                    shiftedCurrent,
                    output.skinMatrices[
                        boneIndex]);
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            output = {};

            error =
                "Animation sampling failed: " +
                std::string(
                    exception.what());

            return false;
        }
        catch (...)
        {
            output = {};

            error =
                "Animation sampling failed with an unknown error.";

            return false;
        }
    }
}