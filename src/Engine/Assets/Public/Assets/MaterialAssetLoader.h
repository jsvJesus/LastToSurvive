#pragma once
#include "Assets/AssetLoader.h"
#include "Assets/MaterialAsset.h"
#include <utility>
namespace engine::assets
{
    class MaterialLoadedAsset final : public LoadedAsset
    {
    public: explicit MaterialLoadedAsset(MaterialAsset&& v) noexcept:value_(std::move(v)){} [[nodiscard]] AssetType GetType()const noexcept override{return AssetType::Material;} [[nodiscard]] const MaterialAsset& GetMaterial()const noexcept{return value_;} [[nodiscard]] MaterialAsset ReleaseMaterial()noexcept{return std::move(value_);}
    private:MaterialAsset value_;
    };
    class MaterialAssetLoader final : public AssetLoader
    {
    public:[[nodiscard]] AssetType GetAssetType()const noexcept override{return AssetType::Material;} [[nodiscard]] AssetResult Load(const AssetMetadata&,const AssetData&,std::unique_ptr<LoadedAsset>&)noexcept override;
    };
}
