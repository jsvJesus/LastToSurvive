#pragma once

#include "Assets/AssetId.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetType.h"

#include <cstdint>

namespace engine::assets
{
    struct AssetMetadata final
    {
        AssetId id;
        AssetPath path;

        AssetType type = AssetType::Unknown;

        std::uint32_t schemaVersion = 1U;

        std::uint64_t sourceSize = 0U;
        std::uint64_t sourceTimestamp = 0U;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return
                id.IsValid() &&
                path.IsValid() &&
                type != AssetType::Unknown &&
                id == path.GetId() &&
                schemaVersion != 0U;
        }
    };
}