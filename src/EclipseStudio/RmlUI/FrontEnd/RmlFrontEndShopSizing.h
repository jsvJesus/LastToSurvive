#pragma once

#include <stdint.h>

#include "ObjectsCode/weapons/BaseItemConfig.h"

namespace RmlFrontEndShopSizing
{
    constexpr int ShopGridColumns =
        12;

    constexpr int ShopGridCellSize =
        64;

    struct FShopGridSize
    {
        int Width = 1;
        int Height = 1;
    };

    FShopGridSize GetDefaultShopGridSize(
        int Category,
        uint32_t ItemId
    );

    FShopGridSize GetShopGridSize(
        const BaseItemConfig* Config,
        int Category
    );

    const char* GetShopItemSizeClass(
        const FShopGridSize& Size
    );

    int GetShopIconWidthPx(
        const FShopGridSize& Size
    );

    int GetShopIconHeightPx(
        const FShopGridSize& Size
    );
}