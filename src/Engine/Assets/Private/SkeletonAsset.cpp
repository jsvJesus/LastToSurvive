#include "Assets/SkeletonAsset.h"

#include <cmath>
#include <cstddef>

namespace engine::assets
{
    void SkeletonAsset::Clear() noexcept
    {
        skeletonId_ = 0U;

        bones_.clear();
        debugName_.clear();
    }

    bool SkeletonAsset::IsValid() const noexcept
    {
        if (
            bones_.empty() ||
            bones_.size() >
                MaximumSkeletonBones)
        {
            return false;
        }

        std::size_t rootCount = 0U;

        for (
            std::size_t boneIndex = 0U;
            boneIndex < bones_.size();
            ++boneIndex)
        {
            const SkeletonBone& bone =
                bones_[boneIndex];

            if (
                bone.name.empty() ||
                !std::isfinite(bone.length))
            {
                return false;
            }

            if (bone.parentIndex < 0)
            {
                ++rootCount;
            }
            else
            {
                const std::size_t parentIndex =
                    static_cast<std::size_t>(
                        bone.parentIndex);

                /*
                 * WarZ skeleton хранится в порядке:
                 *
                 * parent раньше child.
                 */
                if (parentIndex >= boneIndex)
                {
                    return false;
                }
            }

            for (
                const float value :
                bone.absoluteBindMatrix)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }
        }

        return rootCount != 0U;
    }

    std::uint32_t
    SkeletonAsset::GetSkeletonId() const noexcept
    {
        return skeletonId_;
    }

    std::size_t
    SkeletonAsset::GetBoneCount() const noexcept
    {
        return bones_.size();
    }

    const SkeletonBone*
    SkeletonAsset::GetBone(
        const std::size_t index) const noexcept
    {
        return index < bones_.size()
            ? &bones_[index]
            : nullptr;
    }

    const std::string&
    SkeletonAsset::GetDebugName() const noexcept
    {
        return debugName_;
    }
}