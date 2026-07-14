#include "Assets/MeshAsset.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace engine::assets
{
    bool MeshBounds::IsValid() const noexcept
    {
        for (std::size_t i = 0U; i < 3U; ++i)
        {
            if (!std::isfinite(minimum[i]) || !std::isfinite(maximum[i]) ||
                !std::isfinite(sphereCenter[i]) || minimum[i] > maximum[i])
                return false;
        }
        return std::isfinite(sphereRadius) && sphereRadius >= 0.0F;
    }

    void MeshAsset::Clear() noexcept
    {
        vertices_.clear(); indices_.clear(); submeshes_.clear(); debugName_.clear();
        indexFormat_ = engine::graphics::IndexFormat::None;
        materialSlotCount_ = 0U; bounds_ = {};
    }

    bool MeshAsset::IsValid() const noexcept
    {
        if (vertices_.empty() || indices_.empty() || submeshes_.empty() ||
            materialSlotCount_ == 0U || !bounds_.IsValid()) return false;
        const std::size_t elementSize = indexFormat_ == engine::graphics::IndexFormat::UInt16 ? 2U :
            indexFormat_ == engine::graphics::IndexFormat::UInt32 ? 4U : 0U;
        if (elementSize == 0U || indices_.size() % elementSize != 0U) return false;
        const std::size_t indexCount = indices_.size() / elementSize;
        for (const StaticMeshVertex& vertex : vertices_)
        {
            for (const float value : vertex.position) if (!std::isfinite(value)) return false;
            for (const float value : vertex.normal) if (!std::isfinite(value)) return false;
            for (const float value : vertex.tangent) if (!std::isfinite(value)) return false;
            for (const float value : vertex.texcoord0) if (!std::isfinite(value)) return false;
        }
        for (const MeshSubmesh& submesh : submeshes_)
        {
            if (submesh.indexCount == 0U || submesh.materialSlot >= materialSlotCount_ ||
                submesh.firstIndex > indexCount || submesh.indexCount > indexCount - submesh.firstIndex) return false;
            for (std::size_t n = 0U; n < submesh.indexCount; ++n)
            {
                const std::size_t offset = (static_cast<std::size_t>(submesh.firstIndex) + n) * elementSize;
                std::uint32_t raw = 0U;
                if (elementSize == 2U) { std::uint16_t value = 0U; std::memcpy(&value, indices_.data() + offset, 2U); raw = value; }
                else std::memcpy(&raw, indices_.data() + offset, 4U);
                const std::int64_t vertex = static_cast<std::int64_t>(raw) + submesh.baseVertex;
                if (vertex < 0 || static_cast<std::uint64_t>(vertex) >= vertices_.size()) return false;
            }
        }
        return true;
    }

    std::size_t MeshAsset::GetVertexCount() const noexcept { return vertices_.size(); }
    std::size_t MeshAsset::GetIndexCount() const noexcept
    { const std::size_t s = indexFormat_ == engine::graphics::IndexFormat::UInt16 ? 2U : indexFormat_ == engine::graphics::IndexFormat::UInt32 ? 4U : 0U; return s ? indices_.size() / s : 0U; }
    engine::graphics::IndexFormat MeshAsset::GetIndexFormat() const noexcept { return indexFormat_; }
    std::size_t MeshAsset::GetSubmeshCount() const noexcept { return submeshes_.size(); }
    const MeshSubmesh* MeshAsset::GetSubmesh(const std::size_t index) const noexcept { return index < submeshes_.size() ? &submeshes_[index] : nullptr; }
    const MeshBounds& MeshAsset::GetBounds() const noexcept { return bounds_; }
    const StaticMeshVertex* MeshAsset::GetVertexData() const noexcept { return vertices_.empty() ? nullptr : vertices_.data(); }
    const std::byte* MeshAsset::GetIndexData() const noexcept { return indices_.empty() ? nullptr : indices_.data(); }
    std::size_t MeshAsset::GetIndexDataSize() const noexcept { return indices_.size(); }
    std::uint32_t MeshAsset::GetMaterialSlotCount() const noexcept { return materialSlotCount_; }
    const std::string& MeshAsset::GetDebugName() const noexcept { return debugName_; }
}
