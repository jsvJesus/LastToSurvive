#include "D3D9Conversions.h"

namespace engine::graphics::d3d9::detail
{
    GraphicsResult ConvertFormat(
        const Format format,
        D3DFORMAT& outFormat) noexcept
    {
        switch (format)
        {
        case Format::R8UNorm:
            outFormat = D3DFMT_L8;
            return GraphicsResult::Success;
        case Format::R8G8B8A8UNorm:
        case Format::R8G8B8A8UNormSrgb:
            outFormat = D3DFMT_A8B8G8R8;
            return GraphicsResult::Success;
        case Format::B8G8R8A8UNorm:
        case Format::B8G8R8A8UNormSrgb:
            outFormat = D3DFMT_A8R8G8B8;
            return GraphicsResult::Success;
        case Format::R16UNorm:
            outFormat = D3DFMT_L16;
            return GraphicsResult::Success;
        case Format::R16Float:
            outFormat = D3DFMT_R16F;
            return GraphicsResult::Success;
        case Format::R16G16Float:
            outFormat = D3DFMT_G16R16F;
            return GraphicsResult::Success;
        case Format::R16G16B16A16Float:
            outFormat = D3DFMT_A16B16G16R16F;
            return GraphicsResult::Success;
        case Format::R32Float:
            outFormat = D3DFMT_R32F;
            return GraphicsResult::Success;
        case Format::R32G32Float:
            outFormat = D3DFMT_G32R32F;
            return GraphicsResult::Success;
        case Format::R32G32B32A32Float:
            outFormat = D3DFMT_A32B32G32R32F;
            return GraphicsResult::Success;
        case Format::D16UNorm:
            outFormat = D3DFMT_D16;
            return GraphicsResult::Success;
        case Format::D24UNormS8UInt:
            outFormat = D3DFMT_D24S8;
            return GraphicsResult::Success;
        case Format::D32Float:
            outFormat = D3DFMT_D32F_LOCKABLE;
            return GraphicsResult::Success;
        case Format::BC1UNorm:
        case Format::BC1UNormSrgb:
            outFormat = D3DFMT_DXT1;
            return GraphicsResult::Success;
        case Format::BC2UNorm:
        case Format::BC2UNormSrgb:
            outFormat = D3DFMT_DXT3;
            return GraphicsResult::Success;
        case Format::BC3UNorm:
        case Format::BC3UNormSrgb:
            outFormat = D3DFMT_DXT5;
            return GraphicsResult::Success;
        case Format::BC4UNorm:
            outFormat = static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '1'));
            return GraphicsResult::Success;
        case Format::BC5UNorm:
            outFormat = static_cast<D3DFORMAT>(MAKEFOURCC('A', 'T', 'I', '2'));
            return GraphicsResult::Success;
        case Format::Unknown:
        case Format::R8G8UNorm:
        case Format::R32G32B32Float:
        case Format::BC7UNorm:
        case Format::BC7UNormSrgb:
        case Format::Count:
        default:
            outFormat = D3DFMT_UNKNOWN;
            return GraphicsResult::Unsupported;
        }
    }

    GraphicsResult ConvertTextureUsage(
        const TextureDesc& desc,
        DWORD& outUsage,
        D3DPOOL& outPool) noexcept
    {
        outUsage = 0;

        if (HasAnyFlag(desc.bindFlags, TextureBindFlags::UnorderedAccess) ||
            HasAnyFlag(desc.bindFlags, TextureBindFlags::DepthStencil) ||
            desc.sampleCount != 1 ||
            desc.generateMipmaps)
        {
            return GraphicsResult::Unsupported;
        }

        if (HasAnyFlag(desc.bindFlags, TextureBindFlags::RenderTarget))
        {
            outUsage |= D3DUSAGE_RENDERTARGET;
        }

        switch (desc.usage)
        {
        case ResourceUsage::Default:
        case ResourceUsage::Immutable:
            outPool = (outUsage & D3DUSAGE_RENDERTARGET) != 0
                ? D3DPOOL_DEFAULT
                : D3DPOOL_MANAGED;
            break;

        case ResourceUsage::Dynamic:
            if ((outUsage & D3DUSAGE_RENDERTARGET) != 0)
            {
                return GraphicsResult::Unsupported;
            }

            outUsage |= D3DUSAGE_DYNAMIC;
            outPool = D3DPOOL_DEFAULT;
            break;

        case ResourceUsage::Staging:
            outPool = D3DPOOL_SYSTEMMEM;
            break;

        default:
            return GraphicsResult::InvalidArgument;
        }

        return GraphicsResult::Success;
    }

    GraphicsResult ConvertBufferUsage(
        const BufferDesc& desc,
        DWORD& outUsage,
        D3DPOOL& outPool) noexcept
    {
        outUsage = 0;

        if (HasAnyFlag(desc.bindFlags, BufferBindFlags::Constant) ||
            HasAnyFlag(desc.bindFlags, BufferBindFlags::ShaderResource) ||
            HasAnyFlag(desc.bindFlags, BufferBindFlags::UnorderedAccess) ||
            HasAnyFlag(desc.bindFlags, BufferBindFlags::IndirectArguments) ||
            desc.miscFlags != BufferMiscFlags::None ||
            desc.usage == ResourceUsage::Staging)
        {
            return GraphicsResult::Unsupported;
        }

        switch (desc.usage)
        {
        case ResourceUsage::Default:
        case ResourceUsage::Immutable:
            outUsage = D3DUSAGE_WRITEONLY;
            outPool = D3DPOOL_MANAGED;
            break;

        case ResourceUsage::Dynamic:
            outUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
            outPool = D3DPOOL_DEFAULT;
            break;

        default:
            return GraphicsResult::Unsupported;
        }

        return GraphicsResult::Success;
    }

    bool IsDefaultPool(const D3DPOOL pool) noexcept
    {
        return pool == D3DPOOL_DEFAULT;
    }
}
