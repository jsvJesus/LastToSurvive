#pragma once
#include "Assets/AssetLoader.h"
#include "Assets/ShaderAsset.h"
#include <utility>
namespace engine::assets
{
    class ShaderLoadedAsset final : public LoadedAsset
    {
    public:
        explicit ShaderLoadedAsset(ShaderAsset&& value) noexcept : value_(std::move(value)) {}
        [[nodiscard]] AssetType GetType() const noexcept override { return AssetType::Shader; }
        [[nodiscard]] const ShaderAsset& GetShader() const noexcept { return value_; }
        [[nodiscard]] ShaderAsset ReleaseShader() noexcept { return std::move(value_); }
    private: ShaderAsset value_;
    };
    class ShaderAssetLoader final : public AssetLoader
    {
    public:
        [[nodiscard]] AssetType GetAssetType() const noexcept override { return AssetType::Shader; }
        [[nodiscard]] AssetResult Load(const AssetMetadata&, const AssetData&, std::unique_ptr<LoadedAsset>&) noexcept override;
    };
}
