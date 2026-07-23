#pragma once

#include <filesystem>
#include <string>

namespace lts::editor
{
    class TestGameLauncher final
    {
    public:
        TestGameLauncher() = delete;

        [[nodiscard]]
        static bool Launch(
            const std::filesystem::path& levelPath,
            std::wstring& error) noexcept;
    };
}