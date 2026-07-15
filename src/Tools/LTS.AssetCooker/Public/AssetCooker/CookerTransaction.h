#pragma once

#include "Assets/AssetResult.h"

#include <filesystem>
#include <string>

namespace lts::asset_cooker
{
    struct CookPaths final
    {
        std::filesystem::path input;
        std::filesystem::path dataRoot;
        std::filesystem::path outputRoot;
        std::filesystem::path temporary;
        std::filesystem::path backup;
    };

    enum class TransactionTestFailure : std::uint8_t
    {
        None = 0,
        AfterBackup,
        AfterBackupAndRollback
    };

    [[nodiscard]] engine::assets::AssetResult ValidateCookPaths(
        const std::filesystem::path& input,
        const std::filesystem::path& dataRoot,
        const std::filesystem::path& outputRoot,
        std::wstring_view nonce,
        CookPaths& outPaths,
        std::string& outError) noexcept;
    [[nodiscard]] engine::assets::AssetResult PrepareTemporaryDirectory(
        const CookPaths& paths,
        std::string& outError) noexcept;
    [[nodiscard]] engine::assets::AssetResult CommitDirectory(
        const CookPaths& paths,
        bool force,
        std::string& outError,
        TransactionTestFailure testFailure = TransactionTestFailure::None) noexcept;
}
