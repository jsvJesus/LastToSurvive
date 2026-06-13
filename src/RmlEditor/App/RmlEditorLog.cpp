#include "RmlEditorLog.h"

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace
{
    FILE* GLogFile = nullptr;
    std::mutex GLogMutex;

    std::wstring GetLogPath()
    {
        wchar_t ModulePath[MAX_PATH] = {};

        const DWORD Length = GetModuleFileNameW(
            nullptr,
            ModulePath,
            static_cast<DWORD>(_countof(ModulePath))
        );

        if (Length == 0 || Length >= _countof(ModulePath))
            return L"RmlEditor.log";

        std::wstring Result = ModulePath;

        const size_t Slash = Result.find_last_of(L"\\/");

        if (Slash != std::wstring::npos)
            Result.resize(Slash + 1);
        else
            Result.clear();

        Result += L"RmlEditor.log";
        return Result;
    }
}

bool RmlEditorLog::Initialize()
{
    {
        std::lock_guard<std::mutex> Lock(GLogMutex);

        if (GLogFile)
            return true;

        const std::wstring LogPath = GetLogPath();

        FILE* NewLogFile = nullptr;

        const errno_t OpenResult = _wfopen_s(
            &NewLogFile,
            LogPath.c_str(),
            L"wb"
        );

        if (OpenResult != 0 || !NewLogFile)
        {
            OutputDebugStringA(
                "[RmlEditor] Failed to create RmlEditor.log\n"
            );

            return false;
        }

        GLogFile = NewLogFile;
    }

    // Важно: вызывается после освобождения GLogMutex.
    Write("[RmlEditor] Log initialized");

    return true;
}

void RmlEditorLog::Shutdown()
{
    std::lock_guard<std::mutex> Lock(GLogMutex);

    if (!GLogFile)
        return;

    fflush(GLogFile);
    fclose(GLogFile);

    GLogFile = nullptr;
}

void RmlEditorLog::Write(const char* Format, ...)
{
    if (!Format)
        return;

    char Message[4096] = {};

    va_list Arguments;
    va_start(Arguments, Format);

    _vsnprintf_s(
        Message,
        _countof(Message),
        _TRUNCATE,
        Format,
        Arguments
    );

    va_end(Arguments);

    char Output[4352] = {};

    _snprintf_s(
        Output,
        _countof(Output),
        _TRUNCATE,
        "%s\n",
        Message
    );

    OutputDebugStringA(Output);

    std::lock_guard<std::mutex> Lock(GLogMutex);

    if (!GLogFile)
        return;

    fwrite(
        Output,
        1,
        strlen(Output),
        GLogFile
    );

    fflush(GLogFile);
}