#pragma once

#include "Editor/Tools/Import/LegacySkeletalReader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    inline constexpr std::size_t LegacyAnimationMaximumBones =
        128U;

    struct LegacyAnimationKey final
    {
        std::array<float, 4U> rotation
        {
            0.0F,
            0.0F,
            0.0F,
            1.0F
        };

        std::array<float, 3U> translation{};
    };

    struct LegacyAnimationTrack final
    {
        std::string boneName;

        std::uint32_t flags = 0U;
        std::int32_t skeletonBoneIndex = -1;

        std::vector<LegacyAnimationKey> keys;

        [[nodiscard]]
        bool IsRootTrack() const noexcept
        {
            return (flags & (1U << 1U)) != 0U;
        }
    };

    struct LegacyAnimationData final
    {
        std::filesystem::path sourcePath;

        std::uint32_t skeletonId = 0U;
        std::uint32_t frameCount = 0U;

        float frameRate = 0.0F;
        float durationSeconds = 0.0F;

        std::vector<LegacyAnimationTrack> tracks;

        /*
         * skeleton bone index -> animation track index.
         */
        std::vector<std::int32_t> boneToTrack;

        std::size_t mappedTrackCount = 0U;
        std::size_t missingBoneTrackCount = 0U;
        std::size_t duplicateTrackNameCount = 0U;
        std::size_t rootTrackCount = 0U;

        std::size_t invalidQuaternionCount = 0U;
        std::size_t nonNormalizedQuaternionCount = 0U;
        std::size_t invalidTranslationCount = 0U;

        std::size_t trailingByteCount = 0U;

        bool skeletonIdMismatch = false;

        std::string error;

        [[nodiscard]]
        bool IsCompatible() const noexcept
        {
            /*
             * Legacy WarZ связывает animation tracks со skeleton
             * по именам костей, а не запрещает animation из-за
             * несовпадения экспортного Skeleton ID.
             *
             * Skeleton ID остаётся диагностическим предупреждением.
             */
            return
                mappedTrackCount != 0U &&
                !tracks.empty() &&
                frameCount != 0U &&
                frameRate > 0.0F;
        }
    };

    struct LegacyAnimationPose final
    {
        /*
         * Текущие абсолютные матрицы костей.
         * Используются для skeleton overlay.
         */
        std::vector<std::array<float, 16U>>
            absoluteBoneMatrices;

        /*
         * Матрицы для GPU skinning:
         *
         * inverse(bind pose) * current pose
         */
        std::vector<std::array<float, 16U>>
            skinMatrices;
    };

    class LegacyAnimationReader final
    {
    public:
        [[nodiscard]]
        static bool Read(
            const std::filesystem::path& path,
            const LegacySkeletonData* skeleton,
            LegacyAnimationData& output) noexcept;

        [[nodiscard]]
        static bool Sample(
            const LegacyAnimationData& animation,
            const LegacySkeletonData& skeleton,
            const std::array<float, 3U>& meshPivot,
            float frame,
            bool loopInterpolation,
            bool lockRootHorizontalMovement,
            LegacyAnimationPose& output,
            std::string& error) noexcept;
    };
}