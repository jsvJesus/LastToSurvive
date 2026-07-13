#pragma once

#include <cstddef>
#include <cstdint>

namespace engine::graphics
{
    enum class ShaderStage : std::uint8_t
    {
        Unknown = 0,
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute
    };

    struct ShaderBytecodeView final
    {
        const void* data = nullptr;
        std::size_t size = 0U;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return data != nullptr && size != 0U;
        }
    };

    struct ShaderDesc final
    {
        ShaderStage stage = ShaderStage::Unknown;
        ShaderBytecodeView bytecode;
        const char* debugName = nullptr;

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return
                stage != ShaderStage::Unknown &&
                bytecode.IsValid();
        }
    };

    [[nodiscard]] const char* ToString(
        ShaderStage stage) noexcept;
}
