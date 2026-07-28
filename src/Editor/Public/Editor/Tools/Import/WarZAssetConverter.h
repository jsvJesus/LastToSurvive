#pragma once

#include "Editor/Tools/Import/LegacyAnimationReader.h"
#include "Editor/Tools/Import/LegacyMaterialReader.h"
#include "Editor/Tools/Import/LegacyMeshReader.h"
#include "Editor/Tools/Import/LegacySkeletalReader.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    struct WarZConversionRequest final
    {
        /*
         * Абсолютный путь к game/Data.
         */
        std::filesystem::path dataRoot;

        /*
         * Корень оригинального WarZ bin/Data.
         */
        std::filesystem::path sourceRoot;

        /*
         * Например:
         *
         * Characters/char_lms_body_01
         */
        std::filesystem::path packageRelativePath;

        std::filesystem::path sourceSkeletonPath;

        const LegacyMeshData* mesh = nullptr;
        const LegacySkeletonData* skeleton = nullptr;
        const LegacyWeightData* weights = nullptr;
        const LegacyMaterialSet* materials = nullptr;

        const std::vector<std::filesystem::path>*
            animationPaths = nullptr;

        bool writeSkeletalMesh = true;
        bool writeSkeleton = true;
        bool writeMaterials = true;
        bool writeTextures = true;
        bool writeAnimations = true;
    };

    struct WarZConversionResult final
    {
        std::filesystem::path skeletalMeshPath;
        std::filesystem::path skeletonPath;

        std::size_t materialCount = 0U;
        std::size_t textureCount = 0U;
        std::size_t animationCount = 0U;

        std::vector<std::string> warnings;

        std::string error;
        bool success = false;
    };

    class WarZAssetConverter final
    {
    public:
        [[nodiscard]]
        static bool Convert(
            const WarZConversionRequest& request,
            WarZConversionResult& result) noexcept;

        [[nodiscard]]
        static std::filesystem::path
            BuildSkeletonRelativePath(
                const std::filesystem::path&
                    sourceSkeletonPath);
    };
}