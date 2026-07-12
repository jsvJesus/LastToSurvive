#pragma once

#include <cstdint>

namespace engine::platform
{
    enum class WaitResult : std::uint8_t
    {
        Success,
        Timeout,
        Failed
    };

    [[nodiscard]] const char* ToString(
        WaitResult result) noexcept;

    class Mutex final
    {
    public:
        Mutex() noexcept;
        ~Mutex() noexcept = default;

        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;

        Mutex(Mutex&&) = delete;
        Mutex& operator=(Mutex&&) = delete;

        void Lock() noexcept;

        [[nodiscard]] bool TryLock() noexcept;

        void Unlock() noexcept;

    private:
        /*
         * На Win32 здесь хранится SRWLOCK.
         * Win32-тип не выходит в public API.
         */
        alignas(void*) std::uintptr_t nativeState_ = 0;
    };

    class MutexLockGuard final
    {
    public:
        explicit MutexLockGuard(
            Mutex& mutex) noexcept;

        ~MutexLockGuard() noexcept;

        MutexLockGuard(
            const MutexLockGuard&) = delete;

        MutexLockGuard& operator=(
            const MutexLockGuard&) = delete;

        MutexLockGuard(
            MutexLockGuard&&) = delete;

        MutexLockGuard& operator=(
            MutexLockGuard&&) = delete;

    private:
        Mutex* mutex_ = nullptr;
    };

    enum class EventResetMode : std::uint8_t
    {
        Automatic,
        Manual
    };

    class Event final
    {
    public:
        static constexpr std::uint32_t InfiniteWait =
            0xFFFFFFFFu;

        explicit Event(
            EventResetMode resetMode =
                EventResetMode::Automatic,
            bool initiallySignaled = false) noexcept;

        ~Event() noexcept;

        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;

        Event(Event&& other) noexcept;
        Event& operator=(Event&& other) noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] bool Signal() noexcept;

        [[nodiscard]] bool Reset() noexcept;

        [[nodiscard]] WaitResult Wait(
            std::uint32_t timeoutMilliseconds =
                InfiniteWait) const noexcept;

        void Close() noexcept;

        [[nodiscard]] std::uint32_t
            GetLastErrorCode() const noexcept;

    private:
        std::uintptr_t nativeHandle_ = 0;
        mutable std::uint32_t lastErrorCode_ = 0;
    };

    class Semaphore final
    {
    public:
        static constexpr std::uint32_t InfiniteWait =
            0xFFFFFFFFu;

        Semaphore(
            std::uint32_t initialCount,
            std::uint32_t maximumCount) noexcept;

        ~Semaphore() noexcept;

        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        Semaphore(Semaphore&& other) noexcept;
        Semaphore& operator=(Semaphore&& other) noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] explicit operator bool() const noexcept;

        [[nodiscard]] bool Release(
            std::uint32_t count = 1) noexcept;

        [[nodiscard]] WaitResult Wait(
            std::uint32_t timeoutMilliseconds =
                InfiniteWait) const noexcept;

        void Close() noexcept;

        [[nodiscard]] std::uint32_t
            GetLastErrorCode() const noexcept;

    private:
        std::uintptr_t nativeHandle_ = 0;
        mutable std::uint32_t lastErrorCode_ = 0;
    };
}