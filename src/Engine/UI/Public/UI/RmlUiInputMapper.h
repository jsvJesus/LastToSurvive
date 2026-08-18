#pragma once

#include <Platform/Input.h>
#include <RmlUi/Core/Input.h>

namespace Rml { class Context; }

namespace engine::ui
{
    class RmlUiInputMapper final
    {
    public:
        [[nodiscard]] bool ProcessEvents(
            Rml::Context& context,
            const engine::platform::InputSystem& input) const;

        [[nodiscard]] static Rml::Input::KeyIdentifier MapKey(
            engine::platform::KeyCode key) noexcept;

        [[nodiscard]] static int GetModifiers(
            const engine::platform::InputSystem& input) noexcept;
    };
}
