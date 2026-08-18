#pragma once

#include "Assets/AssetResult.h"
#include "Assets/FbxAssetData.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace engine::assets
{
    struct FbxSkeletalMeshImportOptions final
    {
        std::filesystem::path destinationDirectory;

        /*
         * Если пусто и writeSkeleton == true:
         * используется <FBX name>.skeleton.
         *
         * Если writeSkeleton == false:
         * здесь должен быть существующий .skeleton.
         */
        std::filesystem::path skeletonFile;

        bool writeSkeleton = true;
        bool writeMeshes = true;
        bool overwriteExisting = true;

        /*
         * Runtime .skm хранит максимум 4 влияния.
         */
        std::uint32_t maximumBoneInfluences = 4U;
    };

    class FbxSkeletalMeshImporter final
    {
    public:
        FbxSkeletalMeshImporter() = delete;

        [[nodiscard]]
        static bool IsSupportedSource(
            const std::filesystem::path&
                sourcePath) noexcept;

        [[nodiscard]]
        static AssetResult Import(
            const std::filesystem::path& sourcePath,
            const FbxSkeletalMeshImportOptions&
                options,
            FbxImportReport& report,
            std::filesystem::path&
                outputSkeletonFile,
            std::string&
                outputSkeletonAssetPath,
            std::wstring& error) noexcept;
    };
}