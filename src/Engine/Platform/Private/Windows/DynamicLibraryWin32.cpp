#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/DynamicLibrary.h"

#include <utility>

namespace engine::platform
{
    namespace
    {
        [[nodiscard]] HMODULE ToNativeModule(
            const std::uintptr_t nativeHandle) noexcept
        {
            return reinterpret_cast<HMODULE>(nativeHandle);
        }

        [[nodiscard]] std::uintptr_t FromNativeModule(
            const HMODULE module) noexcept
        {
            return reinterpret_cast<std::uintptr_t>(module);
        }
    }

    DynamicLibrary::DynamicLibrary(const Path& path)
    {
        Load(path);
    }

    DynamicLibrary::~DynamicLibrary() noexcept
    {
        Unload();
    }

    DynamicLibrary::DynamicLibrary(
        DynamicLibrary&& other) noexcept
        : nativeHandle_(
              std::exchange(other.nativeHandle_, 0)),
          loadedPath_(
              std::move(other.loadedPath_)),
          lastErrorCode_(
              std::exchange(other.lastErrorCode_, 0))
    {
    }

    DynamicLibrary& DynamicLibrary::operator=(
        DynamicLibrary&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        Unload();

        nativeHandle_ =
            std::exchange(other.nativeHandle_, 0);

        loadedPath_ =
            std::move(other.loadedPath_);

        lastErrorCode_ =
            std::exchange(other.lastErrorCode_, 0);

        return *this;
    }

    bool DynamicLibrary::Load(const Path& path)
    {
        Unload();

        if (path.empty())
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return false;
        }

        const HMODULE module =
            ::LoadLibraryW(path.c_str());

        if (module == nullptr)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return false;
        }

        nativeHandle_ =
            FromNativeModule(module);

        loadedPath_ = path;
        lastErrorCode_ = ERROR_SUCCESS;

        return true;
    }

    void DynamicLibrary::Unload() noexcept
    {
        if (nativeHandle_ == 0)
        {
            loadedPath_.clear();
            lastErrorCode_ = ERROR_SUCCESS;
            return;
        }

        const HMODULE module =
            ToNativeModule(nativeHandle_);

        if (::FreeLibrary(module) == FALSE)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());
        }
        else
        {
            lastErrorCode_ = ERROR_SUCCESS;
        }

        nativeHandle_ = 0;
        loadedPath_.clear();
    }

    bool DynamicLibrary::IsLoaded() const noexcept
    {
        return nativeHandle_ != 0;
    }

    DynamicLibrary::operator bool() const noexcept
    {
        return IsLoaded();
    }

    DynamicFunction DynamicLibrary::FindFunction(
        const char* functionName) const noexcept
    {
        if (nativeHandle_ == 0 ||
            functionName == nullptr ||
            functionName[0] == '\0')
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ERROR_INVALID_PARAMETER);

            return nullptr;
        }

        const HMODULE module =
            ToNativeModule(nativeHandle_);

        const FARPROC function =
            ::GetProcAddress(
                module,
                functionName);

        if (function == nullptr)
        {
            lastErrorCode_ =
                static_cast<std::uint32_t>(
                    ::GetLastError());

            return nullptr;
        }

        lastErrorCode_ = ERROR_SUCCESS;

        return reinterpret_cast<DynamicFunction>(
            function);
    }

    bool DynamicLibrary::HasFunction(
        const char* functionName) const noexcept
    {
        return FindFunction(functionName) != nullptr;
    }

    const Path& DynamicLibrary::GetPath() const noexcept
    {
        return loadedPath_;
    }

    std::uint32_t DynamicLibrary::GetLastErrorCode() const noexcept
    {
        return lastErrorCode_;
    }
}