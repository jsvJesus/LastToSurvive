#pragma once

#include "Assets/MeshAsset.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::assets
{
    /*
     * Формат одной вершины в SKMESH версии 1.
     *
     * Порядок полей должен точно совпадать с
     * WarZAssetConverter::WriteSkeletalMesh.
     */
    struct SkeletalMeshVertex final
    {
        std::array<float, 3U> position{};
        std::array<float, 3U> normal{};
        std::array<float, 3U> tangent{};

        float tangentSign = 1.0F;

        std::array<float, 2U> texcoord0{};

        std::array<std::uint8_t, 4U>
            boneIndices{};

        std::array<float, 4U> boneWeights{};
    };

    static_assert(
        sizeof(SkeletalMeshVertex) == 68U,
        "SKMESH vertex layout must remain 68 bytes.");

    struct SkeletalMeshSection final
    {
        std::uint32_t firstIndex = 0U;
        std::uint32_t indexCount = 0U;
        std::uint32_t materialSlot = 0U;

        /*
         * Например:
         *
         * Materials/Characters/char_lms_body_01_0.material
         */
        std::string materialAssetPath;
    };

    class SkeletalMeshAsset final
    {
    public:
        SkeletalMeshAsset() = default;
        ~SkeletalMeshAsset() noexcept = default;

        SkeletalMeshAsset(
            const SkeletalMeshAsset&) = delete;

        SkeletalMeshAsset& operator=(
            const SkeletalMeshAsset&) = delete;

        SkeletalMeshAsset(
            SkeletalMeshAsset&&) noexcept = default;

        SkeletalMeshAsset& operator=(
            SkeletalMeshAsset&&) noexcept = default;

        void Clear() noexcept;

        [[nodiscard]]
        bool IsValid() const noexcept;

        [[nodiscard]]
        std::size_t GetVertexCount() const noexcept;

        [[nodiscard]]
        std::size_t GetIndexCount() const noexcept;

        [[nodiscard]]
        std::size_t GetSectionCount() const noexcept;

        [[nodiscard]]
        std::uint32_t
            GetMaterialSlotCount() const noexcept;

        [[nodiscard]]
        engine::graphics::IndexFormat
            GetIndexFormat() const noexcept;

        [[nodiscard]]
        const SkeletalMeshVertex*
            GetVertexData() const noexcept;

        [[nodiscard]]
        const std::uint32_t*
            GetIndexData() const noexcept;

        [[nodiscard]]
        std::size_t
            GetIndexDataSize() const noexcept;

        [[nodiscard]]
        const SkeletalMeshSection*
            GetSection(
                std::size_t index) const noexcept;

        [[nodiscard]]
        const std::array<float, 3U>&
            GetPivot() const noexcept;

        [[nodiscard]]
        const MeshBounds&
            GetBounds() const noexcept;

        [[nodiscard]]
        const std::string&
            GetSkeletonAssetPath() const noexcept;

        [[nodiscard]]
        const std::string&
            GetDebugName() const noexcept;

    private:
        friend class SkeletalMeshAssetLoader;

        std::vector<SkeletalMeshVertex>
            vertices_;

        std::vector<std::uint32_t>
            indices_;

        std::vector<SkeletalMeshSection>
            sections_;

        std::array<float, 3U> pivot_{};

        MeshBounds bounds_;

        std::string skeletonAssetPath_;
        std::string debugName_;

        std::uint32_t materialSlotCount_ = 0U;
    };
}