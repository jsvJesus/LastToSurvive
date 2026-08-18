#pragma once

#include <filesystem>
#include <string>

namespace lts::editor
{
    class Launcher final
    {
    public:
        Launcher() = delete;

        [[nodiscard]]
        static bool Launch(
            const std::filesystem::path& levelPath,
            std::wstring& error) noexcept;
    };
}