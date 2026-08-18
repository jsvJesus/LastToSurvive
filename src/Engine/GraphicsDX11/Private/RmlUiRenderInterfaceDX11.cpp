#include "GraphicsDX11/RmlUiRenderInterfaceDX11.h"

#include "GraphicsDX11/D3D11Device.h"

#include <RmlUi/Core.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

namespace engine::graphics::d3d11
{
    namespace
    {
        template<class T>
        void Release(T*& value) noexcept
        {
            if (value != nullptr)
            {
                value->Release();
                value = nullptr;
            }
        }

        struct Geometry final
        {
            ID3D11Buffer* vertices = nullptr;
            ID3D11Buffer* indices = nullptr;
            UINT indexCount = 0;

            ~Geometry() noexcept
            {
                Release(vertices);
                Release(indices);
            }
        };

        struct Texture final
        {
            ID3D11Texture2D* resource = nullptr;
            ID3D11ShaderResourceView* view = nullptr;

            ~Texture() noexcept
            {
                Release(view);
                Release(resource);
            }
        };

        struct Constants final
        {
            float translation[2]{};
            float inverseViewport[2]{};
            float transform[16]{};
        };
    }

    class RmlUiRenderInterfaceDX11::Impl final
    {
    public:
        [[nodiscard]] bool Initialize(
            D3D11Device& renderDevice,
            const std::filesystem::path& shaderPath,
            const int width,
            const int height) noexcept
        {
            device = renderDevice.GetNativeDevice();
            context = renderDevice.GetNativeImmediateContext();
            viewportWidth = std::max(width, 1);
            viewportHeight = std::max(height, 1);
            if (device == nullptr || context == nullptr) return false;

            ID3DBlob* vertexCode = nullptr;
            ID3DBlob* pixelCode = nullptr;
            ID3DBlob* errors = nullptr;
            const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
            HRESULT result = D3DCompileFromFile(
                shaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                "VsMain", "vs_5_0", flags, 0, &vertexCode, &errors);
            if (FAILED(result))
            {
                if (errors != nullptr) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
                Release(errors);
                return false;
            }
            result = D3DCompileFromFile(
                shaderPath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                "PsMain", "ps_5_0", flags, 0, &pixelCode, &errors);
            if (FAILED(result))
            {
                if (errors != nullptr) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
                Release(errors);
                Release(vertexCode);
                return false;
            }
            Release(errors);

            result = device->CreateVertexShader(vertexCode->GetBufferPointer(),
                vertexCode->GetBufferSize(), nullptr, &vertexShader);
            if (SUCCEEDED(result))
                result = device->CreatePixelShader(pixelCode->GetBufferPointer(),
                    pixelCode->GetBufferSize(), nullptr, &pixelShader);

            const D3D11_INPUT_ELEMENT_DESC elements[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                 static_cast<UINT>(offsetof(Rml::Vertex, position)), D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0,
                 static_cast<UINT>(offsetof(Rml::Vertex, colour)), D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                 static_cast<UINT>(offsetof(Rml::Vertex, tex_coord)), D3D11_INPUT_PER_VERTEX_DATA, 0}
            };
            if (SUCCEEDED(result))
                result = device->CreateInputLayout(elements, ARRAYSIZE(elements),
                    vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), &inputLayout);
            Release(vertexCode);
            Release(pixelCode);

            D3D11_BUFFER_DESC constantDesc{};
            constantDesc.ByteWidth = sizeof(Constants);
            constantDesc.Usage = D3D11_USAGE_DYNAMIC;
            constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            constantDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (SUCCEEDED(result)) result = device->CreateBuffer(&constantDesc, nullptr, &constantBuffer);

            D3D11_BLEND_DESC blendDesc{};
            blendDesc.RenderTarget[0].BlendEnable = TRUE;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            if (SUCCEEDED(result)) result = device->CreateBlendState(&blendDesc, &blendState);

            D3D11_RASTERIZER_DESC rasterDesc{};
            rasterDesc.FillMode = D3D11_FILL_SOLID;
            rasterDesc.CullMode = D3D11_CULL_NONE;
            rasterDesc.ScissorEnable = TRUE;
            rasterDesc.DepthClipEnable = TRUE;
            if (SUCCEEDED(result)) result = device->CreateRasterizerState(&rasterDesc, &rasterState);

            D3D11_DEPTH_STENCIL_DESC depthDesc{};
            depthDesc.DepthEnable = FALSE;
            depthDesc.StencilEnable = FALSE;
            if (SUCCEEDED(result)) result = device->CreateDepthStencilState(&depthDesc, &depthState);

