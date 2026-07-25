#include "Editor/Tools/TestGame/Launcher.h"

#include <Windows.h>

#include <array>
#include <filesystem>
#include <string>
#include <system_error>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        std::wstring FormatWindowsError(
            const DWORD errorCode)
        {
            wchar_t* messageBuffer = nullptr;

            const DWORD characterCount =
                FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr,
                    errorCode,
                    MAKELANGID(
                        LANG_NEUTRAL,
                        SUBLANG_DEFAULT),
                    reinterpret_cast<wchar_t*>(
                        &messageBuffer),
                    0U,
                    nullptr);

            std::wstring message;

            if (
                characterCount > 0U &&
                messageBuffer != nullptr)
            {
                message.assign(
                    messageBuffer,
                    characterCount);

                LocalFree(messageBuffer);
            }
            else
            {
                message =
                    L"Unknown Windows error.";
            }

            while (
                !message.empty() &&
                (
                    message.back() == L'\r' ||
                    message.back() == L'\n' ||
                    message.back() == L' '
                ))
            {
                message.pop_back();
            }

            return message;
        }

        [[nodiscard]]
        bool GetEditorExecutablePath(
            std::filesystem::path& output,
            std::wstring& error)
        {
            std::array<wchar_t, 32768U> buffer{};

            const DWORD capacity =
                static_cast<DWORD>(
                    buffer.size());

            const DWORD length =
                GetModuleFileNameW(
                    nullptr,
                    buffer.data(),
                    capacity);

            if (
                length == 0U ||
                length >= capacity)
            {
                error =
                    L"Could not resolve the "
                    L"LTS.Editor.exe directory.";

                return false;
            }

            output =
                std::filesystem::path(
                    std::wstring(
                        buffer.data(),
                        length));

            return true;
        }

        [[nodiscard]]
        std::wstring QuoteArgument(
            const std::filesystem::path& path)
        {
            return
                L"\"" +
                path.wstring() +
                L"\"";
        }
    }

    bool Launcher::Launch(
        const std::filesystem::path& levelPath,
        std::wstring& error) noexcept
    {
        try
        {
            error.clear();

            if (levelPath.empty())
            {
                error =
                    L"The current level has not been saved.";

                return false;
            }

            std::filesystem::path
                editorExecutable;

            if (!GetEditorExecutablePath(
                    editorExecutable,
                    error))
            {
                return false;
            }

            const std::filesystem::path
                editorDirectory =
                    editorExecutable.parent_path();

            const std::filesystem::path
                gameExecutable =
                    editorDirectory /
                    L"LTS.Game.exe";

            std::error_code pathError;

            if (
                !std::filesystem::is_regular_file(
                    gameExecutable,
                    pathError) ||
                pathError)
            {
                error =
                    L"LTS.Game.exe was not found:\n" +
                    gameExecutable.wstring();

                return false;
            }

            std::filesystem::path
                resolvedLevelPath =
                    levelPath;

            if (resolvedLevelPath.is_relative())
            {
                resolvedLevelPath =
                    std::filesystem::absolute(
                        resolvedLevelPath,
                        pathError);

                if (pathError)
                {
                    error =
                        L"Could not resolve the "
                        L"absolute level path.";

                    return false;
                }
            }

            pathError.clear();

            if (
                !std::filesystem::is_regular_file(
                    resolvedLevelPath,
                    pathError) ||
                pathError)
            {
                error =
                    L"The current level file was not found:\n" +
                    resolvedLevelPath.wstring();

                return false;
            }

            std::wstring commandLine =
                QuoteArgument(gameExecutable);

            commandLine +=
                L" --editor-test";

            commandLine +=
                L" --level ";

            commandLine +=
                QuoteArgument(resolvedLevelPath);

            STARTUPINFOW startupInformation{};

            startupInformation.cb =
                static_cast<DWORD>(
                    sizeof(startupInformation));

            PROCESS_INFORMATION
                processInformation{};

            const std::wstring
                workingDirectory =
                    editorDirectory.wstring();

            const BOOL created =
                CreateProcessW(
                    gameExecutable.c_str(),
                    commandLine.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    CREATE_NEW_PROCESS_GROUP,
                    nullptr,
                    workingDirectory.c_str(),
                    &startupInformation,
                    &processInformation);

            if (created == FALSE)
            {
                const DWORD windowsError =
                    GetLastError();

                error =
                    L"Could not start LTS.Game.exe.\n\n" +
                    FormatWindowsError(
                        windowsError);

                return false;
            }

            CloseHandle(
                processInformation.hThread);

            CloseHandle(
                processInformation.hProcess);

            return true;
        }
        catch (...)
        {
            error =
                L"Unexpected error while starting "
                L"LTS.Game.exe.";

            return false;
        }
    }
}