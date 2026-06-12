#pragma once

#include <RmlUi/Core/SystemInterface.h>
#include <windows.h>

class RmlSystemInterface final : public Rml::SystemInterface
{
public:
    RmlSystemInterface();

    double GetElapsedTime() override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
    void JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path) override;

private:
    LARGE_INTEGER Frequency{};
    LARGE_INTEGER StartTime{};

    static bool IsAbsolutePath(const Rml::String& path);
};