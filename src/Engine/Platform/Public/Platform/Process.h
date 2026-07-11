#pragma once

#include "Platform/Path.h"

#include <cstdint>
#include <optional>
#include <string>

namespace engine::platform
{
    enum class ProcessWindowMode : std::uint8_t
    {
        Normal,
        Hidden,
        NoWindow
    };

    enum class ProcessWaitResult : std::uint8_t
    {
        Completed,
        Timeout,
        Failed
    };

    struct ProcessStartInfo final
    {
        Path executablePath;
        std::wstring arguments;
        Path workingDirectory;
        ProcessWindowMode windowMode =
            ProcessWindowMode::Normal;
    };

    [[nodiscard]] std::uint32_t
        GetCurrentProcessId() noexcept;

    [[nodiscard]] Path
        GetCurrentProcessPath();

    [[nodiscard]] const char* ToString(
        ProcessWaitResult result) noexcept;

    class Process final
    {
    public:
        static constexpr std::uint32_t InfiniteWait =
            0xFFFFFFFFu;

        Process() noexcept = default;

        explicit Process(
            const ProcessStartInfo& startInfo);

        ~Process() noexcept;

        Process(const Process&) = delete;
        Process& operator=(const Process&) = delete;

        Process(Process&& other) noexcept;
        Process& operator=(Process&& other) noexcept;

        [[nodiscard]] bool Start(
            const ProcessStartInfo& startInfo);

        void Close() noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] std::uint32_t
            GetId() const noexcept;

        [[nodiscard]] bool
            IsRunning() const noexcept;

        [[nodiscard]] ProcessWaitResult Wait(
            std::uint32_t timeoutMilliseconds =
                InfiniteWait) noexcept;

        [[nodiscard]] std::optional<std::uint32_t>
            GetExitCode() const noexcept;

        [[nodiscard]] bool Terminate(
            std::uint32_t exitCode) noexcept;

        [[nodiscard]] std::uint32_t
            GetLastErrorCode() const noexcept;

    private:
        std::uintptr_t nativeHandle_ = 0;
        std::uint32_t processId_ = 0;
        mutable std::uint32_t lastErrorCode_ = 0;
    };
}