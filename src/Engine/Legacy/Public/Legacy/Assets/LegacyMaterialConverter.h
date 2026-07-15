#pragma once

#include "Assets/MaterialAsset.h"
#include "Legacy/Assets/LegacyMaterialLibraryDecoder.h"

#include <vector>

namespace engine::legacy::assets
{
    class LegacyMaterialConverter final
    {
    public:
        [[nodiscard]] static engine::assets::AssetResult Convert(
            const LegacyMaterialRecord& source,
            const engine::assets::AssetPath* resolvedBaseColorTexture,
            engine::assets::MaterialAsset& outMaterial,
            std::vector<std::string>& outDiagnostics) noexcept;
    };
}
