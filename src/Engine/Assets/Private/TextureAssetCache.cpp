#include "Assets/TextureAssetCache.h"

#include "Assets/AssetLoaderRegistry.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetType.h"
#include "Assets/GpuTexture.h"

#include "Graphics/GraphicsBackend.h"

#include <limits>
#include <new>
#include <unordered_map>
#include <utility>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] bool IsTerminalReleaseResult(
            const engine::graphics::GraphicsResult result) noexcept
        {
            return
                result == engine::graphics::GraphicsResult::Success ||
                result == engine::graphics::GraphicsResult::NotFound ||
                result == engine::graphics::GraphicsResult::DeviceRemoved;
        }

        [[nodiscard]] TextureCacheResult MakeSuccess() noexcept
        {
            return TextureCacheResult{};
        }
    }

    class TextureAssetCache::Impl final
    {
    public:
        using CacheKey = AssetHandle::ValueType;

        struct Entry final
        {
            TextureAsset textureAsset;
            GpuTexture gpuTexture;
            std::uint32_t referenceCount = 0U;
        };

        AssetManager* assetManager = nullptr;
        engine::graphics::RenderDevice* renderDevice = nullptr;

        DdsTextureLoader ddsTextureLoader;
        AssetLoaderRegistry loaderRegistry;

        std::unordered_map<CacheKey, std::unique_ptr<Entry>> entries;
        bool initialized = false;
    };

    namespace
    {
        [[nodiscard]] AssetResult DecodeTexture(
            TextureAssetCache::Impl& impl,
            const AssetMetadata& metadata,
            const AssetData& source,
            TextureAsset& outTexture) noexcept
        {
            outTexture.Clear();

            std::unique_ptr<LoadedAsset> loadedAsset;

            const AssetResult loadResult = impl.loaderRegistry.Load(
                metadata,
                source,
                loadedAsset);

            if (Failed(loadResult))
            {
                return loadResult;
            }

            if (
                !loadedAsset ||
                loadedAsset->GetType() != AssetType::Texture)
            {
                return AssetResult::TypeMismatch;
            }

            auto* const texturePayload =
                static_cast<TextureLoadedAsset*>(loadedAsset.get());

            outTexture = texturePayload->ReleaseTexture();

            return outTexture.IsValid()
                ? AssetResult::Success
                : AssetResult::CorruptData;
        }

        void ReleaseEntryBestEffort(
            TextureAssetCache::Impl& impl,
            TextureAssetCache::Impl::Entry& entry) noexcept
        {
            if (
                impl.renderDevice != nullptr &&
                entry.gpuTexture.IsValid())
            {
                const engine::graphics::GraphicsResult result =
                    entry.gpuTexture.Release(*impl.renderDevice);

                (void)result;
            }
        }
    }

    TextureAssetCache::TextureAssetCache() noexcept
    {
        try
        {
            impl_ = std::make_unique<Impl>();
        }
        catch (...)
        {
            impl_.reset();
        }
    }

    TextureAssetCache::~TextureAssetCache() noexcept
    {
        Abandon();
    }

    TextureCacheResult TextureAssetCache::Initialize(
        AssetManager& assetManager,
        engine::graphics::RenderDevice& renderDevice,
        const DdsTextureDecodeOptions& decodeOptions) noexcept
    {
        if (!impl_)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::OutOfMemory);
        }

        if (impl_->initialized)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (
            !assetManager.IsInitialized() ||
            !renderDevice.IsReady())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        impl_->entries.clear();
        impl_->loaderRegistry.Clear();

        DdsTextureDecodeOptions effectiveOptions = decodeOptions;
        effectiveOptions.allowBc7 =
            decodeOptions.allowBc7 &&
            renderDevice.GetBackend() ==
                engine::graphics::GraphicsBackend::D3D11;

        impl_->ddsTextureLoader.SetOptions(effectiveOptions);

        const AssetResult registerResult =
            impl_->loaderRegistry.Register(
                impl_->ddsTextureLoader);

        if (Failed(registerResult))
        {
            impl_->loaderRegistry.Clear();
            return TextureCacheResult::FromAsset(registerResult);
        }

        impl_->assetManager = &assetManager;
        impl_->renderDevice = &renderDevice;
        impl_->initialized = true;

        return MakeSuccess();
    }

    TextureCacheResult TextureAssetCache::Shutdown() noexcept
    {
        if (!impl_)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!impl_->initialized)
        {
            return MakeSuccess();
        }

        if (impl_->renderDevice == nullptr)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        TextureCacheResult firstFailure = MakeSuccess();
        auto iterator = impl_->entries.begin();

        while (iterator != impl_->entries.end())
        {
            Impl::Entry& entry = *iterator->second;

            const engine::graphics::GraphicsResult releaseResult =
                entry.gpuTexture.Release(*impl_->renderDevice);

            if (IsTerminalReleaseResult(releaseResult))
            {
                if (
                    releaseResult ==
                        engine::graphics::GraphicsResult::DeviceRemoved &&
                    firstFailure.Succeeded())
                {
                    firstFailure =
                        TextureCacheResult::FromGraphics(releaseResult);
                }

                iterator = impl_->entries.erase(iterator);
                continue;
            }

            if (firstFailure.Succeeded())
            {
                firstFailure =
                    TextureCacheResult::FromGraphics(releaseResult);
            }

            ++iterator;
        }

        if (!impl_->entries.empty())
        {
            return firstFailure;
        }

        impl_->loaderRegistry.Clear();
        impl_->assetManager = nullptr;
        impl_->renderDevice = nullptr;
        impl_->initialized = false;

        return firstFailure;
    }

    void TextureAssetCache::Abandon() noexcept
    {
        if (!impl_)
        {
            return;
        }

        for (auto& pair : impl_->entries)
        {
            if (pair.second)
            {
                pair.second->gpuTexture.Abandon();
            }
        }

        impl_->entries.clear();
        impl_->loaderRegistry.Clear();
        impl_->assetManager = nullptr;
        impl_->renderDevice = nullptr;
        impl_->initialized = false;
    }

    bool TextureAssetCache::IsInitialized() const noexcept
    {
        return
            impl_ != nullptr &&
            impl_->initialized &&
            impl_->assetManager != nullptr &&
            impl_->renderDevice != nullptr &&
            impl_->loaderRegistry.Contains(AssetType::Texture);
    }

    TextureCacheResult TextureAssetCache::Acquire(
        const AssetHandle assetHandle,
        engine::graphics::TextureHandle& outTextureHandle) noexcept
    {
        outTextureHandle = engine::graphics::TextureHandle{};

        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!assetHandle.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidArgument);
        }

        const Impl::CacheKey cacheKey = assetHandle.Value();
        const auto existing = impl_->entries.find(cacheKey);

        if (existing != impl_->entries.end())
        {
            Impl::Entry& entry = *existing->second;

            if (
                entry.referenceCount == 0U ||
                !entry.textureAsset.IsValid() ||
                !entry.gpuTexture.IsValid())
            {
                return TextureCacheResult::FromAsset(
                    AssetResult::InternalError);
            }

            if (
                entry.referenceCount ==
                (std::numeric_limits<std::uint32_t>::max)())
            {
                return TextureCacheResult::FromAsset(
                    AssetResult::ReferenceOverflow);
            }

            ++entry.referenceCount;
            outTextureHandle = entry.gpuTexture.GetHandle();
            return MakeSuccess();
        }

        AssetMetadata metadata;

        AssetResult assetResult = impl_->assetManager->GetMetadata(
            assetHandle,
            metadata);

        if (Failed(assetResult))
        {
            return TextureCacheResult::FromAsset(assetResult);
        }

        if (metadata.type != AssetType::Texture)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::TypeMismatch);
        }

        assetResult = impl_->assetManager->Load(assetHandle);

        if (Failed(assetResult))
        {
            return TextureCacheResult::FromAsset(assetResult);
        }

        const AssetData* sourceData = nullptr;

        assetResult = impl_->assetManager->GetData(
            assetHandle,
            sourceData);

        if (Failed(assetResult) || sourceData == nullptr)
        {
            return TextureCacheResult::FromAsset(
                Failed(assetResult)
                    ? assetResult
                    : AssetResult::InternalError);
        }

        std::unique_ptr<Impl::Entry> entry;

        try
        {
            entry = std::make_unique<Impl::Entry>();
        }
        catch (const std::bad_alloc&)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::OutOfMemory);
        }
        catch (...)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        assetResult = DecodeTexture(
            *impl_,
            metadata,
            *sourceData,
            entry->textureAsset);

        if (Failed(assetResult))
        {
            return TextureCacheResult::FromAsset(assetResult);
        }

        const engine::graphics::GraphicsResult uploadResult =
            entry->gpuTexture.Upload(
                *impl_->renderDevice,
                entry->textureAsset);

        if (engine::graphics::Failed(uploadResult))
        {
            return TextureCacheResult::FromGraphics(uploadResult);
        }

        entry->referenceCount = 1U;
        Impl::Entry* insertedEntry = entry.get();

        try
        {
            const auto insertResult = impl_->entries.emplace(
                cacheKey,
                std::move(entry));

            if (!insertResult.second)
            {
                ReleaseEntryBestEffort(*impl_, *insertedEntry);
                return TextureCacheResult::FromAsset(
                    AssetResult::AlreadyExists);
            }
        }
        catch (const std::bad_alloc&)
        {
            ReleaseEntryBestEffort(*impl_, *insertedEntry);
            return TextureCacheResult::FromAsset(
                AssetResult::OutOfMemory);
        }
        catch (...)
        {
            ReleaseEntryBestEffort(*impl_, *insertedEntry);
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        outTextureHandle = insertedEntry->gpuTexture.GetHandle();

        if (!outTextureHandle.IsValid())
        {
            ReleaseEntryBestEffort(*impl_, *insertedEntry);
            impl_->entries.erase(cacheKey);

            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        return MakeSuccess();
    }

    TextureCacheResult TextureAssetCache::AcquireById(
        const AssetId assetId,
        AssetHandle& outAssetHandle,
        engine::graphics::TextureHandle& outTextureHandle) noexcept
    {
        outAssetHandle = AssetHandle{};
        outTextureHandle = engine::graphics::TextureHandle{};

        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        const AssetResult findResult = impl_->assetManager->FindById(
            assetId,
            outAssetHandle);

        if (Failed(findResult))
        {
            return TextureCacheResult::FromAsset(findResult);
        }

        const TextureCacheResult result = Acquire(
            outAssetHandle,
            outTextureHandle);

        if (!result.Succeeded())
        {
            outAssetHandle = AssetHandle{};
        }

        return result;
    }

    TextureCacheResult TextureAssetCache::AcquireByPath(
        const AssetPath& path,
        AssetHandle& outAssetHandle,
        engine::graphics::TextureHandle& outTextureHandle) noexcept
    {
        outAssetHandle = AssetHandle{};
        outTextureHandle = engine::graphics::TextureHandle{};

        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        const AssetResult findResult = impl_->assetManager->FindByPath(
            path,
            outAssetHandle);

        if (Failed(findResult))
        {
            return TextureCacheResult::FromAsset(findResult);
        }

        const TextureCacheResult result = Acquire(
            outAssetHandle,
            outTextureHandle);

        if (!result.Succeeded())
        {
            outAssetHandle = AssetHandle{};
        }

        return result;
    }

    TextureCacheResult TextureAssetCache::Reload(
        const AssetHandle assetHandle) noexcept
    {
        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!assetHandle.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidArgument);
        }

        const auto iterator = impl_->entries.find(assetHandle.Value());

        if (iterator == impl_->entries.end())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::NotFound);
        }

        Impl::Entry& entry = *iterator->second;

        if (
            entry.referenceCount == 0U ||
            !entry.textureAsset.IsValid() ||
            !entry.gpuTexture.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        AssetMetadata metadata;

        AssetResult assetResult = impl_->assetManager->GetMetadata(
            assetHandle,
            metadata);

        if (Failed(assetResult))
        {
            return TextureCacheResult::FromAsset(assetResult);
        }

        if (metadata.type != AssetType::Texture)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::TypeMismatch);
        }

        assetResult = impl_->assetManager->Reload(assetHandle);

        if (Failed(assetResult))
        {
            return TextureCacheResult::FromAsset(assetResult);
        }

        const AssetData* sourceData = nullptr;

        assetResult = impl_->assetManager->GetData(
            assetHandle,
            sourceData);

        if (Failed(assetResult) || sourceData == nullptr)
        {
            return TextureCacheResult::FromAsset(
                Failed(assetResult)
                    ? assetResult
                    : AssetResult::InternalError);
        }

        TextureAsset replacementAsset;

        assetResult = DecodeTexture(
            *impl_,
            metadata,
            *sourceData,
            replacementAsset);

        if (Failed(assetResult))
        {
            return TextureCacheResult::FromAsset(assetResult);
        }

        const engine::graphics::GraphicsResult replaceResult =
            entry.gpuTexture.Replace(
                *impl_->renderDevice,
                replacementAsset);

        if (engine::graphics::Failed(replaceResult))
        {
            return TextureCacheResult::FromGraphics(replaceResult);
        }

        entry.textureAsset = std::move(replacementAsset);
        return MakeSuccess();
    }

    TextureCacheResult TextureAssetCache::Release(
        const AssetHandle assetHandle) noexcept
    {
        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!assetHandle.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidArgument);
        }

        const auto iterator = impl_->entries.find(assetHandle.Value());

        if (iterator == impl_->entries.end())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::NotFound);
        }

        Impl::Entry& entry = *iterator->second;

        if (entry.referenceCount == 0U)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        if (entry.referenceCount > 1U)
        {
            --entry.referenceCount;
            return MakeSuccess();
        }

        const engine::graphics::GraphicsResult releaseResult =
            entry.gpuTexture.Release(*impl_->renderDevice);

        if (!IsTerminalReleaseResult(releaseResult))
        {
            return TextureCacheResult::FromGraphics(releaseResult);
        }

        impl_->entries.erase(iterator);

        if (
            releaseResult ==
            engine::graphics::GraphicsResult::DeviceRemoved)
        {
            return TextureCacheResult::FromGraphics(releaseResult);
        }

        return MakeSuccess();
    }

    TextureCacheResult TextureAssetCache::GetTextureHandle(
        const AssetHandle assetHandle,
        engine::graphics::TextureHandle& outTextureHandle) const noexcept
    {
        outTextureHandle = engine::graphics::TextureHandle{};

        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!assetHandle.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidArgument);
        }

        const auto iterator = impl_->entries.find(assetHandle.Value());

        if (iterator == impl_->entries.end())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::NotFound);
        }

        const Impl::Entry& entry = *iterator->second;

        if (
            entry.referenceCount == 0U ||
            !entry.gpuTexture.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        outTextureHandle = entry.gpuTexture.GetHandle();

        return outTextureHandle.IsValid()
            ? MakeSuccess()
            : TextureCacheResult::FromAsset(
                AssetResult::InternalError);
    }

    TextureCacheResult TextureAssetCache::GetTextureAsset(
        const AssetHandle assetHandle,
        const TextureAsset*& outTextureAsset) const noexcept
    {
        outTextureAsset = nullptr;

        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!assetHandle.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidArgument);
        }

        const auto iterator = impl_->entries.find(assetHandle.Value());

        if (iterator == impl_->entries.end())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::NotFound);
        }

        const Impl::Entry& entry = *iterator->second;

        if (
            entry.referenceCount == 0U ||
            !entry.textureAsset.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        outTextureAsset = &entry.textureAsset;
        return MakeSuccess();
    }

    TextureCacheResult TextureAssetCache::GetReferenceCount(
        const AssetHandle assetHandle,
        std::uint32_t& outReferenceCount) const noexcept
    {
        outReferenceCount = 0U;

        if (!IsInitialized())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidState);
        }

        if (!assetHandle.IsValid())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InvalidArgument);
        }

        const auto iterator = impl_->entries.find(assetHandle.Value());

        if (iterator == impl_->entries.end())
        {
            return TextureCacheResult::FromAsset(
                AssetResult::NotFound);
        }

        const Impl::Entry& entry = *iterator->second;

        if (entry.referenceCount == 0U)
        {
            return TextureCacheResult::FromAsset(
                AssetResult::InternalError);
        }

        outReferenceCount = entry.referenceCount;
        return MakeSuccess();
    }

    bool TextureAssetCache::Contains(
        const AssetHandle assetHandle) const noexcept
    {
        return
            IsInitialized() &&
            assetHandle.IsValid() &&
            impl_->entries.find(assetHandle.Value()) !=
                impl_->entries.end();
    }

    std::size_t TextureAssetCache::GetCount() const noexcept
    {
        return IsInitialized() ? impl_->entries.size() : 0U;
    }
}
