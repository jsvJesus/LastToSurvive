#pragma once
#include "Assets/AssetData.h"
#include "Assets/ShaderAsset.h"
namespace engine::assets
{
    class LtsShaderWriter final
    { public: [[nodiscard]] static AssetResult Encode(const ShaderAsset&, AssetData&) noexcept; };
}
