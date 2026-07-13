#include "Graphics/GraphicsResult.h"

namespace engine::graphics
{
    const char* ToString(
        const GraphicsResult result) noexcept
    {
        switch (result)
        {
        case GraphicsResult::Success:
            return "Success";
        case GraphicsResult::InvalidArgument:
            return "InvalidArgument";
        case GraphicsResult::InvalidState:
            return "InvalidState";
        case GraphicsResult::Unsupported:
            return "Unsupported";
        case GraphicsResult::OutOfMemory:
            return "OutOfMemory";
        case GraphicsResult::NotFound:
            return "NotFound";
        case GraphicsResult::DeviceLost:
            return "DeviceLost";
        case GraphicsResult::DeviceRemoved:
            return "DeviceRemoved";
        case GraphicsResult::BackendFailure:
            return "BackendFailure";
        default:
            return "Unknown";
        }
    }
}
