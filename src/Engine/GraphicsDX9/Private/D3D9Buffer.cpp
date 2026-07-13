#include "D3D9Buffer.h"

#include "D3D9Conversions.h"

#include <cstring>
#include <limits>
#include <utility>

namespace engine::graphics::d3d9::detail
{
    namespace
    {
        [[nodiscard]] GraphicsResult DetermineKind(
            const BufferDesc& desc,
            D3D9BufferKind& outKind) noexcept
        {
            const bool vertex = HasAnyFlag(desc.bindFlags, BufferBindFlags::Vertex);
            const bool index = HasAnyFlag(desc.bindFlags, BufferBindFlags::Index);

            if (vertex == index)
            {
                outKind = D3D9BufferKind::None;
                return GraphicsResult::Unsupported;
            }

            outKind = vertex
                ? D3D9BufferKind::Vertex
                : D3D9BufferKind::Index;
            return GraphicsResult::Success;
        }

        [[nodiscard]] GraphicsResult UploadInitialData(
            D3D9BufferResource& resource) noexcept
        {
            if (resource.initialData.empty())
            {
                return GraphicsResult::Success;
            }

            void* destination = nullptr;
            const DWORD flags = resource.desc.usage == ResourceUsage::Dynamic
                ? D3DLOCK_DISCARD
                : 0;
            HRESULT hr = E_FAIL;

            if (resource.kind == D3D9BufferKind::Vertex)
            {
                hr = resource.vertexBuffer.Get()->Lock(
                    0,
                    static_cast<UINT>(resource.initialData.size()),
                    &destination,
                    flags);
            }
            else
            {
                hr = resource.indexBuffer.Get()->Lock(
                    0,
                    static_cast<UINT>(resource.initialData.size()),
                    &destination,
                    flags);
            }

            if (FAILED(hr) || destination == nullptr)
            {
                return hr == D3DERR_DEVICELOST
                    ? GraphicsResult::DeviceLost
                    : GraphicsResult::BackendFailure;
            }

            std::memcpy(
                destination,
                resource.initialData.data(),
                resource.initialData.size());

            if (resource.kind == D3D9BufferKind::Vertex)
            {
                resource.vertexBuffer.Get()->Unlock();
            }
            else
            {
                resource.indexBuffer.Get()->Unlock();
            }

            return GraphicsResult::Success;
        }
    }

    GraphicsResult BuildBufferResource(
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        D3D9BufferResource& outResource) noexcept
    {
        if (!desc.IsValid() ||
            desc.byteSize > (std::numeric_limits<UINT>::max)())
        {
            return GraphicsResult::InvalidArgument;
        }

        DWORD usage = 0;
        D3DPOOL pool = D3DPOOL_MANAGED;
        const GraphicsResult usageResult = ConvertBufferUsage(desc, usage, pool);
        if (Failed(usageResult))
        {
            return usageResult;
        }

        D3D9BufferKind kind = D3D9BufferKind::None;
        const GraphicsResult kindResult = DetermineKind(desc, kind);
        if (Failed(kindResult))
        {
            return kindResult;
        }

        if (desc.usage == ResourceUsage::Immutable && initialData == nullptr)
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D9BufferResource resource;
        resource.desc = desc;
        resource.pool = pool;
        resource.kind = kind;

        if (initialData != nullptr)
        {
            if (!initialData->IsValid() || initialData->dataSize > desc.byteSize)
            {
                return GraphicsResult::InvalidArgument;
            }

            resource.initialData.assign(
                initialData->data,
                initialData->data + initialData->dataSize);
        }

        outResource = std::move(resource);
        return GraphicsResult::Success;
    }

    GraphicsResult CreateNativeBuffer(
        IDirect3DDevice9* device,
        D3D9BufferResource& resource) noexcept
    {
        if (device == nullptr)
        {
            return GraphicsResult::InvalidArgument;
        }

        DWORD usage = 0;
        D3DPOOL pool = D3DPOOL_MANAGED;
        const GraphicsResult usageResult = ConvertBufferUsage(
            resource.desc,
            usage,
            pool);
        if (Failed(usageResult))
        {
            return usageResult;
        }

        resource.vertexBuffer.Reset();
        resource.indexBuffer.Reset();
        HRESULT hr = E_FAIL;

        if (resource.kind == D3D9BufferKind::Vertex)
        {
            IDirect3DVertexBuffer9* buffer = nullptr;
            hr = device->CreateVertexBuffer(
                static_cast<UINT>(resource.desc.byteSize),
                usage,
                0,
                pool,
                &buffer,
                nullptr);
            resource.vertexBuffer.Attach(buffer);
        }
        else if (resource.kind == D3D9BufferKind::Index)
        {
            const D3DFORMAT format = resource.desc.indexFormat == IndexFormat::UInt16
                ? D3DFMT_INDEX16
                : D3DFMT_INDEX32;
            IDirect3DIndexBuffer9* buffer = nullptr;
            hr = device->CreateIndexBuffer(
                static_cast<UINT>(resource.desc.byteSize),
                usage,
                format,
                pool,
                &buffer,
                nullptr);
            resource.indexBuffer.Attach(buffer);
        }
        else
        {
            return GraphicsResult::InvalidState;
        }

        if (FAILED(hr))
        {
            resource.vertexBuffer.Reset();
            resource.indexBuffer.Reset();
            return hr == D3DERR_DEVICELOST
                ? GraphicsResult::DeviceLost
                : hr == E_OUTOFMEMORY
                    ? GraphicsResult::OutOfMemory
                    : GraphicsResult::BackendFailure;
        }

        const GraphicsResult uploadResult = UploadInitialData(resource);
        if (Failed(uploadResult))
        {
            resource.vertexBuffer.Reset();
            resource.indexBuffer.Reset();
            return uploadResult;
        }

        resource.pool = pool;
        resource.needsRestore = false;
        return GraphicsResult::Success;
    }

    void ReleaseBufferForDeviceLost(
        D3D9BufferResource& resource) noexcept
    {
        if (IsDefaultPool(resource.pool))
        {
            resource.vertexBuffer.Reset();
            resource.indexBuffer.Reset();
            resource.needsRestore = true;
        }
    }
}
