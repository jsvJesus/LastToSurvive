#pragma once

#include "Assets/AssetData.h"
#include "Assets/MeshAsset.h"

#include <array>
#include <string>
#include <vector>

namespace engine::legacy::assets
{
    struct LegacyStaticMeshData final
    {
        engine::assets::MeshAsset mesh;
        std::string sourceName;
        std::array<float, 3U> pivot{};
        std::vector<std::string> materialSlotNames;

        void Clear() noexcept;
        [[nodiscard]] bool IsEmpty() const noexcept;
    };

    class LegacyScbMeshDecoder final
    {
    public:
        static constexpr std::uint32_t SupportedVersion = 0xFADC0038U;
        [[nodiscard]] static engine::assets::AssetResult Decode(
            const engine::assets::AssetData&, LegacyStaticMeshData&) noexcept;
    };
}
