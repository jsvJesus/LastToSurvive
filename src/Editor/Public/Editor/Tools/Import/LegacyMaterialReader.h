#pragma once

#include "Editor/Tools/Import/LegacyMeshReader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace lts::editor
{
    enum class LegacyTextureSlot : std::uint8_t
    {
        Diffuse = 0,
        Normal,
        Specular,
        Roughness,
        Glow,
        DetailNormal,
        Density,
        CamouflageMask,
        Distortion,
        SpecularPower,
        Count
    };

    [[nodiscard]]
    const char* ToString(
        LegacyTextureSlot slot) noexcept;

    struct LegacyDdsInfo final
    {
        std::filesystem::path path;

        std::uint64_t fileSize = 0U;

        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        std::uint32_t mipCount = 0U;
        std::uint32_t arraySize = 1U;

        std::string format;
        std::string error;

        bool exists = false;
        bool valid = false;
        bool hasAlpha = false;
        bool isCubeMap = false;
    };

    struct LegacyMaterialTexture final
    {
        LegacyTextureSlot slot =
            LegacyTextureSlot::Diffuse;

        std::string sourceName;

        LegacyDdsInfo dds;
    };

    struct LegacyMaterialData final
    {
        std::filesystem::path sourcePath;
        std::filesystem::path imagesDirectory;

        std::string name;
        std::string typeName;
        std::string imagesDirectorySource;

        std::array<float, 3U> diffuseColor
        {
            1.0F,
            1.0F,
            1.0F
        };

        float specularPower = 0.0F;
        float specularPower1 = 0.5F;
        float reflectionPower = 0.0F;

        float detailScale = 10.0F;
        float detailAmount = 0.3F;

        float displacementDepth = 0.1F;

        float lowQualitySelfIllumination = 0.0F;
        float lowQualityMetalness = 0.0F;
        float selfIlluminationMultiplier = 0.0F;

        bool displacement = false;
        bool transparent = false;
        bool forceAlpha = false;
        bool doubleSided = false;
        bool camouflage = false;

        std::size_t parseWarningCount = 0U;

        std::array<
            LegacyMaterialTexture,
            static_cast<std::size_t>(
                LegacyTextureSlot::Count)> textures{};

        std::string error;

        [[nodiscard]]
        const LegacyMaterialTexture* FindTexture(
            LegacyTextureSlot slot) const noexcept;

        [[nodiscard]]
        LegacyMaterialTexture* FindTexture(
            LegacyTextureSlot slot) noexcept;
    };

    struct LegacyMaterialSet final
    {
        std::vector<LegacyMaterialData> materials;

        std::vector<std::string>
            missingMaterialNames;

        std::size_t missingMaterialCount = 0U;
        std::size_t missingTextureCount = 0U;
        std::size_t invalidDdsCount = 0U;
        std::size_t parseWarningCount = 0U;

        std::string error;

        [[nodiscard]]
        const LegacyMaterialData* Find(
            std::string_view materialName) const noexcept;
    };

    class LegacyMaterialReader final
    {
    public:
        [[nodiscard]]
        static bool ReadForMesh(
            const std::filesystem::path& sourceRoot,
            const std::filesystem::path& packageDirectory,
            const std::vector<LegacyMaterialChunk>& chunks,
            LegacyMaterialSet& output) noexcept;
    };
}