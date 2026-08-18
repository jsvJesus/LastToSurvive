#include "Graphics/Texture.h"

namespace engine::graphics
{
    namespace
    {
        [[nodiscard]] bool IsCpuAccessValid(
            const ResourceUsage usage,
            const CpuAccessFlags cpuAccess) noexcept
        {
            switch (usage)
            {
            case ResourceUsage::Default:
            case ResourceUsage::Immutable:
                return cpuAccess == CpuAccessFlags::None;

            case ResourceUsage::Dynamic:
                return cpuAccess == CpuAccessFlags::Write;

            case ResourceUsage::Staging:
                return cpuAccess != CpuAccessFlags::None;

            default:
                return false;
            }
        }
    }

    bool TextureDesc::IsValid() const noexcept
    {
        if (width == 0 ||
            height == 0 ||
            depth == 0 ||
            arrayLayers == 0 ||
            mipLevels == 0 ||
            sampleCount == 0 ||
            format == Format::Unknown ||
            format == Format::Count)
        {
            return false;
        }

        if (!IsCpuAccessValid(usage, cpuAccess))
        {
            return false;
        }

        if (usage == ResourceUsage::Staging &&
            bindFlags != TextureBindFlags::None)
        {
            return false;
        }

        switch (dimension)
        {
        case TextureDimension::Texture1D:
            if (height != 1 || depth != 1)
            {
                return false;
            }
            break;

        case TextureDimension::Texture2D:
            if (depth != 1)
            {
                return false;
            }
            break;

        case TextureDimension::Texture3D:
            if (arrayLayers != 1 || sampleCount != 1)
            {
                return false;
            }
            break;

        case TextureDimension::TextureCube:
            if (depth != 1 ||
                arrayLayers < 6 ||
                (arrayLayers % 6) != 0)
            {
                return false;
            }
            break;

        default:
            return false;
        }

        if (sampleCount > 1 &&
            (dimension != TextureDimension::Texture2D ||
                mipLevels != 1))
        {
            return false;
        }

        const bool depthFormat = IsDepthFormat(format);
        const bool depthBinding = HasAnyFlag(
            bindFlags,
            TextureBindFlags::DepthStencil);
        const bool colorTargetBinding = HasAnyFlag(
            bindFlags,
            TextureBindFlags::RenderTarget);

        if (depthFormat != depthBinding)
        {
            return false;
        }

        if (depthFormat && colorTargetBinding)
        {
            return false;
        }

        if (generateMipmaps)
        {
            if (mipLevels <= 1 ||
                !HasAnyFlag(
                    bindFlags,
                    TextureBindFlags::ShaderResource) ||
                !HasAnyFlag(
                    bindFlags,
                    TextureBindFlags::RenderTarget))
            {
                return false;
            }
        }

        return true;
    }

    bool TextureSubresourceData::IsValid() const noexcept
    {
        if (data == nullptr || dataSize == 0 || rowPitch == 0)
        {
            return false;
        }

        if (slicePitch != 0 && slicePitch < rowPitch)
        {
            return false;
        }

        return true;
    }
}
