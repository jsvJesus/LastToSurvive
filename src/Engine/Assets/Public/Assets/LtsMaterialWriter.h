#pragma once
#include "Assets/AssetData.h"
#include "Assets/MaterialAsset.h"
namespace engine::assets
{
    class LtsMaterialWriter final
    { public: [[nodiscard]] static AssetResult Encode(const MaterialAsset&, AssetData&) noexcept; };
}
