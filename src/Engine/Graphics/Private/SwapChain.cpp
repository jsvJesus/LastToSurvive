#include "Graphics/SwapChain.h"

namespace engine::graphics
{
    bool SwapChainDesc::IsValid() const noexcept
    {
        if (!window.IsValid() ||
            width == 0 ||
            height == 0 ||
            bufferCount == 0 ||
            !IsColorFormat(format))
        {
            return false;
        }

        if (enableTearing &&
            presentMode != PresentMode::Immediate)
        {
            return false;
        }

        return true;
    }

    const char* ToString(
        const PresentStatus status) noexcept
    {
        switch (status)
        {
        case PresentStatus::Presented:
            return "Presented";
        case PresentStatus::Occluded:
            return "Occluded";
        case PresentStatus::DeviceLost:
            return "DeviceLost";
        case PresentStatus::DeviceRemoved:
            return "DeviceRemoved";
        case PresentStatus::Failed:
            return "Failed";
        default:
            return "Unknown";
        }
    }
}
