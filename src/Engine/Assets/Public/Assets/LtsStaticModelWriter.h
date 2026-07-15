#pragma once

#include "Assets/AssetData.h"
#include "Assets/StaticModelAsset.h"

namespace engine::assets
{
    class LtsStaticModelWriter final
    {
    public:
        [[nodiscard]] static AssetResult Encode(const StaticModelAsset& asset, AssetData& outData) noexcept;
    };
}
