#pragma once

#include <cstddef>
#include <limits>

namespace engine::platform
{
    struct MessagePumpResult final
    {
        std::size_t processedMessageCount = 0;
        bool quitRequested = false;
        int exitCode = 0;
    };

    class MessagePump final
    {
    public:
        [[nodiscard]] static bool HasPendingMessages() noexcept;

        [[nodiscard]] static MessagePumpResult ProcessPendingMessages(
            std::size_t maximumMessageCount =
                std::numeric_limits<std::size_t>::max()) noexcept;

        [[nodiscard]] static bool WaitForMessage() noexcept;
    };
}