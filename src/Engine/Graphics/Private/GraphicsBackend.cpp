#include "Graphics/GraphicsBackend.h"

namespace engine::graphics
{
    const char* ToString(
        const GraphicsBackend backend) noexcept
    {
        switch (backend)
        {
        case GraphicsBackend::None:
            return "None";
        case GraphicsBackend::D3D9:
            return "D3D9";
        case GraphicsBackend::D3D11:
            return "D3D11";
        default:
            return "Unknown";
        }
    }

    const char* ToString(
        const DeviceState state) noexcept
    {
        switch (state)
        {
        case DeviceState::Uninitialized:
            return "Uninitialized";
        case DeviceState::Initializing:
            return "Initializing";
        case DeviceState::Ready:
            return "Ready";
        case DeviceState::Lost:
            return "Lost";
        case DeviceState::Recovering:
            return "Recovering";
        case DeviceState::Removed:
            return "Removed";
        case DeviceState::Failed:
            return "Failed";
        case DeviceState::Stopped:
            return "Stopped";
        default:
            return "Unknown";
        }
    }

    const char* ToString(
        const PresentMode mode) noexcept
    {
        switch (mode)
        {
        case PresentMode::Immediate:
            return "Immediate";
        case PresentMode::VSync:
            return "VSync";
        default:
            return "Unknown";
        }
    }
}
