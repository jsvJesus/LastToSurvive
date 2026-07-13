#pragma once

#include <cstdint>

namespace engine::graphics
{
    enum class GraphicsBackend : std::uint8_t
    {
        None = 0,
        D3D9,
        D3D11
    };

    enum class DeviceState : std::uint8_t
    {
        Uninitialized = 0,
        Initializing,
        Ready,
        Lost,
        Recovering,
        Removed,
        Failed,
        Stopped
    };

    enum class PresentMode : std::uint8_t
    {
        Immediate = 0,
        VSync
    };

    [[nodiscard]] const char* ToString(
        GraphicsBackend backend) noexcept;

    [[nodiscard]] const char* ToString(
        DeviceState state) noexcept;

    [[nodiscard]] const char* ToString(
        PresentMode mode) noexcept;
}
