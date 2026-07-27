#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    struct LegacyBone final
    {
        std::string name;

        std::int32_t parentIndex = -1;
        float length = 0.0F;

        /*
         * Row-major 4x4 absolute bind matrix.
         */
        std::array<float, 16U> absoluteBindMatrix
        {
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
            0.0F, 0.0F, 0.0F, 1.0F
        };
    };

    struct LegacySkeletonData final
    {
        std::filesystem::path sourcePath;

        std::uint32_t skeletonId = 0U;

        std::vector<LegacyBone> bones;

        std::size_t rootCount = 0U;
        std::size_t emptyNameCount = 0U;
        std::size_t duplicateNameCount = 0U;
        std::size_t invalidParentCount = 0U;
        std::size_t trailingByteCount = 0U;

        std::string error;
    };

    struct LegacySkinVertex final
    {
        std::array<std::uint8_t, 4U> boneIndices{};
        std::array<float, 4U> weights{};
    };

    struct LegacyWeightData final
    {
        std::filesystem::path sourcePath;

        std::uint32_t skeletonId = 0U;

        std::vector<LegacySkinVertex> vertices;

        std::size_t zeroWeightVertexCount = 0U;
        std::size_t nonNormalizedVertexCount = 0U;

        std::size_t invalidWeightValueCount = 0U;
        std::size_t invalidBoneReferenceCount = 0U;

        std::size_t trailingByteCount = 0U;

        std::uint32_t maximumBoneIndex = 0U;

        float minimumWeightSum = 0.0F;
        float maximumWeightSum = 0.0F;

        bool skeletonIdMismatch = false;

        std::string error;
    };

    class LegacySkeletalReader final
    {
    public:
        [[nodiscard]]
        static bool ReadSkeleton(
            const std::filesystem::path& path,
            LegacySkeletonData& output) noexcept;

        [[nodiscard]]
        static bool ReadWeights(
            const std::filesystem::path& path,
            const LegacySkeletonData* skeleton,
            LegacyWeightData& output) noexcept;
    };
}