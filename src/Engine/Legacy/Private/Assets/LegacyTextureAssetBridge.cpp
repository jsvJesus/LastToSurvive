#include "Legacy/Assets/LegacyTextureAssetBridge.h"

#include "Legacy/Graphics/D3D9RendererBridge.h"

#include "Assets/AssetManager.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetType.h"
#include "Assets/FileAssetSource.h"
#include "Assets/TextureAssetCache.h"

#include "GraphicsDX9/D3D9Device.h"

#include <new>

namespace engine::legacy::assets
{
    namespace
    {
        using AssetHandle = engine::assets::AssetHandle;
        using AssetManager = engine::assets::AssetManager;
        using AssetMetadata = engine::assets::AssetMetadata;
        using AssetPath = engine::assets::AssetPath;
        using AssetResult = engine::assets::AssetResult;
        using AssetType = engine::assets::AssetType;
        using FileAssetSource = engine::assets::FileAssetSource;
        using TextureAsset = engine::assets::TextureAsset;
        using TextureAssetCache = engine::assets::TextureAssetCache;
        using TextureCacheResult = engine::assets::TextureCacheResult;
        using D3D9Device = engine::graphics::d3d9::D3D9Device;

        class LegacyTextureAssetService final
        {
        public:
            FileAssetSource source;
            AssetManager manager;
            TextureAssetCache textureCache;
        };

        LegacyTextureAssetService* g_service = nullptr;

        [[nodiscard]] LegacyTextureBridgeResult ConvertResult(
            const TextureCacheResult& result) noexcept
        {
            if (engine::assets::Failed(result.assetResult))
            {
                return LegacyTextureBridgeResult::FromAsset(
                    result.assetResult);
            }

            if (engine::graphics::Failed(result.graphicsResult))
            {
                return LegacyTextureBridgeResult::FromGraphics(
                    result.graphicsResult);
            }

            return LegacyTextureBridgeResult{};
        }

        [[nodiscard]] D3D9Device* GetReadyDevice() noexcept
        {
            D3D9Device* const device =
                engine::legacy::graphics::GetD3D9CompatibilityDevice();

            return device != nullptr && device->IsReady()
                ? device
                : nullptr;
        }

        [[nodiscard]] LegacyTextureBridgeResult BuildView(
            const AssetHandle assetHandle,
            LegacyTextureAssetView& outView) noexcept
        {
            outView = LegacyTextureAssetView{};

            if (g_service == nullptr)
            {
                return LegacyTextureBridgeResult::FromAsset(
                    AssetResult::InvalidState);
            }

            D3D9Device* const device = GetReadyDevice();

            if (device == nullptr)
            {
                return LegacyTextureBridgeResult::FromGraphics(
                    engine::graphics::GraphicsResult::InvalidState);
            }

            engine::graphics::TextureHandle textureHandle;

            const TextureCacheResult handleResult =
                g_service->textureCache.GetTextureHandle(
                    assetHandle,
                    textureHandle);

            if (!handleResult.Succeeded())
            {
                return ConvertResult(handleResult);
            }

            const TextureAsset* textureAsset = nullptr;

            const TextureCacheResult assetResult =
                g_service->textureCache.GetTextureAsset(
                    assetHandle,
                    textureAsset);

            if (!assetResult.Succeeded())
            {
                return ConvertResult(assetResult);
            }

            if (textureAsset == nullptr || !textureAsset->IsValid())
            {
                return LegacyTextureBridgeResult::FromAsset(
                    AssetResult::InternalError);
            }

            IDirect3DBaseTexture9* const nativeTexture =
                device->GetNativeTexture(textureHandle);

            if (nativeTexture == nullptr)
            {
                return LegacyTextureBridgeResult::FromGraphics(
                    engine::graphics::GraphicsResult::NotFound);
            }

            outView.assetHandle = assetHandle;
            outView.textureHandle = textureHandle;
            outView.desc = textureAsset->GetDesc();
            outView.nativeTexture = nativeTexture;

            return outView.IsValid()
                ? LegacyTextureBridgeResult{}
                : LegacyTextureBridgeResult::FromAsset(
                    AssetResult::InternalError);
        }

        [[nodiscard]] LegacyTextureBridgeResult FindOrRegisterTexture(
            const char* const relativePath,
            AssetHandle& outAssetHandle) noexcept
        {
            outAssetHandle = AssetHandle{};

            if (g_service == nullptr)
            {
                return LegacyTextureBridgeResult::FromAsset(
                    AssetResult::InvalidState);
            }

            if (relativePath == nullptr || relativePath[0] == '\0')
            {
                return LegacyTextureBridgeResult::FromAsset(
                    AssetResult::InvalidArgument);
            }

            AssetPath path;

            const AssetResult pathResult = AssetPath::TryCreate(
                relativePath,
                path);

            if (engine::assets::Failed(pathResult))
            {
                return LegacyTextureBridgeResult::FromAsset(pathResult);
            }

            const AssetResult findResult =
                g_service->manager.FindByPath(
                    path,
                    outAssetHandle);

            if (findResult == AssetResult::Success)
            {
                return LegacyTextureBridgeResult{};
            }

            if (findResult != AssetResult::NotFound)
            {
                return LegacyTextureBridgeResult::FromAsset(findResult);
            }

            AssetMetadata metadata;
            metadata.id = path.GetId();
            metadata.path = path;
            metadata.type = AssetType::Texture;

            const AssetResult registerResult =
                g_service->manager.Register(
                    metadata,
                    outAssetHandle);

            if (
                registerResult == AssetResult::AlreadyExists &&
                outAssetHandle.IsValid())
            {
                return LegacyTextureBridgeResult{};
            }

            return engine::assets::Succeeded(registerResult)
                ? LegacyTextureBridgeResult{}
                : LegacyTextureBridgeResult::FromAsset(registerResult);
        }
    }

