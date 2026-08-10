#include "Editor/Application/Application.h"
#include "Editor/LevelEditor/Terrain/TerrainImporter.h"
#include <Assets/LegacyTerrain2Importer.h>
#include <Assets/TerrainAsset.h>

#include <Windows.h>
#include <Shellapi.h>

#include <string>

int WINAPI wWinMain(
    HINSTANCE,
    HINSTANCE,
    PWSTR,
    int)
{
    int argumentCount = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    if (arguments != nullptr && argumentCount == 4 &&
        std::wstring_view(arguments[1]) == L"--import-terrain2")
    {
        const auto result = engine::assets::LegacyTerrain2Importer::Import(
            arguments[2], arguments[3]);
        LocalFree(arguments);
        return static_cast<int>(result);
    }
    if (arguments != nullptr && argumentCount == 3 &&
        std::wstring_view(arguments[1]) == L"--validate-terrain")
    {
        engine::assets::TerrainAsset terrain;
        const auto result = engine::assets::TerrainAsset::Load(arguments[2], terrain);
        LocalFree(arguments);
        return static_cast<int>(result);
    }
    if (arguments != nullptr && argumentCount == 4 &&
        std::wstring_view(arguments[1]) == L"--import-r16")
    {
        lts::editor::R16TerrainImportSettings settings;
        float terrainCenterHeight = 0.0F;
        std::string status;
        const bool detected = lts::editor::DetectR16TerrainImportSettings(
            arguments[2], settings, terrainCenterHeight, status);

        if (detected)
        {
            settings.destinationPath = arguments[3];
        }

        const bool imported = detected &&
            lts::editor::WriteR16TerrainAsset(settings, status);
        LocalFree(arguments);
        return imported ? 0 : 1;
    }
    if (arguments != nullptr) LocalFree(arguments);

    lts::editor::Application application;

    const auto result =
        application.Run();

    return static_cast<int>(result);
}
