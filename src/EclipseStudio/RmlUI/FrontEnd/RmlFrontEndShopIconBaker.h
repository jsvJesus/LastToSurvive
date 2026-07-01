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
};