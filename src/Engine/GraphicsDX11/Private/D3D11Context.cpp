#include "GraphicsDX11/D3D11Context.h"

#include "D3D11ResourceRegistry.h"
#include "D3D11ResourceTypes.h"
#include "GraphicsDX11/D3D11SwapChain.h"

#include <d3d11.h>

#include <array>
#include <cmath>

namespace engine::graphics::d3d11
{
    bool D3D11Context::IsValid() const noexcept
    {
        return context_ != nullptr && resources_ != nullptr;
    }

    ID3D11DeviceContext* D3D11Context::GetNativeContext() const noexcept
    {
        return context_;
    }

    GraphicsResult D3D11Context::SetViewport(
        const Viewport& viewport) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (!viewport.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11_VIEWPORT nativeViewport{};
        nativeViewport.TopLeftX = viewport.x;
        nativeViewport.TopLeftY = viewport.y;
        nativeViewport.Width = viewport.width;
        nativeViewport.Height = viewport.height;
        nativeViewport.MinDepth = viewport.minDepth;
        nativeViewport.MaxDepth = viewport.maxDepth;

        context_->RSSetViewports(1U, &nativeViewport);
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::SetScissorRect(
        const ScissorRect& scissorRect) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (!scissorRect.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11_RECT nativeRect{};
        nativeRect.left = scissorRect.left;
        nativeRect.top = scissorRect.top;
        nativeRect.right = scissorRect.right;
        nativeRect.bottom = scissorRect.bottom;

        context_->RSSetScissorRects(1U, &nativeRect);
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::SetSwapChainRenderTarget(
        SwapChain& swapChain) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (swapChain.GetBackend() != GraphicsBackend::D3D11)
        {
            return GraphicsResult::InvalidArgument;
        }
        auto& d3d11SwapChain = static_cast<D3D11SwapChain&>(swapChain);
        ID3D11RenderTargetView* const renderTarget =
            d3d11SwapChain.GetBackBufferRenderTargetView();
        if (renderTarget == nullptr)
        {
            return GraphicsResult::InvalidState;
        }
        context_->OMSetRenderTargets(1U, &renderTarget, nullptr);
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::SetSwapChainRenderTarget(
        SwapChain& swapChain,
        const TextureHandle depthStencilTarget) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (swapChain.GetBackend() != GraphicsBackend::D3D11)
        {
            return GraphicsResult::InvalidArgument;
        }
        if (!depthStencilTarget.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        auto& d3d11SwapChain = static_cast<D3D11SwapChain&>(swapChain);
        ID3D11RenderTargetView* const renderTarget =
            d3d11SwapChain.GetBackBufferRenderTargetView();

        if (renderTarget == nullptr)
        {
            return GraphicsResult::InvalidState;
        }

        ID3D11DepthStencilView* depthStencil = nullptr;
        {
            const detail::D3D11TextureResource* const resource =
                resources_->GetTexture(depthStencilTarget);
            if (resource == nullptr)
            {
                return GraphicsResult::NotFound;
            }
            if (!resource->depthStencilView)
            {
                return GraphicsResult::InvalidArgument;
            }
            depthStencil = resource->depthStencilView.Get();
        }

        context_->OMSetRenderTargets(1U, &renderTarget, depthStencil);
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::SetRenderTargets(
        const TextureHandle* const colorTargets,
        const std::size_t colorTargetCount,
        const TextureHandle depthStencilTarget) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (
            colorTargetCount > MaxColorRenderTargets ||
            (colorTargetCount != 0U && colorTargets == nullptr))
        {
            return GraphicsResult::InvalidArgument;
        }

        std::array<ID3D11RenderTargetView*, MaxColorRenderTargets>
            nativeColorTargets{};

        for (std::size_t index = 0U; index < colorTargetCount; ++index)
        {
            if (!colorTargets[index].IsValid())
            {
                return GraphicsResult::InvalidArgument;
            }

            const detail::D3D11TextureResource* const resource =
                resources_->GetTexture(colorTargets[index]);

            if (resource == nullptr)
            {
                return GraphicsResult::NotFound;
            }
            if (!resource->renderTargetView)
            {
                return GraphicsResult::InvalidArgument;
            }

            nativeColorTargets[index] = resource->renderTargetView.Get();
        }

        ID3D11DepthStencilView* nativeDepthStencil = nullptr;

        if (depthStencilTarget.IsValid())
        {
            const detail::D3D11TextureResource* const resource =
                resources_->GetTexture(depthStencilTarget);

            if (resource == nullptr)
            {
                return GraphicsResult::NotFound;
            }
            if (!resource->depthStencilView)
            {
                return GraphicsResult::InvalidArgument;
            }

            nativeDepthStencil = resource->depthStencilView.Get();
        }

        context_->OMSetRenderTargets(
            static_cast<UINT>(colorTargetCount),
            nativeColorTargets.data(),
            nativeDepthStencil);

        return GraphicsResult::Success;
    }

    void D3D11Context::UnbindRenderTargets() noexcept
    {
        if (context_ != nullptr)
        {
            context_->OMSetRenderTargets(0U, nullptr, nullptr);
        }
    }

    GraphicsResult D3D11Context::ClearSwapChainColor(
        SwapChain& swapChain,
        const ClearColor& color) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (!color.IsValid() || swapChain.GetBackend() != GraphicsBackend::D3D11)
        {
            return GraphicsResult::InvalidArgument;
        }

        auto& d3d11SwapChain = static_cast<D3D11SwapChain&>(swapChain);
        ID3D11RenderTargetView* const renderTarget =
            d3d11SwapChain.GetBackBufferRenderTargetView();

        if (renderTarget == nullptr)
        {
            return GraphicsResult::InvalidState;
        }

        const std::array<float, 4U> values = color.ToArray();
        context_->ClearRenderTargetView(renderTarget, values.data());
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::ClearColorTarget(
        const TextureHandle texture,
        const ClearColor& color) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (!texture.IsValid() || !color.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const detail::D3D11TextureResource* const resource =
            resources_->GetTexture(texture);

        if (resource == nullptr)
        {
            return GraphicsResult::NotFound;
        }
        if (!resource->renderTargetView)
        {
            return GraphicsResult::InvalidArgument;
        }

        const std::array<float, 4U> values = color.ToArray();
        context_->ClearRenderTargetView(
            resource->renderTargetView.Get(),
            values.data());

        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::ClearDepthStencilTarget(
        const TextureHandle texture,
        const ClearDepthStencilFlags flags,
        const float depth,
        const std::uint8_t stencil) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (
            !texture.IsValid() ||
            flags == ClearDepthStencilFlags::None ||
            !std::isfinite(depth) ||
            depth < 0.0F ||
            depth > 1.0F)
        {
            return GraphicsResult::InvalidArgument;
        }

        const detail::D3D11TextureResource* const resource =
            resources_->GetTexture(texture);

        if (resource == nullptr)
        {
            return GraphicsResult::NotFound;
        }
        if (!resource->depthStencilView)
        {
            return GraphicsResult::InvalidArgument;
        }

        UINT nativeFlags = 0U;
        if (HasAnyFlag(flags, ClearDepthStencilFlags::Depth))
        {
            nativeFlags |= D3D11_CLEAR_DEPTH;
        }
        if (HasAnyFlag(flags, ClearDepthStencilFlags::Stencil))
        {
            nativeFlags |= D3D11_CLEAR_STENCIL;
        }

        context_->ClearDepthStencilView(
            resource->depthStencilView.Get(),
            nativeFlags,
            depth,
            stencil);

        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::SetGraphicsPipeline(
        const PipelineStateHandle pipeline) noexcept
    {
        return BindGraphicsPipeline(pipeline);
    }

    GraphicsResult D3D11Context::BindGraphicsPipeline(
        const PipelineStateHandle pipeline) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (!pipeline.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const detail::D3D11GraphicsPipelineResource* const resource =
            resources_->GetGraphicsPipeline(pipeline);

        if (resource == nullptr)
        {
            return GraphicsResult::NotFound;
        }

        context_->IASetInputLayout(resource->inputLayout.Get());
        context_->IASetPrimitiveTopology(resource->topology);

        context_->VSSetShader(resource->vertexShader.Get(), nullptr, 0U);
        context_->PSSetShader(resource->pixelShader.Get(), nullptr, 0U);
        context_->GSSetShader(resource->geometryShader.Get(), nullptr, 0U);
        context_->HSSetShader(resource->hullShader.Get(), nullptr, 0U);
        context_->DSSetShader(resource->domainShader.Get(), nullptr, 0U);

        // Graphics and compute pipelines are mutually exclusive on the
        // immediate context. A graphics bind clears any previous CS stage.
        context_->CSSetShader(nullptr, nullptr, 0U);

        context_->RSSetState(resource->rasterizerState.Get());
        context_->OMSetBlendState(
            resource->blendState.Get(),
            resource->blendConstants.data(),
            resource->sampleMask);
        context_->OMSetDepthStencilState(
            resource->depthStencilState.Get(),
            resource->stencilReference);

        return GraphicsResult::Success;
    }

    void D3D11Context::UnbindGraphicsPipeline() noexcept
    {
        if (context_ == nullptr)
        {
            return;
        }

        context_->IASetInputLayout(nullptr);
        context_->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED);

        context_->VSSetShader(nullptr, nullptr, 0U);
        context_->PSSetShader(nullptr, nullptr, 0U);
        context_->GSSetShader(nullptr, nullptr, 0U);
        context_->HSSetShader(nullptr, nullptr, 0U);
        context_->DSSetShader(nullptr, nullptr, 0U);

        context_->RSSetState(nullptr);
        context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFU);
        context_->OMSetDepthStencilState(nullptr, 0U);
    }

    GraphicsResult D3D11Context::SetVertexBuffers(
        const std::uint32_t firstSlot,
        const VertexBufferBinding* const bindings,
        const std::size_t bindingCount) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (
            bindings == nullptr ||
            bindingCount == 0U ||
            bindingCount > MaxVertexBufferSlots ||
            firstSlot >= MaxVertexBufferSlots ||
            firstSlot + bindingCount > MaxVertexBufferSlots)
        {
            return GraphicsResult::InvalidArgument;
        }

        std::array<ID3D11Buffer*, MaxVertexBufferSlots> nativeBuffers{};
        std::array<UINT, MaxVertexBufferSlots> nativeStrides{};
        std::array<UINT, MaxVertexBufferSlots> nativeOffsets{};

        for (std::size_t index = 0U; index < bindingCount; ++index)
        {
            if (!bindings[index].IsValid())
            {
                return GraphicsResult::InvalidArgument;
            }

            const detail::D3D11BufferResource* const resource =
                resources_->GetBuffer(bindings[index].buffer);

            if (resource == nullptr)
            {
                return GraphicsResult::NotFound;
            }
            if (
                !resource->native ||
                !HasAnyFlag(resource->desc.bindFlags, BufferBindFlags::Vertex))
            {
                return GraphicsResult::InvalidArgument;
            }

            nativeBuffers[index] = resource->native.Get();
            nativeStrides[index] = bindings[index].stride;
            nativeOffsets[index] = bindings[index].offset;
        }

        context_->IASetVertexBuffers(
            firstSlot,
            static_cast<UINT>(bindingCount),
            nativeBuffers.data(),
            nativeStrides.data(),
            nativeOffsets.data());

        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::SetIndexBuffer(
        const IndexBufferBinding& binding) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (!binding.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const detail::D3D11BufferResource* const resource =
            resources_->GetBuffer(binding.buffer);

        if (resource == nullptr)
        {
            return GraphicsResult::NotFound;
        }
        if (
            !resource->native ||
            !HasAnyFlag(resource->desc.bindFlags, BufferBindFlags::Index))
        {
            return GraphicsResult::InvalidArgument;
        }

        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        switch (resource->desc.indexFormat)
        {
        case IndexFormat::UInt16:
            format = DXGI_FORMAT_R16_UINT;
            break;
        case IndexFormat::UInt32:
            format = DXGI_FORMAT_R32_UINT;
            break;
        default:
            return GraphicsResult::InvalidArgument;
        }

        context_->IASetIndexBuffer(
            resource->native.Get(),
            format,
            binding.offset);

        return GraphicsResult::Success;
    }

    void D3D11Context::UnbindIndexBuffer() noexcept
    {
        if (context_ != nullptr)
        {
            context_->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0U);
        }
    }

