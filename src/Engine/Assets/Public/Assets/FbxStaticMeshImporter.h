#pragma once

#include "Assets/AssetResult.h"
#include "Assets/FbxAssetData.h"

#include <filesystem>
#include <string>

namespace engine::assets
{
    struct FbxStaticMeshImportOptions final
    {
        std::filesystem::path destinationDirectory;

        /*
         * Если FBX содержит несколько static mesh nodes,
         * рядом с .sm будет создан общий .prefab.
         */
        bool createPrefab = true;

        /*
         * Создавать prefab даже для одного .sm.
         */
        bool createSingleMeshPrefab = false;

        bool overwriteExisting = true;
    };

    class FbxStaticMeshImporter final
    {
    public:
        FbxStaticMeshImporter() = delete;

        [[nodiscard]]
        static bool IsSupportedSource(
            const std::filesystem::path&
                sourcePath) noexcept;

        [[nodiscard]]
        static AssetResult Import(
            const std::filesystem::path& sourcePath,
            const FbxStaticMeshImportOptions&
                options,
            FbxImportReport& report,
            std::wstring& error) noexcept;
    };
}