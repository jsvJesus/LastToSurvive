#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Synchronization.h"

#include <climits>
#include <utility>

namespace engine::platform
{
    namespace
    {
        static_assert(
            sizeof(SRWLOCK) <= sizeof(std::uintptr_t),
            "Mutex native storage is too small for SRWLOCK.");

        static_assert(
            alignof(SRWLOCK) <= alignof(std::uintptr_t),
            "Mutex native storage has insufficient alignment.");

        [[nodiscard]] PSRWLOCK ToNativeMutex(
            std::uintptr_t& nativeState) noexcept
        {
            return reinterpret_cast<PSRWLOCK>(
                &nativeState);
        }

        [[nodiscard]] HANDLE ToNativeHandle(
            const std::uintptr_t nativeHandle) noexcept
        {
            return reinterpret_cast<HANDLE>(
                nativeHandle);
        }

        [[nodiscard]] std::uintptr_t FromNativeHandle(
            const HANDLE nativeHandle) noexcept
        {
            return reinterpret_cast<std::uintptr_t>(
                nativeHandle);
        }

        [[nodiscard]] WaitResult WaitForNativeHandle(
            const HANDLE nativeHandle,
            const std::uint32_t timeoutMilliseconds,
            std::uint32_t& errorCode) noexcept
        {
            if (nativeHandle == nullptr)
            {
                errorCode =
                    static_cast<std::uint32_t>(
                        ERROR_INVALID_HANDLE);

                return WaitResult::Failed;
            }

            const DWORD result =
                ::WaitForSingleObject(
                    nativeHandle,
                    static_cast<DWORD>(
                        timeoutMilliseconds));

            switch (result)
            {
                case WAIT_OBJECT_0:
                    errorCode = ERROR_SUCCESS;
                    return WaitResult::Success;

                case WAIT_TIMEOUT:
                    errorCode = ERROR_SUCCESS;
                    return WaitResult::Timeout;

                case WAIT_FAILED:
                    errorCode =
                        static_cast<std::uint32_t>(
                            ::GetLastError());

                    return WaitResult::Failed;

                default:
                    errorCode =
                        static_cast<std::uint32_t>(
                            ERROR_INVALID_FUNCTION);

                    return WaitResult::Failed;
            }
        }
    }

    const char* ToString(
        const WaitResult result) noexcept
    {
        switch (result)
        {
            case WaitResult::Success:
                return "success";

            case WaitResult::Timeout:
                return "timeout";

            case WaitResult::Failed:
                return "failed";

            default:
                return "unknown";
        }
    }

    Mutex::Mutex() noexcept
    {
        ::InitializeSRWLock(
            ToNativeMutex(nativeState_));
    }

    void Mutex::Lock() noexcept
    {
        ::AcquireSRWLockExclusive(
            ToNativeMutex(nativeState_));
    }

    bool Mutex::TryLock() noexcept
    {
        return ::TryAcquireSRWLockExclusive(
            ToNativeMutex(nativeState_)) != FALSE;
    }

    void Mutex::Unlock() noexcept
    {
        ::ReleaseSRWLockExclusive(
            ToNativeMutex(nativeState_));
    }

    MutexLockGuard::MutexLockGuard(
        Mutex& mutex) noexcept
        : mutex_(&mutex)
    {
        mutex_->Lock();
    }

    MutexLockGuard::~MutexLockGuard() noexcept
    {
        if (mutex_ != nullptr)
        {
            mutex_->Unlock();
        }
    }

    Event::Event(
        const EventResetMode resetMode,
        const bool initiallySignaled) noexcept
    {
        const BOOL manualReset =
            resetMode == EventResetMode::Manual
                ? TRUE
                : FALSE;

        const HANDLE eventHandle =
            ::CreateEventW(
                nullptr,
                manualReset,
                initiallySignaled ? TRUE : FALSE,
                nullptr);

        if (eventHandle == nullptr)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return;
        }

        nativeHandle_ =
            FromNativeHandle(eventHandle);

