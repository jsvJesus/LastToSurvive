#include "Legacy/LoggingBridge.h"

#include "Core/Log.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <string_view>

extern bool r3dOutToLog(const char* format, ...);

namespace
{
    std::atomic_bool g_loggingBridgeInitialized = false;

    [[nodiscard]]
    const char* GetLevelName(
        const engine::core::LogLevel level) noexcept
    {
        using engine::core::LogLevel;

        switch (level)
        {
            case LogLevel::Trace:
                return "TRACE";

            case LogLevel::Debug:
                return "DEBUG";

            case LogLevel::Information:
                return "INFO";

            case LogLevel::Warning:
                return "WARNING";

            case LogLevel::Error:
                return "ERROR";

            case LogLevel::Critical:
                return "CRITICAL";
        }

        return "UNKNOWN";
    }

    [[nodiscard]]
    int GetPrintLength(
        const std::string_view value) noexcept
    {
        constexpr std::size_t maximumLength =
            static_cast<std::size_t>(
                std::numeric_limits<int>::max()
            );

        return static_cast<int>(
            std::min(value.size(), maximumLength)
        );
    }

    void WriteToLegacyLog(
        const engine::core::LogMessage& message,
        void*) noexcept
    {
        const std::string_view category =
            message.category.empty()
                ? std::string_view{"General"}
                : message.category;

        const std::string_view text = message.text;

        const char* const categoryData =
            category.empty()
                ? ""
                : category.data();

        const char* const textData =
            text.empty()
                ? ""
                : text.data();

        const bool alreadyHasNewLine =
            !text.empty() &&
            text.back() == '\n';

        r3dOutToLog(
            "[%.*s][%s] %.*s%s",
            GetPrintLength(category),
            categoryData,
            GetLevelName(message.level),
            GetPrintLength(text),
            textData,
            alreadyHasNewLine ? "" : "\n"
        );
    }
}

namespace engine::legacy
{
    void InitializeLoggingBridge() noexcept
    {
        bool expected = false;

        if (
            !g_loggingBridgeInitialized.compare_exchange_strong(
                expected,
                true,
                std::memory_order_acq_rel
            )
        )
        {
            return;
        }

        core::GetLogger().SetSink(
            &WriteToLegacyLog,
            nullptr
        );

        core::GetLogger().Write(
            core::LogLevel::Information,
            "Legacy",
            "Logging bridge initialized"
        );
    }

    void ShutdownLoggingBridge() noexcept
    {
        const bool wasInitialized =
            g_loggingBridgeInitialized.exchange(
                false,
                std::memory_order_acq_rel
            );

        if (!wasInitialized)
        {
            return;
        }

        core::GetLogger().Write(
            core::LogLevel::Information,
            "Legacy",
            "Logging bridge shutting down"
        );

        core::GetLogger().ClearSink();
    }

    bool IsLoggingBridgeInitialized() noexcept
    {
        return g_loggingBridgeInitialized.load(
            std::memory_order_acquire
        );
    }
}