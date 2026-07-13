#pragma once

#include "Assets/AssetHandle.h"
#include "Assets/AssetId.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"
#include "Assets/DdsTextureDecoder.h"
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
        AssetResult assetResult =
            AssetResult::Success;

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

        [[nodiscard]] static constexpr TextureCacheResult
            FromAsset(
                const AssetResult result) noexcept
        {
            return TextureCacheResult{
                result,
                engine::graphics::GraphicsResult::Success
            };
        }

        [[nodiscard]] static constexpr TextureCacheResult
            FromGraphics(
                const engine::graphics::GraphicsResult result) noexcept
        {
            return TextureCacheResult{
                AssetResult::Success,
                result
            };
        }
    };

    // Синхронный cache. Использовать с owning/render thread.
    //
    // AssetManager и RenderDevice являются non-owning зависимостями
    // и должны жить дольше TextureAssetCache.
    //
    // Shutdown() необходимо вызвать до RenderDevice::Shutdown().
    // После уже выполненного RenderDevice::Shutdown() используется
    // только Abandon().
    class TextureAssetCache final
    {
    public:
        TextureAssetCache() noexcept;
        ~TextureAssetCache() noexcept;

        TextureAssetCache(
            const TextureAssetCache&) = delete;

        TextureAssetCache& operator=(
            const TextureAssetCache&) = delete;

        TextureAssetCache(
            TextureAssetCache&&) = delete;

        TextureAssetCache& operator=(
            TextureAssetCache&&) = delete;

        [[nodiscard]] TextureCacheResult Initialize(
            AssetManager& assetManager,
            engine::graphics::RenderDevice& renderDevice,
            const DdsTextureDecodeOptions& decodeOptions =
                DdsTextureDecodeOptions{}) noexcept;

        // Освобождает все GPU textures.
        // Вызывать до RenderDevice::Shutdown().
        [[nodiscard]] TextureCacheResult Shutdown() noexcept;

        // Забывает handles без вызова DestroyTexture().
        // Использовать только после того, как RenderDevice уже
        // очистил собственный resource registry.
        void Abandon() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

        // При первом Acquire выполняется load/decode/upload.
        // Повторный Acquire только увеличивает reference count.
        [[nodiscard]] TextureCacheResult Acquire(
            AssetHandle assetHandle,
            engine::graphics::TextureHandle&
                outTextureHandle) noexcept;

        [[nodiscard]] TextureCacheResult AcquireById(
            AssetId assetId,
            AssetHandle& outAssetHandle,
            engine::graphics::TextureHandle&
                outTextureHandle) noexcept;

        [[nodiscard]] TextureCacheResult AcquireByPath(
            const AssetPath& path,
            AssetHandle& outAssetHandle,
            engine::graphics::TextureHandle&
                outTextureHandle) noexcept;

        // Перечитывает исходный asset, декодирует DDS и атомарно
        // заменяет GPU texture. При ошибке старая texture остаётся.
        [[nodiscard]] TextureCacheResult Reload(
            AssetHandle assetHandle) noexcept;

        // Уменьшает reference count. При достижении нуля
        // освобождает GPU texture и удаляет cache entry.
        [[nodiscard]] TextureCacheResult Release(
            AssetHandle assetHandle) noexcept;

        [[nodiscard]] TextureCacheResult GetTextureHandle(
            AssetHandle assetHandle,
            engine::graphics::TextureHandle&
                outTextureHandle) const noexcept;

        // Указатель действителен до Reload(), Release(),
        // Shutdown() или Abandon().
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
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}