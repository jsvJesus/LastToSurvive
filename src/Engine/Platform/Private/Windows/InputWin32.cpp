#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Input.h"

#include <algorithm>

namespace engine::platform
{
    namespace
    {
        constexpr std::uintptr_t ExtendedKeyMask =
            std::uintptr_t{1} << 24;

        constexpr std::uintptr_t PreviousKeyStateMask =
            std::uintptr_t{1} << 30;

        [[nodiscard]] std::int32_t SignedLowWord(
            const std::intptr_t value) noexcept
        {
            return static_cast<std::int32_t>(
                static_cast<std::int16_t>(
                    static_cast<std::uint16_t>(
                        static_cast<std::uintptr_t>(value) &
                        0xFFFFu)));
        }

        [[nodiscard]] std::int32_t SignedHighWord(
            const std::intptr_t value) noexcept
        {
            return static_cast<std::int32_t>(
                static_cast<std::int16_t>(
                    static_cast<std::uint16_t>(
                        (static_cast<std::uintptr_t>(value) >>
                         16) &
                        0xFFFFu)));
        }

        [[nodiscard]] std::int32_t WheelDeltaFromWord(
            const std::uintptr_t value) noexcept
        {
            return static_cast<std::int32_t>(
                static_cast<std::int16_t>(
                    static_cast<std::uint16_t>(
                        (value >> 16) &
                        0xFFFFu)));
        }

