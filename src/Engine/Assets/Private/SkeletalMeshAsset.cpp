#include "Assets/SkeletalMeshAsset.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace engine::assets
{
    namespace
    {
        template<std::size_t Size>
        [[nodiscard]]
        bool IsFiniteArray(
            const std::array<float, Size>& values) noexcept
        {
            for (const float value : values)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            return true;
        }
    }

    void SkeletalMeshAsset::Clear() noexcept
    {
        vertices_.clear();
        indices_.clear();
        sections_.clear();

        pivot_ = {};
        bounds_ = {};

        skeletonAssetPath_.clear();
        debugName_.clear();

        materialSlotCount_ = 0U;
    }

    bool SkeletalMeshAsset::IsValid() const noexcept
    {
        if (
            vertices_.empty() ||
            indices_.empty() ||
            sections_.empty() ||
            skeletonAssetPath_.empty() ||
            materialSlotCount_ == 0U ||
            !IsFiniteArray(pivot_) ||
            !bounds_.IsValid())
        {
            return false;
        }

        for (const SkeletalMeshVertex& vertex :
             vertices_)
        {
            if (
                !IsFiniteArray(vertex.position) ||
                !IsFiniteArray(vertex.normal) ||
                !IsFiniteArray(vertex.tangent) ||
                !IsFiniteArray(vertex.texcoord0) ||
                !IsFiniteArray(vertex.boneWeights) ||
                !std::isfinite(vertex.tangentSign))
            {
                return false;
            }

            for (const float weight :
                 vertex.boneWeights)
            {
                if (weight < 0.0F)
                {
                    return false;
                }
            }
        }

        for (const std::uint32_t index :
             indices_)
        {
            if (
                index >=
                static_cast<std::uint64_t>(
                    vertices_.size()))
            {
                return false;
            }
        }

        for (const SkeletalMeshSection& section :
             sections_)
        {
            if (
                section.indexCount == 0U ||
                section.materialSlot >=
                    materialSlotCount_ ||
                section.firstIndex >
                    indices_.size() ||
                section.indexCount >
                    indices_.size() -
                        section.firstIndex)
            {
                return false;
            }
        }

        return true;
    }

    std::size_t
    SkeletalMeshAsset::GetVertexCount() const noexcept
    {
        return vertices_.size();
    }

    std::size_t
    SkeletalMeshAsset::GetIndexCount() const noexcept
    {
        return indices_.size();
    }

    std::size_t
    SkeletalMeshAsset::GetSectionCount() const noexcept
    {
        return sections_.size();
    }

    std::uint32_t
    SkeletalMeshAsset::
        GetMaterialSlotCount() const noexcept
    {
        return materialSlotCount_;
    }

    engine::graphics::IndexFormat
    SkeletalMeshAsset::GetIndexFormat() const noexcept
    {
        return indices_.empty()
            ? engine::graphics::IndexFormat::None
            : engine::graphics::IndexFormat::UInt32;
    }

    const SkeletalMeshVertex*
    SkeletalMeshAsset::GetVertexData() const noexcept
    {
        return vertices_.empty()
            ? nullptr
            : vertices_.data();
    }

    const std::uint32_t*
    SkeletalMeshAsset::GetIndexData() const noexcept
    {
        return indices_.empty()
            ? nullptr
            : indices_.data();
    }

    std::size_t
    SkeletalMeshAsset::GetIndexDataSize() const noexcept
    {
        return
            indices_.size() *
            sizeof(std::uint32_t);
    }

    const SkeletalMeshSection*
    SkeletalMeshAsset::GetSection(
        const std::size_t index) const noexcept
    {
        return index < sections_.size()
            ? &sections_[index]
            : nullptr;
    }

    const std::array<float, 3U>&
    SkeletalMeshAsset::GetPivot() const noexcept
    {
        return pivot_;
    }

    const MeshBounds&
    SkeletalMeshAsset::GetBounds() const noexcept
    {
        return bounds_;
    }

    const std::string&
    SkeletalMeshAsset::
        GetSkeletonAssetPath() const noexcept
    {
        return skeletonAssetPath_;
    }

    const std::string&
    SkeletalMeshAsset::GetDebugName() const noexcept
    {
        return debugName_;
    }
}