    GraphicsResult D3D11Context::Draw(
        const std::uint32_t vertexCount,
        const std::uint32_t firstVertex) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (vertexCount == 0U)
        {
            return GraphicsResult::InvalidArgument;
        }

        context_->Draw(vertexCount, firstVertex);
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::DrawIndexed(
        const std::uint32_t indexCount,
        const std::uint32_t firstIndex,
        const std::int32_t baseVertex) noexcept
    {
        if (!IsValid())
        {
            return GraphicsResult::InvalidState;
        }
        if (indexCount == 0U)
        {
            return GraphicsResult::InvalidArgument;
        }

        context_->DrawIndexed(indexCount, firstIndex, baseVertex);
        return GraphicsResult::Success;
    }

    void D3D11Context::ClearState() noexcept
    {
        if (context_ != nullptr)
        {
            context_->ClearState();
        }
    }

    void D3D11Context::Flush() noexcept
    {
        if (context_ != nullptr)
        {
            context_->Flush();
        }
    }

    void D3D11Context::Attach(
        ID3D11DeviceContext* context,
        detail::D3D11ResourceRegistry* resources) noexcept
    {
        context_ = context;
        resources_ = resources;
    }

    void D3D11Context::Detach() noexcept
    {
        resources_ = nullptr;
        context_ = nullptr;
    }
}
