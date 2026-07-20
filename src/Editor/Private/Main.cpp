#include "Editor/EditorApplication.h"
#include <Assets/LegacyTerrain2Importer.h>
#include <Assets/TerrainAsset.h>

#include <Windows.h>
#include <Shellapi.h>

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
    if (arguments != nullptr) LocalFree(arguments);

    lts::editor::EditorApplication application;

    const auto result =
        application.Run();

    return static_cast<int>(result);
}