        [[nodiscard]] KeyCode MapVirtualKeyToKeyCode(
            const std::uintptr_t wordParameter,
            const std::intptr_t longParameter) noexcept
        {
            const UINT virtualKey =
                static_cast<UINT>(
                    wordParameter &
                    0xFFFFu);

            const std::uintptr_t messageBits =
                static_cast<std::uintptr_t>(
                    longParameter);

            if (virtualKey >= '0' &&
                virtualKey <= '9')
            {
                const auto base =
                    static_cast<std::uint16_t>(
                        KeyCode::Digit0);

                return static_cast<KeyCode>(
                    base +
                    static_cast<std::uint16_t>(
                        virtualKey - '0'));
            }

            if (virtualKey >= 'A' &&
                virtualKey <= 'Z')
            {
                const auto base =
                    static_cast<std::uint16_t>(
                        KeyCode::A);

                return static_cast<KeyCode>(
                    base +
                    static_cast<std::uint16_t>(
                        virtualKey - 'A'));
            }

            if (virtualKey >= VK_F1 &&
                virtualKey <= VK_F12)
            {
                const auto base =
                    static_cast<std::uint16_t>(
                        KeyCode::F1);

                return static_cast<KeyCode>(
                    base +
                    static_cast<std::uint16_t>(
                        virtualKey - VK_F1));
            }

            if (virtualKey >= VK_NUMPAD0 &&
                virtualKey <= VK_NUMPAD9)
            {
                const auto base =
                    static_cast<std::uint16_t>(
                        KeyCode::Numpad0);

                return static_cast<KeyCode>(
                    base +
                    static_cast<std::uint16_t>(
                        virtualKey - VK_NUMPAD0));
            }

            switch (virtualKey)
            {
                case VK_ESCAPE:
                    return KeyCode::Escape;

                case VK_TAB:
                    return KeyCode::Tab;

                case VK_RETURN:
                    return
                        (messageBits &
                         ExtendedKeyMask) != 0
                            ? KeyCode::NumpadEnter
                            : KeyCode::Enter;

                case VK_BACK:
                    return KeyCode::Backspace;

                case VK_SPACE:
                    return KeyCode::Space;

                case VK_SHIFT:
                {
                    const UINT scanCode =
                        static_cast<UINT>(
                            (messageBits >> 16) &
                            0xFFu);

                    const UINT mappedKey =
                        ::MapVirtualKeyW(
                            scanCode,
                            MAPVK_VSC_TO_VK_EX);

                    return mappedKey == VK_RSHIFT
                        ? KeyCode::RightShift
                        : KeyCode::LeftShift;
                }

                case VK_LSHIFT:
                    return KeyCode::LeftShift;

                case VK_RSHIFT:
                    return KeyCode::RightShift;

                case VK_CONTROL:
                    return
                        (messageBits &
                         ExtendedKeyMask) != 0
                            ? KeyCode::RightControl
                            : KeyCode::LeftControl;

                case VK_LCONTROL:
                    return KeyCode::LeftControl;

                case VK_RCONTROL:
                    return KeyCode::RightControl;

                case VK_MENU:
                    return
                        (messageBits &
                         ExtendedKeyMask) != 0
                            ? KeyCode::RightAlt
                            : KeyCode::LeftAlt;

                case VK_LMENU:
                    return KeyCode::LeftAlt;

                case VK_RMENU:
                    return KeyCode::RightAlt;

                case VK_LWIN:
                    return KeyCode::LeftSuper;

                case VK_RWIN:
                    return KeyCode::RightSuper;

                case VK_CAPITAL:
                    return KeyCode::CapsLock;

                case VK_NUMLOCK:
                    return KeyCode::NumLock;

                case VK_SCROLL:
                    return KeyCode::ScrollLock;

                case VK_SNAPSHOT:
                    return KeyCode::PrintScreen;

                case VK_PAUSE:
                    return KeyCode::Pause;

                case VK_INSERT:
                    return KeyCode::Insert;

                case VK_DELETE:
                    return KeyCode::Delete;

                case VK_HOME:
                    return KeyCode::Home;

                case VK_END:
                    return KeyCode::End;

                case VK_PRIOR:
                    return KeyCode::PageUp;

                case VK_NEXT:
                    return KeyCode::PageDown;

                case VK_LEFT:
                    return KeyCode::ArrowLeft;

                case VK_RIGHT:
                    return KeyCode::ArrowRight;

                case VK_UP:
                    return KeyCode::ArrowUp;

                case VK_DOWN:
                    return KeyCode::ArrowDown;

                case VK_OEM_3:
                    return KeyCode::Grave;

                case VK_OEM_MINUS:
                    return KeyCode::Minus;

                case VK_OEM_PLUS:
                    return KeyCode::Equal;

                case VK_OEM_4:
                    return KeyCode::LeftBracket;

                case VK_OEM_6:
                    return KeyCode::RightBracket;

                case VK_OEM_5:
                    return KeyCode::Backslash;

                case VK_OEM_1:
                    return KeyCode::Semicolon;

                case VK_OEM_7:
                    return KeyCode::Apostrophe;

                case VK_OEM_COMMA:
                    return KeyCode::Comma;

                case VK_OEM_PERIOD:
                    return KeyCode::Period;

                case VK_OEM_2:
                    return KeyCode::Slash;

                case VK_DECIMAL:
                    return KeyCode::NumpadDecimal;

                case VK_ADD:
                    return KeyCode::NumpadAdd;

                case VK_SUBTRACT:
                    return KeyCode::NumpadSubtract;

                case VK_MULTIPLY:
                    return KeyCode::NumpadMultiply;

                case VK_DIVIDE:
                    return KeyCode::NumpadDivide;

                default:
                    return KeyCode::Unknown;
            }
        }

        [[nodiscard]] MouseButton MapXButton(
            const std::uintptr_t wordParameter) noexcept
        {
            const std::uint16_t button =
                static_cast<std::uint16_t>(
                    (wordParameter >> 16) &
                    0xFFFFu);

            return button == XBUTTON2
                ? MouseButton::X2
                : MouseButton::X1;
        }
    }

    InputSystem::InputSystem() noexcept
    {
        Reset();
    }

    void InputSystem::BeginFrame() noexcept
    {
        std::fill(
            keyPressed_.begin(),
            keyPressed_.end(),
            0);

        std::fill(
            keyReleased_.begin(),
            keyReleased_.end(),
            0);

        std::fill(
            mouseButtonPressed_.begin(),
            mouseButtonPressed_.end(),
            0);

        std::fill(
            mouseButtonReleased_.begin(),
            mouseButtonReleased_.end(),
            0);

        mouseDeltaX_ = 0;
        mouseDeltaY_ = 0;

        mouseWheelDelta_ = 0;
        mouseHorizontalWheelDelta_ = 0;

        eventCount_ = 0;
        droppedEventCount_ = 0;
    }

