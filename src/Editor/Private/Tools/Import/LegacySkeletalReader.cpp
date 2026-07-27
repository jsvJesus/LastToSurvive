#include "Editor/Tools/Import/LegacySkeletalReader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

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

        constexpr std::size_t BoneNameSize = 32U;
        constexpr std::size_t SkeletonBoneDiskSize = 88U;
        constexpr std::size_t WeightVertexDiskSize = 20U;

        constexpr std::uint32_t MaximumBoneCount = 4096U;
        constexpr std::uint32_t MaximumVertexCount = 50000000U;

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

        constexpr std::uint32_t SkeletonAssetId =
            MakeLegacyId('t', 'l', 'k', 's');

        constexpr std::uint32_t SkeletonVersion = 1U;

        constexpr std::uint32_t WeightAssetId =
            MakeLegacyId('t', 'h', 'g', 'w');

        constexpr std::uint32_t WeightVersion = 1U;

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

            return stream.gcount() ==
                static_cast<std::streamsize>(
                    sizeof(Value));
        }

        [[nodiscard]]
        bool ReadBytes(
            std::ifstream& stream,
            void* destination,
            const std::size_t size) noexcept
        {
            if (destination == nullptr || size == 0U)
            {
                return false;
            }

            if (size >
                static_cast<std::size_t>(
                    (std::numeric_limits<
                        std::streamsize>::max)()))
            {
                return false;
            }

            stream.read(
                static_cast<char*>(destination),
                static_cast<std::streamsize>(size));

            return stream.gcount() ==
                static_cast<std::streamsize>(size);
        }

        [[nodiscard]]
        bool GetFileSize(
            const std::filesystem::path& path,
            std::uintmax_t& size) noexcept
        {
            std::error_code error;

            size = std::filesystem::file_size(
                path,
                error);

            return !error;
        }

        [[nodiscard]]
        bool ReadHeader(
            std::ifstream& stream,
            const std::uint32_t expectedAssetId,
            const std::uint32_t expectedVersion,
            std::string& error) noexcept
        {
            BinaryHeader header;

            if (!ReadValue(stream, header))
            {
                error = "Failed to read binary header.";
                return false;
            }

            if (header.fileId != BinaryFileId)
            {
                error = "Invalid WarZ binary file ID.";
                return false;
            }

            if (header.assetId != expectedAssetId)
            {
                error = "Unexpected WarZ asset type.";
                return false;
            }

            if (header.version != expectedVersion)
            {
                error =
                    "Unsupported WarZ asset version: " +
                    std::to_string(header.version) +
                    '.';

                return false;
            }

            return true;
        }

        [[nodiscard]]
        std::string ReadBoneName(
            const std::array<char, BoneNameSize>& buffer)
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
    }

    bool LegacySkeletalReader::ReadSkeleton(
        const std::filesystem::path& path,
        LegacySkeletonData& output) noexcept
    {
        output = {};
        output.sourcePath = path;

        try
        {
            std::uintmax_t fileSize = 0U;

            if (!GetFileSize(path, fileSize))
            {
                output.error =
                    "Failed to query skeleton file size.";

                return false;
            }

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream)
            {
                output.error =
                    "Failed to open skeleton file.";

                return false;
            }

            if (!ReadHeader(
                    stream,
                    SkeletonAssetId,
                    SkeletonVersion,
                    output.error))
            {
                return false;
            }

            std::uint32_t boneCount = 0U;

            if (!ReadValue(stream, output.skeletonId) ||
                !ReadValue(stream, boneCount))
            {
                output.error =
                    "Failed to read skeleton information.";

                return false;
            }

            if (boneCount == 0U ||
                boneCount > MaximumBoneCount)
            {
                output.error =
                    "Invalid skeleton bone count: " +
                    std::to_string(boneCount) +
                    '.';

                return false;
            }

            const std::uintmax_t expectedSize =
                sizeof(BinaryHeader) +
                sizeof(std::uint32_t) * 2U +
                static_cast<std::uintmax_t>(
                    boneCount) *
                SkeletonBoneDiskSize;

            if (fileSize < expectedSize)
            {
                output.error =
                    "Skeleton file is truncated.";

                return false;
            }

            output.trailingByteCount =
                static_cast<std::size_t>(
                    fileSize - expectedSize);

            output.bones.reserve(boneCount);

            std::unordered_set<std::string> boneNames;
            boneNames.reserve(boneCount);

            for (std::uint32_t boneIndex = 0U;
                 boneIndex < boneCount;
                 ++boneIndex)
            {
                std::array<char, BoneNameSize>
                    nameBuffer{};

                std::uint32_t parentId = 0U;
                float length = 0.0F;

                std::array<float, 12U>
                    diskMatrix{};

                if (!ReadBytes(
                        stream,
                        nameBuffer.data(),
                        nameBuffer.size()) ||
                    !ReadValue(stream, parentId) ||
                    !ReadValue(stream, length) ||
                    !ReadBytes(
                        stream,
                        diskMatrix.data(),
                        sizeof(diskMatrix)))
                {
                    output.error =
                        "Failed to read skeleton bone " +
                        std::to_string(boneIndex) +
                        '.';

                    output.bones.clear();
                    return false;
                }

                LegacyBone bone;
                bone.name =
                    ReadBoneName(nameBuffer);

                bone.parentIndex =
                    parentId == 0xFFFFFFFFU
                        ? -1
                        : static_cast<std::int32_t>(
                            parentId);

                bone.length = length;

                /*
                 * The legacy file stores:
                 *
                 * _11 _21 _31 _41
                 * _12 _22 _32 _42
                 * _13 _23 _33 _43
                 */
                bone.absoluteBindMatrix =
                {
                    diskMatrix[0U],
                    diskMatrix[4U],
                    diskMatrix[8U],
                    0.0F,

                    diskMatrix[1U],
                    diskMatrix[5U],
                    diskMatrix[9U],
                    0.0F,

                    diskMatrix[2U],
                    diskMatrix[6U],
                    diskMatrix[10U],
                    0.0F,

                    diskMatrix[3U],
                    diskMatrix[7U],
                    diskMatrix[11U],
                    1.0F
                };

                if (bone.name.empty())
                {
                    ++output.emptyNameCount;
                }
                else
                {
                    const std::string normalizedName =
                        ToLowerAscii(bone.name);

                    if (!boneNames.insert(
                            normalizedName).second)
                    {
                        ++output.duplicateNameCount;
                    }
                }

                if (bone.parentIndex == -1)
                {
                    ++output.rootCount;
                }
                else if (
                    bone.parentIndex < 0 ||
                    bone.parentIndex >=
                        static_cast<std::int32_t>(
                            boneCount) ||
                    bone.parentIndex ==
                        static_cast<std::int32_t>(
                            boneIndex))
                {
                    ++output.invalidParentCount;
                }

                output.bones.push_back(
                    std::move(bone));
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            output.error =
                "Skeleton read failed: " +
                std::string(exception.what());

            output.bones.clear();
            return false;
        }
        catch (...)
        {
            output.error =
                "Skeleton read failed with an unknown error.";

            output.bones.clear();
            return false;
        }
    }

    bool LegacySkeletalReader::ReadWeights(
        const std::filesystem::path& path,
        const LegacySkeletonData* skeleton,
        LegacyWeightData& output) noexcept
    {
        output = {};
        output.sourcePath = path;

        try
        {
            std::uintmax_t fileSize = 0U;

            if (!GetFileSize(path, fileSize))
            {
                output.error =
                    "Failed to query weight file size.";

                return false;
            }

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream)
            {
                output.error =
                    "Failed to open weight file.";

                return false;
            }

            if (!ReadHeader(
                    stream,
                    WeightAssetId,
                    WeightVersion,
                    output.error))
            {
                return false;
            }

            std::uint32_t vertexCount = 0U;

            if (!ReadValue(stream, output.skeletonId) ||
                !ReadValue(stream, vertexCount))
            {
                output.error =
                    "Failed to read weight information.";

                return false;
            }

            if (vertexCount == 0U ||
                vertexCount > MaximumVertexCount)
            {
                output.error =
                    "Invalid weight vertex count: " +
                    std::to_string(vertexCount) +
                    '.';

                return false;
            }

            const std::uintmax_t expectedSize =
                sizeof(BinaryHeader) +
                sizeof(std::uint32_t) * 2U +
                static_cast<std::uintmax_t>(
                    vertexCount) *
                WeightVertexDiskSize;

            if (fileSize < expectedSize)
            {
                output.error =
                    "Weight file is truncated.";

                return false;
            }

            output.trailingByteCount =
                static_cast<std::size_t>(
                    fileSize - expectedSize);

            output.vertices.resize(vertexCount);

            output.minimumWeightSum =
                (std::numeric_limits<float>::max)();

            const std::size_t skeletonBoneCount =
                skeleton != nullptr
                    ? skeleton->bones.size()
                    : 0U;

            if (skeleton != nullptr &&
                output.skeletonId != 0U &&
                skeleton->skeletonId != 0U &&
                output.skeletonId !=
                    skeleton->skeletonId)
            {
                output.skeletonIdMismatch = true;
            }

            for (std::uint32_t vertexIndex = 0U;
                 vertexIndex < vertexCount;
                 ++vertexIndex)
            {
                LegacySkinVertex& vertex =
                    output.vertices[vertexIndex];

                if (!ReadBytes(
                        stream,
                        vertex.boneIndices.data(),
                        vertex.boneIndices.size()) ||
                    !ReadBytes(
                        stream,
                        vertex.weights.data(),
                        sizeof(vertex.weights)))
                {
                    output.error =
                        "Failed to read weights for vertex " +
                        std::to_string(vertexIndex) +
                        '.';

                    output.vertices.clear();
                    return false;
                }

                float weightSum = 0.0F;

                for (std::size_t influenceIndex = 0U;
                     influenceIndex <
                         vertex.weights.size();
                     ++influenceIndex)
                {
                    const float weight =
                        vertex.weights[influenceIndex];

                    const std::uint32_t boneIndex =
                        vertex.boneIndices[
                            influenceIndex];

                    output.maximumBoneIndex =
                        (std::max)(
                            output.maximumBoneIndex,
                            boneIndex);

                    if (!std::isfinite(weight) ||
                        weight < 0.0F ||
                        weight > 1.0F)
                    {
                        ++output.invalidWeightValueCount;
                        continue;
                    }

                    weightSum += weight;

                    if (weight > 0.000001F &&
                        skeleton != nullptr &&
                        boneIndex >= skeletonBoneCount)
                    {
                        ++output.invalidBoneReferenceCount;
                    }
                }

                output.minimumWeightSum =
                    (std::min)(
                        output.minimumWeightSum,
                        weightSum);

                output.maximumWeightSum =
                    (std::max)(
                        output.maximumWeightSum,
                        weightSum);

                if (weightSum <= 0.000001F)
                {
                    ++output.zeroWeightVertexCount;
                }
                else if (
                    std::abs(weightSum - 1.0F) >
                    0.01F)
                {
                    ++output.nonNormalizedVertexCount;
                }
            }

            if (output.vertices.empty())
            {
                output.minimumWeightSum = 0.0F;
            }

            return true;
        }
        catch (const std::exception& exception)
        {
            output.error =
                "Weight read failed: " +
                std::string(exception.what());

            output.vertices.clear();
            return false;
        }
        catch (...)
        {
            output.error =
                "Weight read failed with an unknown error.";

            output.vertices.clear();
            return false;
        }
    }
}