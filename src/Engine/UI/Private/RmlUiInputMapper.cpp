#include "UI/RmlUiInputMapper.h"

#include <RmlUi/Core/Context.h>

namespace engine::ui
{
    namespace
    {
        [[nodiscard]] int MapButton(const engine::platform::MouseButton button) noexcept
        {
            switch (button)
            {
                case engine::platform::MouseButton::Left: return 0;
                case engine::platform::MouseButton::Right: return 1;
                case engine::platform::MouseButton::Middle: return 2;
                case engine::platform::MouseButton::X1: return 3;
                case engine::platform::MouseButton::X2: return 4;
                default: return 0;
            }
        }
    }

    bool RmlUiInputMapper::ProcessEvents(
        Rml::Context& context,
        const engine::platform::InputSystem& input) const
    {
        bool consumed = false;
        for (std::size_t index = 0; index < input.GetEventCount(); ++index)
        {
            const auto* event = input.GetEvent(index);
            if (event == nullptr) continue;
            const int modifiers = GetModifiers(input);
            bool propagate = true;
            switch (event->type)
            {
                case engine::platform::InputEventType::KeyPressed:
                    propagate = context.ProcessKeyDown(MapKey(event->key), modifiers); break;
                case engine::platform::InputEventType::KeyReleased:
                    propagate = context.ProcessKeyUp(MapKey(event->key), modifiers); break;
                case engine::platform::InputEventType::TextInput:
                    propagate = context.ProcessTextInput(static_cast<Rml::Character>(event->codepoint)); break;
                case engine::platform::InputEventType::MouseButtonPressed:
                    propagate = context.ProcessMouseButtonDown(MapButton(event->mouseButton), modifiers); break;
                case engine::platform::InputEventType::MouseButtonReleased:
                    propagate = context.ProcessMouseButtonUp(MapButton(event->mouseButton), modifiers); break;
                case engine::platform::InputEventType::MouseMoved:
                    propagate = context.ProcessMouseMove(event->mouseX, event->mouseY, modifiers); break;
                case engine::platform::InputEventType::MouseWheel:
                    propagate = context.ProcessMouseWheel(
                        -static_cast<float>(event->wheelDelta) / 120.0F, modifiers); break;
                case engine::platform::InputEventType::FocusLost:
                    propagate = context.ProcessMouseLeave(); break;
                case engine::platform::InputEventType::MouseHorizontalWheel:
                case engine::platform::InputEventType::FocusGained:
                default: break;
            }
            consumed = consumed || !propagate;
        }
        return consumed;
    }

    Rml::Input::KeyIdentifier RmlUiInputMapper::MapKey(
        const engine::platform::KeyCode key) noexcept
    {
        using K = engine::platform::KeyCode;
        using namespace Rml::Input;
        if (key >= K::Digit0 && key <= K::Digit9)
            return static_cast<KeyIdentifier>(KI_0 + (static_cast<int>(key) - static_cast<int>(K::Digit0)));
        if (key >= K::A && key <= K::Z)
            return static_cast<KeyIdentifier>(KI_A + (static_cast<int>(key) - static_cast<int>(K::A)));
        if (key >= K::F1 && key <= K::F12)
            return static_cast<KeyIdentifier>(KI_F1 + (static_cast<int>(key) - static_cast<int>(K::F1)));
        if (key >= K::Numpad0 && key <= K::Numpad9)
            return static_cast<KeyIdentifier>(KI_NUMPAD0 + (static_cast<int>(key) - static_cast<int>(K::Numpad0)));
        switch (key)
        {
            case K::Escape: return KI_ESCAPE; case K::Tab: return KI_TAB;
            case K::Enter: return KI_RETURN; case K::Backspace: return KI_BACK;
            case K::Space: return KI_SPACE; case K::LeftShift: return KI_LSHIFT;
            case K::RightShift: return KI_RSHIFT; case K::LeftControl: return KI_LCONTROL;
            case K::RightControl: return KI_RCONTROL; case K::LeftAlt: return KI_LMENU;
            case K::RightAlt: return KI_RMENU; case K::LeftSuper: return KI_LWIN;
            case K::RightSuper: return KI_RWIN; case K::CapsLock: return KI_CAPITAL;
            case K::NumLock: return KI_NUMLOCK; case K::ScrollLock: return KI_SCROLL;
            case K::PrintScreen: return KI_SNAPSHOT; case K::Pause: return KI_PAUSE;
            case K::Insert: return KI_INSERT; case K::Delete: return KI_DELETE;
            case K::Home: return KI_HOME; case K::End: return KI_END;
            case K::PageUp: return KI_PRIOR; case K::PageDown: return KI_NEXT;
            case K::ArrowLeft: return KI_LEFT; case K::ArrowRight: return KI_RIGHT;
            case K::ArrowUp: return KI_UP; case K::ArrowDown: return KI_DOWN;
            case K::Grave: return KI_OEM_3; case K::Minus: return KI_OEM_MINUS;
            case K::Equal: return KI_OEM_PLUS; case K::LeftBracket: return KI_OEM_4;
            case K::RightBracket: return KI_OEM_6; case K::Backslash: return KI_OEM_5;
            case K::Semicolon: return KI_OEM_1; case K::Apostrophe: return KI_OEM_7;
            case K::Comma: return KI_OEM_COMMA; case K::Period: return KI_OEM_PERIOD;
            case K::Slash: return KI_OEM_2; case K::NumpadDecimal: return KI_DECIMAL;
            case K::NumpadAdd: return KI_ADD; case K::NumpadSubtract: return KI_SUBTRACT;
            case K::NumpadMultiply: return KI_MULTIPLY; case K::NumpadDivide: return KI_DIVIDE;
            case K::NumpadEnter: return KI_NUMPADENTER; default: return KI_UNKNOWN;
        }
    }

    int RmlUiInputMapper::GetModifiers(const engine::platform::InputSystem& input) noexcept
    {
        using K = engine::platform::KeyCode;
        int modifiers = 0;
        if (input.IsKeyDown(K::LeftControl) || input.IsKeyDown(K::RightControl)) modifiers |= Rml::Input::KM_CTRL;
        if (input.IsKeyDown(K::LeftShift) || input.IsKeyDown(K::RightShift)) modifiers |= Rml::Input::KM_SHIFT;
        if (input.IsKeyDown(K::LeftAlt) || input.IsKeyDown(K::RightAlt)) modifiers |= Rml::Input::KM_ALT;
        if (input.IsKeyDown(K::LeftSuper) || input.IsKeyDown(K::RightSuper)) modifiers |= Rml::Input::KM_META;
        return modifiers;
    }
}
