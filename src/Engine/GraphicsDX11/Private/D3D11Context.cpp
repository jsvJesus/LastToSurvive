#include "GraphicsDX11/D3D11Context.h"

#include "D3D11ResourceRegistry.h"
#include "D3D11ResourceTypes.h"

#include <d3d11.h>

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

        context_->VSSetShader(
            resource->vertexShader.Get(),
            nullptr,
            0U);
        context_->PSSetShader(
            resource->pixelShader.Get(),
            nullptr,
            0U);
        context_->GSSetShader(
            resource->geometryShader.Get(),
            nullptr,
            0U);
        context_->HSSetShader(
            resource->hullShader.Get(),
            nullptr,
            0U);
        context_->DSSetShader(
            resource->domainShader.Get(),
            nullptr,
            0U);

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