            D3D11_SAMPLER_DESC samplerDesc{};
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
            if (SUCCEEDED(result)) result = device->CreateSamplerState(&samplerDesc, &sampler);

            const std::array<Rml::byte, 4> white{255, 255, 255, 255};
            if (SUCCEEDED(result)) whiteTexture = CreateTexture(white.data(), 1, 1);
            initialized = SUCCEEDED(result) && whiteTexture != nullptr;
            if (!initialized) Shutdown();
            return initialized;
        }

        void Shutdown() noexcept
        {
            delete whiteTexture;
            whiteTexture = nullptr;
            Release(sampler);
            Release(depthState);
            Release(rasterState);
            Release(blendState);
            Release(constantBuffer);
            Release(inputLayout);
            Release(pixelShader);
            Release(vertexShader);
            device = nullptr;
            context = nullptr;
            initialized = false;
        }

        [[nodiscard]] Texture* CreateTexture(
            const Rml::byte* pixels,
            const int width,
            const int height) noexcept
        {
            if (pixels == nullptr || width <= 0 || height <= 0) return nullptr;
            auto* texture = new (std::nothrow) Texture();
            if (texture == nullptr) return nullptr;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = static_cast<UINT>(width);
            desc.Height = static_cast<UINT>(height);
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            D3D11_SUBRESOURCE_DATA data{};
            data.pSysMem = pixels;
            data.SysMemPitch = static_cast<UINT>(width * 4);
            HRESULT result = device->CreateTexture2D(&desc, &data, &texture->resource);
            if (SUCCEEDED(result))
                result = device->CreateShaderResourceView(texture->resource, nullptr, &texture->view);
            if (FAILED(result))
            {
                delete texture;
                return nullptr;
            }
            return texture;
        }

        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        ID3D11VertexShader* vertexShader = nullptr;
        ID3D11PixelShader* pixelShader = nullptr;
        ID3D11InputLayout* inputLayout = nullptr;
        ID3D11Buffer* constantBuffer = nullptr;
        ID3D11BlendState* blendState = nullptr;
        ID3D11RasterizerState* rasterState = nullptr;
        ID3D11DepthStencilState* depthState = nullptr;
        ID3D11SamplerState* sampler = nullptr;
        Texture* whiteTexture = nullptr;
        Rml::Matrix4f transform = Rml::Matrix4f::Identity();
        int viewportWidth = 1;
        int viewportHeight = 1;
        bool scissorEnabled = false;
        bool initialized = false;
    };

    RmlUiRenderInterfaceDX11::RmlUiRenderInterfaceDX11() : impl_(std::make_unique<Impl>()) {}
    RmlUiRenderInterfaceDX11::~RmlUiRenderInterfaceDX11() noexcept { Shutdown(); }

    bool RmlUiRenderInterfaceDX11::Initialize(
        D3D11Device& device, const std::filesystem::path& shaderPath,
        const int width, const int height) noexcept
    {
        return impl_->Initialize(device, shaderPath, width, height);
    }

    void RmlUiRenderInterfaceDX11::Shutdown() noexcept { impl_->Shutdown(); }

    void RmlUiRenderInterfaceDX11::SetViewportSize(const int width, const int height) noexcept
    {
        impl_->viewportWidth = std::max(width, 1);
        impl_->viewportHeight = std::max(height, 1);
    }

    void RmlUiRenderInterfaceDX11::PrepareRender() noexcept
    {
        auto* context = impl_->context;
        if (!impl_->initialized || context == nullptr) return;
        const float factors[4]{0.0F, 0.0F, 0.0F, 0.0F};
        context->IASetInputLayout(impl_->inputLayout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(impl_->vertexShader, nullptr, 0);
        context->PSSetShader(impl_->pixelShader, nullptr, 0);
        context->VSSetConstantBuffers(0, 1, &impl_->constantBuffer);
        context->PSSetSamplers(0, 1, &impl_->sampler);
        context->OMSetBlendState(impl_->blendState, factors, 0xffffffffU);
        context->OMSetDepthStencilState(impl_->depthState, 0);
        context->RSSetState(impl_->rasterState);
        SetScissorRegion(Rml::Rectanglei::FromPositionSize(
            Rml::Vector2i(0, 0), Rml::Vector2i(impl_->viewportWidth, impl_->viewportHeight)));
    }

    void RmlUiRenderInterfaceDX11::FinishRender() noexcept
    {
        if (impl_->context == nullptr) return;
        ID3D11ShaderResourceView* empty = nullptr;
        impl_->context->PSSetShaderResources(0, 1, &empty);
    }

    Rml::CompiledGeometryHandle RmlUiRenderInterfaceDX11::CompileGeometry(
        const Rml::Span<const Rml::Vertex> vertices,
        const Rml::Span<const int> indices)
    {
        if (!impl_->initialized || vertices.empty() || indices.empty()) return {};
        auto* geometry = new (std::nothrow) Geometry();
        if (geometry == nullptr) return {};
        D3D11_BUFFER_DESC vertexDesc{};
        vertexDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Rml::Vertex));
        vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vertexData{vertices.data(), 0, 0};
        HRESULT result = impl_->device->CreateBuffer(&vertexDesc, &vertexData, &geometry->vertices);
        D3D11_BUFFER_DESC indexDesc{};
        indexDesc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(int));
        indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA indexData{indices.data(), 0, 0};
        if (SUCCEEDED(result)) result = impl_->device->CreateBuffer(&indexDesc, &indexData, &geometry->indices);
        if (FAILED(result))
        {
            delete geometry;
            return {};
        }
        geometry->indexCount = static_cast<UINT>(indices.size());
        return reinterpret_cast<Rml::CompiledGeometryHandle>(geometry);
    }

    void RmlUiRenderInterfaceDX11::RenderGeometry(
        const Rml::CompiledGeometryHandle handle,
        const Rml::Vector2f translation,
        const Rml::TextureHandle textureHandle)
    {
        auto* geometry = reinterpret_cast<Geometry*>(handle);
        if (geometry == nullptr || impl_->context == nullptr) return;
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(impl_->context->Map(impl_->constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
        Constants constants{};
        constants.translation[0] = translation.x;
        constants.translation[1] = translation.y;
        constants.inverseViewport[0] = 1.0F / static_cast<float>(impl_->viewportWidth);
        constants.inverseViewport[1] = 1.0F / static_cast<float>(impl_->viewportHeight);
        std::memcpy(constants.transform, impl_->transform.data(), sizeof(constants.transform));
        std::memcpy(mapped.pData, &constants, sizeof(constants));
        impl_->context->Unmap(impl_->constantBuffer, 0);
        const UINT stride = sizeof(Rml::Vertex);
        const UINT offset = 0;
        impl_->context->IASetVertexBuffers(0, 1, &geometry->vertices, &stride, &offset);
        impl_->context->IASetIndexBuffer(geometry->indices, DXGI_FORMAT_R32_UINT, 0);
        auto* texture = textureHandle != Rml::TextureHandle{}
            ? reinterpret_cast<Texture*>(textureHandle) : impl_->whiteTexture;
        impl_->context->PSSetShaderResources(0, 1, &texture->view);
        impl_->context->DrawIndexed(geometry->indexCount, 0, 0);
    }

    void RmlUiRenderInterfaceDX11::ReleaseGeometry(const Rml::CompiledGeometryHandle geometry)
    {
        delete reinterpret_cast<Geometry*>(geometry);
    }

    Rml::TextureHandle RmlUiRenderInterfaceDX11::LoadTexture(
        Rml::Vector2i&, const Rml::String&)
    {
        return {};
    }

    Rml::TextureHandle RmlUiRenderInterfaceDX11::GenerateTexture(
        const Rml::Span<const Rml::byte> source, const Rml::Vector2i dimensions)
    {
        return reinterpret_cast<Rml::TextureHandle>(
            impl_->CreateTexture(source.data(), dimensions.x, dimensions.y));
    }

    void RmlUiRenderInterfaceDX11::ReleaseTexture(const Rml::TextureHandle texture)
    {
        delete reinterpret_cast<Texture*>(texture);
    }

    void RmlUiRenderInterfaceDX11::EnableScissorRegion(const bool enable)
    {
        impl_->scissorEnabled = enable;
        if (!enable)
            SetScissorRegion(Rml::Rectanglei::FromPositionSize(
                Rml::Vector2i(0, 0), Rml::Vector2i(impl_->viewportWidth, impl_->viewportHeight)));
    }

    void RmlUiRenderInterfaceDX11::SetScissorRegion(const Rml::Rectanglei region)
    {
        if (impl_->context == nullptr) return;
        const auto position = region.Position();
        const auto size = region.Size();
        const D3D11_RECT rectangle{
            std::max(position.x, 0), std::max(position.y, 0),
            std::min(position.x + size.x, impl_->viewportWidth),
            std::min(position.y + size.y, impl_->viewportHeight)};
        impl_->context->RSSetScissorRects(1, &rectangle);
    }

    void RmlUiRenderInterfaceDX11::SetTransform(const Rml::Matrix4f* transform)
    {
        impl_->transform = transform != nullptr ? *transform : Rml::Matrix4f::Identity();
    }
}
