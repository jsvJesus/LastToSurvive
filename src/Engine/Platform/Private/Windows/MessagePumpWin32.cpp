#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/MessagePump.h"

namespace engine::platform
{
    bool MessagePump::HasPendingMessages() noexcept
    {
        MSG message{};

        return ::PeekMessageW(
            &message,
            nullptr,
            0,
            0,
            PM_NOREMOVE) != FALSE;
    }

    MessagePumpResult MessagePump::ProcessPendingMessages(
        const std::size_t maximumMessageCount) noexcept
    {
        MessagePumpResult result{};

        if (maximumMessageCount == 0)
        {
            return result;
        }

        MSG message{};

        while (
            result.processedMessageCount <
                maximumMessageCount &&
            ::PeekMessageW(
                &message,
                nullptr,
                0,
                0,
                PM_REMOVE) != FALSE)
        {
            ++result.processedMessageCount;

            if (message.message == WM_QUIT)
            {
                result.quitRequested = true;
                result.exitCode =
                    static_cast<int>(
                        message.wParam);

                break;
            }

            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        return result;
    }

    bool MessagePump::WaitForMessage() noexcept
    {
        return ::WaitMessage() != FALSE;
    }
}