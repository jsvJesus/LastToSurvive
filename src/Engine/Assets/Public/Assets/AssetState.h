#pragma once

#include <cstdint>

namespace engine::assets
{
    enum class AssetState : std::uint8_t
    {
        Unloaded = 0,
        Queued,
        Loading,
        Ready,
        Failed,
        Reloading,
        Unloading
    };

    [[nodiscard]] const char* ToString(
        AssetState state) noexcept;
}