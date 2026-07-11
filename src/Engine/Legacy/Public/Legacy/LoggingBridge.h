#pragma once

namespace engine::legacy
{
    void InitializeLoggingBridge() noexcept;
    void ShutdownLoggingBridge() noexcept;

    [[nodiscard]]
    bool IsLoggingBridgeInitialized() noexcept;
}