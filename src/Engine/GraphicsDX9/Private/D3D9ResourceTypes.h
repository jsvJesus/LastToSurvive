#pragma once

#include "D3D9ComPtr.h"

#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

#include <d3d9.h>

#include <cstddef>
#include <vector>

namespace engine::graphics::d3d9::detail
{
    struct OwnedTextureSubresource final
    {
        std::vector<std::byte> bytes;
        std::size_t rowPitch = 0;
        std::size_t slicePitch = 0;
    };

    struct D3D9TextureResource final
    {
        TextureDesc desc;
        D3DPOOL pool = D3DPOOL_MANAGED;
        ComPtr<IDirect3DBaseTexture9> native;
        std::vector<OwnedTextureSubresource> initialData;
        bool needsRestore = false;
    };

    enum class D3D9BufferKind : unsigned char
    {
        None = 0,
        Vertex,
        Index
    };

    struct D3D9BufferResource final
    {
        BufferDesc desc;
        D3DPOOL pool = D3DPOOL_MANAGED;
        D3D9BufferKind kind = D3D9BufferKind::None;
        ComPtr<IDirect3DVertexBuffer9> vertexBuffer;
        ComPtr<IDirect3DIndexBuffer9> indexBuffer;
        std::vector<std::byte> initialData;
        bool needsRestore = false;
    };
}
