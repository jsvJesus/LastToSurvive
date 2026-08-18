#pragma once

#include <cstdint>

namespace engine::graphics
{
    enum class GraphicsResult : std::uint8_t
    {
        Success = 0,
        InvalidArgument,
        InvalidState,
        Unsupported,
        OutOfMemory,
        NotFound,
        DeviceLost,
        DeviceRemoved,
        BackendFailure
    };

    [[nodiscard]] constexpr bool Succeeded(
        const GraphicsResult result) noexcept
    {
        return result == GraphicsResult::Success;
    }

    [[nodiscard]] constexpr bool Failed(
        const GraphicsResult result) noexcept
    {
        return !Succeeded(result);
    }

    [[nodiscard]] const char* ToString(
        GraphicsResult result) noexcept;
}
