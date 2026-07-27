#pragma once

#include "Editor/Tools/Import/LegacySkeletalReader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    enum class LegacyMeshFormat : std::uint8_t
    {
        Unknown = 0,
        Scb,
        Sco
    };

    struct LegacyMeshVertex final
    {
        std::array<float, 3U> position{};
        std::array<float, 3U> normal{};
        std::array<float, 3U> tangent{};

        std::array<float, 2U> uv{};

        float tangentSign = 1.0F;
    };

    struct LegacyMaterialChunk final
    {
        std::string materialName;

        std::uint32_t firstIndex = 0U;
        std::uint32_t indexCount = 0U;
    };

    struct LegacyMeshData final
    {
        std::filesystem::path sourcePath;

        LegacyMeshFormat format =
            LegacyMeshFormat::Unknown;

        std::string name;

        std::array<float, 3U> pivot{};

        std::vector<LegacyMeshVertex> vertices;
        std::vector<std::uint32_t> indices;

        std::vector<LegacyMaterialChunk> materialChunks;

        LegacyWeightData embeddedWeights;

        std::size_t invalidIndexCount = 0U;
        std::size_t degenerateTriangleCount = 0U;
        std::size_t nonFiniteVertexCount = 0U;
        std::size_t uvConflictCount = 0U;
        std::size_t invalidMaterialRangeCount = 0U;
        std::size_t trailingByteCount = 0U;

        bool hasEmbeddedWeights = false;
        bool embeddedWeightVertexCountMismatch = false;
        bool hasVertexColors = false;
        bool indexCountNotTriangleList = false;
        bool usedScoFallback = false;

        std::string warning;
        std::string error;
    };

    class LegacyMeshReader final
    {
    public:
        [[nodiscard]]
        static bool Read(
            const std::filesystem::path& scbPath,
            const std::filesystem::path& scoPath,
            const LegacySkeletonData* skeleton,
            LegacyMeshData& output) noexcept;

    private:
        [[nodiscard]]
        static bool ReadScb(
            const std::filesystem::path& path,
            const LegacySkeletonData* skeleton,
            LegacyMeshData& output) noexcept;

        [[nodiscard]]
        static bool ReadSco(
            const std::filesystem::path& path,
            LegacyMeshData& output) noexcept;
    };
}