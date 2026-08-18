#pragma once

#include "Assets/AssetResult.h"
#include "Assets/FbxAssetData.h"

#include <filesystem>
#include <string>

namespace engine::assets
{
    struct FbxAnimationImportOptions final
    {
        std::filesystem::path destinationDirectory;

        /*
         * Существующий или только что созданный
         * файл .skeleton.
         */
        std::filesystem::path skeletonFile;

        float sampleRate = 30.0F;

        bool overwriteExisting = true;
    };

    class FbxAnimationsImporter final
    {
    public:
        FbxAnimationsImporter() = delete;

        [[nodiscard]]
        static bool IsSupportedSource(
            const std::filesystem::path&
                sourcePath) noexcept;

        [[nodiscard]]
        static AssetResult Import(
            const std::filesystem::path& sourcePath,
            const FbxAnimationImportOptions&
                options,
            FbxImportReport& report,
            std::wstring& error) noexcept;
    };
}