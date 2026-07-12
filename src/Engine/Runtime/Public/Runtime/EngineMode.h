#pragma once

#include <cstdint>

namespace engine::runtime
{
    enum class EngineMode : std::uint8_t
    {
        Studio = 0,
        Client,
        Server
    };

    [[nodiscard]] const char* ToString(
        EngineMode mode) noexcept;
}