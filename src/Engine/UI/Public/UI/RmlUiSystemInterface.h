#pragma once

#include <RmlUi/Core/SystemInterface.h>

#include <chrono>

namespace engine::ui
{
    class RmlUiSystemInterface final : public Rml::SystemInterface
    {
    public:
        RmlUiSystemInterface() noexcept;

        double GetElapsedTime() override;
        bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
        void SetMouseCursor(const Rml::String& cursorName) override;
        void SetClipboardText(const Rml::String& text) override;
        void GetClipboardText(Rml::String& text) override;

    private:
        std::chrono::steady_clock::time_point startTime_;
    };
}
