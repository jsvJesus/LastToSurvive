#pragma once
#include "Assets/AssetResult.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace engine::assets
{
    struct TerrainLayer final
    {
        std::string name, diffusePath, normalPath, materialType;
        float scaleU=1.0F, scaleV=1.0F, specular=0.0F;
    };
    struct TerrainEmbeddedTexture final { std::uint64_t offset=0U, size=0U; };
    class TerrainAsset final
    {
    public:
        [[nodiscard]] static AssetResult Load(
            const std::filesystem::path& path, TerrainAsset& output) noexcept;
        std::uint32_t width=0U,height=0U,splatWidth=0U,splatHeight=0U;
        float tileSize=0.0F,heightOffset=0.0F,heightScale=0.0F;
        std::vector<std::int16_t> heights;
        std::vector<TerrainLayer> layers;
        std::vector<TerrainEmbeddedTexture> masks;
        TerrainEmbeddedTexture colorMap, normalMap;
        std::filesystem::path sourcePath;
        [[nodiscard]] float GetHeight(std::uint32_t x,std::uint32_t z) const noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
    };
}