    void InputSystem::Reset() noexcept
    {
        std::fill(
            keyDown_.begin(),
            keyDown_.end(),
            0);

        std::fill(
            keyPressed_.begin(),
            keyPressed_.end(),
            0);

        std::fill(
            keyReleased_.begin(),
            keyReleased_.end(),
            0);

        std::fill(
            mouseButtonDown_.begin(),
            mouseButtonDown_.end(),
            0);

        std::fill(
            mouseButtonPressed_.begin(),
            mouseButtonPressed_.end(),
            0);

        std::fill(
            mouseButtonReleased_.begin(),
            mouseButtonReleased_.end(),
            0);

        eventCount_ = 0;
        droppedEventCount_ = 0;

        mouseX_ = 0;
        mouseY_ = 0;

        mouseDeltaX_ = 0;
        mouseDeltaY_ = 0;

        mouseWheelDelta_ = 0;
        mouseHorizontalWheelDelta_ = 0;

        pendingHighSurrogate_ = 0;

        hasFocus_ = false;
        hasMousePosition_ = false;
    }

    bool InputSystem::HandleNativeMessage(
        const std::uint32_t message,
        const std::uintptr_t wordParameter,
        const std::intptr_t longParameter) noexcept
    {
        switch (message)
        {
            case WM_SETFOCUS:
            {
                hasFocus_ = true;

                InputEvent event;
                event.type =
                    InputEventType::FocusGained;

                PushEvent(event);
                return true;
            }

            case WM_KILLFOCUS:
            {
                ReleaseAllInputs();

                hasFocus_ = false;
                hasMousePosition_ = false;
                pendingHighSurrogate_ = 0;

                InputEvent event;
                event.type =
                    InputEventType::FocusLost;

                PushEvent(event);
                return true;
            }

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                const KeyCode key =
                    MapVirtualKeyToKeyCode(
                        wordParameter,
                        longParameter);

                const bool repeated =
                    (static_cast<std::uintptr_t>(
                         longParameter) &
                     PreviousKeyStateMask) != 0;

                SetKeyState(
                    key,
                    true,
                    repeated);

                return key != KeyCode::Unknown;
            }

            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const KeyCode key =
                    MapVirtualKeyToKeyCode(
                        wordParameter,
                        longParameter);

                SetKeyState(
                    key,
                    false,
                    false);

                return key != KeyCode::Unknown;
            }

            case WM_CHAR:
            {
                const std::uint32_t codeUnit =
                    static_cast<std::uint32_t>(
                        wordParameter &
                        0xFFFFu);

                if (codeUnit >= 0xD800u &&
                    codeUnit <= 0xDBFFu)
                {
                    pendingHighSurrogate_ =
                        static_cast<std::uint16_t>(
                            codeUnit);

                    return true;
                }

                if (codeUnit >= 0xDC00u &&
                    codeUnit <= 0xDFFFu)
                {
                    if (pendingHighSurrogate_ != 0)
                    {
                        const std::uint32_t high =
                            static_cast<std::uint32_t>(
                                pendingHighSurrogate_ -
                                0xD800u);

                        const std::uint32_t low =
                            codeUnit -
                            0xDC00u;

                        const std::uint32_t codepoint =
                            0x10000u +
                            ((high << 10u) | low);

                        pendingHighSurrogate_ = 0;

                        AddTextCodepoint(
                            codepoint);
                    }

                    return true;
                }

                pendingHighSurrogate_ = 0;

                AddTextCodepoint(
                    codeUnit);

                return true;
            }

            case WM_MOUSEMOVE:
            {
                SetMousePosition(
                    SignedLowWord(
                        longParameter),
                    SignedHighWord(
                        longParameter));

                return true;
            }

            case WM_LBUTTONDOWN:
                SetMouseButtonState(
                    MouseButton::Left,
                    true);
                return true;

            case WM_LBUTTONUP:
                SetMouseButtonState(
                    MouseButton::Left,
                    false);
                return true;

            case WM_RBUTTONDOWN:
                SetMouseButtonState(
                    MouseButton::Right,
                    true);
                return true;

