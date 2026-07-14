#include "GraphicsDX11/D3D11Context.h"

#include "D3D11ResourceRegistry.h"
#include "D3D11ResourceTypes.h"
#include "GraphicsDX11/D3D11SwapChain.h"

#include <d3d11.h>

#include <array>
#include <cmath>
#include <cstring>
#include <type_traits>

namespace
{
    [[nodiscard]] bool IsSlotRangeValid(
        const std::uint32_t firstSlot,
        const std::size_t count,
        const std::size_t maximum) noexcept
    {
        return firstSlot <= maximum && count <= maximum - firstSlot;
    }

    template<typename Resource>
    [[nodiscard]] bool SetStageResources(
        ID3D11DeviceContext* const context,
        const engine::graphics::ShaderStage stage,
        const UINT firstSlot,
        const UINT count,
        Resource* const* resources) noexcept
    {
        switch (stage)
        {
        case engine::graphics::ShaderStage::Vertex:
            if constexpr (std::is_same_v<Resource, ID3D11ShaderResourceView>)
                context->VSSetShaderResources(firstSlot, count, resources);
            else if constexpr (std::is_same_v<Resource, ID3D11SamplerState>)
                context->VSSetSamplers(firstSlot, count, resources);
            else context->VSSetConstantBuffers(firstSlot, count, resources);
            return true;
        case engine::graphics::ShaderStage::Pixel:
            if constexpr (std::is_same_v<Resource, ID3D11ShaderResourceView>)
                context->PSSetShaderResources(firstSlot, count, resources);
            else if constexpr (std::is_same_v<Resource, ID3D11SamplerState>)
                context->PSSetSamplers(firstSlot, count, resources);
            else context->PSSetConstantBuffers(firstSlot, count, resources);
            return true;
        case engine::graphics::ShaderStage::Geometry:
            if constexpr (std::is_same_v<Resource, ID3D11ShaderResourceView>)
                context->GSSetShaderResources(firstSlot, count, resources);
            else if constexpr (std::is_same_v<Resource, ID3D11SamplerState>)
                context->GSSetSamplers(firstSlot, count, resources);
            else context->GSSetConstantBuffers(firstSlot, count, resources);
            return true;
        case engine::graphics::ShaderStage::Hull:
            if constexpr (std::is_same_v<Resource, ID3D11ShaderResourceView>)
                context->HSSetShaderResources(firstSlot, count, resources);
            else if constexpr (std::is_same_v<Resource, ID3D11SamplerState>)
                context->HSSetSamplers(firstSlot, count, resources);
            else context->HSSetConstantBuffers(firstSlot, count, resources);
            return true;
        case engine::graphics::ShaderStage::Domain:
            if constexpr (std::is_same_v<Resource, ID3D11ShaderResourceView>)
                context->DSSetShaderResources(firstSlot, count, resources);
            else if constexpr (std::is_same_v<Resource, ID3D11SamplerState>)
                context->DSSetSamplers(firstSlot, count, resources);
            else context->DSSetConstantBuffers(firstSlot, count, resources);
            return true;
        case engine::graphics::ShaderStage::Compute:
            if constexpr (std::is_same_v<Resource, ID3D11ShaderResourceView>)
                context->CSSetShaderResources(firstSlot, count, resources);
            else if constexpr (std::is_same_v<Resource, ID3D11SamplerState>)
                context->CSSetSamplers(firstSlot, count, resources);
            else context->CSSetConstantBuffers(firstSlot, count, resources);
            return true;
        default:
            return false;
        }
    }
}

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

    GraphicsResult D3D11Context::SetShaderResources(
        const ShaderStage stage,
        const std::uint32_t firstSlot,
        const TextureHandle* const textures,
        const std::size_t textureCount) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (textureCount == 0U) return GraphicsResult::Success;
        if (textures == nullptr ||
            !IsSlotRangeValid(firstSlot, textureCount, MaxShaderResourceSlots))
            return GraphicsResult::InvalidArgument;

        std::array<ID3D11ShaderResourceView*, MaxShaderResourceSlots> views{};
        for (std::size_t index = 0U; index < textureCount; ++index)
        {
            if (!textures[index].IsValid()) return GraphicsResult::InvalidArgument;
            const detail::D3D11TextureResource* const resource =
                resources_->GetTexture(textures[index]);
            if (resource == nullptr) return GraphicsResult::NotFound;
            if (!resource->shaderResourceView ||
                !HasAnyFlag(resource->desc.bindFlags,
                    TextureBindFlags::ShaderResource))
                return GraphicsResult::InvalidArgument;
            views[index] = resource->shaderResourceView.Get();
        }
        if (!SetStageResources(context_, stage, firstSlot,
                static_cast<UINT>(textureCount), views.data()))
            return GraphicsResult::Unsupported;
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::UnbindShaderResources(
        const ShaderStage stage,
        const std::uint32_t firstSlot,
        const std::size_t textureCount) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (textureCount == 0U) return GraphicsResult::Success;
        if (!IsSlotRangeValid(firstSlot, textureCount, MaxShaderResourceSlots))
            return GraphicsResult::InvalidArgument;
        std::array<ID3D11ShaderResourceView*, MaxShaderResourceSlots> views{};
        return SetStageResources(context_, stage, firstSlot,
            static_cast<UINT>(textureCount), views.data())
            ? GraphicsResult::Success : GraphicsResult::Unsupported;
    }

    GraphicsResult D3D11Context::SetSamplers(
        const ShaderStage stage,
        const std::uint32_t firstSlot,
        const SamplerHandle* const samplers,
        const std::size_t samplerCount) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (samplerCount == 0U) return GraphicsResult::Success;
        if (samplers == nullptr ||
            !IsSlotRangeValid(firstSlot, samplerCount, MaxSamplerSlots))
            return GraphicsResult::InvalidArgument;
        std::array<ID3D11SamplerState*, MaxSamplerSlots> states{};
        for (std::size_t index = 0U; index < samplerCount; ++index)
        {
            if (!samplers[index].IsValid()) return GraphicsResult::InvalidArgument;
            const detail::D3D11SamplerResource* const resource =
                resources_->GetSampler(samplers[index]);
            if (resource == nullptr) return GraphicsResult::NotFound;
            if (!resource->native) return GraphicsResult::InvalidArgument;
            states[index] = resource->native.Get();
        }
        if (!SetStageResources(context_, stage, firstSlot,
                static_cast<UINT>(samplerCount), states.data()))
            return GraphicsResult::Unsupported;
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::UnbindSamplers(
        const ShaderStage stage,
        const std::uint32_t firstSlot,
        const std::size_t samplerCount) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (samplerCount == 0U) return GraphicsResult::Success;
        if (!IsSlotRangeValid(firstSlot, samplerCount, MaxSamplerSlots))
            return GraphicsResult::InvalidArgument;
        std::array<ID3D11SamplerState*, MaxSamplerSlots> states{};
        return SetStageResources(context_, stage, firstSlot,
            static_cast<UINT>(samplerCount), states.data())
            ? GraphicsResult::Success : GraphicsResult::Unsupported;
    }

    GraphicsResult D3D11Context::SetConstantBuffers(
        const ShaderStage stage,
        const std::uint32_t firstSlot,
        const BufferHandle* const buffers,
        const std::size_t bufferCount) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (bufferCount == 0U) return GraphicsResult::Success;
        if (buffers == nullptr ||
            !IsSlotRangeValid(firstSlot, bufferCount, MaxConstantBufferSlots))
            return GraphicsResult::InvalidArgument;
        std::array<ID3D11Buffer*, MaxConstantBufferSlots> nativeBuffers{};
        for (std::size_t index = 0U; index < bufferCount; ++index)
        {
            if (!buffers[index].IsValid()) return GraphicsResult::InvalidArgument;
            const detail::D3D11BufferResource* const resource =
                resources_->GetBuffer(buffers[index]);
            if (resource == nullptr) return GraphicsResult::NotFound;
            if (!resource->native ||
                !HasAnyFlag(resource->desc.bindFlags, BufferBindFlags::Constant) ||
                resource->desc.byteSize % 16U != 0U)
                return GraphicsResult::InvalidArgument;
            nativeBuffers[index] = resource->native.Get();
        }
        if (!SetStageResources(context_, stage, firstSlot,
                static_cast<UINT>(bufferCount), nativeBuffers.data()))
            return GraphicsResult::Unsupported;
        return GraphicsResult::Success;
    }

    GraphicsResult D3D11Context::UnbindConstantBuffers(
        const ShaderStage stage,
        const std::uint32_t firstSlot,
        const std::size_t bufferCount) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (bufferCount == 0U) return GraphicsResult::Success;
        if (!IsSlotRangeValid(firstSlot, bufferCount, MaxConstantBufferSlots))
            return GraphicsResult::InvalidArgument;
        std::array<ID3D11Buffer*, MaxConstantBufferSlots> buffers{};
        return SetStageResources(context_, stage, firstSlot,
            static_cast<UINT>(bufferCount), buffers.data())
            ? GraphicsResult::Success : GraphicsResult::Unsupported;
    }

    GraphicsResult D3D11Context::UpdateBuffer(
        const BufferHandle buffer,
        const void* const data,
        const std::size_t dataSize) noexcept
    {
        if (!IsValid()) return GraphicsResult::InvalidState;
        if (!buffer.IsValid() || data == nullptr || dataSize == 0U)
            return GraphicsResult::InvalidArgument;
        const detail::D3D11BufferResource* const resource =
            resources_->GetBuffer(buffer);
        if (resource == nullptr) return GraphicsResult::NotFound;
        if (!resource->native || dataSize > resource->desc.byteSize ||
            dataSize != resource->desc.byteSize)
            return GraphicsResult::InvalidArgument;
        if (resource->desc.usage == ResourceUsage::Immutable)
            return GraphicsResult::InvalidArgument;
        if (resource->desc.usage == ResourceUsage::Staging)
            return GraphicsResult::Unsupported;
        if (resource->desc.usage == ResourceUsage::Dynamic)
        {
            if (!HasAnyFlag(resource->desc.cpuAccess, CpuAccessFlags::Write))
                return GraphicsResult::InvalidArgument;
            D3D11_MAPPED_SUBRESOURCE mapped{};
            const HRESULT result = context_->Map(resource->native.Get(), 0U,
                D3D11_MAP_WRITE_DISCARD, 0U, &mapped);
            if (FAILED(result)) return GraphicsResult::BackendFailure;
            std::memcpy(mapped.pData, data, dataSize);
            context_->Unmap(resource->native.Get(), 0U);
            return GraphicsResult::Success;
        }
        context_->UpdateSubresource(
            resource->native.Get(), 0U, nullptr, data, 0U, 0U);
        return GraphicsResult::Success;
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