    LegacyTextureBridgeResult InitializeLegacyTextureAssetBridge(
        const engine::platform::Path& rootDirectory) noexcept
    {
        if (g_service != nullptr)
        {
            return LegacyTextureBridgeResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (rootDirectory.empty())
        {
            return LegacyTextureBridgeResult::FromAsset(
                AssetResult::InvalidPath);
        }

        D3D9Device* const device = GetReadyDevice();

        if (device == nullptr)
        {
            return LegacyTextureBridgeResult::FromGraphics(
                engine::graphics::GraphicsResult::InvalidState);
        }

        LegacyTextureAssetService* const service =
            new (std::nothrow) LegacyTextureAssetService();

        if (service == nullptr)
        {
            return LegacyTextureBridgeResult::FromAsset(
                AssetResult::OutOfMemory);
        }

        const AssetResult sourceResult = service->source.Initialize(
            rootDirectory);

        if (engine::assets::Failed(sourceResult))
        {
            delete service;
            return LegacyTextureBridgeResult::FromAsset(sourceResult);
        }

        const AssetResult managerResult = service->manager.Initialize(
            service->source);

        if (engine::assets::Failed(managerResult))
        {
            service->source.Shutdown();
            delete service;
            return LegacyTextureBridgeResult::FromAsset(managerResult);
        }

        engine::assets::DdsTextureDecodeOptions decodeOptions;
        decodeOptions.allowBc7 = false;
        decodeOptions.forceSrgb = false;

        const TextureCacheResult cacheResult =
            service->textureCache.Initialize(
                service->manager,
                *device,
                decodeOptions);

        if (!cacheResult.Succeeded())
        {
            service->manager.Shutdown();
            service->source.Shutdown();
            delete service;
            return ConvertResult(cacheResult);
        }

        g_service = service;
        return LegacyTextureBridgeResult{};
    }

    LegacyTextureBridgeResult ShutdownLegacyTextureAssetBridge() noexcept
    {
        if (g_service == nullptr)
        {
            return LegacyTextureBridgeResult{};
        }

        const TextureCacheResult cacheResult =
            g_service->textureCache.Shutdown();

        if (!cacheResult.Succeeded())
        {
            return ConvertResult(cacheResult);
        }

        g_service->manager.Shutdown();
        g_service->source.Shutdown();

        delete g_service;
        g_service = nullptr;

        return LegacyTextureBridgeResult{};
    }

    void AbandonLegacyTextureAssetBridge() noexcept
    {
        if (g_service == nullptr)
        {
            return;
        }

        g_service->textureCache.Abandon();
        g_service->manager.Shutdown();
        g_service->source.Shutdown();

        delete g_service;
        g_service = nullptr;
    }

    bool IsLegacyTextureAssetBridgeReady() noexcept
    {
        return
            g_service != nullptr &&
            g_service->manager.IsInitialized() &&
            g_service->textureCache.IsInitialized() &&
            GetReadyDevice() != nullptr;
    }

    LegacyTextureBridgeResult AcquireLegacyTextureAsset(
        const char* const relativePath,
        LegacyTextureAssetView& outView) noexcept
    {
        outView = LegacyTextureAssetView{};

        if (!IsLegacyTextureAssetBridgeReady())
        {
            return LegacyTextureBridgeResult::FromAsset(
                AssetResult::InvalidState);
        }

        AssetHandle assetHandle;

        const LegacyTextureBridgeResult registrationResult =
            FindOrRegisterTexture(
                relativePath,
                assetHandle);

        if (!registrationResult.Succeeded())
        {
            return registrationResult;
        }

        engine::graphics::TextureHandle textureHandle;

        const TextureCacheResult acquireResult =
            g_service->textureCache.Acquire(
                assetHandle,
                textureHandle);

        if (!acquireResult.Succeeded())
        {
            return ConvertResult(acquireResult);
        }

        const LegacyTextureBridgeResult viewResult =
            BuildView(assetHandle, outView);

        if (!viewResult.Succeeded())
        {
            const TextureCacheResult releaseResult =
                g_service->textureCache.Release(assetHandle);

            (void)releaseResult;
            outView = LegacyTextureAssetView{};
        }

        return viewResult;
    }

    LegacyTextureBridgeResult ReloadLegacyTextureAsset(
        const AssetHandle assetHandle,
        LegacyTextureAssetView& outView) noexcept
    {
        outView = LegacyTextureAssetView{};

        if (!IsLegacyTextureAssetBridgeReady())
        {
            return LegacyTextureBridgeResult::FromAsset(
                AssetResult::InvalidState);
        }

        const TextureCacheResult reloadResult =
            g_service->textureCache.Reload(assetHandle);

        if (!reloadResult.Succeeded())
        {
            return ConvertResult(reloadResult);
        }

        return BuildView(assetHandle, outView);
    }

    LegacyTextureBridgeResult ReleaseLegacyTextureAsset(
        const AssetHandle assetHandle) noexcept
    {
        if (!IsLegacyTextureAssetBridgeReady())
        {
            return LegacyTextureBridgeResult::FromAsset(
                AssetResult::InvalidState);
        }

        return ConvertResult(
            g_service->textureCache.Release(assetHandle));
    }

    std::size_t GetLegacyTextureAssetCount() noexcept
    {
        return IsLegacyTextureAssetBridgeReady()
            ? g_service->textureCache.GetCount()
            : 0U;
    }
}
