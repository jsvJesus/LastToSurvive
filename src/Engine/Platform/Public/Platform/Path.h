#pragma once

#include <filesystem>

namespace engine::platform
{
    using Path = std::filesystem::path;

    [[nodiscard]] Path GetExecutablePath();

    [[nodiscard]] Path GetExecutableDirectory();

    [[nodiscard]] Path GetCurrentWorkingDirectory();

    [[nodiscard]] Path MakeAbsolute(const Path& path);

    [[nodiscard]] Path NormalizePath(const Path& path);

    [[nodiscard]] bool PathExists(const Path& path) noexcept;

    [[nodiscard]] bool IsDirectory(const Path& path) noexcept;
}