#include "D3D11ResourceRegistry.h"

#include "D3D11Buffer.h"
#include "D3D11Texture.h"

#include <new>
#include <utility>

namespace engine::graphics::d3d11::detail
{
    GraphicsResult D3D11ResourceRegistry::CreateTexture(
        ID3D11Device* device,
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        const std::size_t initialDataCount,
        TextureHandle& outTexture) noexcept
    {
        outTexture = TextureHandle{};
        D3D11TextureResource resource;
        GraphicsResult result = CreateTextureResource(
            device, desc, initialData, initialDataCount, resource);
        if (Failed(result)) return result;

        try
        {
            outTexture = textures_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyTexture(
        const TextureHandle texture) noexcept
    {
        return textures_.Remove(texture)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D11ResourceRegistry::CreateBuffer(
        ID3D11Device* device,
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        BufferHandle& outBuffer) noexcept
    {
        outBuffer = BufferHandle{};
        D3D11BufferResource resource;
        GraphicsResult result = CreateBufferResource(
            device, desc, initialData, resource);
        if (Failed(result)) return result;

        try
        {
            outBuffer = buffers_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyBuffer(
        const BufferHandle buffer) noexcept
    {
        return buffers_.Remove(buffer)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    void D3D11ResourceRegistry::Clear() noexcept
    {
        textures_.Clear();
        buffers_.Clear();
    }

    const D3D11TextureResource* D3D11ResourceRegistry::GetTexture(
        const TextureHandle texture) const noexcept
    {
        return textures_.Get(texture);
    }

    const D3D11BufferResource* D3D11ResourceRegistry::GetBuffer(
        const BufferHandle buffer) const noexcept
    {
        return buffers_.Get(buffer);
    }

    std::size_t D3D11ResourceRegistry::GetTextureCount() const noexcept
    {
        return textures_.Size();
    }

    std::size_t D3D11ResourceRegistry::GetBufferCount() const noexcept
    {
        return buffers_.Size();
    }
}
