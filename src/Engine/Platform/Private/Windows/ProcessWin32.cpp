#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Process.h"

#include <utility>

namespace engine::platform
{
    namespace
    {
        [[nodiscard]] HANDLE ToNativeProcess(
            const std::uintptr_t nativeHandle) noexcept
        {
            return reinterpret_cast<HANDLE>(
                nativeHandle);
        }

        [[nodiscard]] std::uintptr_t FromNativeProcess(
            const HANDLE handle) noexcept
        {
            return reinterpret_cast<std::uintptr_t>(
                handle);
        }

        [[nodiscard]] DWORD GetCreationFlags(
            const ProcessWindowMode windowMode) noexcept
        {
            DWORD flags =
                CREATE_UNICODE_ENVIRONMENT;

            if (windowMode ==
                ProcessWindowMode::NoWindow)
            {
                flags |= CREATE_NO_WINDOW;
            }

            return flags;
        }

        void ConfigureStartupInfo(
            STARTUPINFOW& startupInfo,
            const ProcessWindowMode windowMode) noexcept
        {
            startupInfo = {};
            startupInfo.cb =
                sizeof(startupInfo);

            if (windowMode ==
                ProcessWindowMode::Hidden)
            {
                startupInfo.dwFlags |=
                    STARTF_USESHOWWINDOW;

                startupInfo.wShowWindow =
                    SW_HIDE;
            }
        }

        [[nodiscard]] bool ResolveExecutablePath(
        const Path& executablePath,
        Path& resolvedPath,
        DWORD& errorCode)
        {
            if (executablePath.empty())
            {
                errorCode = ERROR_INVALID_PARAMETER;
                return false;
            }

            /*
             * Полный путь либо путь с указанием каталога
             * передаём как есть.
             *
             * Примеры:
             *   C:\Tools\Tool.exe
             *   .\Tools\Tool.exe
             *   Tools\Tool.exe
             */
            if (executablePath.is_absolute() ||
                executablePath.has_parent_path())
            {
                resolvedPath = executablePath;
                errorCode = ERROR_SUCCESS;
                return true;
            }

            /*
             * Простое имя вроде cmd.exe или tool.exe
             * разрешаем в полный путь до CreateProcessW.
             */
            std::wstring buffer(
                MAX_PATH,
                L'\0');

            const wchar_t* extension =
                executablePath.has_extension()
                    ? nullptr
                    : L".exe";

            for (;;)
            {
                const DWORD length =
                    ::SearchPathW(
                        nullptr,
                        executablePath.c_str(),
                        extension,
                        static_cast<DWORD>(
                            buffer.size()),
                        buffer.data(),
                        nullptr);

                if (length == 0)
                {
                    errorCode = ::GetLastError();
                    return false;
                }

                if (length < buffer.size())
                {
                    buffer.resize(
                        static_cast<std::size_t>(
                            length));

                    resolvedPath =
                        Path(std::move(buffer));

                    errorCode = ERROR_SUCCESS;
                    return true;
                }

                buffer.resize(
                    static_cast<std::size_t>(
                        length) + 1);
            }
        }

        [[nodiscard]] std::wstring BuildCommandLine(
        const Path& executablePath,
        const std::wstring& arguments)
        {
            std::wstring commandLine;

            commandLine.reserve(
                executablePath.native().size() +
                arguments.size() +
                4);

            commandLine += L'"';
            commandLine += executablePath.native();
            commandLine += L'"';

            if (!arguments.empty())
            {
                commandLine += L' ';
                commandLine += arguments;
            }

            return commandLine;
        }
    }

    std::uint32_t GetCurrentProcessId() noexcept
    {
        return static_cast<std::uint32_t>(
            ::GetCurrentProcessId());
    }

    Path GetCurrentProcessPath()
    {
        return GetExecutablePath();
    }

    const char* ToString(
        const ProcessWaitResult result) noexcept
    {
        switch (result)
        {
            case ProcessWaitResult::Completed:
                return "completed";

            case ProcessWaitResult::Timeout:
                return "timeout";

            case ProcessWaitResult::Failed:
                return "failed";

            default:
                return "unknown";
        }
    }

    Process::Process(
        const ProcessStartInfo& startInfo)
    {
        Start(startInfo);
    }

    Process::~Process() noexcept
    {
        Close();
    }

    Process::Process(Process&& other) noexcept
        : nativeHandle_(
              std::exchange(
                  other.nativeHandle_,
                  0)),
          processId_(
              std::exchange(
                  other.processId_,
                  0)),
          lastErrorCode_(
              std::exchange(
                  other.lastErrorCode_,
                  ERROR_SUCCESS))
    {
    }

