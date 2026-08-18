#include "GraphicsDX11/D3D11SwapChain.h"

#include "D3D11ComPtr.h"
#include "D3D11Conversions.h"

#include <d3d11.h>
#include <dxgi1_5.h>
#include <windows.h>

#include <new>

namespace engine::graphics::d3d11
{
    class D3D11SwapChain::Impl final
    {
    public:
        detail::ComPtr<ID3D11Device> device;
        detail::ComPtr<IDXGISwapChain1> swapChain;
        detail::ComPtr<ID3D11Texture2D> backBuffer;
        detail::ComPtr<ID3D11RenderTargetView> renderTargetView;
        SwapChainDesc desc;
        SwapChainHandle handle = SwapChainHandle::FromParts(1, 1);
        bool tearingSupported = false;
    };

    D3D11SwapChain::D3D11SwapChain()
        : impl_(new (std::nothrow) Impl())
    {
    }

    D3D11SwapChain::~D3D11SwapChain() noexcept = default;

    GraphicsResult D3D11SwapChain::Create(
        ID3D11Device* device,
        IDXGIFactory2* factory,
        const bool tearingSupported,
        const SwapChainDesc& desc,
        std::unique_ptr<SwapChain>& outSwapChain) noexcept
    {
        outSwapChain.reset();
        if (device == nullptr || factory == nullptr || !desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }
        if (desc.bufferCount < 2)
        {
            return GraphicsResult::Unsupported;
        }
        if (desc.enableTearing && !tearingSupported)
        {
            return GraphicsResult::Unsupported;
        }

        try
        {
            std::unique_ptr<D3D11SwapChain> swapChain(
                new (std::nothrow) D3D11SwapChain());
            if (!swapChain || !swapChain->impl_)
            {
                return GraphicsResult::OutOfMemory;
            }

            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            GraphicsResult conversionResult = detail::ConvertFormat(
                desc.format,
                detail::TextureViewKind::Resource,
                false,
                format);
            if (Failed(conversionResult))
            {
                return conversionResult;
            }

            DXGI_SWAP_CHAIN_DESC1 nativeDesc{};
            nativeDesc.Width = desc.width;
            nativeDesc.Height = desc.height;
            nativeDesc.Format = format;
            nativeDesc.Stereo = FALSE;
            nativeDesc.SampleDesc.Count = 1;
            nativeDesc.SampleDesc.Quality = 0;
            nativeDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            nativeDesc.BufferCount = desc.bufferCount;
            nativeDesc.Scaling = DXGI_SCALING_STRETCH;
            nativeDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            nativeDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            nativeDesc.Flags = desc.enableTearing
                ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
                : 0U;

            const HWND window = reinterpret_cast<HWND>(desc.window.Value());
            const HRESULT result = factory->CreateSwapChainForHwnd(
                device,
                window,
                &nativeDesc,
                nullptr,
                nullptr,
                swapChain->impl_->swapChain.Put());
            if (FAILED(result))
            {
                return detail::ConvertFailure(result);
            }

            factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
            swapChain->impl_->device.CopyFrom(device);
            swapChain->impl_->desc = desc;
            swapChain->impl_->tearingSupported = tearingSupported;

            conversionResult = swapChain->RecreateBackBuffer();
            if (Failed(conversionResult))
            {
                return conversionResult;
            }

            outSwapChain = std::move(swapChain);
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
    }

    GraphicsResult D3D11SwapChain::RecreateBackBuffer() noexcept
    {
        if (!impl_ || !impl_->device || !impl_->swapChain)
        {
            return GraphicsResult::InvalidState;
        }

        HRESULT result = impl_->swapChain.Get()->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            reinterpret_cast<void**>(impl_->backBuffer.Put()));
        if (FAILED(result))
        {
            return detail::ConvertFailure(result);
        }

        result = impl_->device.Get()->CreateRenderTargetView(
            impl_->backBuffer.Get(),
            nullptr,
            impl_->renderTargetView.Put());
        if (FAILED(result))
        {
            impl_->backBuffer.Reset();
            return detail::ConvertFailure(result);
        }
        return GraphicsResult::Success;
    }

    GraphicsBackend D3D11SwapChain::GetBackend() const noexcept
    {
        return GraphicsBackend::D3D11;
    }

    SwapChainHandle D3D11SwapChain::GetHandle() const noexcept
    {
        return impl_ ? impl_->handle : SwapChainHandle{};
    }

    const SwapChainDesc& D3D11SwapChain::GetDesc() const noexcept
    {
        static const SwapChainDesc empty{};
        return impl_ ? impl_->desc : empty;
    }

    GraphicsResult D3D11SwapChain::Resize(
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (!impl_ || !impl_->swapChain || width == 0 || height == 0)
        {
            return GraphicsResult::InvalidArgument;
        }

        impl_->renderTargetView.Reset();
        impl_->backBuffer.Reset();

        const UINT flags = impl_->desc.enableTearing
            ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
            : 0U;
        const HRESULT result = impl_->swapChain.Get()->ResizeBuffers(
            0,
            width,
            height,
            DXGI_FORMAT_UNKNOWN,
            flags);
        if (FAILED(result))
        {
            return detail::ConvertFailure(result);
        }

        impl_->desc.width = width;
        impl_->desc.height = height;
        return RecreateBackBuffer();
    }

    GraphicsResult D3D11SwapChain::Present(
        PresentStatus& outStatus) noexcept
    {
        outStatus = PresentStatus::Failed;
        if (!impl_ || !impl_->swapChain)
        {
            return GraphicsResult::InvalidState;
        }

        const UINT syncInterval =
            impl_->desc.presentMode == PresentMode::VSync ? 1U : 0U;
        const UINT flags =
            impl_->desc.enableTearing && syncInterval == 0U
                ? DXGI_PRESENT_ALLOW_TEARING
                : 0U;
        const HRESULT result = impl_->swapChain.Get()->Present(
            syncInterval,
            flags);

        if (result == DXGI_STATUS_OCCLUDED)
        {
            outStatus = PresentStatus::Occluded;
            return GraphicsResult::Success;
        }
        if (result == DXGI_ERROR_DEVICE_RESET)
        {
            outStatus = PresentStatus::DeviceLost;
            return GraphicsResult::DeviceLost;
        }
        if (detail::IsDeviceRemovedResult(result))
        {
            outStatus = PresentStatus::DeviceRemoved;
            return GraphicsResult::DeviceRemoved;
        }
        if (FAILED(result))
        {
            return detail::ConvertFailure(result);
        }

        outStatus = PresentStatus::Presented;
        return GraphicsResult::Success;
    }

    IDXGISwapChain1* D3D11SwapChain::GetNativeSwapChain() const noexcept
    {
        return impl_ ? impl_->swapChain.Get() : nullptr;
    }

    ID3D11Texture2D* D3D11SwapChain::GetBackBufferTexture() const noexcept
    {
        return impl_ ? impl_->backBuffer.Get() : nullptr;
    }

    ID3D11RenderTargetView*
    D3D11SwapChain::GetBackBufferRenderTargetView() const noexcept
    {
        return impl_ ? impl_->renderTargetView.Get() : nullptr;
    }
}
