#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine::platform
{
    enum class KeyCode : std::uint16_t
    {
        Unknown = 0,

        Escape,

        Digit0,
        Digit1,
        Digit2,
        Digit3,
        Digit4,
        Digit5,
        Digit6,
        Digit7,
        Digit8,
        Digit9,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        Tab,
        Enter,
        Backspace,
        Space,

        LeftShift,
        RightShift,
        LeftControl,
        RightControl,
        LeftAlt,
        RightAlt,
        LeftSuper,
        RightSuper,

        CapsLock,
        NumLock,
        ScrollLock,
        PrintScreen,
        Pause,

        Insert,
        Delete,
        Home,
        End,
        PageUp,
        PageDown,

        ArrowLeft,
        ArrowRight,
        ArrowUp,
        ArrowDown,

        Grave,
        Minus,
        Equal,
        LeftBracket,
        RightBracket,
        Backslash,
        Semicolon,
        Apostrophe,
        Comma,
        Period,
        Slash,

        Numpad0,
        Numpad1,
        Numpad2,
        Numpad3,
        Numpad4,
        Numpad5,
        Numpad6,
        Numpad7,
        Numpad8,
        Numpad9,

        NumpadDecimal,
        NumpadAdd,
        NumpadSubtract,
        NumpadMultiply,
        NumpadDivide,
        NumpadEnter,

        Count
    };

    enum class MouseButton : std::uint8_t
    {
        Left = 0,
        Right,
        Middle,
        X1,
        X2,

        Count
    };

    enum class InputEventType : std::uint8_t
    {
        KeyPressed,
        KeyReleased,
        TextInput,

        MouseButtonPressed,
        MouseButtonReleased,
        MouseMoved,
        MouseWheel,
        MouseHorizontalWheel,

        FocusGained,
        FocusLost
    };

    struct InputEvent final
    {
        InputEventType type =
            InputEventType::KeyPressed;

        KeyCode key = KeyCode::Unknown;

        MouseButton mouseButton =
            MouseButton::Left;

        std::uint32_t codepoint = 0;

        std::int32_t mouseX = 0;
        std::int32_t mouseY = 0;

        std::int32_t deltaX = 0;
        std::int32_t deltaY = 0;

        std::int32_t wheelDelta = 0;

        bool repeated = false;
    };

    struct MousePosition final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
    };

    struct MouseDelta final
    {
        std::int32_t x = 0;
        std::int32_t y = 0;
    };

    class InputSystem final
    {
    public:
        static constexpr std::size_t MaximumEventCount =
            256;

        InputSystem() noexcept;

        void BeginFrame() noexcept;

        void Reset() noexcept;

        /*
         * Временный migration-вход для сообщения платформы.
         * Win32-типы в public API не выходят.
         */
        [[nodiscard]] bool HandleNativeMessage(
            std::uint32_t message,
            std::uintptr_t wordParameter,
            std::intptr_t longParameter) noexcept;

        [[nodiscard]] bool IsKeyDown(
            KeyCode key) const noexcept;

        [[nodiscard]] bool WasKeyPressed(
            KeyCode key) const noexcept;

        [[nodiscard]] bool WasKeyReleased(
            KeyCode key) const noexcept;

        [[nodiscard]] bool IsMouseButtonDown(
            MouseButton button) const noexcept;

        [[nodiscard]] bool WasMouseButtonPressed(
            MouseButton button) const noexcept;

        [[nodiscard]] bool WasMouseButtonReleased(
            MouseButton button) const noexcept;

        [[nodiscard]] bool HasFocus() const noexcept;

        [[nodiscard]] bool HasMousePosition() const noexcept;

        [[nodiscard]] MousePosition
            GetMousePosition() const noexcept;

        [[nodiscard]] MouseDelta
            GetMouseDelta() const noexcept;

        [[nodiscard]] std::int32_t
            GetMouseWheelDelta() const noexcept;

        [[nodiscard]] std::int32_t
            GetMouseHorizontalWheelDelta() const noexcept;

        [[nodiscard]] std::size_t
            GetEventCount() const noexcept;

        [[nodiscard]] const InputEvent* GetEvent(
            std::size_t index) const noexcept;

        [[nodiscard]] std::size_t
            GetDroppedEventCount() const noexcept;

    private:
        static constexpr std::size_t KeyCount =
            static_cast<std::size_t>(
                KeyCode::Count);

        static constexpr std::size_t MouseButtonCount =
            static_cast<std::size_t>(
                MouseButton::Count);

        [[nodiscard]] static constexpr std::size_t
            ToIndex(KeyCode key) noexcept
        {
            return static_cast<std::size_t>(key);
        }

        [[nodiscard]] static constexpr std::size_t
            ToIndex(MouseButton button) noexcept
        {
            return static_cast<std::size_t>(button);
        }

        void SetKeyState(
            KeyCode key,
            bool pressed,
            bool repeated) noexcept;

        void SetMouseButtonState(
            MouseButton button,
            bool pressed) noexcept;

        void SetMousePosition(
            std::int32_t x,
            std::int32_t y) noexcept;

        void AddMouseWheel(
            std::int32_t delta,
            bool horizontal) noexcept;

        void AddTextCodepoint(
            std::uint32_t codepoint) noexcept;

        void ReleaseAllInputs() noexcept;

        void PushEvent(
            const InputEvent& event) noexcept;

        std::array<std::uint8_t, KeyCount>
            keyDown_{};

        std::array<std::uint8_t, KeyCount>
            keyPressed_{};

        std::array<std::uint8_t, KeyCount>
            keyReleased_{};

        std::array<std::uint8_t, MouseButtonCount>
            mouseButtonDown_{};

        std::array<std::uint8_t, MouseButtonCount>
            mouseButtonPressed_{};

        std::array<std::uint8_t, MouseButtonCount>
            mouseButtonReleased_{};

        std::array<InputEvent, MaximumEventCount>
            events_{};

        std::size_t eventCount_ = 0;
        std::size_t droppedEventCount_ = 0;

        std::int32_t mouseX_ = 0;
        std::int32_t mouseY_ = 0;

        std::int32_t mouseDeltaX_ = 0;
        std::int32_t mouseDeltaY_ = 0;

        std::int32_t mouseWheelDelta_ = 0;
        std::int32_t mouseHorizontalWheelDelta_ = 0;

        std::uint16_t pendingHighSurrogate_ = 0;

        bool hasFocus_ = false;
        bool hasMousePosition_ = false;
    };
}