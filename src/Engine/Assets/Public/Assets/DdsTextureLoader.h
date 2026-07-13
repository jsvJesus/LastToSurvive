#pragma once

#include "Assets/AssetLoader.h"
#include "Assets/DdsTextureDecoder.h"
#include "Assets/TextureAsset.h"

#include <utility>

namespace engine::assets
{
    class TextureLoadedAsset final : public LoadedAsset
    {
    public:
        explicit TextureLoadedAsset(
            TextureAsset&& textureAsset) noexcept
            : textureAsset_(std::move(textureAsset))
        {
        }

        [[nodiscard]] AssetType GetType() const noexcept override
        {
            return AssetType::Texture;
        }

        [[nodiscard]] const TextureAsset& GetTexture() const noexcept
        {
            return textureAsset_;
        }

        [[nodiscard]] TextureAsset ReleaseTexture() noexcept
        {
            return std::move(textureAsset_);
        }

    private:
        TextureAsset textureAsset_;
    };

    class DdsTextureLoader final : public AssetLoader
    {
    public:
        DdsTextureLoader() noexcept = default;

        explicit DdsTextureLoader(
            const DdsTextureDecodeOptions& options) noexcept
            : options_(options)
        {
        }

        [[nodiscard]] AssetType GetAssetType() const noexcept override
        {
            return AssetType::Texture;
        }

        [[nodiscard]] AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>& outAsset) noexcept override;

        void SetOptions(
            const DdsTextureDecodeOptions& options) noexcept
        {
            options_ = options;
        }

        [[nodiscard]] const DdsTextureDecodeOptions& GetOptions() const noexcept
        {
            return options_;
        }

    private:
        DdsTextureDecodeOptions options_;
    };
}
