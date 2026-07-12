#pragma once

#include <cstdint>
#include <functional>

namespace engine::tasks
{
    class JobSystem;
    class MainThreadDispatcher;
}

namespace studio
{
    using MainThreadCallback =
        std::function<void()>;

    [[nodiscard]] bool
        InitializeTasksRuntimeBridge() noexcept;

    void ShutdownTasksRuntimeBridge() noexcept;

    [[nodiscard]] bool
        IsTasksRuntimeBridgeInitialized() noexcept;

    [[nodiscard]] engine::tasks::JobSystem*
        TryGetJobSystem() noexcept;

    [[nodiscard]] engine::tasks::MainThreadDispatcher*
        TryGetMainThreadDispatcher() noexcept;

    [[nodiscard]] bool PostToMainThread(
        MainThreadCallback callback);

    [[nodiscard]] std::uint64_t
        GetOutstandingLegacyTaskCount() noexcept;
}