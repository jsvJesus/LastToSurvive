#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Thread.h"

namespace engine::platform
{
    namespace
    {
        using SetThreadDescriptionFunction =
            HRESULT(WINAPI*)(
                HANDLE thread,
                PCWSTR description);

        [[nodiscard]] SetThreadDescriptionFunction
            ResolveSetThreadDescription() noexcept
        {
            const HMODULE kernelModule =
                ::GetModuleHandleW(
                    L"Kernel32.dll");

            if (kernelModule == nullptr)
            {
                return nullptr;
            }

            const FARPROC function =
                ::GetProcAddress(
                    kernelModule,
                    "SetThreadDescription");

            if (function == nullptr)
            {
                return nullptr;
            }

            return reinterpret_cast<
                SetThreadDescriptionFunction>(
                    function);
        }
    }

    void SleepForMilliseconds(
        const std::uint32_t milliseconds) noexcept
    {
        ::Sleep(
            static_cast<DWORD>(
                milliseconds));
    }

    void YieldCurrentThread() noexcept
    {
        if (::SwitchToThread() == FALSE)
        {
            ::Sleep(0);
        }
    }

    std::uint32_t GetCurrentThreadId() noexcept
    {
        return static_cast<std::uint32_t>(
            ::GetCurrentThreadId());
    }

    bool SetCurrentThreadName(
        const wchar_t* name) noexcept
    {
        if (name == nullptr ||
            name[0] == L'\0')
        {
            return false;
        }

        static const SetThreadDescriptionFunction
            setThreadDescription =
                ResolveSetThreadDescription();

        if (setThreadDescription == nullptr)
        {
            return false;
        }

        const HRESULT result =
            setThreadDescription(
                ::GetCurrentThread(),
                name);

        return SUCCEEDED(result);
    }
}