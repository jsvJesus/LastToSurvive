#pragma once

#include "Assets/AssetLoader.h"

#include <cstddef>
#include <memory>

namespace engine::assets
{
    class AssetLoaderRegistry final
    {
    public:
        AssetLoaderRegistry() noexcept;
        ~AssetLoaderRegistry() noexcept;

        AssetLoaderRegistry(const AssetLoaderRegistry&) = delete;
        AssetLoaderRegistry& operator=(const AssetLoaderRegistry&) = delete;

        AssetLoaderRegistry(AssetLoaderRegistry&&) = delete;
        AssetLoaderRegistry& operator=(AssetLoaderRegistry&&) = delete;

        // Registry does not own loaders. Registered loaders must remain alive
        // until they are unregistered or Clear() is called.
        [[nodiscard]] AssetResult Register(
            AssetLoader& loader) noexcept;

        [[nodiscard]] AssetResult Unregister(
            AssetType type,
            const AssetLoader* expectedLoader = nullptr) noexcept;

        [[nodiscard]] AssetLoader* Find(
            AssetType type) noexcept;

        [[nodiscard]] const AssetLoader* Find(
            AssetType type) const noexcept;

        [[nodiscard]] AssetResult Load(
            const AssetMetadata& metadata,
            const AssetData& source,
            std::unique_ptr<LoadedAsset>& outAsset) const noexcept;

        [[nodiscard]] bool Contains(
            AssetType type) const noexcept;

        [[nodiscard]] std::size_t GetCount() const noexcept;

        void Clear() noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
