#include "D3D11ResourceRegistry.h"

#include "D3D11Buffer.h"
#include "D3D11PipelineState.h"
#include "D3D11Shader.h"
#include "D3D11Texture.h"

#include <new>
#include <cstring>
#include <utility>

namespace engine::graphics::d3d11::detail
{
    namespace
    {
        [[nodiscard]] D3D11_FILTER ConvertFilter(
            const TextureFilter filter) noexcept
        {
            switch (filter)
            {
            case TextureFilter::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case TextureFilter::Linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            case TextureFilter::Anisotropic: return D3D11_FILTER_ANISOTROPIC;
            case TextureFilter::ComparisonPoint:
                return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            case TextureFilter::ComparisonLinear:
                return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            default: return D3D11_FILTER_MIN_MAG_MIP_POINT;
            }
        }

        [[nodiscard]] D3D11_TEXTURE_ADDRESS_MODE ConvertAddress(
            const TextureAddressMode address) noexcept
        {
            switch (address)
            {
            case TextureAddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
            case TextureAddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
            case TextureAddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
            case TextureAddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
            default: return D3D11_TEXTURE_ADDRESS_CLAMP;
            }
        }

        [[nodiscard]] D3D11_COMPARISON_FUNC ConvertComparison(
            const ComparisonFunction comparison) noexcept
        {
            return static_cast<D3D11_COMPARISON_FUNC>(
                static_cast<unsigned int>(comparison) + 1U);
        }
    }

