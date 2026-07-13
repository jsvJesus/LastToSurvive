#pragma once

#include "Assets/AssetSource.h"

#include "Platform/Path.h"

#include <cstdint>

namespace engine::assets
{
    class FileAssetSource final : public AssetSource
    {
    public:
        static constexpr std::uint64_t DefaultMaximumAssetSize =
            512ULL * 1024ULL * 1024ULL;

        FileAssetSource() noexcept = default;

        [[nodiscard]] AssetResult Initialize(
            const engine::platform::Path& rootDirectory,
            std::uint64_t maximumAssetSize =
                DefaultMaximumAssetSize) noexcept;

        void Shutdown() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

        [[nodiscard]] AssetResult Read(
            const AssetPath& path,
            AssetData& outData) noexcept override;

        [[nodiscard]] bool Exists(
            const AssetPath& path) const noexcept override;

        [[nodiscard]] const engine::platform::Path&
            GetRootDirectory() const noexcept;

        [[nodiscard]] std::uint64_t
            GetMaximumAssetSize() const noexcept;

    private:
        [[nodiscard]] AssetResult ResolvePath(
            const AssetPath& assetPath,
            engine::platform::Path& outPath) const noexcept;

        engine::platform::Path rootDirectory_;

        std::uint64_t maximumAssetSize_ =
            DefaultMaximumAssetSize;

        bool initialized_ = false;
    };
}