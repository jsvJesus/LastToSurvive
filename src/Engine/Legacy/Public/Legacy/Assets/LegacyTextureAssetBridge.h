#pragma once

#include "Assets/AssetHandle.h"
#include "Assets/AssetResult.h"

#include "Graphics/GraphicsResult.h"
#include "Graphics/ResourceHandle.h"
#include "Graphics/Texture.h"

#include "Platform/Path.h"

#include <cstddef>

struct IDirect3DBaseTexture9;

namespace engine::legacy::assets
{
    struct LegacyTextureBridgeResult final
    {
        engine::assets::AssetResult assetResult =
            engine::assets::AssetResult::Success;

        engine::graphics::GraphicsResult graphicsResult =
            engine::graphics::GraphicsResult::Success;

        [[nodiscard]] constexpr bool Succeeded() const noexcept
        {
            return
                assetResult == engine::assets::AssetResult::Success &&
                graphicsResult == engine::graphics::GraphicsResult::Success;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return Succeeded();
        }

        [[nodiscard]] static constexpr LegacyTextureBridgeResult FromAsset(
            const engine::assets::AssetResult result) noexcept
        {
            return LegacyTextureBridgeResult{
                result,
                engine::graphics::GraphicsResult::Success};
        }

        [[nodiscard]] static constexpr LegacyTextureBridgeResult FromGraphics(
            const engine::graphics::GraphicsResult result) noexcept
        {
            return LegacyTextureBridgeResult{
                engine::assets::AssetResult::Success,
                result};
        }
    };

    struct LegacyTextureAssetView final
    {
        engine::assets::AssetHandle assetHandle;
        engine::graphics::TextureHandle textureHandle;
        engine::graphics::TextureDesc desc;

        // Non-owning native pointer. Do not call Release().
        // Valid until ReleaseLegacyTextureAsset(), reload, bridge shutdown,
        // or renderer/device teardown.
        IDirect3DBaseTexture9* nativeTexture = nullptr;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return
                assetHandle.IsValid() &&
                textureHandle.IsValid() &&
                desc.IsValid() &&
                nativeTexture != nullptr;
        }
    };

    // Initializes the bridge over the already attached D3D9 compatibility
    // device. The root directory is the filesystem root used by AssetPath.
    [[nodiscard]] LegacyTextureBridgeResult InitializeLegacyTextureAssetBridge(
        const engine::platform::Path& rootDirectory) noexcept;

    // Releases every texture before the compatibility device is destroyed.
    [[nodiscard]] LegacyTextureBridgeResult ShutdownLegacyTextureAssetBridge()
        noexcept;

    // For emergency teardown after the graphics adapter already cleared its
    // registry. This drops handles without calling DestroyTexture().
    void AbandonLegacyTextureAssetBridge() noexcept;

    [[nodiscard]] bool IsLegacyTextureAssetBridgeReady() noexcept;

    // Finds or registers a Texture asset, loads it, decodes DDS, uploads it to
    // the current D3D9 compatibility device, and acquires one cache reference.
    [[nodiscard]] LegacyTextureBridgeResult AcquireLegacyTextureAsset(
        const char* relativePath,
        LegacyTextureAssetView& outView) noexcept;

    [[nodiscard]] LegacyTextureBridgeResult ReloadLegacyTextureAsset(
        engine::assets::AssetHandle assetHandle,
        LegacyTextureAssetView& outView) noexcept;

    [[nodiscard]] LegacyTextureBridgeResult ReleaseLegacyTextureAsset(
        engine::assets::AssetHandle assetHandle) noexcept;

    [[nodiscard]] std::size_t GetLegacyTextureAssetCount() noexcept;
}
