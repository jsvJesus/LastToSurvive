#pragma once

#include <cstdint>

namespace studio
{
    [[nodiscard]] bool WantsDX11Shell() noexcept;

    // Returns true when DX11 owned the Studio window until shutdown. A false
    // result leaves the caller free to start the unchanged DX9 path.
    [[nodiscard]] bool RunDX11Shell(
        std::uintptr_t nativeWindow) noexcept;
}
