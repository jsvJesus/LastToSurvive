#include "Core/Logger.h"

#include <mutex>

namespace engine::core
{
    void Logger::SetSink(
        const LogSink sink,
        void* const userData) noexcept
    {
        const std::lock_guard<std::mutex> lock(mutex_);

        sink_ = sink;
        userData_ = userData;
    }

    void Logger::ClearSink() noexcept
    {
        const std::lock_guard<std::mutex> lock(mutex_);

        sink_ = nullptr;
        userData_ = nullptr;
    }

    void Logger::Write(
        const LogLevel level,
        const std::string_view category,
        const std::string_view text) noexcept
    {
        LogSink currentSink = nullptr;
        void* currentUserData = nullptr;

        {
            const std::lock_guard<std::mutex> lock(mutex_);

            currentSink = sink_;
            currentUserData = userData_;
        }

        if (currentSink == nullptr)
        {
            return;
        }

        const LogMessage message
        {
            level,
            category,
            text
        };

        currentSink(message, currentUserData);
    }

    bool Logger::HasSink() const noexcept
    {
        const std::lock_guard<std::mutex> lock(mutex_);

        return sink_ != nullptr;
    }
}