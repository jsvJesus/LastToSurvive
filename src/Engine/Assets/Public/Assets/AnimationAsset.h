#pragma once

#include "Assets/SkeletonAsset.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::assets
{
    struct AnimationKey final
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

    static_assert(
        sizeof(AnimationKey) == 28U);

    struct AnimationTrack final
    {
        std::string boneName;

        std::int32_t skeletonBoneIndex = -1;
        std::uint32_t flags = 0U;

        std::vector<AnimationKey> keys;

        [[nodiscard]]
        bool IsRootTrack() const noexcept
        {
            return
                (flags & (1U << 1U)) != 0U;
        }
    };

    class AnimationAsset final
    {
    public:
        AnimationAsset() noexcept;
        ~AnimationAsset() noexcept = default;

        AnimationAsset(
            const AnimationAsset&) = delete;

        AnimationAsset& operator=(
            const AnimationAsset&) = delete;

        AnimationAsset(
            AnimationAsset&&) noexcept = default;

        AnimationAsset& operator=(
            AnimationAsset&&) noexcept = default;

        void Clear() noexcept;

        [[nodiscard]]
        bool IsValid() const noexcept;

        [[nodiscard]]
        bool IsCompatibleWith(
            const SkeletonAsset& skeleton) const noexcept;

        [[nodiscard]]
        const std::string&
            GetSkeletonAssetPath() const noexcept;

        [[nodiscard]]
        std::uint32_t GetSkeletonId() const noexcept;

        [[nodiscard]]
        std::uint32_t GetFrameCount() const noexcept;

        [[nodiscard]]
        float GetFrameRate() const noexcept;

        [[nodiscard]]
        float GetDurationSeconds() const noexcept;

        [[nodiscard]]
        std::size_t GetTrackCount() const noexcept;

        [[nodiscard]]
        const AnimationTrack* GetTrack(
            std::size_t index) const noexcept;

        [[nodiscard]]
        const AnimationTrack* GetTrackForBone(
            std::size_t boneIndex) const noexcept;

        [[nodiscard]]
        const std::string&
            GetDebugName() const noexcept;

    private:
        friend class AnimationAssetLoader;

        std::string skeletonAssetPath_;
        std::string debugName_;

        std::uint32_t skeletonId_ = 0U;
        std::uint32_t frameCount_ = 0U;

        float frameRate_ = 0.0F;
        float durationSeconds_ = 0.0F;

        std::vector<AnimationTrack> tracks_;

        /*
         * skeleton bone index -> track index
         */
        std::array<
            std::int32_t,
            MaximumSkeletonBones>
            boneToTrack_{};
    };
}