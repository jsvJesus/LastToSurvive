#include "Assets/AssetLoaderRegistry.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetMetadata.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetRegistry.h"
#include "Assets/AssetSource.h"
#include "Assets/DdsTextureLoader.h"
#include "Assets/TextureAssetCache.h"

#include "Graphics/RenderDevice.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_set>
#include <vector>

namespace
{
    using namespace engine;

    [[nodiscard]] bool Check(
        const bool condition,
        const char* const message) noexcept
    {
        if (condition)
        {
            return true;
        }

        std::fprintf(stderr, "FAILED: %s\n", message);
        return false;
    }

    void WriteU32(
        std::byte* const data,
        const std::size_t offset,
        const std::uint32_t value) noexcept
    {
        std::memcpy(data + offset, &value, sizeof(value));
    }

    [[nodiscard]] assets::AssetResult BuildTestDds(
        assets::AssetData& outData) noexcept
    {
        constexpr std::size_t fileSize = 132U;
        constexpr std::size_t pixelOffset = 128U;

        const assets::AssetResult resizeResult =
            outData.Resize(fileSize);

        if (assets::Failed(resizeResult))
        {
            return resizeResult;
        }

        std::byte* const bytes = outData.GetData();
        std::memset(bytes, 0, fileSize);

        WriteU32(bytes, 0U, 0x20534444U);
        WriteU32(bytes, 4U, 124U);
        WriteU32(bytes, 12U, 1U);
        WriteU32(bytes, 16U, 1U);
        WriteU32(bytes, 20U, 4U);
        WriteU32(bytes, 28U, 1U);

        WriteU32(bytes, 76U, 32U);
        WriteU32(bytes, 80U, 0x00000041U);
        WriteU32(bytes, 88U, 32U);
        WriteU32(bytes, 92U, 0x000000FFU);
        WriteU32(bytes, 96U, 0x0000FF00U);
        WriteU32(bytes, 100U, 0x00FF0000U);
        WriteU32(bytes, 104U, 0xFF000000U);
        WriteU32(bytes, 108U, 0x00001000U);

        bytes[pixelOffset + 0U] = std::byte{0x11U};
        bytes[pixelOffset + 1U] = std::byte{0x22U};
        bytes[pixelOffset + 2U] = std::byte{0x33U};
        bytes[pixelOffset + 3U] = std::byte{0xFFU};

        return assets::AssetResult::Success;
    }

    class MemoryAssetSource final : public assets::AssetSource
    {
    public:
        MemoryAssetSource(
            assets::AssetPath path,
            const assets::AssetData& source)
            : path_(std::move(path))
        {
            bytes_.resize(source.GetSize());

            if (!bytes_.empty())
            {
                std::memcpy(
                    bytes_.data(),
                    source.GetData(),
                    source.GetSize());
            }
        }

        [[nodiscard]] assets::AssetResult Read(
            const assets::AssetPath& path,
            assets::AssetData& outData) noexcept override
        {
            outData.Clear();

            if (path != path_)
            {
                return assets::AssetResult::NotFound;
            }

            const assets::AssetResult resizeResult =
                outData.Resize(bytes_.size());

            if (assets::Failed(resizeResult))
            {
                return resizeResult;
            }

            if (!bytes_.empty())
            {
                std::memcpy(
                    outData.GetData(),
                    bytes_.data(),
                    bytes_.size());
            }

            return assets::AssetResult::Success;
        }

        [[nodiscard]] bool Exists(
            const assets::AssetPath& path) const noexcept override
        {
            return path == path_;
        }

    private:
        assets::AssetPath path_;
        std::vector<std::byte> bytes_;
    };

    class FakeRenderDevice final : public graphics::RenderDevice
    {
    public:
        [[nodiscard]] graphics::GraphicsBackend GetBackend() const noexcept override
        {
            return backend_;
        }

        [[nodiscard]] graphics::DeviceState GetState() const noexcept override
        {
            return state_;
        }

