#pragma once

#include "Graphics/GraphicsResult.h"
#include "Graphics/ResourceHandle.h"

struct ID3D11DeviceContext;

namespace engine::graphics::d3d11::detail
{
    class D3D11ResourceRegistry;
}

namespace engine::graphics::d3d11
{
    // Lightweight non-owning view over the immediate D3D11 context.
    // Lifetime is controlled by D3D11Device.
    class D3D11Context final
    {
    public:
        D3D11Context() noexcept = default;

        D3D11Context(const D3D11Context&) = delete;
        D3D11Context& operator=(const D3D11Context&) = delete;

        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] ID3D11DeviceContext* GetNativeContext() const noexcept;

        [[nodiscard]] GraphicsResult BindGraphicsPipeline(
            PipelineStateHandle pipeline) noexcept;

        void UnbindGraphicsPipeline() noexcept;
        void ClearState() noexcept;
        void Flush() noexcept;

    private:
        friend class D3D11Device;

        void Attach(
            ID3D11DeviceContext* context,
            detail::D3D11ResourceRegistry* resources) noexcept;

        void Detach() noexcept;

        ID3D11DeviceContext* context_ = nullptr;
        detail::D3D11ResourceRegistry* resources_ = nullptr;
    };
}
