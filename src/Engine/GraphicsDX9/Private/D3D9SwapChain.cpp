#include "D3D9SwapChain.h"

#include <d3d9.h>

#include <atomic>
#include <new>
#include <utility>

namespace engine::graphics::d3d9::detail
{
    namespace
    {
        std::atomic<std::uint32_t> nextSwapChainIndex{1};
    }

    GraphicsResult D3D9SwapChain::Create(
        IDirect3DDevice9* device,
        const SwapChainDesc& desc,
        std::unique_ptr<SwapChain>& outSwapChain) noexcept
    {
        outSwapChain.reset();

        if (device == nullptr || !desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const std::uint32_t index = nextSwapChainIndex.fetch_add(
            1,
            std::memory_order_relaxed);
        const SwapChainHandle handle = SwapChainHandle::FromParts(
            index == 0 ? 1U : index,
            1U);

        std::unique_ptr<D3D9SwapChain> swapChain(
            new (std::nothrow) D3D9SwapChain(device, desc, handle));
        if (!swapChain)
        {
            return GraphicsResult::OutOfMemory;
        }

        const GraphicsResult refreshResult = swapChain->RefreshNativeSwapChain();
        if (Failed(refreshResult))
        {
            return refreshResult;
        }

        outSwapChain = std::move(swapChain);
        return GraphicsResult::Success;
    }

    D3D9SwapChain::D3D9SwapChain(
        IDirect3DDevice9* device,
        SwapChainDesc desc,
        const SwapChainHandle handle) noexcept
        : device_(device),
          desc_(std::move(desc)),
          handle_(handle)
    {
    }

    GraphicsBackend D3D9SwapChain::GetBackend() const noexcept
    {
        return GraphicsBackend::D3D9;
    }

    SwapChainHandle D3D9SwapChain::GetHandle() const noexcept
    {
        return handle_;
    }

    const SwapChainDesc& D3D9SwapChain::GetDesc() const noexcept
    {
        return desc_;
    }

    GraphicsResult D3D9SwapChain::Resize(
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (width == desc_.width && height == desc_.height)
        {
            return GraphicsResult::Success;
        }

        // The implicit Studio swap chain is reset by the legacy renderer.
        return GraphicsResult::Unsupported;
    }

    GraphicsResult D3D9SwapChain::Present(
        PresentStatus& outStatus) noexcept
    {
        outStatus = PresentStatus::Failed;

        if (device_ == nullptr)
        {
            return GraphicsResult::InvalidState;
        }

        const HRESULT cooperativeResult = device_->TestCooperativeLevel();
        if (cooperativeResult == D3DERR_DEVICELOST ||
            cooperativeResult == D3DERR_DEVICENOTRESET)
        {
            outStatus = PresentStatus::DeviceLost;
            return GraphicsResult::DeviceLost;
        }

        if (FAILED(cooperativeResult))
        {
            return GraphicsResult::BackendFailure;
        }

        if (!native_)
        {
            const GraphicsResult refreshResult = RefreshNativeSwapChain();
            if (Failed(refreshResult))
            {
                return refreshResult;
            }
        }

        const HRESULT result = native_.Get()->Present(
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            0);

        if (result == D3DERR_DEVICELOST)
        {
            native_.Reset();
            outStatus = PresentStatus::DeviceLost;
            return GraphicsResult::DeviceLost;
        }

        if (FAILED(result))
        {
            outStatus = PresentStatus::Failed;
            return GraphicsResult::BackendFailure;
        }

        outStatus = PresentStatus::Presented;
        return GraphicsResult::Success;
    }

    GraphicsResult D3D9SwapChain::RefreshNativeSwapChain() noexcept
    {
        native_.Reset();

        if (device_ == nullptr)
        {
            return GraphicsResult::InvalidState;
        }

        IDirect3DSwapChain9* swapChain = nullptr;
        const HRESULT result = device_->GetSwapChain(0, &swapChain);
        native_.Attach(swapChain);

        if (FAILED(result) || !native_)
        {
            native_.Reset();
            return result == D3DERR_DEVICELOST
                ? GraphicsResult::DeviceLost
                : GraphicsResult::BackendFailure;
        }

        return GraphicsResult::Success;
    }
}
