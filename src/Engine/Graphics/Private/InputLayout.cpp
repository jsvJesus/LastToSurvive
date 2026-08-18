#include "Graphics/InputLayout.h"

namespace engine::graphics
{
    namespace
    {
        [[nodiscard]] bool IsSupportedVertexFormat(
            const Format format) noexcept
        {
            switch (format)
            {
            case Format::R8UNorm:
            case Format::R8G8UNorm:
            case Format::R8G8B8A8UNorm:

                /*
                 * uint4 BLENDINDICES для skeletal mesh.
                 */
            case Format::R8G8B8A8UInt:

            case Format::R16UNorm:
            case Format::R16Float:
            case Format::R16G16Float:
            case Format::R16G16B16A16Float:
            case Format::R32Float:
            case Format::R32G32Float:
            case Format::R32G32B32Float:
            case Format::R32G32B32A32Float:
                return true;

            case Format::Unknown:
            case Format::R8G8B8A8UNormSrgb:
            case Format::B8G8R8A8UNorm:
            case Format::B8G8R8A8UNormSrgb:
            case Format::D16UNorm:
            case Format::D24UNormS8UInt:
            case Format::D32Float:
            case Format::BC1UNorm:
            case Format::BC1UNormSrgb:
            case Format::BC2UNorm:
            case Format::BC2UNormSrgb:
            case Format::BC3UNorm:
            case Format::BC3UNormSrgb:
            case Format::BC4UNorm:
            case Format::BC5UNorm:
            case Format::BC7UNorm:
            case Format::BC7UNormSrgb:
            case Format::Count:
            default:
                return false;
            }
        }
    }

    bool VertexElementDesc::IsValid() const noexcept
    {
        if (
            semanticName == nullptr ||
            semanticName[0] == '\0' ||
            inputSlot >= MaxVertexInputSlots ||
            !IsSupportedVertexFormat(format))
        {
            return false;
        }

        switch (inputRate)
        {
        case VertexInputRate::PerVertex:
            return instanceStepRate == 0U;

        case VertexInputRate::PerInstance:
            return instanceStepRate != 0U;

        default:
            return false;
        }
    }

    bool InputLayoutDesc::IsValid() const noexcept
    {
        if (
            !vertexShader.IsValid() ||
            elements == nullptr ||
            elementCount == 0U ||
            elementCount > MaxVertexInputElements)
        {
            return false;
        }

        for (std::size_t index = 0U; index < elementCount; ++index)
        {
            if (!elements[index].IsValid())
            {
                return false;
            }
        }

        return true;
    }

    const char* ToString(
        const VertexInputRate inputRate) noexcept
    {
        switch (inputRate)
        {
        case VertexInputRate::PerVertex:
            return "PerVertex";
        case VertexInputRate::PerInstance:
            return "PerInstance";
        default:
            return "Unknown";
        }
    }
}
