#include "GraphicsDX11/D3D11Device.h"

#include "D3D11ComPtr.h"
#include "D3D11Conversions.h"
#include "D3D11Debug.h"
#include "D3D11ResourceRegistry.h"
#include "D3D11Shader.h"
#include "GraphicsDX11/D3D11SwapChain.h"

#include <d3d11.h>
#include <dxgi1_5.h>

#include <array>
#include <new>

namespace engine::graphics::d3d11
{
    class D3D11Device::Impl final
    {
    public:
        detail::ComPtr<ID3D11Device> device;
        detail::ComPtr<ID3D11DeviceContext> immediateContext;
        detail::ComPtr<IDXGIFactory2> factory;
        D3D11Context contextView;
        detail::D3D11ResourceRegistry resources;
        RenderDeviceDesc desc;
        DeviceState state = DeviceState::Uninitialized;
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_9_1;
        bool tearingSupported = false;
        bool debugLayerEnabled = false;
    };

    namespace
    {
        [[nodiscard]] HRESULT CreateNativeDevice(
            const bool requestDebug,
            const bool forceSoftwareAdapter,
            ID3D11Device** outDevice,
            D3D_FEATURE_LEVEL* outFeatureLevel,
            ID3D11DeviceContext** outContext,
            bool& outDebugEnabled) noexcept
        {
            outDebugEnabled = false;

            const std::array<D3D_FEATURE_LEVEL, 4U> featureLevels = {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0};

            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            if (requestDebug)
            {
                flags |= D3D11_CREATE_DEVICE_DEBUG;
            }

            auto createForDriver = [
                &featureLevels,
                outDevice,
                outFeatureLevel,
                outContext](
                    const D3D_DRIVER_TYPE driver,
                    const UINT createFlags) noexcept
            {
                HRESULT result = D3D11CreateDevice(
                    nullptr,
                    driver,
                    nullptr,
                    createFlags,
                    featureLevels.data(),
                    static_cast<UINT>(featureLevels.size()),
                    D3D11_SDK_VERSION,
                    outDevice,
                    outFeatureLevel,
                    outContext);

                if (result == E_INVALIDARG)
                {
                    result = D3D11CreateDevice(
                        nullptr,
                        driver,
                        nullptr,
                        createFlags,
                        featureLevels.data() + 1,
                        static_cast<UINT>(featureLevels.size() - 1U),
                        D3D11_SDK_VERSION,
                        outDevice,
                        outFeatureLevel,
                        outContext);
                }

                return result;
            };

            const D3D_DRIVER_TYPE requestedDriver = forceSoftwareAdapter
                ? D3D_DRIVER_TYPE_WARP
                : D3D_DRIVER_TYPE_HARDWARE;
            HRESULT result = createForDriver(requestedDriver, flags);

            if (FAILED(result) && requestDebug)
            {
                flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                result = createForDriver(
                    requestedDriver,
                    flags);
            }

            if (FAILED(result) && requestedDriver != D3D_DRIVER_TYPE_WARP)
            {
                result = createForDriver(
                    D3D_DRIVER_TYPE_WARP,
                    flags);
            }

            outDebugEnabled =
                SUCCEEDED(result) &&
                (flags & D3D11_CREATE_DEVICE_DEBUG) != 0U;

            return result;
        }

        [[nodiscard]] GraphicsResult AcquireFactory(
            ID3D11Device* device,
            detail::ComPtr<IDXGIFactory2>& outFactory,
            bool& outTearingSupported) noexcept
        {
            detail::ComPtr<IDXGIDevice> dxgiDevice;
            HRESULT result = device->QueryInterface(
                __uuidof(IDXGIDevice),
                reinterpret_cast<void**>(dxgiDevice.Put()));

            if (FAILED(result))
            {
                return detail::ConvertFailure(result);
            }

            detail::ComPtr<IDXGIAdapter> adapter;
            result = dxgiDevice.Get()->GetAdapter(adapter.Put());

            if (FAILED(result))
            {
                return detail::ConvertFailure(result);
            }

            result = adapter.Get()->GetParent(
                __uuidof(IDXGIFactory2),
                reinterpret_cast<void**>(outFactory.Put()));

            if (FAILED(result))
            {
                return detail::ConvertFailure(result);
            }

            outTearingSupported = false;
            detail::ComPtr<IDXGIFactory5> factory5;
            result = outFactory.Get()->QueryInterface(
                __uuidof(IDXGIFactory5),
                reinterpret_cast<void**>(factory5.Put()));

            if (SUCCEEDED(result))
            {
                BOOL allowTearing = FALSE;
                result = factory5.Get()->CheckFeatureSupport(
                    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                    &allowTearing,
                    sizeof(allowTearing));

                outTearingSupported =
                    SUCCEEDED(result) &&
                    allowTearing == TRUE;
            }

            return GraphicsResult::Success;
        }

