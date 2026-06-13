#include "RmlEditorSystemInterface.h"

#include "../App/RmlEditorLog.h"

RmlEditorSystemInterface::RmlEditorSystemInterface()
{
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&StartTime);
}

double RmlEditorSystemInterface::GetElapsedTime()
{
    LARGE_INTEGER CurrentTime{};
    QueryPerformanceCounter(&CurrentTime);

    if (Frequency.QuadPart == 0)
        return 0.0;

    const double ElapsedTicks = static_cast<double>(
        CurrentTime.QuadPart - StartTime.QuadPart
    );

    return ElapsedTicks / static_cast<double>(Frequency.QuadPart);
}

bool RmlEditorSystemInterface::LogMessage(
    Rml::Log::Type Type,
    const Rml::String& Message
)
{
    const char* TypeText = "Info";

    switch (Type)
    {
    case Rml::Log::LT_ALWAYS:
        TypeText = "Always";
        break;

    case Rml::Log::LT_ERROR:
        TypeText = "Error";
        break;

    case Rml::Log::LT_ASSERT:
        TypeText = "Assert";
        break;

    case Rml::Log::LT_WARNING:
        TypeText = "Warning";
        break;

    case Rml::Log::LT_INFO:
        TypeText = "Info";
        break;

    case Rml::Log::LT_DEBUG:
        TypeText = "Debug";
        break;

    default:
        break;
    }

    RmlEditorLog::Write(
        "[RmlEditor][RmlUi][%s] %s",
        TypeText,
        Message.c_str()
    );

    return true;
}

bool RmlEditorSystemInterface::IsAbsolutePath(const Rml::String& Path)
{
    if (Path.size() >= 2 && Path[1] == ':')
        return true;

    if (!Path.empty() && (Path[0] == '/' || Path[0] == '\\'))
        return true;

    return false;
}

void RmlEditorSystemInterface::JoinPath(
    Rml::String& TranslatedPath,
    const Rml::String& DocumentPath,
    const Rml::String& Path
)
{
    if (IsAbsolutePath(Path))
    {
        TranslatedPath = Path;
        return;
    }

    const size_t Slash = DocumentPath.find_last_of("/\\");

    if (Slash == Rml::String::npos)
    {
        TranslatedPath = Path;
        return;
    }

    TranslatedPath =
        DocumentPath.substr(0, Slash + 1) +
        Path;
}