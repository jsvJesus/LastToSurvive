#pragma once

#include <cstdint>

namespace engine::runtime
{
    enum class RendererBackend : std::uint8_t
    {
        None = 0,
        D3D11
    };

    [[nodiscard]] const char* ToString(
        RendererBackend backend) noexcept;
}
