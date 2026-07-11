#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

#include "Platform/Path.h"

#include <algorithm>
#include <system_error>
#include <vector>

namespace engine::platform
{
    Path GetExecutablePath()
    {
        constexpr std::size_t initialBufferSize = 512;
        constexpr std::size_t maximumBufferSize = 32768;

        std::vector<wchar_t> buffer(initialBufferSize);

        while (buffer.size() <= maximumBufferSize)
        {
            const DWORD bufferLength =
                static_cast<DWORD>(buffer.size());

            const DWORD characterCount =
                ::GetModuleFileNameW(
                    nullptr,
                    buffer.data(),
                    bufferLength);

            if (characterCount == 0)
            {
                return {};
            }

            if (characterCount < bufferLength)
            {
                return Path(
                    std::wstring(
                        buffer.data(),
                        static_cast<std::size_t>(characterCount)));
            }

            if (buffer.size() == maximumBufferSize)
            {
                return {};
            }

            const std::size_t newSize =
                std::min(
                    buffer.size() * 2,
                    maximumBufferSize);

            buffer.resize(newSize);
        }

        return {};
    }

    Path GetExecutableDirectory()
    {
        const Path executablePath = GetExecutablePath();

        if (executablePath.empty())
        {
            return {};
        }

        return executablePath.parent_path();
    }

    Path GetCurrentWorkingDirectory()
    {
        std::error_code error;

        Path result = std::filesystem::current_path(error);

        if (error)
        {
            return {};
        }

        return result;
    }

    Path MakeAbsolute(const Path& path)
    {
        if (path.empty())
        {
            return {};
        }

        std::error_code error;

        Path absolutePath =
            std::filesystem::absolute(path, error);

        if (error)
        {
            return {};
        }

        return absolutePath.lexically_normal();
    }

    Path NormalizePath(const Path& path)
    {
        if (path.empty())
        {
            return {};
        }

        return path.lexically_normal();
    }

    bool PathExists(const Path& path) noexcept
    {
        if (path.empty())
        {
            return false;
        }

        std::error_code error;

        const bool exists =
            std::filesystem::exists(path, error);

        return !error && exists;
    }

    bool IsDirectory(const Path& path) noexcept
    {
        if (path.empty())
        {
            return false;
        }

        std::error_code error;

        const bool isDirectory =
            std::filesystem::is_directory(path, error);

        return !error && isDirectory;
    }
}