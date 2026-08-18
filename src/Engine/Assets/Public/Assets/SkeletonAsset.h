#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::assets
{
    /*
     * Текущий GPU skinning использует constant buffer
     * на 128 костей — столько же поддерживает старый preview.
     */
    inline constexpr std::size_t MaximumSkeletonBones =
        128U;

    struct SkeletonBone final
    {
        std::string name;

        std::int32_t parentIndex = -1;
        float length = 0.0F;

        /*
         * Row-major абсолютная bind-матрица.
         */
        std::array<float, 16U> absoluteBindMatrix
        {
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
            0.0F, 0.0F, 0.0F, 1.0F
        };
    };

    class SkeletonAsset final
    {
    public:
        SkeletonAsset() = default;
        ~SkeletonAsset() noexcept = default;

        SkeletonAsset(
            const SkeletonAsset&) = delete;

        SkeletonAsset& operator=(
            const SkeletonAsset&) = delete;

        SkeletonAsset(
            SkeletonAsset&&) noexcept = default;

        SkeletonAsset& operator=(
            SkeletonAsset&&) noexcept = default;

        void Clear() noexcept;

        [[nodiscard]]
        bool IsValid() const noexcept;

        [[nodiscard]]
        std::uint32_t GetSkeletonId() const noexcept;

        [[nodiscard]]
        std::size_t GetBoneCount() const noexcept;

        [[nodiscard]]
        const SkeletonBone* GetBone(
            std::size_t index) const noexcept;

        [[nodiscard]]
        const std::string& GetDebugName() const noexcept;

    private:
        friend class SkeletonAssetLoader;

        std::uint32_t skeletonId_ = 0U;

        std::vector<SkeletonBone> bones_;
        std::string debugName_;
    };
}