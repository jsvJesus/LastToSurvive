#pragma once

#include "Assets/AssetResult.h"
#include "Assets/MeshAsset.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace engine::assets
{
    class MeshAssetBuilder final
    {
    public:
        [[nodiscard]] static AssetResult Build(
            const StaticMeshVertex* vertices, std::size_t vertexCount,
            const std::uint16_t* indices, std::size_t indexCount,
            const MeshSubmesh* submeshes, std::size_t submeshCount,
            std::uint32_t materialSlotCount, std::string_view debugName,
            MeshAsset& outAsset) noexcept;
        [[nodiscard]] static AssetResult Build(
            const StaticMeshVertex* vertices, std::size_t vertexCount,
            const std::uint32_t* indices, std::size_t indexCount,
            const MeshSubmesh* submeshes, std::size_t submeshCount,
            std::uint32_t materialSlotCount, std::string_view debugName,
            MeshAsset& outAsset) noexcept;
    private:
        template<typename Index>
        [[nodiscard]] static AssetResult BuildImpl(
            const StaticMeshVertex*, std::size_t, const Index*, std::size_t,
            const MeshSubmesh*, std::size_t, std::uint32_t, std::string_view,
            MeshAsset&) noexcept;
    };
}