    GraphicsResult D3D11ResourceRegistry::CreateTexture(
        ID3D11Device* device,
        const TextureDesc& desc,
        const TextureSubresourceData* initialData,
        const std::size_t initialDataCount,
        TextureHandle& outTexture) noexcept
    {
        outTexture = TextureHandle{};
        D3D11TextureResource resource;
        const GraphicsResult result = CreateTextureResource(
            device,
            desc,
            initialData,
            initialDataCount,
            resource);

        if (Failed(result))
        {
            return result;
        }

        try
        {
            outTexture = textures_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            return GraphicsResult::BackendFailure;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyTexture(
        const TextureHandle texture) noexcept
    {
        return textures_.Remove(texture)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D11ResourceRegistry::CreateBuffer(
        ID3D11Device* device,
        const BufferDesc& desc,
        const BufferInitialData* initialData,
        BufferHandle& outBuffer) noexcept
    {
        outBuffer = BufferHandle{};
        D3D11BufferResource resource;
        const GraphicsResult result = CreateBufferResource(
            device,
            desc,
            initialData,
            resource);

        if (Failed(result))
        {
            return result;
        }

        try
        {
            outBuffer = buffers_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            return GraphicsResult::BackendFailure;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyBuffer(
        const BufferHandle buffer) noexcept
    {
        return buffers_.Remove(buffer)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D11ResourceRegistry::CreateSampler(
        ID3D11Device* const device,
        const SamplerDesc& desc,
        SamplerHandle& outSampler) noexcept
    {
        outSampler = SamplerHandle{};
        if (device == nullptr || !desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11_SAMPLER_DESC nativeDesc{};
        nativeDesc.Filter = ConvertFilter(desc.filter);
        nativeDesc.AddressU = ConvertAddress(desc.addressU);
        nativeDesc.AddressV = ConvertAddress(desc.addressV);
        nativeDesc.AddressW = ConvertAddress(desc.addressW);
        nativeDesc.MipLODBias = desc.mipLodBias;
        nativeDesc.MaxAnisotropy = desc.maximumAnisotropy;
        nativeDesc.ComparisonFunc = ConvertComparison(desc.comparisonFunction);
        for (std::size_t index = 0U; index < desc.borderColor.size(); ++index)
            nativeDesc.BorderColor[index] = desc.borderColor[index];
        nativeDesc.MinLOD = desc.minimumLod;
        nativeDesc.MaxLOD = desc.maximumLod;

        D3D11SamplerResource resource;
        const HRESULT result = device->CreateSamplerState(
            &nativeDesc, resource.native.Put());
        if (FAILED(result))
        {
            return GraphicsResult::BackendFailure;
        }
        resource.desc = desc;
        if (desc.debugName != nullptr && *desc.debugName != '\0')
        {
            resource.native.Get()->SetPrivateData(
                WKPDID_D3DDebugObjectName,
                static_cast<UINT>(std::strlen(desc.debugName)),
                desc.debugName);
        }
        try
        {
            outSampler = samplers_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            return GraphicsResult::BackendFailure;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroySampler(
        const SamplerHandle sampler) noexcept
    {
        if (!sampler.IsValid())
            return GraphicsResult::InvalidArgument;
        return samplers_.Remove(sampler)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D11ResourceRegistry::CreateShader(
        ID3D11Device* device,
        const ShaderDesc& desc,
        ShaderHandle& outShader) noexcept
    {
        outShader = ShaderHandle{};
        D3D11ShaderResource resource;
        const GraphicsResult result = CreateShaderResource(
            device,
            desc,
            resource);

        if (Failed(result))
        {
            return result;
        }

        try
        {
            outShader = shaders_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            return GraphicsResult::BackendFailure;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyShader(
        const ShaderHandle shader) noexcept
    {
        return shaders_.Remove(shader)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D11ResourceRegistry::CreateInputLayout(
        ID3D11Device* device,
        const InputLayoutDesc& desc,
        InputLayoutHandle& outInputLayout) noexcept
    {
        outInputLayout = InputLayoutHandle{};

        if (!desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const D3D11ShaderResource* const vertexShader =
            shaders_.Get(desc.vertexShader);

        if (vertexShader == nullptr)
        {
            return GraphicsResult::NotFound;
        }

        if (vertexShader->stage != ShaderStage::Vertex)
        {
            return GraphicsResult::InvalidArgument;
        }

        D3D11InputLayoutResource resource;
        const GraphicsResult result = CreateInputLayoutResource(
            device,
            desc,
            *vertexShader,
            resource);

        if (Failed(result))
        {
            return result;
        }

        try
        {
            outInputLayout = inputLayouts_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            return GraphicsResult::BackendFailure;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyInputLayout(
        const InputLayoutHandle inputLayout) noexcept
    {
        return inputLayouts_.Remove(inputLayout)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    GraphicsResult D3D11ResourceRegistry::CreateGraphicsPipeline(
        ID3D11Device* device,
        const GraphicsPipelineDesc& desc,
        PipelineStateHandle& outPipeline) noexcept
    {
        outPipeline = PipelineStateHandle{};

        if (!desc.IsValid())
        {
            return GraphicsResult::InvalidArgument;
        }

        const D3D11ShaderResource* const vertexShader =
            shaders_.Get(desc.vertexShader);

        if (vertexShader == nullptr)
        {
            return GraphicsResult::NotFound;
        }

        const D3D11ShaderResource* pixelShader = nullptr;
        const D3D11ShaderResource* geometryShader = nullptr;
        const D3D11ShaderResource* hullShader = nullptr;
        const D3D11ShaderResource* domainShader = nullptr;
        const D3D11InputLayoutResource* inputLayout = nullptr;

        if (desc.pixelShader.IsValid())
        {
            pixelShader = shaders_.Get(desc.pixelShader);
            if (pixelShader == nullptr)
            {
                return GraphicsResult::NotFound;
            }
        }

        if (desc.geometryShader.IsValid())
        {
            geometryShader = shaders_.Get(desc.geometryShader);
            if (geometryShader == nullptr)
            {
                return GraphicsResult::NotFound;
            }
        }

        if (desc.hullShader.IsValid())
        {
            hullShader = shaders_.Get(desc.hullShader);
            if (hullShader == nullptr)
            {
                return GraphicsResult::NotFound;
            }
        }

        if (desc.domainShader.IsValid())
        {
            domainShader = shaders_.Get(desc.domainShader);
            if (domainShader == nullptr)
            {
                return GraphicsResult::NotFound;
            }
        }

        if (desc.inputLayout.IsValid())
        {
            inputLayout = inputLayouts_.Get(desc.inputLayout);
            if (inputLayout == nullptr)
            {
                return GraphicsResult::NotFound;
            }
        }

        D3D11GraphicsPipelineResource resource;
        const GraphicsResult result = CreateGraphicsPipelineResource(
            device,
            desc,
            *vertexShader,
            pixelShader,
            geometryShader,
            hullShader,
            domainShader,
            inputLayout,
            resource);

        if (Failed(result))
        {
            return result;
        }

        try
        {
            outPipeline = graphicsPipelines_.Insert(std::move(resource));
            return GraphicsResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            return GraphicsResult::BackendFailure;
        }
    }

    GraphicsResult D3D11ResourceRegistry::DestroyGraphicsPipeline(
        const PipelineStateHandle pipeline) noexcept
    {
        return graphicsPipelines_.Remove(pipeline)
            ? GraphicsResult::Success
            : GraphicsResult::NotFound;
    }

    void D3D11ResourceRegistry::Clear() noexcept
    {
        graphicsPipelines_.Clear();
        inputLayouts_.Clear();
        shaders_.Clear();
        samplers_.Clear();
        textures_.Clear();
        buffers_.Clear();
    }

    const D3D11TextureResource* D3D11ResourceRegistry::GetTexture(
        const TextureHandle texture) const noexcept
    {
        return textures_.Get(texture);
    }

    const D3D11BufferResource* D3D11ResourceRegistry::GetBuffer(
        const BufferHandle buffer) const noexcept
    {
        return buffers_.Get(buffer);
    }

    const D3D11SamplerResource* D3D11ResourceRegistry::GetSampler(
        const SamplerHandle sampler) const noexcept
    {
        return samplers_.Get(sampler);
    }

    const D3D11ShaderResource* D3D11ResourceRegistry::GetShader(
        const ShaderHandle shader) const noexcept
    {
        return shaders_.Get(shader);
    }

    const D3D11InputLayoutResource*
    D3D11ResourceRegistry::GetInputLayout(
        const InputLayoutHandle inputLayout) const noexcept
    {
        return inputLayouts_.Get(inputLayout);
    }

    const D3D11GraphicsPipelineResource*
    D3D11ResourceRegistry::GetGraphicsPipeline(
        const PipelineStateHandle pipeline) const noexcept
    {
        return graphicsPipelines_.Get(pipeline);
    }

    std::size_t D3D11ResourceRegistry::GetTextureCount() const noexcept
    {
        return textures_.Size();
    }

    std::size_t D3D11ResourceRegistry::GetBufferCount() const noexcept
    {
        return buffers_.Size();
    }

    std::size_t D3D11ResourceRegistry::GetSamplerCount() const noexcept
    {
        return samplers_.Size();
    }

    std::size_t D3D11ResourceRegistry::GetShaderCount() const noexcept
    {
        return shaders_.Size();
    }

    std::size_t D3D11ResourceRegistry::GetInputLayoutCount() const noexcept
    {
        return inputLayouts_.Size();
    }

    std::size_t
    D3D11ResourceRegistry::GetGraphicsPipelineCount() const noexcept
    {
        return graphicsPipelines_.Size();
    }
}
