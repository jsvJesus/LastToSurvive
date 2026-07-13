#include "Graphics/PipelineState.h"

#include <cmath>

namespace engine::graphics
{
    namespace
    {
        [[nodiscard]] bool IsValidFillMode(
            const FillMode value) noexcept
        {
            return
                value == FillMode::Solid ||
                value == FillMode::Wireframe;
        }

        [[nodiscard]] bool IsValidCullMode(
            const CullMode value) noexcept
        {
            return
                value == CullMode::None ||
                value == CullMode::Front ||
                value == CullMode::Back;
        }

        [[nodiscard]] bool IsValidComparison(
            const ComparisonFunction value) noexcept
        {
            switch (value)
            {
            case ComparisonFunction::Never:
            case ComparisonFunction::Less:
            case ComparisonFunction::Equal:
            case ComparisonFunction::LessEqual:
            case ComparisonFunction::Greater:
            case ComparisonFunction::NotEqual:
            case ComparisonFunction::GreaterEqual:
            case ComparisonFunction::Always:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool IsValidStencilOperation(
            const StencilOperation value) noexcept
        {
            switch (value)
            {
            case StencilOperation::Keep:
            case StencilOperation::Zero:
            case StencilOperation::Replace:
            case StencilOperation::IncrementSaturate:
            case StencilOperation::DecrementSaturate:
            case StencilOperation::Invert:
            case StencilOperation::Increment:
            case StencilOperation::Decrement:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool IsValidBlendFactor(
            const BlendFactor value) noexcept
        {
            switch (value)
            {
            case BlendFactor::Zero:
            case BlendFactor::One:
            case BlendFactor::SourceColor:
            case BlendFactor::InverseSourceColor:
            case BlendFactor::SourceAlpha:
            case BlendFactor::InverseSourceAlpha:
            case BlendFactor::DestinationAlpha:
            case BlendFactor::InverseDestinationAlpha:
            case BlendFactor::DestinationColor:
            case BlendFactor::InverseDestinationColor:
            case BlendFactor::SourceAlphaSaturate:
            case BlendFactor::Constant:
            case BlendFactor::InverseConstant:
            case BlendFactor::Source1Color:
            case BlendFactor::InverseSource1Color:
            case BlendFactor::Source1Alpha:
            case BlendFactor::InverseSource1Alpha:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool IsValidBlendOperation(
            const BlendOperation value) noexcept
        {
            switch (value)
            {
            case BlendOperation::Add:
            case BlendOperation::Subtract:
            case BlendOperation::ReverseSubtract:
            case BlendOperation::Minimum:
            case BlendOperation::Maximum:
                return true;
            default:
                return false;
            }
        }

        [[nodiscard]] bool IsValidTopology(
            const PrimitiveTopology value) noexcept
        {
            switch (value)
            {
            case PrimitiveTopology::PointList:
            case PrimitiveTopology::LineList:
            case PrimitiveTopology::LineStrip:
            case PrimitiveTopology::TriangleList:
            case PrimitiveTopology::TriangleStrip:
                return true;
            case PrimitiveTopology::Undefined:
            default:
                return false;
            }
        }
    }

    bool RasterizerDesc::IsValid() const noexcept
    {
        return
            IsValidFillMode(fillMode) &&
            IsValidCullMode(cullMode) &&
            std::isfinite(depthBiasClamp) &&
            std::isfinite(slopeScaledDepthBias);
    }

    bool RenderTargetBlendDesc::IsValid() const noexcept
    {
        const std::uint8_t mask =
            static_cast<std::uint8_t>(writeMask);

        return
            IsValidBlendFactor(sourceColor) &&
            IsValidBlendFactor(destinationColor) &&
            IsValidBlendOperation(colorOperation) &&
            IsValidBlendFactor(sourceAlpha) &&
            IsValidBlendFactor(destinationAlpha) &&
            IsValidBlendOperation(alphaOperation) &&
            (mask & 0xF0U) == 0U;
    }

    bool BlendDesc::IsValid() const noexcept
    {
        for (const RenderTargetBlendDesc& target : renderTargets)
        {
            if (!target.IsValid())
            {
                return false;
            }
        }

        return true;
    }

    bool DepthStencilOperationDesc::IsValid() const noexcept
    {
        return
            IsValidStencilOperation(stencilFail) &&
            IsValidStencilOperation(depthFail) &&
            IsValidStencilOperation(pass) &&
            IsValidComparison(function);
    }

    bool DepthStencilDesc::IsValid() const noexcept
    {
        return
            IsValidComparison(depthFunction) &&
            frontFace.IsValid() &&
            backFace.IsValid();
    }

    bool GraphicsPipelineDesc::IsValid() const noexcept
    {
        if (
            !vertexShader.IsValid() ||
            !IsValidTopology(topology) ||
            !rasterizer.IsValid() ||
            !blend.IsValid() ||
            !depthStencil.IsValid() ||
            stencilReference > 0xFFU)
        {
            return false;
        }

        const bool hasHull = hullShader.IsValid();
        const bool hasDomain = domainShader.IsValid();

        if (hasHull != hasDomain)
        {
            return false;
        }

        for (const float value : blendConstants)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
        }

        return true;
    }

    const char* ToString(
        const PrimitiveTopology topology) noexcept
    {
        switch (topology)
        {
        case PrimitiveTopology::PointList:
            return "PointList";
        case PrimitiveTopology::LineList:
            return "LineList";
        case PrimitiveTopology::LineStrip:
            return "LineStrip";
        case PrimitiveTopology::TriangleList:
            return "TriangleList";
        case PrimitiveTopology::TriangleStrip:
            return "TriangleStrip";
        case PrimitiveTopology::Undefined:
        default:
            return "Undefined";
        }
    }
}
