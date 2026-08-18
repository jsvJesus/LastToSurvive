#include "D3D11Conversions.h"

#include <dxgi1_5.h>

namespace engine::graphics::d3d11::detail
{
    namespace
    {
        [[nodiscard]] DXGI_FORMAT ConvertColorFormat(
            const Format format) noexcept
        {
            switch (format)
            {
            case Format::R8UNorm: return DXGI_FORMAT_R8_UNORM;
            case Format::R8G8UNorm: return DXGI_FORMAT_R8G8_UNORM;
            case Format::R8G8B8A8UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
            case Format::R8G8B8A8UInt: return DXGI_FORMAT_R8G8B8A8_UINT;
            case Format::R8G8B8A8UNormSrgb: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            case Format::B8G8R8A8UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
            case Format::B8G8R8A8UNormSrgb: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            case Format::R16UNorm: return DXGI_FORMAT_R16_UNORM;
            case Format::R16Float: return DXGI_FORMAT_R16_FLOAT;
            case Format::R16G16Float: return DXGI_FORMAT_R16G16_FLOAT;
            case Format::R16G16B16A16Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case Format::R32Float: return DXGI_FORMAT_R32_FLOAT;
            case Format::R32G32Float: return DXGI_FORMAT_R32G32_FLOAT;
            case Format::R32G32B32Float: return DXGI_FORMAT_R32G32B32_FLOAT;
            case Format::R32G32B32A32Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case Format::BC1UNorm: return DXGI_FORMAT_BC1_UNORM;
            case Format::BC1UNormSrgb: return DXGI_FORMAT_BC1_UNORM_SRGB;
            case Format::BC2UNorm: return DXGI_FORMAT_BC2_UNORM;
            case Format::BC2UNormSrgb: return DXGI_FORMAT_BC2_UNORM_SRGB;
            case Format::BC3UNorm: return DXGI_FORMAT_BC3_UNORM;
            case Format::BC3UNormSrgb: return DXGI_FORMAT_BC3_UNORM_SRGB;
            case Format::BC4UNorm: return DXGI_FORMAT_BC4_UNORM;
            case Format::BC5UNorm: return DXGI_FORMAT_BC5_UNORM;
            case Format::BC7UNorm: return DXGI_FORMAT_BC7_UNORM;
            case Format::BC7UNormSrgb: return DXGI_FORMAT_BC7_UNORM_SRGB;
            default: return DXGI_FORMAT_UNKNOWN;
            }
        }
    }

    GraphicsResult ConvertFormat(
        const Format format,
        const TextureViewKind viewKind,
        const bool depthShaderResource,
        DXGI_FORMAT& outFormat) noexcept
    {
        outFormat = ConvertColorFormat(format);
        if (outFormat != DXGI_FORMAT_UNKNOWN)
        {
            return GraphicsResult::Success;
        }

        switch (format)
        {
        case Format::D16UNorm:
            if (viewKind == TextureViewKind::Resource && depthShaderResource)
                outFormat = DXGI_FORMAT_R16_TYPELESS;
            else if (viewKind == TextureViewKind::ShaderResource)
                outFormat = DXGI_FORMAT_R16_UNORM;
            else
                outFormat = DXGI_FORMAT_D16_UNORM;
            break;

        case Format::D24UNormS8UInt:
            if (viewKind == TextureViewKind::Resource && depthShaderResource)
                outFormat = DXGI_FORMAT_R24G8_TYPELESS;
            else if (viewKind == TextureViewKind::ShaderResource)
                outFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            else
                outFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            break;

        case Format::D32Float:
            if (viewKind == TextureViewKind::Resource && depthShaderResource)
                outFormat = DXGI_FORMAT_R32_TYPELESS;
            else if (viewKind == TextureViewKind::ShaderResource)
                outFormat = DXGI_FORMAT_R32_FLOAT;
            else
                outFormat = DXGI_FORMAT_D32_FLOAT;
            break;

        default:
            return GraphicsResult::Unsupported;
        }

        if (viewKind == TextureViewKind::RenderTarget ||
            viewKind == TextureViewKind::UnorderedAccess)
        {
            return GraphicsResult::Unsupported;
        }

        return GraphicsResult::Success;
    }

