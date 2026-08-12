#pragma once

#include "Assets/AssetData.h"
#include "Assets/MaterialAsset.h"

namespace engine::assets
{
    class MaterialAssetWriter final
    {
    public:
        [[nodiscard]]
        static AssetResult Encode(
            const MaterialAsset& asset,
            AssetData& output) noexcept;
    };
}