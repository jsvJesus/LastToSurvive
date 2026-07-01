#pragma once

class RmlFrontEndShopIconBaker
{
public:
    static bool WriteBakeList(
        const char* OutputFileName
    );

    static bool CopyExistingStoreIcons(
        const char* OutputDirectory,
        bool bOverwriteExisting
    );

    static bool BakeResizedStoreIcons(
        const char* OutputDirectory,
        bool bOverwriteExisting
    );

    static bool BakeSingleItem3D(
        uint32_t ItemId,
        const char* OutputDirectory,
        int Width,
        int Height,
        bool bOverwriteExisting
    );
};