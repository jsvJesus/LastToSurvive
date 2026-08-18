#pragma once

#include <cstdint>
#include <mutex>
#include <string_view>

namespace engine::core
{
    enum class LogLevel : std::uint8_t
    {
        Trace,
        Debug,
        Information,
        Warning,
        Error,
        Critical
    };

    struct LogMessage final
    {
        LogLevel level = LogLevel::Information;
        std::string_view category;
        std::string_view text;
    };

    using LogSink = void (*)(
        const LogMessage& message,
        void* userData) noexcept;

    class Logger final
    {
    public:
        Logger() noexcept = default;
        ~Logger() = default;

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        void SetSink(
            LogSink sink,
            void* userData = nullptr) noexcept;

        void ClearSink() noexcept;

        void Write(
            LogLevel level,
            std::string_view category,
            std::string_view text) noexcept;

        [[nodiscard]]
        bool HasSink() const noexcept;

    private:
        mutable std::mutex mutex_;
        LogSink sink_ = nullptr;
        void* userData_ = nullptr;
    };
}