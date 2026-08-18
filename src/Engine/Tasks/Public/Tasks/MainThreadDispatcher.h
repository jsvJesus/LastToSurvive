#pragma once

#include <Platform/Synchronization.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>

namespace engine::tasks
{
    class MainThreadDispatcher final
    {
    public:
        using Callback =
            std::function<void()>;

        static constexpr std::size_t
            MaximumDispatchCount =
                static_cast<std::size_t>(-1);

        MainThreadDispatcher() noexcept = default;

        ~MainThreadDispatcher() noexcept;

        MainThreadDispatcher(
            const MainThreadDispatcher&) = delete;

        MainThreadDispatcher& operator=(
            const MainThreadDispatcher&) = delete;

        MainThreadDispatcher(
            MainThreadDispatcher&&) = delete;

        MainThreadDispatcher& operator=(
            MainThreadDispatcher&&) = delete;

        [[nodiscard]] bool Initialize() noexcept;

        void Shutdown() noexcept;

        [[nodiscard]] bool
            IsInitialized() const noexcept;

        [[nodiscard]] bool
            IsOwnerThread() const noexcept;

        /*
         * Возвращает false после Shutdown
         * или до Initialize.
         */
        [[nodiscard]] bool Post(
            Callback callback);

        /*
         * Выполняется только на owner thread.
         * Callback выполняются без удержания mutex.
         */
        [[nodiscard]] std::size_t Dispatch(
            std::size_t maximumCount =
                MaximumDispatchCount);

        [[nodiscard]] std::size_t
            GetPendingCount() const noexcept;

    private:
        mutable engine::platform::Mutex mutex_;

        std::deque<Callback> pendingCallbacks_;

        std::uint32_t ownerThreadId_ = 0;

        bool initialized_ = false;
        bool acceptingCallbacks_ = false;
    };
}