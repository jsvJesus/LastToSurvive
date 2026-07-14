#pragma once
#include "Assets/AssetLoader.h"
#include "Assets/MeshAsset.h"
#include <utility>
namespace engine::assets
{
    class MeshLoadedAsset final : public LoadedAsset
    {
    public: explicit MeshLoadedAsset(MeshAsset&& v) noexcept : value_(std::move(v)) {} [[nodiscard]] AssetType GetType() const noexcept override { return AssetType::Mesh; } [[nodiscard]] const MeshAsset& GetMesh() const noexcept { return value_; } [[nodiscard]] MeshAsset ReleaseMesh() noexcept { return std::move(value_); }
    private: MeshAsset value_;
    };
    class MeshAssetLoader final : public AssetLoader
    {
    public: [[nodiscard]] AssetType GetAssetType() const noexcept override { return AssetType::Mesh; } [[nodiscard]] AssetResult Load(const AssetMetadata&, const AssetData&, std::unique_ptr<LoadedAsset>&) noexcept override;
    };
}