    Process& Process::operator=(
        Process&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Close();

        nativeHandle_ =
            std::exchange(
                other.nativeHandle_,
                0);

        processId_ =
            std::exchange(
                other.processId_,
                0);

        lastErrorCode_ =
            std::exchange(
                other.lastErrorCode_,
                ERROR_SUCCESS);

        return *this;
    }

    bool Process::Start(
    const ProcessStartInfo& startInfo)
    {
        Close();

        if (startInfo.executablePath.empty())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return false;
        }

        Path resolvedExecutablePath;
        DWORD resolveError = ERROR_SUCCESS;

        if (!ResolveExecutablePath(
                startInfo.executablePath,
                resolvedExecutablePath,
                resolveError))
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    resolveError);

            return false;
        }

        std::wstring commandLine =
            BuildCommandLine(
                resolvedExecutablePath,
                startInfo.arguments);

        STARTUPINFOW startupInfo{};

        ConfigureStartupInfo(
            startupInfo,
            startInfo.windowMode);

        PROCESS_INFORMATION processInformation{};

        const wchar_t* workingDirectory =
            startInfo.workingDirectory.empty()
                ? nullptr
                : startInfo.workingDirectory.c_str();

        const BOOL created =
            ::CreateProcessW(
                resolvedExecutablePath.c_str(),
                commandLine.data(),
                nullptr,
                nullptr,
                FALSE,
                GetCreationFlags(
                    startInfo.windowMode),
                nullptr,
                workingDirectory,
                &startupInfo,
                &processInformation);

        if (created == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        if (processInformation.hThread != nullptr)
        {
            ::CloseHandle(
                processInformation.hThread);
        }

        nativeHandle_ =
            FromNativeProcess(
                processInformation.hProcess);

        processId_ =
            static_cast<std::uint32_t>(
                processInformation.dwProcessId);

        lastErrorCode_ = ERROR_SUCCESS;

        return true;
    }

    void Process::Close() noexcept
    {
        if (!IsValid())
        {
            processId_ = 0;
            lastErrorCode_ = ERROR_SUCCESS;
            return;
        }

        const HANDLE process =
            ToNativeProcess(nativeHandle_);

        nativeHandle_ = 0;
        processId_ = 0;

        if (::CloseHandle(process) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return;
        }

        lastErrorCode_ = ERROR_SUCCESS;
    }

    bool Process::IsValid() const noexcept
    {
        return nativeHandle_ != 0;
    }

    Process::operator bool() const noexcept
    {
        return IsValid();
    }

    std::uint32_t Process::GetId() const noexcept
    {
        return processId_;
    }

    bool Process::IsRunning() const noexcept
    {
        const std::optional<std::uint32_t> exitCode =
            GetExitCode();

        return exitCode.has_value() &&
            exitCode.value() == STILL_ACTIVE;
    }

    ProcessWaitResult Process::Wait(
        const std::uint32_t timeoutMilliseconds) noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return ProcessWaitResult::Failed;
        }

        const DWORD result =
            ::WaitForSingleObject(
                ToNativeProcess(nativeHandle_),
                static_cast<DWORD>(
                    timeoutMilliseconds));

        switch (result)
        {
            case WAIT_OBJECT_0:
                lastErrorCode_ = ERROR_SUCCESS;
                return ProcessWaitResult::Completed;

            case WAIT_TIMEOUT:
                lastErrorCode_ = ERROR_SUCCESS;
                return ProcessWaitResult::Timeout;

            case WAIT_FAILED:
                lastErrorCode_ =
                    static_cast<std::uint32_t>(
                        ::GetLastError());

                return ProcessWaitResult::Failed;

            default:
                lastErrorCode_ =
                    static_cast<std::uint32_t>(
                        ERROR_INVALID_FUNCTION);

                return ProcessWaitResult::Failed;
        }
    }

    std::optional<std::uint32_t>
        Process::GetExitCode() const noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return std::nullopt;
        }

        DWORD exitCode = 0;

        if (::GetExitCodeProcess(
                ToNativeProcess(nativeHandle_),
                &exitCode) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return std::nullopt;
        }

        lastErrorCode_ = ERROR_SUCCESS;

        return static_cast<std::uint32_t>(
            exitCode);
    }

    bool Process::Terminate(
        const std::uint32_t exitCode) noexcept
    {
        if (!IsValid())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_HANDLE);

            return false;
        }

        if (::TerminateProcess(
                ToNativeProcess(nativeHandle_),
                static_cast<UINT>(
                    exitCode)) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        lastErrorCode_ = ERROR_SUCCESS;
        return true;
    }

    std::uint32_t
        Process::GetLastErrorCode() const noexcept
    {
        return lastErrorCode_;
    }
}