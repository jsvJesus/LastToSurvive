#pragma once

#include <cstdint>
#include <string>

namespace engine::platform
{
    struct WindowSize final
    {
        std::uint32_t width = 0;
        std::uint32_t height = 0;

        [[nodiscard]] constexpr bool IsEmpty() const noexcept
        {
            return width == 0 || height == 0;
        }
    };

    class NativeWindowHandle final
    {
    public:
        constexpr NativeWindowHandle() noexcept = default;

        [[nodiscard]] static constexpr NativeWindowHandle FromValue(
            const std::uintptr_t value) noexcept
        {
            return NativeWindowHandle(value);
        }

        [[nodiscard]] constexpr std::uintptr_t Value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value_ != 0;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return IsValid();
        }

    private:
        explicit constexpr NativeWindowHandle(
            const std::uintptr_t value) noexcept
            : value_(value)
        {
        }

        std::uintptr_t value_ = 0;
    };

    // На этапе миграции Window является non-owning представлением
    // существующего Win32-окна. Деструктор не уничтожает HWND.
    class Window final
    {
    public:
        Window() noexcept = default;

        explicit Window(
            NativeWindowHandle nativeHandle) noexcept;

        void Attach(
            NativeWindowHandle nativeHandle) noexcept;

        [[nodiscard]] NativeWindowHandle Detach() noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] NativeWindowHandle GetNativeHandle() const noexcept;

        [[nodiscard]] WindowSize GetClientSize() const noexcept;

        [[nodiscard]] bool IsVisible() const noexcept;

        [[nodiscard]] bool IsMinimized() const noexcept;

        [[nodiscard]] bool IsMaximized() const noexcept;

        [[nodiscard]] bool IsForeground() const noexcept;

        [[nodiscard]] std::wstring GetTitle() const;

        [[nodiscard]] bool SetTitle(
            const std::wstring& title) noexcept;

    private:
        NativeWindowHandle nativeHandle_;
    };
}