#include "Assets/AnimationAsset.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace engine::assets
{
    AnimationAsset::AnimationAsset() noexcept
    {
        boneToTrack_.fill(-1);
    }

    void AnimationAsset::Clear() noexcept
    {
        skeletonAssetPath_.clear();
        debugName_.clear();

        skeletonId_ = 0U;
        frameCount_ = 0U;

        frameRate_ = 0.0F;
        durationSeconds_ = 0.0F;

        tracks_.clear();
        boneToTrack_.fill(-1);
    }

    bool AnimationAsset::IsValid() const noexcept
    {
        if (
            skeletonAssetPath_.empty() ||
            frameCount_ == 0U ||
            !std::isfinite(frameRate_) ||
            frameRate_ <= 0.0F ||
            !std::isfinite(durationSeconds_) ||
            durationSeconds_ < 0.0F ||
            tracks_.empty() ||
            tracks_.size() >
                MaximumSkeletonBones)
        {
            return false;
        }

        std::array<
            bool,
            MaximumSkeletonBones>
            usedBones{};

        for (
            std::size_t trackIndex = 0U;
            trackIndex < tracks_.size();
            ++trackIndex)
        {
            const AnimationTrack& track =
                tracks_[trackIndex];

            if (
                track.boneName.empty() ||
                track.skeletonBoneIndex < 0)
            {
                return false;
            }

            const std::size_t boneIndex =
                static_cast<std::size_t>(
                    track.skeletonBoneIndex);

            if (
                boneIndex >=
                    MaximumSkeletonBones ||
                usedBones[boneIndex] ||
                track.keys.size() !=
                    frameCount_ ||
                boneToTrack_[boneIndex] !=
                    static_cast<std::int32_t>(
                        trackIndex))
            {
                return false;
            }

            usedBones[boneIndex] = true;

            for (
                const AnimationKey& key :
                track.keys)
            {
                float quaternionLengthSquared =
                    0.0F;

                for (
                    const float value :
                    key.rotation)
                {
                    if (!std::isfinite(value))
                    {
                        return false;
                    }

                    quaternionLengthSquared +=
                        value * value;
                }

                if (
                    quaternionLengthSquared <=
                    0.0000001F)
                {
                    return false;
                }

                for (
                    const float value :
                    key.translation)
                {
                    if (!std::isfinite(value))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool AnimationAsset::IsCompatibleWith(
        const SkeletonAsset& skeleton) const noexcept
    {
        if (
            !IsValid() ||
            !skeleton.IsValid())
        {
            return false;
        }

        for (
            const AnimationTrack& track :
            tracks_)
        {
            if (
                track.skeletonBoneIndex < 0 ||
                static_cast<std::size_t>(
                    track.skeletonBoneIndex) >=
                    skeleton.GetBoneCount())
            {
                return false;
            }
        }

        /*
         * Skeleton ID у старых WarZ-анимаций может
         * отличаться после повторного экспорта.
         *
         * Индексы треков уже были сопоставлены
         * конвертером с выбранным Skeleton.
         */
        return true;
    }

    const std::string&
    AnimationAsset::
        GetSkeletonAssetPath() const noexcept
    {
        return skeletonAssetPath_;
    }

    std::uint32_t
    AnimationAsset::GetSkeletonId() const noexcept
    {
        return skeletonId_;
    }

    std::uint32_t
    AnimationAsset::GetFrameCount() const noexcept
    {
        return frameCount_;
    }

    float
    AnimationAsset::GetFrameRate() const noexcept
    {
        return frameRate_;
    }

    float
    AnimationAsset::
        GetDurationSeconds() const noexcept
    {
        return durationSeconds_;
    }

    std::size_t
    AnimationAsset::GetTrackCount() const noexcept
    {
        return tracks_.size();
    }

    const AnimationTrack*
    AnimationAsset::GetTrack(
        const std::size_t index) const noexcept
    {
        return index < tracks_.size()
            ? &tracks_[index]
            : nullptr;
    }

    const AnimationTrack*
    AnimationAsset::GetTrackForBone(
        const std::size_t boneIndex) const noexcept
    {
        if (boneIndex >= boneToTrack_.size())
        {
            return nullptr;
        }

        const std::int32_t trackIndex =
            boneToTrack_[boneIndex];

        if (
            trackIndex < 0 ||
            static_cast<std::size_t>(
                trackIndex) >= tracks_.size())
        {
            return nullptr;
        }

        return &tracks_[
            static_cast<std::size_t>(
                trackIndex)];
    }

    const std::string&
    AnimationAsset::GetDebugName() const noexcept
    {
        return debugName_;
    }
}