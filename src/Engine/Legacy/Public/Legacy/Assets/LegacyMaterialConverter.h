#pragma once

#include "Assets/MaterialAsset.h"
#include "Assets/TextureAsset.h"
#include "Legacy/Assets/LegacyMaterialLibraryDecoder.h"

#include <vector>

namespace engine::legacy::assets
{
    enum class LegacyTextureAlpha : std::uint8_t { NoAlpha = 0, HasAlpha, Ambiguous };

    class LegacyMaterialConverter final
    {
    public:
        [[nodiscard]] static engine::assets::AssetResult Convert(
            const LegacyMaterialRecord& source,
            const engine::assets::AssetPath* resolvedBaseColorTexture,
            LegacyTextureAlpha textureAlpha,
            engine::assets::MaterialAsset& outMaterial,
            std::vector<std::string>& outDiagnostics) noexcept;
        [[nodiscard]] static engine::assets::AssetResult Convert(
            const LegacyMaterialRecord& source,
            const engine::assets::AssetPath* resolvedBaseColorTexture,
            engine::assets::MaterialAsset& outMaterial,
            std::vector<std::string>& outDiagnostics) noexcept
        { return Convert(source, resolvedBaseColorTexture, LegacyTextureAlpha::NoAlpha, outMaterial, outDiagnostics); }
        [[nodiscard]] static LegacyTextureAlpha DetectTextureAlpha(
            const engine::assets::TextureAsset& texture) noexcept;
    };
}
