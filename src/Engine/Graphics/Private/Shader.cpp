#include "Graphics/Shader.h"

namespace engine::graphics
{
    const char* ToString(
        const ShaderStage stage) noexcept
    {
        switch (stage)
        {
        case ShaderStage::Vertex:
            return "Vertex";
        case ShaderStage::Pixel:
            return "Pixel";
        case ShaderStage::Geometry:
            return "Geometry";
        case ShaderStage::Hull:
            return "Hull";
        case ShaderStage::Domain:
            return "Domain";
        case ShaderStage::Compute:
            return "Compute";
        case ShaderStage::Unknown:
        default:
            return "Unknown";
        }
    }
}
