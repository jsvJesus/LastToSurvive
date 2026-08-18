#pragma once

#include <cstdint>

namespace engine::runtime
{
    struct FrameContext final
    {
        std::uint64_t frameIndex = 0;

        double deltaSeconds = 0.0;

        double elapsedSeconds = 0.0;
    };
}