        [[nodiscard]] bool IsDestroyState(
            const DeviceState state) noexcept
        {
            return
                state == DeviceState::Ready ||
                state == DeviceState::Lost ||
                state == DeviceState::Removed;
        }
    }

    D3D11Device::D3D11Device()
        : impl_(new (std::nothrow) Impl())
    {
    }

    D3D11Device::~D3D11Device() noexcept
    {
        Shutdown();
    }

    GraphicsBackend D3D11Device::GetBackend() const noexcept
    {
        return GraphicsBackend::D3D11;
    }

    DeviceState D3D11Device::GetState() const noexcept
    {
        return impl_ ? impl_->state : DeviceState::Failed;
    }

    GraphicsResult D3D11Device::Initialize(
        const RenderDeviceDesc& desc) noexcept
    {
        if (!impl_)
        {
            return GraphicsResult::OutOfMemory;
        }

        if (!desc.IsValid() || desc.backend != GraphicsBackend::D3D11)
        {
            return GraphicsResult::InvalidArgument;
        }

        if (
            impl_->state != DeviceState::Uninitialized &&
            impl_->state != DeviceState::Stopped)
        {
            return GraphicsResult::InvalidState;
        }

        impl_->state = DeviceState::Initializing;
        impl_->desc = desc;

        const HRESULT result = CreateNativeDevice(
            desc.enableValidation,
            desc.forceSoftwareAdapter,
            impl_->device.Put(),
            &impl_->featureLevel,
            impl_->immediateContext.Put(),
            impl_->debugLayerEnabled);

        if (FAILED(result))
        {
            impl_->state = DeviceState::Failed;
            return detail::ConvertFailure(result);
        }

        const GraphicsResult factoryResult = AcquireFactory(
            impl_->device.Get(),
            impl_->factory,
            impl_->tearingSupported);

        if (Failed(factoryResult))
        {
            Shutdown();
            impl_->state = DeviceState::Failed;
            return factoryResult;
        }

        impl_->contextView.Attach(
            impl_->immediateContext.Get(),
            &impl_->resources);
        impl_->state = DeviceState::Ready;
        return GraphicsResult::Success;
    }

    void D3D11Device::Shutdown() noexcept
    {
        if (!impl_)
        {
            return;
        }

        impl_->contextView.ClearState();
        impl_->resources.Clear();
        impl_->contextView.Flush();
        impl_->contextView.Detach();
        impl_->immediateContext.Reset();
        impl_->factory.Reset();

        if (impl_->debugLayerEnabled)
        {
            detail::ReportLiveObjects(impl_->device.Get());
        }

        impl_->device.Reset();
        impl_->desc = RenderDeviceDesc{};
        impl_->featureLevel = D3D_FEATURE_LEVEL_9_1;
        impl_->tearingSupported = false;
        impl_->debugLayerEnabled = false;
        impl_->state = DeviceState::Stopped;
    }

    GraphicsResult D3D11Device::CreateSwapChain(
        const SwapChainDesc& desc,
        std::unique_ptr<SwapChain>& outSwapChain) noexcept
    {
        outSwapChain.reset();

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        return D3D11SwapChain::Create(
            impl_->device.Get(),
            impl_->factory.Get(),
            impl_->tearingSupported,
            desc,
            outSwapChain);
    }

    GraphicsResult D3D11Device::CreateTexture(
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

        const GraphicsResult result = impl_->resources.CreateTexture(
            impl_->device.Get(),
            desc,
            initialData,
            initialDataCount,
            outTexture);

        if (
            result == GraphicsResult::DeviceLost ||
            result == GraphicsResult::DeviceRemoved)
        {
            (void)CheckDeviceStatus();
        }

        return result;
    }

    GraphicsResult D3D11Device::DestroyTexture(
        const TextureHandle texture) noexcept
    {
        if (!impl_ || !IsDestroyState(impl_->state))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyTexture(texture);
    }

    GraphicsResult D3D11Device::CreateBuffer(
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        BufferHandle& outBuffer) noexcept
    {
        outBuffer = BufferHandle{};

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        const GraphicsResult result = impl_->resources.CreateBuffer(
            impl_->device.Get(),
            desc,
            initialData,
            outBuffer);

        if (
            result == GraphicsResult::DeviceLost ||
            result == GraphicsResult::DeviceRemoved)
        {
            (void)CheckDeviceStatus();
        }

        return result;
    }

