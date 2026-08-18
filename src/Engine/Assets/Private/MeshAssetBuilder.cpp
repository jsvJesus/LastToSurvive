#include "Assets/MeshAssetBuilder.h"
#include "Assets/AssetPath.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace engine::assets
{
        template<typename Index>
        AssetResult MeshAssetBuilder::BuildImpl(const StaticMeshVertex* vertices, const std::size_t vertexCount,
            const Index* indices, const std::size_t indexCount, const MeshSubmesh* submeshes,
            const std::size_t submeshCount, const std::uint32_t materialSlotCount,
            const std::string_view debugName, MeshAsset& outAsset) noexcept
        {
            if (!vertices || !indices || !submeshes || vertexCount == 0U || indexCount == 0U ||
                submeshCount == 0U || materialSlotCount == 0U) return AssetResult::InvalidArgument;
            if (vertexCount > UINT32_MAX || indexCount > UINT32_MAX || submeshCount > UINT32_MAX ||
                debugName.size() > AssetPath::MaximumLength) return AssetResult::ReferenceOverflow;
            MeshAsset candidate;
            try
            {
                candidate.vertices_.assign(vertices, vertices + vertexCount);
                candidate.indices_.resize(indexCount * sizeof(Index));
                std::memcpy(candidate.indices_.data(), indices, candidate.indices_.size());
                candidate.submeshes_.assign(submeshes, submeshes + submeshCount);
                candidate.debugName_.assign(debugName.data(), debugName.size());
            }
            catch (const std::bad_alloc&) { return AssetResult::OutOfMemory; }
            catch (...) { return AssetResult::InternalError; }
            candidate.indexFormat_ = sizeof(Index) == 2U ? engine::graphics::IndexFormat::UInt16 : engine::graphics::IndexFormat::UInt32;
            candidate.materialSlotCount_ = materialSlotCount;
            for (const StaticMeshVertex& vertex : candidate.vertices_)
                for (const float value : vertex.position) if (!std::isfinite(value)) return AssetResult::InvalidArgument;
            candidate.bounds_.minimum = candidate.vertices_[0].position;
            candidate.bounds_.maximum = candidate.vertices_[0].position;
            for (const StaticMeshVertex& vertex : candidate.vertices_)
                for (std::size_t axis = 0U; axis < 3U; ++axis) { candidate.bounds_.minimum[axis] = (std::min)(candidate.bounds_.minimum[axis], vertex.position[axis]); candidate.bounds_.maximum[axis] = (std::max)(candidate.bounds_.maximum[axis], vertex.position[axis]); }
            for (std::size_t axis = 0U; axis < 3U; ++axis) candidate.bounds_.sphereCenter[axis] = (candidate.bounds_.minimum[axis] + candidate.bounds_.maximum[axis]) * 0.5F;
            float radiusSquared = 0.0F;
            for (const StaticMeshVertex& vertex : candidate.vertices_) { float d = 0.0F; for (std::size_t axis = 0U; axis < 3U; ++axis) { const float x = vertex.position[axis] - candidate.bounds_.sphereCenter[axis]; d += x * x; } radiusSquared = (std::max)(radiusSquared, d); }
            candidate.bounds_.sphereRadius = std::sqrt(radiusSquared);
            if (!candidate.IsValid()) return AssetResult::CorruptData;
            outAsset = std::move(candidate);
            return AssetResult::Success;
        }
    AssetResult MeshAssetBuilder::Build(const StaticMeshVertex* v, std::size_t vc, const std::uint16_t* i, std::size_t ic, const MeshSubmesh* s, std::size_t sc, std::uint32_t mc, std::string_view n, MeshAsset& o) noexcept { return BuildImpl(v, vc, i, ic, s, sc, mc, n, o); }
    AssetResult MeshAssetBuilder::Build(const StaticMeshVertex* v, std::size_t vc, const std::uint32_t* i, std::size_t ic, const MeshSubmesh* s, std::size_t sc, std::uint32_t mc, std::string_view n, MeshAsset& o) noexcept { return BuildImpl(v, vc, i, ic, s, sc, mc, n, o); }
}
