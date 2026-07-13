#include "Assets/GpuTexture.h"

#include <new>
#include <vector>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]]
        engine::graphics::GraphicsResult BuildInitialData(
            const TextureAsset& textureAsset,
            std::vector<
                engine::graphics::TextureSubresourceData>&
                outInitialData) noexcept
        {
            outInitialData.clear();

            if (!textureAsset.IsValid())
            {
                return
                    engine::graphics::GraphicsResult::
                        InvalidArgument;
            }

            const std::size_t subresourceCount =
                textureAsset.GetSubresourceCount();

            if (subresourceCount == 0U)
            {
                return
                    engine::graphics::GraphicsResult::
                        InvalidArgument;
            }

            try
            {
                outInitialData.resize(
                    subresourceCount);
            }
            catch (const std::bad_alloc&)
            {
                return
                    engine::graphics::GraphicsResult::
                        OutOfMemory;
            }
            catch (...)
            {
                return
                    engine::graphics::GraphicsResult::
                        BackendFailure;
            }

            for (
                std::size_t index = 0U;
                index < subresourceCount;
                ++index
            )
            {
                const AssetResult assetResult =
                    textureAsset.GetSubresourceData(
                        index,
                        outInitialData[index]);

                if (Failed(assetResult))
                {
                    outInitialData.clear();

                    return
                        engine::graphics::GraphicsResult::
                            InvalidArgument;
                }

                if (!outInitialData[index].IsValid())
                {
                    outInitialData.clear();

                    return
                        engine::graphics::GraphicsResult::
                            InvalidArgument;
                }
            }

            return
                engine::graphics::GraphicsResult::Success;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult CreateTexture(
            engine::graphics::RenderDevice& device,
            const TextureAsset& textureAsset,
            engine::graphics::TextureHandle&
                outHandle) noexcept
        {
            outHandle =
                engine::graphics::TextureHandle{};

            if (!device.IsReady())
            {
                return
                    engine::graphics::GraphicsResult::
                        InvalidState;
            }

            if (!textureAsset.IsValid())
            {
                return
                    engine::graphics::GraphicsResult::
                        InvalidArgument;
            }

            std::vector<
                engine::graphics::TextureSubresourceData>
                    initialData;

            const engine::graphics::GraphicsResult
                buildResult =
                    BuildInitialData(
                        textureAsset,
                        initialData);

            if (
                engine::graphics::Failed(
                    buildResult)
            )
            {
                return buildResult;
            }

            const engine::graphics::GraphicsResult
                createResult =
                    device.CreateTexture(
                        textureAsset.GetDesc(),
                        initialData.data(),
                        initialData.size(),
                        outHandle);

            if (
                engine::graphics::Failed(
                    createResult)
            )
            {
                outHandle =
                    engine::graphics::TextureHandle{};

                return createResult;
            }

            if (!outHandle.IsValid())
            {
                return
                    engine::graphics::GraphicsResult::
                        BackendFailure;
            }

            return
                engine::graphics::GraphicsResult::Success;
        }

        [[nodiscard]] bool IsSuccessfulDestroyResult(
            const engine::graphics::GraphicsResult result) noexcept
        {
            return
                result ==
                    engine::graphics::GraphicsResult::
                        Success ||
                result ==
                    engine::graphics::GraphicsResult::
                        NotFound;
        }
    }

    engine::graphics::GraphicsResult
    GpuTexture::Upload(
        engine::graphics::RenderDevice& device,
        const TextureAsset& textureAsset) noexcept
    {
        if (
            handle_.IsValid() ||
            backend_ !=
                engine::graphics::GraphicsBackend::None
        )
        {
            return
                engine::graphics::GraphicsResult::
                    InvalidState;
        }

        engine::graphics::TextureHandle newHandle;

        const engine::graphics::GraphicsResult result =
            CreateTexture(
                device,
                textureAsset,
                newHandle);

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        handle_ = newHandle;
        desc_ = textureAsset.GetDesc();
        backend_ = device.GetBackend();

        return
            engine::graphics::GraphicsResult::Success;
    }

    engine::graphics::GraphicsResult
    GpuTexture::Replace(
        engine::graphics::RenderDevice& device,
        const TextureAsset& textureAsset) noexcept
    {
        if (!IsValid())
        {
            return
                engine::graphics::GraphicsResult::
                    InvalidState;
        }

        if (
            device.GetBackend() != backend_
        )
        {
            return
                engine::graphics::GraphicsResult::
                    InvalidArgument;
        }

        if (!device.IsReady())
        {
            return
                engine::graphics::GraphicsResult::
                    InvalidState;
        }

        engine::graphics::TextureHandle replacementHandle;

        const engine::graphics::GraphicsResult
            createResult =
                CreateTexture(
                    device,
                    textureAsset,
                    replacementHandle);

        if (
            engine::graphics::Failed(
                createResult)
        )
        {
            return createResult;
        }

        const engine::graphics::TextureHandle
            previousHandle = handle_;

        const engine::graphics::GraphicsResult
            destroyResult =
                device.DestroyTexture(
                    previousHandle);

        if (
            !IsSuccessfulDestroyResult(
                destroyResult)
        )
        {
            const engine::graphics::GraphicsResult
                cleanupResult =
                    device.DestroyTexture(
                        replacementHandle);

            (void)cleanupResult;

            return destroyResult;
        }

        handle_ = replacementHandle;
        desc_ = textureAsset.GetDesc();

        return
            engine::graphics::GraphicsResult::Success;
    }

    engine::graphics::GraphicsResult
    GpuTexture::Release(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (!handle_.IsValid())
        {
            ClearState();

            return
                engine::graphics::GraphicsResult::
                    Success;
        }

        if (
            backend_ ==
                engine::graphics::GraphicsBackend::None ||
            device.GetBackend() != backend_
        )
        {
            return
                engine::graphics::GraphicsResult::
                    InvalidArgument;
        }

        const engine::graphics::GraphicsResult result =
            device.DestroyTexture(handle_);

        if (
            IsSuccessfulDestroyResult(result) ||
            result ==
                engine::graphics::GraphicsResult::
                    DeviceRemoved
        )
        {
            ClearState();
        }

        return result;
    }

    void GpuTexture::Abandon() noexcept
    {
        ClearState();
    }

    bool GpuTexture::IsValid() const noexcept
    {
        return
            handle_.IsValid() &&
            backend_ !=
                engine::graphics::GraphicsBackend::None;
    }

    engine::graphics::TextureHandle
    GpuTexture::GetHandle() const noexcept
    {
        return handle_;
    }

    engine::graphics::GraphicsBackend
    GpuTexture::GetBackend() const noexcept
    {
        return backend_;
    }

    const engine::graphics::TextureDesc&
    GpuTexture::GetDesc() const noexcept
    {
        return desc_;
    }

    void GpuTexture::ClearState() noexcept
    {
        handle_ =
            engine::graphics::TextureHandle{};

        desc_ =
            engine::graphics::TextureDesc{};

        backend_ =
            engine::graphics::GraphicsBackend::None;
    }
}