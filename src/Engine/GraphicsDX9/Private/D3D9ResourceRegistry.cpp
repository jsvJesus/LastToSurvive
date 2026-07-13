#include "D3D9ResourceRegistry.h"

#include "D3D9Buffer.h"
#include "D3D9Texture.h"

namespace engine::graphics::d3d9::detail
{
    GraphicsResult D3D9ResourceRegistry::CreateTexture(
        IDirect3DDevice9* device,
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        const std::size_t initialDataCount,
        TextureHandle& outTexture) noexcept
    {
        outTexture = TextureHandle{};

        D3D9TextureResource resource;
        const GraphicsResult buildResult = BuildTextureResource(
            desc,
            initialData,
            initialDataCount,
            resource);
        if (Failed(buildResult))
        {
            return buildResult;
        }

        const GraphicsResult createResult = CreateNativeTexture(device, resource);
        if (Failed(createResult))
        {
            return createResult;
        }

        outTexture = textures_.Insert(std::move(resource));
        return outTexture.IsValid()
            ? GraphicsResult::Success
            : GraphicsResult::OutOfMemory;
    }

    GraphicsResult D3D9ResourceRegistry::DestroyTexture(
        const TextureHandle texture) noexcept
    {
        return textures_.Remove(texture)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D9ResourceRegistry::CreateBuffer(
        IDirect3DDevice9* device,
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        BufferHandle& outBuffer) noexcept
    {
        outBuffer = BufferHandle{};

        D3D9BufferResource resource;
        const GraphicsResult buildResult = BuildBufferResource(
            desc,
            initialData,
            resource);
        if (Failed(buildResult))
        {
            return buildResult;
        }

        const GraphicsResult createResult = CreateNativeBuffer(device, resource);
        if (Failed(createResult))
        {
            return createResult;
        }

        outBuffer = buffers_.Insert(std::move(resource));
        return outBuffer.IsValid()
            ? GraphicsResult::Success
            : GraphicsResult::OutOfMemory;
    }

    GraphicsResult D3D9ResourceRegistry::DestroyBuffer(
        const BufferHandle buffer) noexcept
    {
        return buffers_.Remove(buffer)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    void D3D9ResourceRegistry::OnDeviceLost() noexcept
    {
        textures_.ForEach([](D3D9TextureResource& texture)
        {
            ReleaseTextureForDeviceLost(texture);
        });

        buffers_.ForEach([](D3D9BufferResource& buffer)
        {
            ReleaseBufferForDeviceLost(buffer);
        });
    }

    GraphicsResult D3D9ResourceRegistry::OnDeviceReset(
        IDirect3DDevice9* device) noexcept
    {
        if (device == nullptr)
        {
            return GraphicsResult::InvalidArgument;
        }

        GraphicsResult result = GraphicsResult::Success;

        textures_.ForEach([device, &result](D3D9TextureResource& texture)
        {
            if (texture.needsRestore)
            {
                const GraphicsResult restoreResult = CreateNativeTexture(device, texture);
                if (Failed(restoreResult) && Succeeded(result))
                {
                    result = restoreResult;
                }
            }
        });

        buffers_.ForEach([device, &result](D3D9BufferResource& buffer)
        {
            if (buffer.needsRestore)
            {
                const GraphicsResult restoreResult = CreateNativeBuffer(device, buffer);
                if (Failed(restoreResult) && Succeeded(result))
                {
                    result = restoreResult;
                }
            }
        });

        return result;
    }

    void D3D9ResourceRegistry::Clear() noexcept
    {
        textures_.Clear();
        buffers_.Clear();
    }

    IDirect3DBaseTexture9* D3D9ResourceRegistry::GetTexture(
        const TextureHandle texture) const noexcept
    {
        const D3D9TextureResource* resource = textures_.Get(texture);
        return resource != nullptr ? resource->native.Get() : nullptr;
    }

    IDirect3DVertexBuffer9* D3D9ResourceRegistry::GetVertexBuffer(
        const BufferHandle buffer) const noexcept
    {
        const D3D9BufferResource* resource = buffers_.Get(buffer);
        return resource != nullptr ? resource->vertexBuffer.Get() : nullptr;
    }

    IDirect3DIndexBuffer9* D3D9ResourceRegistry::GetIndexBuffer(
        const BufferHandle buffer) const noexcept
    {
        const D3D9BufferResource* resource = buffers_.Get(buffer);
        return resource != nullptr ? resource->indexBuffer.Get() : nullptr;
    }

    std::size_t D3D9ResourceRegistry::GetTextureCount() const noexcept
    {
        return textures_.Size();
    }

    std::size_t D3D9ResourceRegistry::GetBufferCount() const noexcept
    {
        return buffers_.Size();
    }
}