            case WM_RBUTTONUP:
                SetMouseButtonState(
                    MouseButton::Right,
                    false);
                return true;

            case WM_MBUTTONDOWN:
                SetMouseButtonState(
                    MouseButton::Middle,
                    true);
                return true;

            case WM_MBUTTONUP:
                SetMouseButtonState(
                    MouseButton::Middle,
                    false);
                return true;

            case WM_XBUTTONDOWN:
                SetMouseButtonState(
                    MapXButton(wordParameter),
                    true);
                return true;

            case WM_XBUTTONUP:
                SetMouseButtonState(
                    MapXButton(wordParameter),
                    false);
                return true;

            case WM_MOUSEWHEEL:
                AddMouseWheel(
                    WheelDeltaFromWord(
                        wordParameter),
                    false);
                return true;

            case WM_MOUSEHWHEEL:
                AddMouseWheel(
                    WheelDeltaFromWord(
                        wordParameter),
                    true);
                return true;

            default:
                return false;
        }
    }

    bool InputSystem::IsKeyDown(
        const KeyCode key) const noexcept
    {
        const std::size_t index =
            ToIndex(key);

        return index < KeyCount &&
            key != KeyCode::Unknown &&
            keyDown_[index] != 0;
    }

    bool InputSystem::WasKeyPressed(
        const KeyCode key) const noexcept
    {
        const std::size_t index =
            ToIndex(key);

        return index < KeyCount &&
            key != KeyCode::Unknown &&
            keyPressed_[index] != 0;
    }

    bool InputSystem::WasKeyReleased(
        const KeyCode key) const noexcept
    {
        const std::size_t index =
            ToIndex(key);

        return index < KeyCount &&
            key != KeyCode::Unknown &&
            keyReleased_[index] != 0;
    }

    bool InputSystem::IsMouseButtonDown(
        const MouseButton button) const noexcept
    {
        const std::size_t index =
            ToIndex(button);

        return index < MouseButtonCount &&
            mouseButtonDown_[index] != 0;
    }

    bool InputSystem::WasMouseButtonPressed(
        const MouseButton button) const noexcept
    {
        const std::size_t index =
            ToIndex(button);

        return index < MouseButtonCount &&
            mouseButtonPressed_[index] != 0;
    }

    bool InputSystem::WasMouseButtonReleased(
        const MouseButton button) const noexcept
    {
        const std::size_t index =
            ToIndex(button);

        return index < MouseButtonCount &&
            mouseButtonReleased_[index] != 0;
    }

    bool InputSystem::HasFocus() const noexcept
    {
        return hasFocus_;
    }

    bool InputSystem::HasMousePosition() const noexcept
    {
        return hasMousePosition_;
    }

    MousePosition InputSystem::GetMousePosition() const noexcept
    {
        return
        {
            mouseX_,
            mouseY_
        };
    }

    MouseDelta InputSystem::GetMouseDelta() const noexcept
    {
        return
        {
            mouseDeltaX_,
            mouseDeltaY_
        };
    }

    std::int32_t
        InputSystem::GetMouseWheelDelta() const noexcept
    {
        return mouseWheelDelta_;
    }

    std::int32_t
        InputSystem::GetMouseHorizontalWheelDelta() const noexcept
    {
        return mouseHorizontalWheelDelta_;
    }

    std::size_t
        InputSystem::GetEventCount() const noexcept
    {
        return eventCount_;
    }

    const InputEvent* InputSystem::GetEvent(
        const std::size_t index) const noexcept
    {
        if (index >= eventCount_)
        {
            return nullptr;
        }

        return &events_[index];
    }

    std::size_t
        InputSystem::GetDroppedEventCount() const noexcept
    {
        return droppedEventCount_;
    }

    void InputSystem::SetKeyState(
        const KeyCode key,
        const bool pressed,
        const bool repeated) noexcept
    {
        const std::size_t index =
            ToIndex(key);

        if (key == KeyCode::Unknown ||
            index >= KeyCount)
        {
            return;
        }

        const bool wasDown =
            keyDown_[index] != 0;

        if (pressed)
        {
            keyDown_[index] = 1;

            if (!wasDown)
            {
                keyPressed_[index] = 1;
            }

            InputEvent event;
            event.type =
                InputEventType::KeyPressed;
            event.key = key;
            event.repeated =
                repeated || wasDown;

            PushEvent(event);
            return;
        }

        keyDown_[index] = 0;

        if (!wasDown)
        {
            return;
        }

        keyReleased_[index] = 1;

        InputEvent event;
        event.type =
            InputEventType::KeyReleased;
        event.key = key;

        PushEvent(event);
    }

    void InputSystem::SetMouseButtonState(
        const MouseButton button,
        const bool pressed) noexcept
    {
        const std::size_t index =
            ToIndex(button);

        if (index >= MouseButtonCount)
        {
            return;
        }

        const bool wasDown =
            mouseButtonDown_[index] != 0;

        if (pressed)
        {
            mouseButtonDown_[index] = 1;

            if (!wasDown)
            {
                mouseButtonPressed_[index] = 1;
            }

            InputEvent event;
            event.type =
                InputEventType::MouseButtonPressed;
            event.mouseButton = button;

            PushEvent(event);
            return;
        }

        mouseButtonDown_[index] = 0;

        if (!wasDown)
        {
            return;
        }

        mouseButtonReleased_[index] = 1;

        InputEvent event;
        event.type =
            InputEventType::MouseButtonReleased;
        event.mouseButton = button;

        PushEvent(event);
    }

    void InputSystem::SetMousePosition(
        const std::int32_t x,
        const std::int32_t y) noexcept
    {
        std::int32_t deltaX = 0;
        std::int32_t deltaY = 0;

        if (hasMousePosition_)
        {
            deltaX = x - mouseX_;
            deltaY = y - mouseY_;

            mouseDeltaX_ += deltaX;
            mouseDeltaY_ += deltaY;
        }

        mouseX_ = x;
        mouseY_ = y;
        hasMousePosition_ = true;

        InputEvent event;
        event.type =
            InputEventType::MouseMoved;
        event.mouseX = x;
        event.mouseY = y;
        event.deltaX = deltaX;
        event.deltaY = deltaY;

        PushEvent(event);
    }

    void InputSystem::AddMouseWheel(
        const std::int32_t delta,
        const bool horizontal) noexcept
    {
        InputEvent event;
        event.wheelDelta = delta;

        if (horizontal)
        {
            mouseHorizontalWheelDelta_ +=
                delta;

            event.type =
                InputEventType::
                    MouseHorizontalWheel;
        }
        else
        {
            mouseWheelDelta_ +=
                delta;

            event.type =
                InputEventType::MouseWheel;
        }

        PushEvent(event);
    }

    void InputSystem::AddTextCodepoint(
        const std::uint32_t codepoint) noexcept
    {
        if (codepoint == 0 ||
            codepoint > 0x10FFFFu)
        {
            return;
        }

        InputEvent event;
        event.type =
            InputEventType::TextInput;
        event.codepoint = codepoint;

        PushEvent(event);
    }

    void InputSystem::ReleaseAllInputs() noexcept
    {
        for (std::size_t index = 0;
             index < KeyCount;
             ++index)
        {
            if (keyDown_[index] == 0)
            {
                continue;
            }

            keyDown_[index] = 0;
            keyReleased_[index] = 1;

            InputEvent event;
            event.type =
                InputEventType::KeyReleased;

            event.key =
                static_cast<KeyCode>(
                    index);

            PushEvent(event);
        }

        for (std::size_t index = 0;
             index < MouseButtonCount;
             ++index)
        {
            if (mouseButtonDown_[index] == 0)
            {
                continue;
            }

            mouseButtonDown_[index] = 0;
            mouseButtonReleased_[index] = 1;

            InputEvent event;
            event.type =
                InputEventType::
                    MouseButtonReleased;

            event.mouseButton =
                static_cast<MouseButton>(
                    index);

            PushEvent(event);
        }
    }

    void InputSystem::PushEvent(
        const InputEvent& event) noexcept
    {
        if (eventCount_ >= MaximumEventCount)
        {
            ++droppedEventCount_;
            return;
        }

        events_[eventCount_] = event;
        ++eventCount_;
    }
}