        lastErrorCode_ = ERROR_SUCCESS;
    }

    Event::~Event() noexcept
    {
        Close();
    }

    Event::Event(Event&& other) noexcept
        : nativeHandle_(
              std::exchange(
                  other.nativeHandle_,
                  0)),
          lastErrorCode_(
              std::exchange(
                  other.lastErrorCode_,
                  ERROR_SUCCESS))
    {
    }

    Event& Event::operator=(
        Event&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Close();

        nativeHandle_ =
            std::exchange(
                other.nativeHandle_,
                0);

        lastErrorCode_ =
            std::exchange(
                other.lastErrorCode_,
                ERROR_SUCCESS);

        return *this;
    }

    bool Event::IsValid() const noexcept
    {
        return nativeHandle_ != 0;
    }

    Event::operator bool() const noexcept
    {
        return IsValid();
    }

    bool Event::Signal() noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return false;
        }

        if (::SetEvent(
                ToNativeHandle(nativeHandle_)) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        lastErrorCode_ = ERROR_SUCCESS;
        return true;
    }

    bool Event::Reset() noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return false;
        }

        if (::ResetEvent(
                ToNativeHandle(nativeHandle_)) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        lastErrorCode_ = ERROR_SUCCESS;
        return true;
    }

    WaitResult Event::Wait(
        const std::uint32_t timeoutMilliseconds) const noexcept
    {
        return WaitForNativeHandle(
            ToNativeHandle(nativeHandle_),
            timeoutMilliseconds,
            lastErrorCode_);
    }

    void Event::Close() noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ = ERROR_SUCCESS;
            return;
        }

        const HANDLE eventHandle =
            ToNativeHandle(nativeHandle_);

        nativeHandle_ = 0;

        if (::CloseHandle(eventHandle) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return;
        }

        lastErrorCode_ = ERROR_SUCCESS;
    }

    std::uint32_t Event::GetLastErrorCode() const noexcept
    {
        return lastErrorCode_;
    }

    Semaphore::Semaphore(
        const std::uint32_t initialCount,
        const std::uint32_t maximumCount) noexcept
    {
        if (maximumCount == 0 ||
            initialCount > maximumCount)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return;
        }

        const HANDLE semaphoreHandle =
            ::CreateSemaphoreW(
                nullptr,
                static_cast<LONG>(
                    initialCount),
                static_cast<LONG>(
                    maximumCount),
                nullptr);

        if (semaphoreHandle == nullptr)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return;
        }

        nativeHandle_ =
            FromNativeHandle(semaphoreHandle);

        lastErrorCode_ = ERROR_SUCCESS;
    }

    Semaphore::~Semaphore() noexcept
    {
        Close();
    }

    Semaphore::Semaphore(
        Semaphore&& other) noexcept
        : nativeHandle_(
              std::exchange(
                  other.nativeHandle_,
                  0)),
          lastErrorCode_(
              std::exchange(
                  other.lastErrorCode_,
                  ERROR_SUCCESS))
    {
    }

    Semaphore& Semaphore::operator=(
        Semaphore&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Close();

        nativeHandle_ =
            std::exchange(
                other.nativeHandle_,
                0);

        lastErrorCode_ =
            std::exchange(
                other.lastErrorCode_,
                ERROR_SUCCESS);

        return *this;
    }

    bool Semaphore::IsValid() const noexcept
    {
        return nativeHandle_ != 0;
    }

    Semaphore::operator bool() const noexcept
    {
        return IsValid();
    }

    bool Semaphore::Release(
        const std::uint32_t count) noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return false;
        }

        if (count == 0 ||
            count >
                static_cast<std::uint32_t>(
                    LONG_MAX))
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return false;
        }

        if (::ReleaseSemaphore(
                ToNativeHandle(nativeHandle_),
                static_cast<LONG>(count),
                nullptr) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        lastErrorCode_ = ERROR_SUCCESS;
        return true;
    }

    WaitResult Semaphore::Wait(
        const std::uint32_t timeoutMilliseconds) const noexcept
    {
        return WaitForNativeHandle(
            ToNativeHandle(nativeHandle_),
            timeoutMilliseconds,
            lastErrorCode_);
    }

    void Semaphore::Close() noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ = ERROR_SUCCESS;
            return;
        }

        const HANDLE semaphoreHandle =
            ToNativeHandle(nativeHandle_);

        nativeHandle_ = 0;

        if (::CloseHandle(
                semaphoreHandle) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return;
        }

        lastErrorCode_ = ERROR_SUCCESS;
    }

    std::uint32_t Semaphore::GetLastErrorCode() const noexcept
    {
        return lastErrorCode_;
    }
}