#include "Runtime/EngineMode.h"

namespace engine::runtime
{
    const char* ToString(
        const EngineMode mode) noexcept
    {
        switch (mode)
        {
        case EngineMode::Studio:
            return "studio";

        case EngineMode::Client:
            return "client";

        case EngineMode::Server:
            return "server";

        default:
            return "unknown";
        }
    }
}