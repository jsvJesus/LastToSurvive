#pragma once

#include <array>
#include <cstdint>

namespace lts::editor
{
    enum class ContentAssetPayloadKind : std::uint8_t
    {
        StaticMesh,
        StaticMeshPrefab
    };

    struct ContentAssetPayload final
    {
        ContentAssetPayloadKind kind =
            ContentAssetPayloadKind::StaticMesh;

        std::array<char, 1024U> gamePath{};
    };

    inline constexpr char ContentAssetPayloadType[] =
        "LTS_CONTENT_ASSET";
}