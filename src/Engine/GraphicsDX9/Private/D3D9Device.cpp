#include "GraphicsDX9/D3D9Device.h"

#include "D3D9ResourceRegistry.h"
#include "D3D9SwapChain.h"

#include <d3d9.h>

#include <new>
#include <utility>

namespace engine::graphics::d3d9
{
    class D3D9Device::Impl final
    {
    public:
        IDirect3DDevice9* device = nullptr;
        RenderDeviceDesc desc;
        DeviceState state = DeviceState::Uninitialized;
        detail::D3D9ResourceRegistry resources;
    };

    D3D9Device::D3D9Device()
        : impl_(new (std::nothrow) Impl())
    {
    }

    D3D9Device::~D3D9Device() noexcept
    {
        Shutdown();
    }

    GraphicsResult D3D9Device::AttachExternalDevice(
        IDirect3DDevice9* device) noexcept
    {
        if (!impl_ || device == nullptr)
        {
            return GraphicsResult::InvalidArgument;
        }

        if (impl_->state != DeviceState::Uninitialized &&
            impl_->state != DeviceState::Stopped)
        {
            return GraphicsResult::InvalidState;
        }

        impl_->device = device;
        return GraphicsResult::Success;
    }

    IDirect3DDevice9* D3D9Device::DetachExternalDevice() noexcept
    {
        if (!impl_ ||
            (impl_->state != DeviceState::Uninitialized &&
             impl_->state != DeviceState::Stopped))
        {
            return nullptr;
        }

        IDirect3DDevice9* device = impl_->device;
        impl_->device = nullptr;
        return device;
    }

    bool D3D9Device::IsExternalDeviceAttached() const noexcept
    {
        return impl_ && impl_->device != nullptr;
    }

    IDirect3DDevice9* D3D9Device::GetNativeDevice() const noexcept
    {
        return impl_ ? impl_->device : nullptr;
    }

    IDirect3DBaseTexture9* D3D9Device::GetNativeTexture(
        const TextureHandle texture) const noexcept
    {
        return impl_ ? impl_->resources.GetTexture(texture) : nullptr;
    }

    IDirect3DVertexBuffer9* D3D9Device::GetNativeVertexBuffer(
        const BufferHandle buffer) const noexcept
    {
        return impl_ ? impl_->resources.GetVertexBuffer(buffer) : nullptr;
    }

    IDirect3DIndexBuffer9* D3D9Device::GetNativeIndexBuffer(
        const BufferHandle buffer) const noexcept
    {
        return impl_ ? impl_->resources.GetIndexBuffer(buffer) : nullptr;
    }

    std::size_t D3D9Device::GetTextureCount() const noexcept
    {
        return impl_ ? impl_->resources.GetTextureCount() : 0;
    }

    std::size_t D3D9Device::GetBufferCount() const noexcept
    {
        return impl_ ? impl_->resources.GetBufferCount() : 0;
    }

    void D3D9Device::OnDeviceLost() noexcept
    {
        if (!impl_ ||
            (impl_->state != DeviceState::Ready &&
             impl_->state != DeviceState::Recovering))
        {
            return;
        }

        impl_->resources.OnDeviceLost();
        impl_->state = DeviceState::Lost;
    }

    GraphicsResult D3D9Device::OnDeviceReset(
        IDirect3DDevice9* device) noexcept
    {
        if (!impl_ ||
            (impl_->state != DeviceState::Lost &&
             impl_->state != DeviceState::Recovering))
        {
            return GraphicsResult::InvalidState;
        }

        if (device != nullptr)
        {
            impl_->device = device;
        }

        if (impl_->device == nullptr)
        {
            impl_->state = DeviceState::Failed;
            return GraphicsResult::InvalidState;
        }

        impl_->state = DeviceState::Recovering;
        const GraphicsResult result = impl_->resources.OnDeviceReset(
            impl_->device);
        impl_->state = Succeeded(result)
            ? DeviceState::Ready
            : DeviceState::Lost;
        return result;
    }

    GraphicsBackend D3D9Device::GetBackend() const noexcept
    {
        return GraphicsBackend::D3D9;
    }

    DeviceState D3D9Device::GetState() const noexcept
    {
        return impl_ ? impl_->state : DeviceState::Failed;
    }

    GraphicsResult D3D9Device::Initialize(
        const RenderDeviceDesc& desc) noexcept
    {
        if (!impl_)
        {
            return GraphicsResult::OutOfMemory;
        }

        if (!desc.IsValid() || desc.backend != GraphicsBackend::D3D9)
        {
            return GraphicsResult::InvalidArgument;
        }

        if (impl_->state != DeviceState::Uninitialized &&
            impl_->state != DeviceState::Stopped)
        {
            return GraphicsResult::InvalidState;
        }

        if (impl_->device == nullptr)
        {
            return GraphicsResult::InvalidState;
        }

        impl_->state = DeviceState::Initializing;
        impl_->desc = desc;

        const HRESULT cooperativeResult = impl_->device->TestCooperativeLevel();
        if (cooperativeResult == D3DERR_DEVICELOST ||
            cooperativeResult == D3DERR_DEVICENOTRESET)
        {
            impl_->state = DeviceState::Lost;
            return GraphicsResult::DeviceLost;
        }

        if (FAILED(cooperativeResult))
        {
            impl_->state = DeviceState::Failed;
            return GraphicsResult::BackendFailure;
        }

        impl_->state = DeviceState::Ready;
        return GraphicsResult::Success;
    }

    void D3D9Device::Shutdown() noexcept
    {
        if (!impl_)
        {
            return;
        }

        impl_->resources.Clear();
        impl_->device = nullptr;
        impl_->desc = RenderDeviceDesc{};
        impl_->state = DeviceState::Stopped;
    }

    GraphicsResult D3D9Device::CreateSwapChain(
        const SwapChainDesc& desc,
        std::unique_ptr<SwapChain>& outSwapChain) noexcept
    {
        outSwapChain.reset();

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        return detail::D3D9SwapChain::Create(
            impl_->device,
            desc,
            outSwapChain);
    }

    GraphicsResult D3D9Device::CreateTexture(
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        const std::size_t initialDataCount,
        TextureHandle& outTexture) noexcept
    {
        outTexture = TextureHandle{};

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.CreateTexture(
            impl_->device,
            desc,
            initialData,
            initialDataCount,
            outTexture);
    }

    GraphicsResult D3D9Device::DestroyTexture(
        const TextureHandle texture) noexcept
    {
        if (!impl_ ||
            (impl_->state != DeviceState::Ready &&
             impl_->state != DeviceState::Lost &&
             impl_->state != DeviceState::Recovering))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyTexture(texture);
    }

    GraphicsResult D3D9Device::CreateBuffer(
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        BufferHandle& outBuffer) noexcept
    {
        outBuffer = BufferHandle{};

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.CreateBuffer(
            impl_->device,
            desc,
            initialData,
            outBuffer);
    }

    GraphicsResult D3D9Device::DestroyBuffer(
        const BufferHandle buffer) noexcept
    {
        if (!impl_ ||
            (impl_->state != DeviceState::Ready &&
             impl_->state != DeviceState::Lost &&
             impl_->state != DeviceState::Recovering))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyBuffer(buffer);
    }
}
