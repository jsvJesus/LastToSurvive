#pragma once

namespace engine::platform
{
    class InputSystem;
}

namespace studio
{
    [[nodiscard]] bool InitializePlatformInputBridge() noexcept;

    void ShutdownPlatformInputBridge() noexcept;

    [[nodiscard]] bool IsPlatformInputBridgeInitialized() noexcept;

    [[nodiscard]] engine::platform::InputSystem&
        GetPlatformInputSystem() noexcept;
}