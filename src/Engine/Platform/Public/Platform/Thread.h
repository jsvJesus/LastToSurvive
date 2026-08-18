#pragma once

#include <cstdint>

namespace engine::platform
{
    void SleepForMilliseconds(
        std::uint32_t milliseconds) noexcept;

    void YieldCurrentThread() noexcept;

    [[nodiscard]] std::uint32_t
        GetCurrentThreadId() noexcept;

    [[nodiscard]] bool SetCurrentThreadName(
        const wchar_t* name) noexcept;
}