#pragma once

#include <RmlUi/Core/SystemInterface.h>

#include <windows.h>

class RmlEditorSystemInterface final : public Rml::SystemInterface
{
public:
    RmlEditorSystemInterface();

    double GetElapsedTime() override;

    bool LogMessage(
        Rml::Log::Type Type,
        const Rml::String& Message
    ) override;

    void JoinPath(
        Rml::String& TranslatedPath,
        const Rml::String& DocumentPath,
        const Rml::String& Path
    ) override;

private:
    LARGE_INTEGER Frequency{};
    LARGE_INTEGER StartTime{};

    static bool IsAbsolutePath(const Rml::String& Path);
};