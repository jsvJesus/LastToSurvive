#pragma once

#include <cstdint>

namespace engine::assets
{
    enum class AssetType : std::uint8_t
    {
        Unknown = 0,

        Texture,
        Mesh,
        Material,
        Shader,
        Animation,
        Effect,
        Audio,
        Data,
        StaticModel,
        SkeletalMesh
    };

    [[nodiscard]] const char* ToString(
        AssetType type) noexcept;
}
