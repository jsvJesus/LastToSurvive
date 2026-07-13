#pragma once

#include "Assets/AssetHandle.h"
#include "Assets/AssetId.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"
#include "Assets/DdsTextureLoader.h"
#include "Assets/TextureAsset.h"

#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"
#include "Graphics/ResourceHandle.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace engine::assets
{
    struct TextureCacheResult final
    {
        AssetResult assetResult = AssetResult::Success;

        engine::graphics::GraphicsResult graphicsResult =
            engine::graphics::GraphicsResult::Success;

        [[nodiscard]] constexpr bool Succeeded() const noexcept
        {
            return
                assetResult == AssetResult::Success &&
                graphicsResult ==
                    engine::graphics::GraphicsResult::Success;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return Succeeded();
        }

        [[nodiscard]] static constexpr TextureCacheResult FromAsset(
            const AssetResult result) noexcept
        {
            return TextureCacheResult{
                result,
                engine::graphics::GraphicsResult::Success};
        }

        [[nodiscard]] static constexpr TextureCacheResult FromGraphics(
            const engine::graphics::GraphicsResult result) noexcept
        {
            return TextureCacheResult{
                AssetResult::Success,
                result};
        }
    };

    // Synchronous reference-counted texture cache.
    // Use from the owning/render thread.
    //
    // AssetManager and RenderDevice are non-owning dependencies and must
    // outlive this cache. Shutdown() must run before RenderDevice::Shutdown().
    class TextureAssetCache final
    {
    public:
        // Public only as an incomplete PIMPL type so translation-unit helpers
        // can remain strongly typed. No implementation details are exposed.
        class Impl;

        TextureAssetCache() noexcept;
        ~TextureAssetCache() noexcept;

        TextureAssetCache(const TextureAssetCache&) = delete;
        TextureAssetCache& operator=(const TextureAssetCache&) = delete;

        TextureAssetCache(TextureAssetCache&&) = delete;
        TextureAssetCache& operator=(TextureAssetCache&&) = delete;

        [[nodiscard]] TextureCacheResult Initialize(
            AssetManager& assetManager,
            engine::graphics::RenderDevice& renderDevice,
            const DdsTextureDecodeOptions& decodeOptions =
                DdsTextureDecodeOptions{}) noexcept;

        [[nodiscard]] TextureCacheResult Shutdown() noexcept;

        // Forget handles without calling DestroyTexture(). Use only after the
        // render device has already destroyed its resource registry.
        void Abandon() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

        [[nodiscard]] TextureCacheResult Acquire(
            AssetHandle assetHandle,
            engine::graphics::TextureHandle& outTextureHandle) noexcept;

        [[nodiscard]] TextureCacheResult AcquireById(
            AssetId assetId,
            AssetHandle& outAssetHandle,
            engine::graphics::TextureHandle& outTextureHandle) noexcept;

        [[nodiscard]] TextureCacheResult AcquireByPath(
            const AssetPath& path,
            AssetHandle& outAssetHandle,
            engine::graphics::TextureHandle& outTextureHandle) noexcept;

        [[nodiscard]] TextureCacheResult Reload(
            AssetHandle assetHandle) noexcept;

        [[nodiscard]] TextureCacheResult Release(
            AssetHandle assetHandle) noexcept;

        [[nodiscard]] TextureCacheResult GetTextureHandle(
            AssetHandle assetHandle,
            engine::graphics::TextureHandle& outTextureHandle) const noexcept;

        [[nodiscard]] TextureCacheResult GetTextureAsset(
            AssetHandle assetHandle,
            const TextureAsset*& outTextureAsset) const noexcept;

        [[nodiscard]] TextureCacheResult GetReferenceCount(
            AssetHandle assetHandle,
            std::uint32_t& outReferenceCount) const noexcept;

        [[nodiscard]] bool Contains(
            AssetHandle assetHandle) const noexcept;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        std::unique_ptr<Impl> impl_;
    };
}
