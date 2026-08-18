#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Window.h"

#include <utility>

namespace engine::platform
{
    namespace
    {
        [[nodiscard]] HWND ToNativeWindow(
            const NativeWindowHandle handle) noexcept
        {
            return reinterpret_cast<HWND>(
                handle.Value());
        }
    }

    Window::Window(
        const NativeWindowHandle nativeHandle) noexcept
        : nativeHandle_(nativeHandle)
    {
    }

    void Window::Attach(
        const NativeWindowHandle nativeHandle) noexcept
    {
        nativeHandle_ = nativeHandle;
    }

    NativeWindowHandle Window::Detach() noexcept
    {
        return std::exchange(
            nativeHandle_,
            NativeWindowHandle{});
    }

    bool Window::IsValid() const noexcept
    {
        if (!nativeHandle_)
        {
            return false;
        }

        return ::IsWindow(
            ToNativeWindow(nativeHandle_)) != FALSE;
    }

    NativeWindowHandle Window::GetNativeHandle() const noexcept
    {
        return nativeHandle_;
    }

    WindowSize Window::GetClientSize() const noexcept
    {
        if (!IsValid())
        {
            return {};
        }

        RECT clientRectangle{};

        if (::GetClientRect(
                ToNativeWindow(nativeHandle_),
                &clientRectangle) == FALSE)
        {
            return {};
        }

        const LONG width =
            clientRectangle.right -
            clientRectangle.left;

        const LONG height =
            clientRectangle.bottom -
            clientRectangle.top;

        if (width <= 0 || height <= 0)
        {
            return {};
        }

        return
        {
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height)
        };
    }

    bool Window::IsVisible() const noexcept
    {
        return IsValid() &&
            ::IsWindowVisible(
                ToNativeWindow(nativeHandle_)) != FALSE;
    }

    bool Window::IsMinimized() const noexcept
    {
        return IsValid() &&
            ::IsIconic(
                ToNativeWindow(nativeHandle_)) != FALSE;
    }

    bool Window::IsMaximized() const noexcept
    {
        return IsValid() &&
            ::IsZoomed(
                ToNativeWindow(nativeHandle_)) != FALSE;
    }

    bool Window::IsForeground() const noexcept
    {
        return IsValid() &&
            ::GetForegroundWindow() ==
                ToNativeWindow(nativeHandle_);
    }

    std::wstring Window::GetTitle() const
    {
        if (!IsValid())
        {
            return {};
        }

        const HWND window =
            ToNativeWindow(nativeHandle_);

        const int titleLength =
            ::GetWindowTextLengthW(window);

        if (titleLength <= 0)
        {
            return {};
        }

        std::wstring title(
            static_cast<std::size_t>(titleLength),
            L'\0');

        const int copiedCharacters =
            ::GetWindowTextW(
                window,
                title.data(),
                titleLength + 1);

        if (copiedCharacters <= 0)
        {
            return {};
        }

        title.resize(
            static_cast<std::size_t>(
                copiedCharacters));

        return title;
    }

    bool Window::SetTitle(
        const std::wstring& title) noexcept
    {
        if (!IsValid())
        {
            return false;
        }

        return ::SetWindowTextW(
            ToNativeWindow(nativeHandle_),
            title.c_str()) != FALSE;
    }
}