#pragma once

#include "Graphics/Format.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/ResourceHandle.h"

#include <Platform/Window.h>

#include <cstdint>

namespace engine::graphics
{
    enum class PresentStatus : std::uint8_t
    {
        Presented = 0,
        Occluded,
        DeviceLost,
        DeviceRemoved,
        Failed
    };

    struct SwapChainDesc final
    {
        engine::platform::NativeWindowHandle window;

        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint32_t bufferCount = 2;

        Format format = Format::B8G8R8A8UNorm;
        PresentMode presentMode = PresentMode::VSync;

        bool windowed = true;
        bool allowModeSwitch = false;
        bool enableTearing = false;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    class SwapChain
    {
    public:
        virtual ~SwapChain() noexcept = default;

        SwapChain(const SwapChain&) = delete;
        SwapChain& operator=(const SwapChain&) = delete;

        SwapChain(SwapChain&&) = delete;
        SwapChain& operator=(SwapChain&&) = delete;

        [[nodiscard]] virtual GraphicsBackend GetBackend() const noexcept = 0;

        [[nodiscard]] virtual SwapChainHandle GetHandle() const noexcept = 0;

        [[nodiscard]] virtual const SwapChainDesc& GetDesc() const noexcept = 0;

        [[nodiscard]] virtual GraphicsResult Resize(
            std::uint32_t width,
            std::uint32_t height) noexcept = 0;

        [[nodiscard]] virtual GraphicsResult Present(
            PresentStatus& outStatus) noexcept = 0;

    protected:
        SwapChain() noexcept = default;
    };

    [[nodiscard]] const char* ToString(
        PresentStatus status) noexcept;
}