        [[nodiscard]] graphics::GraphicsResult Initialize(
            const graphics::RenderDeviceDesc& desc) noexcept override
        {
            if (!desc.IsValid() || state_ == graphics::DeviceState::Ready)
            {
                return graphics::GraphicsResult::InvalidArgument;
            }

            backend_ = desc.backend;
            state_ = graphics::DeviceState::Ready;
            return graphics::GraphicsResult::Success;
        }

        void Shutdown() noexcept override
        {
            textures_.clear();
            backend_ = graphics::GraphicsBackend::None;
            state_ = graphics::DeviceState::Stopped;
        }

        [[nodiscard]] graphics::GraphicsResult CreateSwapChain(
            const graphics::SwapChainDesc&,
            std::unique_ptr<graphics::SwapChain>& outSwapChain) noexcept override
        {
            outSwapChain.reset();
            return graphics::GraphicsResult::Unsupported;
        }

        [[nodiscard]] graphics::GraphicsResult CreateTexture(
            const graphics::TextureDesc& desc,
            const graphics::TextureSubresourceData* initialData,
            const std::size_t initialDataCount,
            graphics::TextureHandle& outTexture) noexcept override
        {
            outTexture = graphics::TextureHandle{};

            if (state_ != graphics::DeviceState::Ready)
            {
                return graphics::GraphicsResult::InvalidState;
            }

            if (
                !desc.IsValid() ||
                initialData == nullptr ||
                initialDataCount == 0U)
            {
                return graphics::GraphicsResult::InvalidArgument;
            }

            for (std::size_t index = 0U; index < initialDataCount; ++index)
            {
                if (!initialData[index].IsValid())
                {
                    return graphics::GraphicsResult::InvalidArgument;
                }
            }

            outTexture = graphics::TextureHandle::FromParts(
                nextTextureIndex_++,
                1U);

            try
            {
                textures_.insert(outTexture.Value());
            }
            catch (...)
            {
                outTexture = graphics::TextureHandle{};
                return graphics::GraphicsResult::OutOfMemory;
            }

            return graphics::GraphicsResult::Success;
        }

        [[nodiscard]] graphics::GraphicsResult DestroyTexture(
            const graphics::TextureHandle texture) noexcept override
        {
            if (!texture.IsValid())
            {
                return graphics::GraphicsResult::InvalidArgument;
            }

            return textures_.erase(texture.Value()) != 0U
                ? graphics::GraphicsResult::Success
                : graphics::GraphicsResult::NotFound;
        }

        [[nodiscard]] graphics::GraphicsResult CreateBuffer(
            const graphics::BufferDesc&,
            const graphics::BufferInitialData*,
            graphics::BufferHandle& outBuffer) noexcept override
        {
            outBuffer = graphics::BufferHandle{};
            return graphics::GraphicsResult::Unsupported;
        }

        [[nodiscard]] graphics::GraphicsResult DestroyBuffer(
            const graphics::BufferHandle) noexcept override
        {
            return graphics::GraphicsResult::Unsupported;
        }

        [[nodiscard]] std::size_t GetTextureCount() const noexcept
        {
            return textures_.size();
        }

    private:
        graphics::GraphicsBackend backend_ =
            graphics::GraphicsBackend::None;

        graphics::DeviceState state_ =
            graphics::DeviceState::Uninitialized;

        std::uint32_t nextTextureIndex_ = 1U;
        std::unordered_set<std::uint64_t> textures_;
    };
}

