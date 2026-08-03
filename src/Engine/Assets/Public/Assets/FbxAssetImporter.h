#pragma once

#include "Assets/AssetResult.h"
#include "Assets/FbxAssetData.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine::assets
{
    struct FbxImportOptions final
    {
        /*
         * UE5 FBX is converted into the engine coordinate system:
         *
         * - right-handed
         * - Y up
         * - meters
         */
        float animationSampleRate = 30.0F;

        std::uint32_t maximumBoneInfluences = 8U;

        bool importStaticMeshes = true;
        bool importSkeletalMeshes = true;
        bool importAnimations = true;
    };

    class FbxAssetImporter final
    {
    public:
        FbxAssetImporter() = delete;

        [[nodiscard]]
        static bool IsSupportedSource(
            const std::filesystem::path&
                sourcePath) noexcept;
        
        [[nodiscard]]
        static AssetResult Import(
            const std::filesystem::path& sourcePath,
            const FbxImportOptions& options,
            FbxImportedScene& output,
            std::wstring& error,
            std::vector<std::wstring>* warnings =
                nullptr) noexcept;
    };
}