    GraphicsResult D3D11Device::DestroyBuffer(
        const BufferHandle buffer) noexcept
    {
        if (!impl_ || !IsDestroyState(impl_->state))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyBuffer(buffer);
    }

    GraphicsResult D3D11Device::CreateSampler(
        const SamplerDesc& desc,
        SamplerHandle& outSampler) noexcept
    {
        outSampler = SamplerHandle{};
        if (!impl_ || !impl_->device || impl_->state != DeviceState::Ready)
            return GraphicsResult::InvalidState;
        return impl_->resources.CreateSampler(
            impl_->device.Get(), desc, outSampler);
    }

    GraphicsResult D3D11Device::DestroySampler(
        const SamplerHandle sampler) noexcept
    {
        if (!impl_ || impl_->state != DeviceState::Ready)
            return GraphicsResult::InvalidState;
        return impl_->resources.DestroySampler(sampler);
    }

    GraphicsResult D3D11Device::CreateShader(
        const ShaderDesc& desc,
        ShaderHandle& outShader) noexcept
    {
        outShader = ShaderHandle{};

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        const GraphicsResult result = impl_->resources.CreateShader(
            impl_->device.Get(),
            desc,
            outShader);

        if (
            result == GraphicsResult::DeviceLost ||
            result == GraphicsResult::DeviceRemoved)
        {
            (void)CheckDeviceStatus();
        }

        return result;
    }

    GraphicsResult D3D11Device::DestroyShader(
        const ShaderHandle shader) noexcept
    {
        if (!impl_ || !IsDestroyState(impl_->state))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyShader(shader);
    }

    GraphicsResult D3D11Device::CreateInputLayout(
        const InputLayoutDesc& desc,
        InputLayoutHandle& outInputLayout) noexcept
    {
        outInputLayout = InputLayoutHandle{};

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        const GraphicsResult result = impl_->resources.CreateInputLayout(
            impl_->device.Get(),
            desc,
            outInputLayout);

        if (
            result == GraphicsResult::DeviceLost ||
            result == GraphicsResult::DeviceRemoved)
        {
            (void)CheckDeviceStatus();
        }

        return result;
    }

    GraphicsResult D3D11Device::DestroyInputLayout(
        const InputLayoutHandle inputLayout) noexcept
    {
        if (!impl_ || !IsDestroyState(impl_->state))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyInputLayout(inputLayout);
    }

    GraphicsResult D3D11Device::CreateGraphicsPipeline(
        const GraphicsPipelineDesc& desc,
        PipelineStateHandle& outPipeline) noexcept
    {
        outPipeline = PipelineStateHandle{};

        if (!impl_ || impl_->state != DeviceState::Ready)
        {
            return GraphicsResult::InvalidState;
        }

        const GraphicsResult result =
            impl_->resources.CreateGraphicsPipeline(
                impl_->device.Get(),
                desc,
                outPipeline);

        if (
            result == GraphicsResult::DeviceLost ||
            result == GraphicsResult::DeviceRemoved)
        {
            (void)CheckDeviceStatus();
        }

        return result;
    }

    GraphicsResult D3D11Device::DestroyGraphicsPipeline(
        const PipelineStateHandle pipeline) noexcept
    {
        if (!impl_ || !IsDestroyState(impl_->state))
        {
            return GraphicsResult::InvalidState;
        }

        return impl_->resources.DestroyGraphicsPipeline(pipeline);
    }

    ID3D11Device* D3D11Device::GetNativeDevice() const noexcept
    {
        return impl_ ? impl_->device.Get() : nullptr;
    }

    ID3D11DeviceContext*
    D3D11Device::GetNativeImmediateContext() const noexcept
    {
        return impl_ ? impl_->immediateContext.Get() : nullptr;
    }

    IDXGIFactory2* D3D11Device::GetNativeFactory() const noexcept
    {
        return impl_ ? impl_->factory.Get() : nullptr;
    }

    D3D11Context* D3D11Device::GetImmediateContext() noexcept
    {
        return impl_ ? &impl_->contextView : nullptr;
    }

    const D3D11Context* D3D11Device::GetImmediateContext() const noexcept
    {
        return impl_ ? &impl_->contextView : nullptr;
    }

    std::uint32_t D3D11Device::GetFeatureLevel() const noexcept
    {
        return impl_
            ? static_cast<std::uint32_t>(impl_->featureLevel)
            : 0U;
    }

