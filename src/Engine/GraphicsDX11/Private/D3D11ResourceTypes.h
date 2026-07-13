#pragma once

#include "D3D11ComPtr.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

#include <d3d11.h>

namespace engine::graphics::d3d11::detail
{
    struct D3D11TextureResource final
    {
        TextureDesc desc;
        ComPtr<ID3D11Resource> native;
        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        ComPtr<ID3D11RenderTargetView> renderTargetView;
        ComPtr<ID3D11DepthStencilView> depthStencilView;
        ComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
    };

    struct D3D11BufferResource final
    {
        BufferDesc desc;
        ComPtr<ID3D11Buffer> native;
        ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        ComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
    };
}
