#pragma once

#include "Graphics/Format.h"
#include "Graphics/ResourceHandle.h"

#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    inline constexpr std::uint32_t AppendAlignedVertexElement =
        0xFFFFFFFFU;

    inline constexpr std::size_t MaxVertexInputElements = 32U;
    inline constexpr std::uint32_t MaxVertexInputSlots = 32U;

    enum class VertexInputRate : std::uint8_t
    {
        PerVertex = 0,
        PerInstance
    };

    struct VertexElementDesc final
    {
        const char* semanticName = nullptr;
        std::uint32_t semanticIndex = 0U;
        Format format = Format::Unknown;
        std::uint32_t inputSlot = 0U;
        std::uint32_t alignedByteOffset =
            AppendAlignedVertexElement;
        VertexInputRate inputRate =
            VertexInputRate::PerVertex;
        std::uint32_t instanceStepRate = 0U;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct InputLayoutDesc final
    {
        ShaderHandle vertexShader;
        const VertexElementDesc* elements = nullptr;
        std::size_t elementCount = 0U;
        const char* debugName = nullptr;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    [[nodiscard]] const char* ToString(
        VertexInputRate inputRate) noexcept;
}
