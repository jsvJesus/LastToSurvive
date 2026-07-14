#pragma once

#include <cstdint>

namespace studio
{
    enum class StudioGraphicsShellResult : std::uint8_t
    {
        NotRequested = 0,
        Completed,
        InitializationFailed,
        RuntimeInitializationFailed,
        FrameFailed,
        DeviceLost,
        DeviceRemoved
    };

    [[nodiscard]] bool WantsDX11Shell() noexcept;

    [[nodiscard]] StudioGraphicsShellResult RunDX11Shell(
        std::uintptr_t nativeWindow) noexcept;

    [[nodiscard]] const char* ToString(
        StudioGraphicsShellResult result) noexcept;
}
