#include "GraphicsDX11/D3D11Context.h"

#include <d3d11.h>

namespace engine::graphics::d3d11
{
    bool D3D11Context::IsValid() const noexcept
    {
        return context_ != nullptr;
    }

    ID3D11DeviceContext* D3D11Context::GetNativeContext() const noexcept
    {
        return context_;
    }

    void D3D11Context::ClearState() noexcept
    {
        if (context_ != nullptr)
            context_->ClearState();
    }

    void D3D11Context::Flush() noexcept
    {
        if (context_ != nullptr)
            context_->Flush();
    }

    void D3D11Context::Attach(ID3D11DeviceContext* context) noexcept
    {
        context_ = context;
    }

    void D3D11Context::Detach() noexcept
    {
        context_ = nullptr;
    }
}
