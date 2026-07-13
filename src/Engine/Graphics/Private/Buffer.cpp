#include "Graphics/Buffer.h"

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

    bool BufferDesc::IsValid() const noexcept
    {
        if (byteSize == 0 ||
            !IsCpuAccessValid(usage, cpuAccess))
        {
            return false;
        }

        if (usage == ResourceUsage::Staging)
        {
            if (bindFlags != BufferBindFlags::None ||
                miscFlags != BufferMiscFlags::None ||
                indexFormat != IndexFormat::None)
            {
                return false;
            }

            return true;
        }

        if (bindFlags == BufferBindFlags::None)
        {
            return false;
        }

        const bool isVertex = HasAnyFlag(
            bindFlags,
            BufferBindFlags::Vertex);
        const bool isIndex = HasAnyFlag(
            bindFlags,
            BufferBindFlags::Index);
        const bool isConstant = HasAnyFlag(
            bindFlags,
            BufferBindFlags::Constant);
        const bool isStructured = HasAnyFlag(
            miscFlags,
            BufferMiscFlags::Structured);
        const bool isRaw = HasAnyFlag(
            miscFlags,
            BufferMiscFlags::Raw);

        if (isStructured && isRaw)
        {
            return false;
        }

        if ((isVertex || isStructured) && stride == 0)
        {
            return false;
        }

        if (isStructured &&
            (byteSize % static_cast<std::size_t>(stride)) != 0)
        {
            return false;
        }

        if (isRaw && (byteSize % 4U) != 0)
        {
            return false;
        }

        if (isConstant && (byteSize % 16U) != 0)
        {
            return false;
        }

        if (isIndex)
        {
            const std::uint32_t expectedStride =
                indexFormat == IndexFormat::UInt16 ? 2U :
                indexFormat == IndexFormat::UInt32 ? 4U : 0U;

            if (expectedStride == 0 || stride != expectedStride)
            {
                return false;
            }
        }
        else if (indexFormat != IndexFormat::None)
        {
            return false;
        }

        return true;
    }

    bool BufferInitialData::IsValid() const noexcept
    {
        return data != nullptr && dataSize != 0;
    }
}
