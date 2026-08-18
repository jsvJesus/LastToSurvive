#include "Assets/AssetState.h"

namespace engine::assets
{
    const char* ToString(
        const AssetState state) noexcept
    {
        switch (state)
        {
        case AssetState::Unloaded:
            return "Unloaded";

        case AssetState::Queued:
            return "Queued";

        case AssetState::Loading:
            return "Loading";

        case AssetState::Ready:
            return "Ready";

        case AssetState::Failed:
            return "Failed";

        case AssetState::Reloading:
            return "Reloading";

        case AssetState::Unloading:
            return "Unloading";

        default:
            return "Unknown";
        }
    }
}