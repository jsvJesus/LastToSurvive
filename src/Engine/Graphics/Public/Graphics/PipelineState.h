#pragma once

#include "Graphics/ResourceHandle.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    inline constexpr std::size_t MaxColorRenderTargets = 8U;

    enum class PrimitiveTopology : std::uint8_t
    {
        Undefined = 0,
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip
    };

    enum class FillMode : std::uint8_t
    {
        Solid = 0,
        Wireframe
    };

    enum class CullMode : std::uint8_t
    {
        None = 0,
        Front,
        Back
    };

    enum class ComparisonFunction : std::uint8_t
    {
        Never = 0,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    enum class StencilOperation : std::uint8_t
    {
        Keep = 0,
        Zero,
        Replace,
        IncrementSaturate,
        DecrementSaturate,
        Invert,
        Increment,
        Decrement
    };

    enum class BlendFactor : std::uint8_t
    {
        Zero = 0,
        One,
        SourceColor,
        InverseSourceColor,
        SourceAlpha,
        InverseSourceAlpha,
        DestinationAlpha,
        InverseDestinationAlpha,
        DestinationColor,
        InverseDestinationColor,
        SourceAlphaSaturate,
        Constant,
        InverseConstant,
        Source1Color,
        InverseSource1Color,
        Source1Alpha,
        InverseSource1Alpha
    };

    enum class BlendOperation : std::uint8_t
    {
        Add = 0,
        Subtract,
        ReverseSubtract,
        Minimum,
        Maximum
    };

    enum class ColorWriteMask : std::uint8_t
    {
        None = 0,
        Red = 1U << 0U,
        Green = 1U << 1U,
        Blue = 1U << 2U,
        Alpha = 1U << 3U,
        All = 0x0FU
    };

    [[nodiscard]] constexpr ColorWriteMask operator|(
        const ColorWriteMask left,
        const ColorWriteMask right) noexcept
    {
        return static_cast<ColorWriteMask>(
            static_cast<std::uint8_t>(left) |
            static_cast<std::uint8_t>(right));
    }

    struct RasterizerDesc final
    {
        FillMode fillMode = FillMode::Solid;
        CullMode cullMode = CullMode::Back;
        bool frontCounterClockwise = false;
        std::int32_t depthBias = 0;
        float depthBiasClamp = 0.0F;
        float slopeScaledDepthBias = 0.0F;
        bool depthClipEnable = true;
        bool scissorEnable = false;
        bool multisampleEnable = false;
        bool antialiasedLineEnable = false;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct RenderTargetBlendDesc final
    {
        bool blendEnable = false;
        BlendFactor sourceColor = BlendFactor::One;
        BlendFactor destinationColor = BlendFactor::Zero;
        BlendOperation colorOperation = BlendOperation::Add;
        BlendFactor sourceAlpha = BlendFactor::One;
        BlendFactor destinationAlpha = BlendFactor::Zero;
        BlendOperation alphaOperation = BlendOperation::Add;
        ColorWriteMask writeMask = ColorWriteMask::All;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct BlendDesc final
    {
        bool alphaToCoverageEnable = false;
        bool independentBlendEnable = false;
        std::array<RenderTargetBlendDesc, MaxColorRenderTargets>
            renderTargets{};

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct DepthStencilOperationDesc final
    {
        StencilOperation stencilFail = StencilOperation::Keep;
        StencilOperation depthFail = StencilOperation::Keep;
        StencilOperation pass = StencilOperation::Keep;
        ComparisonFunction function = ComparisonFunction::Always;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct DepthStencilDesc final
    {
        bool depthEnable = true;
        bool depthWriteEnable = true;
        ComparisonFunction depthFunction =
            ComparisonFunction::LessEqual;
        bool stencilEnable = false;
        std::uint8_t stencilReadMask = 0xFFU;
        std::uint8_t stencilWriteMask = 0xFFU;
        DepthStencilOperationDesc frontFace;
        DepthStencilOperationDesc backFace;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct GraphicsPipelineDesc final
    {
        ShaderHandle vertexShader;
        ShaderHandle pixelShader;
        ShaderHandle geometryShader;
        ShaderHandle hullShader;
        ShaderHandle domainShader;
        InputLayoutHandle inputLayout;

        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        RasterizerDesc rasterizer;
        BlendDesc blend;
        DepthStencilDesc depthStencil;

        std::array<float, 4U> blendConstants{
            1.0F,
            1.0F,
            1.0F,
            1.0F};

        std::uint32_t sampleMask = 0xFFFFFFFFU;
        std::uint32_t stencilReference = 0U;
        const char* debugName = nullptr;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    [[nodiscard]] const char* ToString(
        PrimitiveTopology topology) noexcept;
}