    bool D3D11Device::IsTearingSupported() const noexcept
    {
        return impl_ && impl_->tearingSupported;
    }

    bool D3D11Device::IsDebugLayerEnabled() const noexcept
    {
        return impl_ && impl_->debugLayerEnabled;
    }

    GraphicsResult D3D11Device::CheckDeviceStatus() noexcept
    {
        if (!impl_ || !impl_->device)
        {
            return GraphicsResult::InvalidState;
        }

        const HRESULT reason =
            impl_->device.Get()->GetDeviceRemovedReason();

        if (reason == S_OK)
        {
            return GraphicsResult::Success;
        }

        const GraphicsResult result = detail::ConvertFailure(reason);
        impl_->state = result == GraphicsResult::DeviceLost
            ? DeviceState::Lost
            : DeviceState::Removed;
        return result;
    }

    ID3D11Resource* D3D11Device::GetNativeTexture(
        const TextureHandle texture) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11TextureResource* const resource =
            impl_->resources.GetTexture(texture);
        return resource ? resource->native.Get() : nullptr;
    }

    ID3D11ShaderResourceView*
    D3D11Device::GetTextureShaderResourceView(
        const TextureHandle texture) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11TextureResource* const resource =
            impl_->resources.GetTexture(texture);
        return resource ? resource->shaderResourceView.Get() : nullptr;
    }

    ID3D11RenderTargetView* D3D11Device::GetTextureRenderTargetView(
        const TextureHandle texture) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11TextureResource* const resource =
            impl_->resources.GetTexture(texture);
        return resource ? resource->renderTargetView.Get() : nullptr;
    }

    ID3D11DepthStencilView* D3D11Device::GetTextureDepthStencilView(
        const TextureHandle texture) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11TextureResource* const resource =
            impl_->resources.GetTexture(texture);
        return resource ? resource->depthStencilView.Get() : nullptr;
    }

    ID3D11UnorderedAccessView*
    D3D11Device::GetTextureUnorderedAccessView(
        const TextureHandle texture) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11TextureResource* const resource =
            impl_->resources.GetTexture(texture);
        return resource ? resource->unorderedAccessView.Get() : nullptr;
    }

    ID3D11Buffer* D3D11Device::GetNativeBuffer(
        const BufferHandle buffer) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11BufferResource* const resource =
            impl_->resources.GetBuffer(buffer);
        return resource ? resource->native.Get() : nullptr;
    }

    ID3D11ShaderResourceView*
    D3D11Device::GetBufferShaderResourceView(
        const BufferHandle buffer) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11BufferResource* const resource =
            impl_->resources.GetBuffer(buffer);
        return resource ? resource->shaderResourceView.Get() : nullptr;
    }

    ID3D11UnorderedAccessView*
    D3D11Device::GetBufferUnorderedAccessView(
        const BufferHandle buffer) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11BufferResource* const resource =
            impl_->resources.GetBuffer(buffer);
        return resource ? resource->unorderedAccessView.Get() : nullptr;
    }

    ID3D11DeviceChild* D3D11Device::GetNativeShader(
        const ShaderHandle shader) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11ShaderResource* const resource =
            impl_->resources.GetShader(shader);
        return resource ? detail::GetNativeShader(*resource) : nullptr;
    }

    ID3D11InputLayout* D3D11Device::GetNativeInputLayout(
        const InputLayoutHandle inputLayout) const noexcept
    {
        if (!impl_)
        {
            return nullptr;
        }

        const detail::D3D11InputLayoutResource* const resource =
            impl_->resources.GetInputLayout(inputLayout);
        return resource ? resource->native.Get() : nullptr;
    }

    std::size_t D3D11Device::GetTextureCount() const noexcept
    {
        return impl_ ? impl_->resources.GetTextureCount() : 0U;
    }

    std::size_t D3D11Device::GetBufferCount() const noexcept
    {
        return impl_ ? impl_->resources.GetBufferCount() : 0U;
    }

    std::size_t D3D11Device::GetSamplerCount() const noexcept
    {
        return impl_ ? impl_->resources.GetSamplerCount() : 0U;
    }

    std::size_t D3D11Device::GetShaderCount() const noexcept
    {
        return impl_ ? impl_->resources.GetShaderCount() : 0U;
    }

    std::size_t D3D11Device::GetInputLayoutCount() const noexcept
    {
        return impl_ ? impl_->resources.GetInputLayoutCount() : 0U;
    }

    std::size_t D3D11Device::GetGraphicsPipelineCount() const noexcept
    {
        return impl_ ? impl_->resources.GetGraphicsPipelineCount() : 0U;
    }
}
