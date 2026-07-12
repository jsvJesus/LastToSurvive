#pragma once

namespace engine::runtime
{
    class Engine;
}

namespace studio
{
    [[nodiscard]] bool
        InitializeStudioRuntimeBridge() noexcept;

    void ShutdownStudioRuntimeBridge() noexcept;

    [[nodiscard]] bool
        IsStudioRuntimeBridgeInitialized() noexcept;

    [[nodiscard]] engine::runtime::Engine*
        TryGetRuntimeEngine() noexcept;
}