    GraphicsResult ConvertVertexFormat(
        const Format format,
        DXGI_FORMAT& outFormat) noexcept
    {
        switch (format)
        {
        case Format::R8UNorm:
            outFormat = DXGI_FORMAT_R8_UNORM;
            return GraphicsResult::Success;
        case Format::R8G8UNorm:
            outFormat = DXGI_FORMAT_R8G8_UNORM;
            return GraphicsResult::Success;
        case Format::R8G8B8A8UNorm:
            outFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            return GraphicsResult::Success;
        case Format::R8G8B8A8UInt:
            outFormat = DXGI_FORMAT_R8G8B8A8_UINT;
            return GraphicsResult::Success;
        case Format::R16UNorm:
            outFormat = DXGI_FORMAT_R16_UNORM;
            return GraphicsResult::Success;
        case Format::R16Float:
            outFormat = DXGI_FORMAT_R16_FLOAT;
            return GraphicsResult::Success;
        case Format::R16G16Float:
            outFormat = DXGI_FORMAT_R16G16_FLOAT;
            return GraphicsResult::Success;
        case Format::R16G16B16A16Float:
            outFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
            return GraphicsResult::Success;
        case Format::R32Float:
            outFormat = DXGI_FORMAT_R32_FLOAT;
            return GraphicsResult::Success;
        case Format::R32G32Float:
            outFormat = DXGI_FORMAT_R32G32_FLOAT;
            return GraphicsResult::Success;
        case Format::R32G32B32Float:
            outFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            return GraphicsResult::Success;
        case Format::R32G32B32A32Float:
            outFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
            return GraphicsResult::Success;
        default:
            outFormat = DXGI_FORMAT_UNKNOWN;
            return GraphicsResult::Unsupported;
        }
    }

    GraphicsResult ConvertTextureUsage(
        const TextureDesc& desc,
        D3D11_USAGE& outUsage,
        UINT& outBindFlags,
        UINT& outCpuAccessFlags,
        UINT& outMiscFlags) noexcept
    {
        outBindFlags = 0;
        outCpuAccessFlags = 0;
        outMiscFlags = 0;

        switch (desc.usage)
        {
        case ResourceUsage::Default: outUsage = D3D11_USAGE_DEFAULT; break;
        case ResourceUsage::Immutable: outUsage = D3D11_USAGE_IMMUTABLE; break;
        case ResourceUsage::Dynamic: outUsage = D3D11_USAGE_DYNAMIC; break;
        case ResourceUsage::Staging: outUsage = D3D11_USAGE_STAGING; break;
        default: return GraphicsResult::Unsupported;
        }

        if (HasAnyFlag(desc.bindFlags, TextureBindFlags::ShaderResource))
            outBindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if (HasAnyFlag(desc.bindFlags, TextureBindFlags::RenderTarget))
            outBindFlags |= D3D11_BIND_RENDER_TARGET;
        if (HasAnyFlag(desc.bindFlags, TextureBindFlags::DepthStencil))
            outBindFlags |= D3D11_BIND_DEPTH_STENCIL;
        if (HasAnyFlag(desc.bindFlags, TextureBindFlags::UnorderedAccess))
            outBindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        if (HasAnyFlag(desc.cpuAccess, CpuAccessFlags::Read))
            outCpuAccessFlags |= D3D11_CPU_ACCESS_READ;
        if (HasAnyFlag(desc.cpuAccess, CpuAccessFlags::Write))
            outCpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;

        if (desc.dimension == TextureDimension::TextureCube)
            outMiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;
        if (desc.generateMipmaps)
            outMiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

        if (desc.usage == ResourceUsage::Dynamic &&
            (outBindFlags & (D3D11_BIND_RENDER_TARGET |
                             D3D11_BIND_DEPTH_STENCIL |
                             D3D11_BIND_UNORDERED_ACCESS)) != 0)
        {
            return GraphicsResult::Unsupported;
        }

        if (desc.sampleCount > 1 &&
            (outBindFlags & D3D11_BIND_UNORDERED_ACCESS) != 0)
        {
            return GraphicsResult::Unsupported;
        }

        if (desc.generateMipmaps)
        {
            // Automatic generation is introduced with the command-context stage.
            return GraphicsResult::Unsupported;
        }

        return GraphicsResult::Success;
    }

