#pragma once
#include "Assets/AssetData.h"
#include "Assets/AssetResult.h"
#include "Graphics/Shader.h"
#include <filesystem>
#include <string>
#include <vector>
namespace lts::asset_cooker
{
    struct ShaderDefine final { std::string name; std::string value; };
    struct ShaderCookOptions final
    {
        std::filesystem::path input;
        std::filesystem::path includeRoot;
        std::string entryPoint;
        std::string targetProfile;
        engine::graphics::ShaderStage stage=engine::graphics::ShaderStage::Unknown;
        std::vector<ShaderDefine> defines;
    };
    [[nodiscard]] engine::assets::AssetResult CookShader(const ShaderCookOptions&,engine::assets::AssetData&,std::string& diagnostics) noexcept;
}
