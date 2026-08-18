#pragma once
#include "Assets/AssetData.h"
#include "Assets/MeshAsset.h"
namespace engine::assets
{
    class LtsMeshWriter final
    { public: [[nodiscard]] static AssetResult Encode(const MeshAsset&, AssetData&) noexcept; };
}