    GraphicsResult ConvertBufferUsage(
        const BufferDesc& desc,
        D3D11_USAGE& outUsage,
        UINT& outBindFlags,
        UINT& outCpuAccessFlags,
        UINT& outMiscFlags) noexcept
    {
        outBindFlags = 0;
        outCpuAccessFlags = 0;
        outMiscFlags = 0;

        switch (desc.usage)
        {
        case ResourceUsage::Default: outUsage = D3D11_USAGE_DEFAULT; break;
        case ResourceUsage::Immutable: outUsage = D3D11_USAGE_IMMUTABLE; break;
        case ResourceUsage::Dynamic: outUsage = D3D11_USAGE_DYNAMIC; break;
        case ResourceUsage::Staging: outUsage = D3D11_USAGE_STAGING; break;
        default: return GraphicsResult::Unsupported;
        }

        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::Vertex))
            outBindFlags |= D3D11_BIND_VERTEX_BUFFER;
        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::Index))
            outBindFlags |= D3D11_BIND_INDEX_BUFFER;
        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::Constant))
            outBindFlags |= D3D11_BIND_CONSTANT_BUFFER;
        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::ShaderResource))
            outBindFlags |= D3D11_BIND_SHADER_RESOURCE;
        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::UnorderedAccess))
            outBindFlags |= D3D11_BIND_UNORDERED_ACCESS;

        if (HasAnyFlag(desc.cpuAccess, CpuAccessFlags::Read))
            outCpuAccessFlags |= D3D11_CPU_ACCESS_READ;
        if (HasAnyFlag(desc.cpuAccess, CpuAccessFlags::Write))
            outCpuAccessFlags |= D3D11_CPU_ACCESS_WRITE;

        if (HasAnyFlag(desc.miscFlags, BufferMiscFlags::Structured))
            outMiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        if (HasAnyFlag(desc.miscFlags, BufferMiscFlags::Raw))
            outMiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::IndirectArguments))
            outMiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

        const bool isConstant =
            HasAnyFlag(desc.bindFlags, BufferBindFlags::Constant);
        if (isConstant && outBindFlags != D3D11_BIND_CONSTANT_BUFFER)
        {
            return GraphicsResult::Unsupported;
        }

        const bool needsTypedView =
            HasAnyFlag(desc.bindFlags, BufferBindFlags::ShaderResource) ||
            HasAnyFlag(desc.bindFlags, BufferBindFlags::UnorderedAccess);
        const bool hasViewShape =
            HasAnyFlag(desc.miscFlags, BufferMiscFlags::Structured) ||
            HasAnyFlag(desc.miscFlags, BufferMiscFlags::Raw);
        if (needsTypedView && !hasViewShape)
        {
            return GraphicsResult::Unsupported;
        }

        return GraphicsResult::Success;
    }

    bool IsDeviceRemovedResult(const HRESULT result) noexcept
    {
        return result == DXGI_ERROR_DEVICE_REMOVED ||
               result == DXGI_ERROR_DEVICE_HUNG ||
               result == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
    }

    GraphicsResult ConvertFailure(const HRESULT result) noexcept
    {
        if (SUCCEEDED(result))
            return GraphicsResult::Success;
        if (result == E_INVALIDARG || result == DXGI_ERROR_INVALID_CALL)
            return GraphicsResult::InvalidArgument;
        if (result == E_OUTOFMEMORY)
            return GraphicsResult::OutOfMemory;
        if (result == DXGI_ERROR_DEVICE_RESET)
            return GraphicsResult::DeviceLost;
        if (IsDeviceRemovedResult(result))
            return GraphicsResult::DeviceRemoved;
        return GraphicsResult::BackendFailure;
    }
}
