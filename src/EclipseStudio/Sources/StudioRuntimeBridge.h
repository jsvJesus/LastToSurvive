#pragma once

namespace engine::runtime
{
    class Engine;
    enum class RendererBackend : unsigned char;
}

namespace studio
{
    [[nodiscard]] bool
        InitializeStudioRuntimeBridge(
            engine::runtime::RendererBackend backend) noexcept;

    void ShutdownStudioRuntimeBridge() noexcept;

    [[nodiscard]] bool
        IsStudioRuntimeBridgeInitialized() noexcept;

    [[nodiscard]] engine::runtime::Engine*
        TryGetRuntimeEngine() noexcept;
}
