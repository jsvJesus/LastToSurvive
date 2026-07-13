#pragma once

#include "Assets/AssetData.h"
#include "Assets/AssetResult.h"
#include "Assets/TextureAsset.h"

namespace engine::assets
{
    struct DdsTextureDecodeOptions final
    {
        // Принудительно преобразует подходящие линейные форматы
        // в их sRGB-вариант.
        bool forceSrgb = false;

        // BC7 работает в DX11, но не поддерживается DX9 backend.
        bool allowBc7 = true;
    };

    class DdsTextureDecoder final
    {
    public:
        [[nodiscard]] static bool IsDds(
            const AssetData& source) noexcept;

        [[nodiscard]] static AssetResult Decode(
            const AssetData& source,
            TextureAsset& outTexture) noexcept;

        [[nodiscard]] static AssetResult Decode(
            const AssetData& source,
            const DdsTextureDecodeOptions& options,
            TextureAsset& outTexture) noexcept;
    };
}