int main()
{
    using namespace engine;

    assets::AssetPath path;

    if (!Check(
            assets::Succeeded(
                assets::AssetPath::TryCreate(
                    "Textures\\Test.DDS",
                    path)),
            "AssetPath::TryCreate"))
    {
        return 1;
    }

    if (!Check(
            path.View() == "textures/test.dds",
            "AssetPath normalization"))
    {
        return 1;
    }

    assets::AssetData ddsData;

    if (!Check(
            assets::Succeeded(BuildTestDds(ddsData)),
            "BuildTestDds"))
    {
        return 1;
    }

    assets::DdsTextureLoader ddsLoader;
    assets::AssetLoaderRegistry loaderRegistry;

    if (!Check(
            assets::Succeeded(loaderRegistry.Register(ddsLoader)),
            "AssetLoaderRegistry::Register"))
    {
        return 1;
    }

    assets::AssetMetadata metadata;
    metadata.path = path;
    metadata.id = path.GetId();
    metadata.type = assets::AssetType::Texture;
    metadata.sourceSize = ddsData.GetSize();

    std::unique_ptr<assets::LoadedAsset> loadedAsset;

    if (!Check(
            assets::Succeeded(
                loaderRegistry.Load(
                    metadata,
                    ddsData,
                    loadedAsset)),
            "AssetLoaderRegistry::Load"))
    {
        return 1;
    }

    if (!Check(
            loadedAsset &&
                loadedAsset->GetType() == assets::AssetType::Texture,
            "typed DDS load"))
    {
        return 1;
    }

    MemoryAssetSource source(path, ddsData);
    assets::AssetManager assetManager;

    if (!Check(
            assets::Succeeded(assetManager.Initialize(source)),
            "AssetManager::Initialize"))
    {
        return 1;
    }

    assets::AssetHandle assetHandle;

    if (!Check(
            assets::Succeeded(
                assetManager.Register(
                    metadata,
                    assetHandle)),
            "AssetManager::Register"))
    {
        return 1;
    }

    FakeRenderDevice renderDevice;
    graphics::RenderDeviceDesc deviceDesc;
    deviceDesc.backend = graphics::GraphicsBackend::D3D11;

    if (!Check(
            graphics::Succeeded(renderDevice.Initialize(deviceDesc)),
            "FakeRenderDevice::Initialize"))
    {
        return 1;
    }

    assets::TextureAssetCache textureCache;

    if (!Check(
            textureCache.Initialize(
                assetManager,
                renderDevice).Succeeded(),
            "TextureAssetCache::Initialize"))
    {
        return 1;
    }

    graphics::TextureHandle firstTexture;

    if (!Check(
            textureCache.Acquire(
                assetHandle,
                firstTexture).Succeeded(),
            "TextureAssetCache::Acquire first"))
    {
        return 1;
    }

    graphics::TextureHandle secondTexture;

    if (!Check(
            textureCache.Acquire(
                assetHandle,
                secondTexture).Succeeded(),
            "TextureAssetCache::Acquire second"))
    {
        return 1;
    }

    if (!Check(
            firstTexture == secondTexture,
            "texture cache reuses GPU handle"))
    {
        return 1;
    }

    std::uint32_t referenceCount = 0U;

    if (!Check(
            textureCache.GetReferenceCount(
                assetHandle,
                referenceCount).Succeeded() &&
                referenceCount == 2U,
            "texture reference count"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Reload(assetHandle).Succeeded(),
            "TextureAssetCache::Reload"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Release(assetHandle).Succeeded(),
            "TextureAssetCache::Release first"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Release(assetHandle).Succeeded(),
            "TextureAssetCache::Release final"))
    {
        return 1;
    }

    if (!Check(
            !textureCache.Contains(assetHandle) &&
                renderDevice.GetTextureCount() == 0U,
            "texture destruction"))
    {
        return 1;
    }

    if (!Check(
            textureCache.Shutdown().Succeeded(),
            "TextureAssetCache::Shutdown"))
    {
        return 1;
    }

    if (!Check(
            assets::Succeeded(assetManager.Unload(assetHandle)),
            "AssetManager::Unload"))
    {
        return 1;
    }

    if (!Check(
            assets::Succeeded(assetManager.Unregister(assetHandle)),
            "AssetManager::Unregister"))
    {
        return 1;
    }

    assetManager.Shutdown();
    renderDevice.Shutdown();

    std::puts("LTS.Assets tests passed");
    return 0;
}
