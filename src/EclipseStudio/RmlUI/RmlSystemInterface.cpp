#include "r3dPCH.h"
#include "r3d.h"

#include "RmlSystemInterface.h"

#include <string>

RmlSystemInterface::RmlSystemInterface()
{
    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&StartTime);
}

double RmlSystemInterface::GetElapsedTime()
{
    LARGE_INTEGER Current{};
    QueryPerformanceCounter(&Current);

    const double Ticks = static_cast<double>(Current.QuadPart - StartTime.QuadPart);
    return Ticks / static_cast<double>(Frequency.QuadPart);
}

bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
    const char* TypeText = "Info";

    switch (type)
    {
    case Rml::Log::LT_ALWAYS:  TypeText = "Always"; break;
    case Rml::Log::LT_ERROR:   TypeText = "Error"; break;
    case Rml::Log::LT_ASSERT:  TypeText = "Assert"; break;
    case Rml::Log::LT_WARNING: TypeText = "Warning"; break;
    case Rml::Log::LT_INFO:    TypeText = "Info"; break;
    case Rml::Log::LT_DEBUG:   TypeText = "Debug"; break;
    default: break;
    }

    std::string Text;
    Text.reserve(message.size() + 64);
    Text += "[RmlUI][";
    Text += TypeText;
    Text += "] ";
    Text += message;
    Text += "\n";

    OutputDebugStringA(Text.c_str());

    return true;
}

bool RmlSystemInterface::IsAbsolutePath(const Rml::String& path)
{
    if (path.size() >= 2 && path[1] == ':')
        return true;

    if (!path.empty() && (path[0] == '/' || path[0] == '\\'))
        return true;

    return false;
}

void RmlSystemInterface::JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path)
{
    if (IsAbsolutePath(path))
    {
        translated_path = path;
        return;
    }

    const size_t SlashA = document_path.find_last_of('/');
    const size_t SlashB = document_path.find_last_of('\\');
    const size_t Slash = std::max(
        SlashA == Rml::String::npos ? 0 : SlashA,
        SlashB == Rml::String::npos ? 0 : SlashB
    );

    if (Slash != 0 && Slash != Rml::String::npos)
        translated_path = document_path.substr(0, Slash + 1) + path;
    else
        translated_path = path;
}