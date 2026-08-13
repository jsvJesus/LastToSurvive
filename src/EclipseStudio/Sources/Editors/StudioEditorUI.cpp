#include "StudioEditorUI.h"
#include "LegacyLevelDataLoader.h"
#include "LegacyLevelDataWriter.h"
#include "ObjectViewTab.h"
#include "StudioToolbar.h"

#include <Editor/Commands/CommandHistory.h>
#include <Editor/LevelEditor/Rendering/ColorCorrectionRenderer.h>
#include <Editor/LevelEditor/Rendering/GridRenderer.h>
#include <Editor/LevelEditor/Rendering/SkyRenderer.h>
#include <Editor/LevelEditor/Rendering/StaticMeshRenderer.h>
#include <Editor/LevelEditor/Scene/SceneDocument.h>
#include <Editor/LevelEditor/Terrain/TerrainImporter.h>
#include <Editor/LevelEditor/Terrain/TerrainRenderer.h>
#include <Editor/LevelEditor/Viewport/CameraController.h>

#include <Assets/AssetResult.h>
#include <Assets/TerrainAsset.h>
#include <Graphics/CommandContext.h>
#include <Graphics/RenderDevice.h>

#include <imgui.h>

#include <Windows.h>
#include <ShObjIdl.h>
#include <Shellapi.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <chrono>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace studio::editor
{
    namespace
    {
        LevelEditorPage g_activePage =
            LevelEditorPage::Settings;

        enum class EditorGraphicsQuality : std::uint8_t
        {
            Low = 0,
            Medium,
            High,
            Custom
        };

        StudioToolbar g_editorToolbar;

        SettingsToolbarPage g_activeSettingsPage =
            SettingsToolbarPage::SystemSettings;

        EditorGraphicsQuality g_graphicsQuality =
            EditorGraphicsQuality::High;

        bool g_simulateDayNight = false;

        engine::graphics::RenderDevice* g_device = nullptr;
        lts::editor::SceneDocument g_sceneDocument;
        lts::editor::CommandHistory g_commandHistory;
        lts::editor::CameraController g_cameraController;
        lts::editor::SkyRenderer g_skyRenderer;
        lts::editor::GridRenderer g_gridRenderer;
        lts::editor::TerrainRenderer g_terrainRenderer;
        lts::editor::StaticMeshRenderer g_staticMeshRenderer;
        lts::editor::ColorCorrectionRenderer g_colorCorrectionRenderer;
        lts::editor::ColorCorrectionSettings g_colorCorrectionSettings;
        lts::editor::TerrainImporter g_terrainImporter;
        ObjectViewTab g_objectViewTab;
        std::filesystem::path g_loadedTerrainPath;
        std::filesystem::path g_loadedLevelDataPath;
        std::vector<std::size_t> g_managedLevelObjectIndices;
        std::string g_settingsSaveStatus;
        std::future<LegacyLevelLoadResult> g_levelLoadFuture;
        LegacyLevelLoadStats g_levelLoadStats;
        std::string g_levelLoadStatus = "No map selected.";
        std::string g_loadedMapName;
        engine::platform::NativeWindowHandle g_window;
        std::size_t g_activeLayer = 0U;
        bool g_initialized = false;

        TerrainToolbarPage g_activeTerrainPage =
            TerrainToolbarPage::TerrainLoader;

        struct TerrainMapEntry final
        {
            std::filesystem::path levelRoot;
            std::string displayName;

            bool hasLevelData = false;
            bool hasTerrain = false;

            [[nodiscard]]
            bool IsLoadable() const noexcept
            {
                return hasLevelData && hasTerrain;
            }
        };

        std::vector<TerrainMapEntry> g_availableTerrainMaps;
        int g_selectedTerrainMap = -1;
        bool g_terrainMapsScanned = false;

        std::string g_terrainMapScanStatus;
        std::string g_loadingMapName;

        [[nodiscard]] bool LoadTerrainAsset(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& path);

        [[nodiscard]] std::filesystem::path FindWorkspaceRoot()
        {
            std::error_code error;
            std::filesystem::path current = std::filesystem::current_path(error);

            if (error)
            {
                return {};
            }

            for (;;)
            {
                if (std::filesystem::is_directory(
                        current / L"bin" / L"Data" / L"TerrainData" / L"Materials",
                        error))
                {
                    return current;
                }

                error.clear();
                const std::filesystem::path parent = current.parent_path();

                if (parent.empty() || parent == current)
                {
                    return {};
                }

                current = parent;
            }
        }

        [[nodiscard]] std::string LowercaseAscii(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        std::string TrimAscii(
            std::string value)
        {
            const auto isSpace =
                [](const unsigned char character)
                {
                    return
                        std::isspace(character) != 0;
                };

            value.erase(
                value.begin(),
                std::find_if(
                    value.begin(),
                    value.end(),
                    [&](const char character)
                    {
                        return
                            !isSpace(
                                static_cast<unsigned char>(
                                    character));
                    }));

            value.erase(
                std::find_if(
                    value.rbegin(),
                    value.rend(),
                    [&](const char character)
                    {
                        return
                            !isSpace(
                                static_cast<unsigned char>(
                                    character));
                    }).
                    base(),
                value.end());

            return value;
        }

        [[nodiscard]]
        bool TryParseFiniteFloat(
            const std::string& text,
            float& output) noexcept
        {
            const std::string value =
                TrimAscii(text);

            if (value.empty())
            {
                return false;
            }

            char* end = nullptr;

            const float parsed =
                std::strtof(
                    value.c_str(),
                    &end);

            if (end == value.c_str())
            {
                return false;
            }

            while (
                *end != '\0' &&
                std::isspace(
                    static_cast<unsigned char>(
                        *end)) != 0)
            {
                ++end;
            }

            if (
                *end != '\0' ||
                !std::isfinite(parsed))
            {
                return false;
            }

            output = parsed;

            return true;
        }

        [[nodiscard]]
        std::string GetTerrainMapDisplayName(
            const std::filesystem::path& levelRoot)
        {
            std::string name =
                levelRoot.filename().generic_u8string();

            const std::string lowercase =
                LowercaseAscii(name);

            if (lowercase.rfind("wz_", 0U) == 0U)
            {
                name.erase(0U, 3U);
            }

            return name.empty()
                ? levelRoot.filename().generic_u8string()
                : name;
        }

        void RefreshAvailableTerrainMaps() noexcept
        {
            g_availableTerrainMaps.clear();
            g_selectedTerrainMap = -1;
            g_terrainMapsScanned = true;

            try
            {
                const std::filesystem::path workspace =
                    FindWorkspaceRoot();

                if (workspace.empty())
                {
                    g_terrainMapScanStatus =
                        "Workspace root was not found.";

                    return;
                }

                const std::filesystem::path levelsRoot =
                    workspace / L"bin" / L"Levels";

                std::error_code error;

                if (!std::filesystem::is_directory(
                        levelsRoot,
                        error) ||
                    error)
                {
                    g_terrainMapScanStatus =
                        "bin/Levels directory was not found.";

                    return;
                }

                std::filesystem::recursive_directory_iterator iterator(
                    levelsRoot,
                    std::filesystem::directory_options::
                        skip_permission_denied,
                    error);

                const std::filesystem::recursive_directory_iterator end;

                while (!error && iterator != end)
                {
                    std::error_code entryError;

                    if (iterator->is_directory(entryError) &&
                        !entryError)
                    {
                        const std::filesystem::path levelRoot =
                            iterator->path();

                        std::error_code levelDataError;
                        std::error_code terrainError;

                        const bool hasLevelData =
                            std::filesystem::is_regular_file(
                                levelRoot / L"LevelData.xml",
                                levelDataError) &&
                            !levelDataError;

                        const bool hasTerrain =
                            std::filesystem::is_regular_file(
                                levelRoot /
                                    L"Terrain" /
                                    L"Terrain.terrain",
                                terrainError) &&
                            !terrainError;

                        if (hasLevelData || hasTerrain)
                        {
                            TerrainMapEntry map;
                            map.levelRoot =
                                levelRoot.lexically_normal();
                            map.displayName =
                                GetTerrainMapDisplayName(levelRoot);
                            map.hasLevelData = hasLevelData;
                            map.hasTerrain = hasTerrain;

                            g_availableTerrainMaps.push_back(
                                std::move(map));

                            iterator.disable_recursion_pending();
                        }
                        else if (iterator.depth() >= 1)
                        {
                            /*
                             * Разрешаем один вложенный уровень:
                             * Levels/WorkInProgress/MyLevel.
                             */
                            iterator.disable_recursion_pending();
                        }
                    }

                    iterator.increment(error);
                }

                std::sort(
                    g_availableTerrainMaps.begin(),
                    g_availableTerrainMaps.end(),
                    [](const TerrainMapEntry& left,
                       const TerrainMapEntry& right)
                    {
                        return
                            LowercaseAscii(left.displayName) <
                            LowercaseAscii(right.displayName);
                    });

                if (!g_availableTerrainMaps.empty())
                {
                    g_selectedTerrainMap = 0;
                }

                g_terrainMapScanStatus =
                    "Found " +
                    std::to_string(
                        g_availableTerrainMaps.size()) +
                    " level folder(s).";
            }
            catch (...)
            {
                g_availableTerrainMaps.clear();
                g_selectedTerrainMap = -1;

                g_terrainMapScanStatus =
                    "Level directory scan failed.";
            }
        }

        [[nodiscard]]
        bool LoadTerrainTransformFromIni(
            const std::filesystem::path& terrainPath,
            lts::editor::EditorTransform& transform) noexcept
        {
            transform =
                lts::editor::EditorTransform{};

            try
            {
                const std::filesystem::path iniPath =
                    terrainPath.parent_path() /
                    L"Terrain.ini";

                std::error_code filesystemError;

                const bool iniExists =
                    std::filesystem::is_regular_file(
                        iniPath,
                        filesystemError);

                if (filesystemError)
                {
                    return false;
                }

                /*
                 * РЎС‚Р°СЂС‹Рµ .terrain РјРѕРіСѓС‚ РЅРµ РёРјРµС‚СЊ Terrain.ini.
                 * Р”Р»СЏ РЅРёС… СЃРѕС…СЂР°РЅСЏРµРј РїСЂРµР¶РЅРёР№ transform:
                 * position = 0, scale = 1.
                 */
                if (!iniExists)
                {
                    return true;
                }

                std::ifstream input(iniPath);

                if (!input)
                {
                    return false;
                }

                float centerHeight = 0.0F;
                float scaleX = 1.0F;
                float scaleY = 1.0F;
                float scaleZ = 1.0F;

                bool hasCenterHeight = false;
                bool hasScaleX = false;
                bool hasScaleY = false;
                bool hasScaleZ = false;

                std::string line;

                while (std::getline(input, line))
                {
                    line =
                        TrimAscii(
                            std::move(line));

                    if (
                        line.empty() ||
                        line.front() == '#' ||
                        line.front() == ';' ||
                        line.front() == '[')
                    {
                        continue;
                    }

                    const std::size_t separator =
                        line.find('=');

                    if (separator ==
                        std::string::npos)
                    {
                        continue;
                    }

                    const std::string key =
                        LowercaseAscii(
                            TrimAscii(
                                line.substr(
                                    0U,
                                    separator)));

                    const std::string value =
                        TrimAscii(
                            line.substr(
                                separator + 1U));

                    float parsed = 0.0F;

                    if (key == "centerheight")
                    {
                        if (!TryParseFiniteFloat(
                                value,
                                parsed))
                        {
                            return false;
                        }

                        centerHeight = parsed;
                        hasCenterHeight = true;
                    }
                    else if (key == "scalex")
                    {
                        if (!TryParseFiniteFloat(
                                value,
                                parsed))
                        {
                            return false;
                        }

                        scaleX = parsed;
                        hasScaleX = true;
                    }
                    else if (key == "scaley")
                    {
                        if (!TryParseFiniteFloat(
                                value,
                                parsed))
                        {
                            return false;
                        }

                        scaleY = parsed;
                        hasScaleY = true;
                    }
                    else if (key == "scalez")
                    {
                        if (!TryParseFiniteFloat(
                                value,
                                parsed))
                        {
                            return false;
                        }

                        scaleZ = parsed;
                        hasScaleZ = true;
                    }
                }

                const bool hasAnyTransformValue =
                    hasCenterHeight ||
                    hasScaleX ||
                    hasScaleY ||
                    hasScaleZ;

                /*
                 * Terrain.ini, СЃРѕР·РґР°РЅРЅС‹Р№ РЅРѕРІС‹Рј R16 importer,
                 * РѕР±СЏР·Р°РЅ СЃРѕРґРµСЂР¶Р°С‚СЊ РїРѕР»РЅС‹Р№ transform.
                 *
                 * Р•СЃР»Рё РІ СЃС‚Р°СЂРѕРј Terrain.ini РЅРµС‚ РЅРё РѕРґРЅРѕРіРѕ
                 * transform-РїРѕР»СЏ, РёСЃРїРѕР»СЊР·СѓРµРј default transform.
                 */
                if (
                    hasAnyTransformValue &&
                    (
                        !hasCenterHeight ||
                        !hasScaleX ||
                        !hasScaleY ||
                        !hasScaleZ
                    ))
                {
                    return false;
                }

                if (
                    scaleX <= 0.0F ||
                    scaleY <= 0.0F ||
                    scaleZ <= 0.0F)
                {
                    return false;
                }

                transform.position[1] =
                    centerHeight;

                transform.scale =
                {
                    scaleX,
                    scaleY,
                    scaleZ
                };

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]] bool SelectTerrainTexture(std::string& logicalPath)
        {
            Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;

            if (FAILED(CoCreateInstance(
                    CLSID_FileOpenDialog,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&dialog))))
            {
                return false;
            }

            constexpr COMDLG_FILTERSPEC filters[]
            {
                {L"DirectDraw Surface (*.dds)", L"*.dds"},
                {L"All files (*.*)", L"*.*"}
            };

            if (FAILED(dialog->SetFileTypes(2U, filters)) ||
                FAILED(dialog->SetTitle(L"Select terrain layer texture")))
            {
                return false;
            }

            const std::filesystem::path workspace = FindWorkspaceRoot();
            const std::filesystem::path materialRoot =
                workspace / L"bin" / L"Data" / L"TerrainData" / L"Materials";
            Microsoft::WRL::ComPtr<IShellItem> defaultFolder;

            if (SUCCEEDED(SHCreateItemFromParsingName(
                    materialRoot.c_str(),
                    nullptr,
                    IID_PPV_ARGS(&defaultFolder))))
            {
                static_cast<void>(dialog->SetDefaultFolder(defaultFolder.Get()));
            }

            const HWND owner = reinterpret_cast<HWND>(g_window.Value());

            if (FAILED(dialog->Show(owner)))
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IShellItem> item;
            PWSTR rawPath = nullptr;

            if (FAILED(dialog->GetResult(&item)) ||
                FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &rawPath)))
            {
                return false;
            }

            const std::filesystem::path selectedPath(rawPath);
            CoTaskMemFree(rawPath);
            std::error_code error;
            const std::filesystem::path relative = std::filesystem::relative(
                selectedPath,
                workspace / L"bin",
                error);

            logicalPath = error
                ? selectedPath.generic_u8string()
                : relative.generic_u8string();
            return true;
        }

        [[nodiscard]] bool IsLevelLoading() noexcept
        {
            return g_levelLoadFuture.valid();
        }

        struct EnvironmentEntities final
        {
            lts::editor::EditorSceneEntity* environment = nullptr;
            lts::editor::EditorSceneEntity* sun = nullptr;
        };

        struct DayNightKeyframe final
        {
            std::array<float, 3U> topColor{};
            std::array<float, 3U> horizonColor{};
            std::array<float, 3U> groundColor{};
            std::array<float, 3U> ambientColor{};
            std::array<float, 3U> fogColor{};
            std::array<float, 3U> cloudColor{};
            std::array<float, 3U> sunColor{};
            float skyIntensity = 1.0F;
            float ambientIntensity = 1.0F;
            float horizonExponent = 0.55F;
            float sunDiskSize = 1.0F;
            float sunIntensity = 4.0F;
            float fogStart = 450.0F;
            float fogEnd = 5000.0F;
            float fogDensity = 0.00018F;
            float cloudCoverage = 0.48F;
            float cloudDensity = 0.72F;
        };

        [[nodiscard]] EnvironmentEntities ResolveEnvironmentEntities() noexcept
        {
            EnvironmentEntities result;

            for (const lts::editor::EditorSceneEntity& entity :
                 g_sceneDocument.GetEntities())
            {
                if (result.environment == nullptr && entity.environment.has_value())
                {
                    result.environment = g_sceneDocument.FindEntityMutable(entity.id);
                }

                if (result.sun == nullptr && entity.directionalLight.has_value())
                {
                    result.sun = g_sceneDocument.FindEntityMutable(entity.id);
                }
            }

            return result;
        }

        [[nodiscard]] std::array<float, 3U> LerpColor(
            const std::array<float, 3U>& left,
            const std::array<float, 3U>& right,
            const float amount) noexcept
        {
            const auto lerp = [amount](const float a, const float b) noexcept
            {
                return a + (b - a) * amount;
            };

            return
            {
                lerp(left[0], right[0]),
                lerp(left[1], right[1]),
                lerp(left[2], right[2])
            };
        }

        [[nodiscard]] DayNightKeyframe LerpKeyframes(
            const DayNightKeyframe& left,
            const DayNightKeyframe& right,
            const float amount) noexcept
        {
            DayNightKeyframe result;
            const auto lerp = [amount](const float a, const float b) noexcept
            {
                return a + (b - a) * amount;
            };
            result.topColor = LerpColor(left.topColor, right.topColor, amount);
            result.horizonColor = LerpColor(left.horizonColor, right.horizonColor, amount);
            result.groundColor = LerpColor(left.groundColor, right.groundColor, amount);
            result.ambientColor = LerpColor(left.ambientColor, right.ambientColor, amount);
            result.fogColor = LerpColor(left.fogColor, right.fogColor, amount);
            result.cloudColor = LerpColor(left.cloudColor, right.cloudColor, amount);
            result.sunColor = LerpColor(left.sunColor, right.sunColor, amount);
            result.skyIntensity = lerp(left.skyIntensity, right.skyIntensity);
            result.ambientIntensity = lerp(left.ambientIntensity, right.ambientIntensity);
            result.horizonExponent = lerp(left.horizonExponent, right.horizonExponent);
            result.sunDiskSize = lerp(left.sunDiskSize, right.sunDiskSize);
            result.sunIntensity = lerp(left.sunIntensity, right.sunIntensity);
            result.fogStart = lerp(left.fogStart, right.fogStart);
            result.fogEnd = lerp(left.fogEnd, right.fogEnd);
            result.fogDensity = lerp(left.fogDensity, right.fogDensity);
            result.cloudCoverage = lerp(left.cloudCoverage, right.cloudCoverage);
            result.cloudDensity = lerp(left.cloudDensity, right.cloudDensity);
            return result;
        }

        [[nodiscard]] DayNightKeyframe GetNightKeyframe() noexcept
        {
            DayNightKeyframe value;
            value.topColor = {0.002F, 0.006F, 0.025F};
            value.horizonColor = {0.018F, 0.030F, 0.070F};
            value.groundColor = {0.003F, 0.004F, 0.008F};
            value.ambientColor = {0.030F, 0.045F, 0.095F};
            value.fogColor = {0.025F, 0.040F, 0.080F};
            value.cloudColor = {0.12F, 0.16F, 0.26F};
            value.sunColor = {0.30F, 0.42F, 0.70F};
            value.skyIntensity = 0.32F;
            value.ambientIntensity = 0.38F;
            value.horizonExponent = 0.70F;
            value.sunDiskSize = 0.50F;
            value.sunIntensity = 0.35F;
            value.fogStart = 180.0F;
            value.fogEnd = 2600.0F;
            value.fogDensity = 0.00032F;
            value.cloudCoverage = 0.58F;
            value.cloudDensity = 0.62F;
            return value;
        }

        [[nodiscard]] DayNightKeyframe GetMorningKeyframe() noexcept
        {
            DayNightKeyframe value;
            value.topColor = {0.025F, 0.060F, 0.170F};
            value.horizonColor = {0.950F, 0.360F, 0.120F};
            value.groundColor = {0.070F, 0.030F, 0.025F};
            value.ambientColor = {0.300F, 0.180F, 0.200F};
            value.fogColor = {0.78F, 0.42F, 0.24F};
            value.cloudColor = {1.00F, 0.70F, 0.52F};
            value.sunColor = {1.00F, 0.39F, 0.13F};
            value.skyIntensity = 0.90F;
            value.ambientIntensity = 0.80F;
            value.horizonExponent = 0.42F;
            value.sunDiskSize = 2.00F;
            value.sunIntensity = 3.20F;
            value.fogStart = 120.0F;
            value.fogEnd = 3200.0F;
            value.fogDensity = 0.00038F;
            value.cloudCoverage = 0.42F;
            value.cloudDensity = 0.74F;
            return value;
        }

        [[nodiscard]] DayNightKeyframe GetDayKeyframe() noexcept
        {
            DayNightKeyframe value;
            value.topColor = {0.055F, 0.200F, 0.550F};
            value.horizonColor = {0.450F, 0.680F, 0.920F};
            value.groundColor = {0.080F, 0.075F, 0.070F};
            value.ambientColor = {0.280F, 0.310F, 0.360F};
            value.fogColor = {0.45F, 0.62F, 0.78F};
            value.cloudColor = {0.92F, 0.95F, 1.00F};
            value.sunColor = {1.00F, 0.94F, 0.82F};
            value.skyIntensity = 1.0F;
            value.ambientIntensity = 1.0F;
            value.horizonExponent = 0.55F;
            value.sunDiskSize = 1.25F;
            value.sunIntensity = 4.0F;
            value.fogStart = 450.0F;
            value.fogEnd = 5000.0F;
            value.fogDensity = 0.00018F;
            value.cloudCoverage = 0.48F;
            value.cloudDensity = 0.72F;
            return value;
        }

        [[nodiscard]] DayNightKeyframe GetEveningKeyframe() noexcept
        {
            DayNightKeyframe value;
            value.topColor = {0.035F, 0.045F, 0.140F};
            value.horizonColor = {1.000F, 0.240F, 0.070F};
            value.groundColor = {0.055F, 0.020F, 0.018F};
            value.ambientColor = {0.270F, 0.135F, 0.180F};
            value.fogColor = {0.62F, 0.24F, 0.16F};
            value.cloudColor = {0.90F, 0.43F, 0.31F};
            value.sunColor = {1.00F, 0.24F, 0.07F};
            value.skyIntensity = 0.85F;
            value.ambientIntensity = 0.72F;
            value.horizonExponent = 0.38F;
            value.sunDiskSize = 2.20F;
            value.sunIntensity = 3.00F;
            value.fogStart = 160.0F;
            value.fogEnd = 3400.0F;
            value.fogDensity = 0.00034F;
            value.cloudCoverage = 0.52F;
            value.cloudDensity = 0.78F;
            return value;
        }

        [[nodiscard]] DayNightKeyframe EvaluateTimeOfDay(const float hour) noexcept
        {
            const float time = std::clamp(hour, 0.0F, 24.0F);

            if (time < 7.0F)
            {
                return LerpKeyframes(GetNightKeyframe(), GetMorningKeyframe(), time / 7.0F);
            }

            if (time < 13.0F)
            {
                return LerpKeyframes(GetMorningKeyframe(), GetDayKeyframe(), (time - 7.0F) / 6.0F);
            }

            if (time < 19.0F)
            {
                return LerpKeyframes(GetDayKeyframe(), GetEveningKeyframe(), (time - 13.0F) / 6.0F);
            }

            return LerpKeyframes(GetEveningKeyframe(), GetNightKeyframe(), (time - 19.0F) / 5.0F);
        }

        void ApplyTimeOfDay(const float requestedHour) noexcept
        {
            const EnvironmentEntities entities = ResolveEnvironmentEntities();

            if (entities.environment == nullptr || !entities.environment->environment.has_value())
            {
                return;
            }

            auto& environment = *entities.environment->environment;
            const float hour = std::clamp(requestedHour, 0.0F, 24.0F);
            const DayNightKeyframe value = EvaluateTimeOfDay(hour);
            environment.timeOfDay = hour;
            environment.preset = engine::scene::SkyPreset::Custom;
            environment.topColor = value.topColor;
            environment.horizonColor = value.horizonColor;
            environment.groundColor = value.groundColor;
            environment.ambientColor = value.ambientColor;
            environment.fogColor = value.fogColor;
            environment.cloudColor = value.cloudColor;
            environment.skyIntensity = value.skyIntensity;
            environment.ambientIntensity = value.ambientIntensity;
            environment.horizonExponent = value.horizonExponent;
            environment.sunDiskSizeDegrees = value.sunDiskSize;
            environment.fogStart = value.fogStart;
            environment.fogEnd = value.fogEnd;
            environment.fogDensity = value.fogDensity;
            environment.cloudCoverage = value.cloudCoverage;
            environment.cloudDensity = value.cloudDensity;

            if (environment.timeControlsSun && entities.sun != nullptr &&
                entities.sun->directionalLight.has_value())
            {
                constexpr float Pi = 3.14159265358979323846F;
                const float solarPhase = (hour - 6.0F) * (Pi / 12.0F);
                const float elevation = std::sin(solarPhase) * 70.0F;
                entities.sun->transform.rotationDegrees =
                {
                    -elevation,
                    hour * 15.0F - 180.0F,
                    0.0F
                };
                entities.sun->directionalLight->color = value.sunColor;
                entities.sun->directionalLight->intensity = value.sunIntensity;
                entities.sun->directionalLight->castShadows =
                    environment.shadowsEnabled;
            }
        }

        void UpdateDayNightSimulation(
            const float deltaSeconds) noexcept
        {
            if (
                !g_simulateDayNight ||
                g_loadedMapName.empty() ||
                IsLevelLoading())
            {
                return;
            }

            const EnvironmentEntities entities =
                ResolveEnvironmentEntities();

            if (
                entities.environment == nullptr ||
                !entities.environment->
                    environment.has_value())
            {
                return;
            }

            constexpr float gameHoursPerSecond =
                1.0F / 60.0F;

            const float safeDeltaSeconds =
                std::clamp(
                    deltaSeconds,
                    0.0F,
                    0.25F);

            float nextTime =
                entities.environment->
                    environment->
                    timeOfDay +
                safeDeltaSeconds *
                    gameHoursPerSecond;

            if (nextTime >= 24.0F)
            {
                nextTime =
                    std::fmod(
                        nextTime,
                        24.0F);
            }

            ApplyTimeOfDay(nextTime);
        }

        void EnsureEnvironmentEntities() noexcept
        {
            const std::size_t previousSelection = g_sceneDocument.GetSelectedIndex();
            EnvironmentEntities entities = ResolveEnvironmentEntities();
            bool created = false;

            if (entities.environment == nullptr)
            {
                lts::editor::EditorTransform transform{};
                static_cast<void>(g_sceneDocument.CreateEntity(
                    L"DX11 Environment",
                    lts::editor::EditorEntityKind::Environment,
                    transform));
                created = true;
            }

            entities = ResolveEnvironmentEntities();

            if (entities.sun == nullptr)
            {
                lts::editor::EditorTransform transform{};
                transform.position[1] = 10.0F;
                transform.rotationDegrees = {-70.0F, 0.0F, 0.0F};
                static_cast<void>(g_sceneDocument.CreateEntity(
                    L"DX11 Sun",
                    lts::editor::EditorEntityKind::DirectionalLight,
                    transform));
                created = true;
            }

            if (previousSelection != lts::editor::InvalidEditorEntityIndex &&
                previousSelection < g_sceneDocument.GetEntities().size())
            {
                static_cast<void>(g_sceneDocument.SelectEntityByIndex(previousSelection));
            }

            entities = ResolveEnvironmentEntities();

            if (created && entities.environment != nullptr &&
                entities.environment->environment.has_value())
            {
                ApplyTimeOfDay(entities.environment->environment->timeOfDay);
            }
        }

        void ApplyLegacyLevelResult(LegacyLevelLoadResult result)
        {
            const std::string loadingMapName =
                std::move(g_loadingMapName);

            g_loadingMapName.clear();
            
            if (!result.succeeded)
            {
                g_loadedLevelDataPath.clear();
                g_managedLevelObjectIndices.clear();
                g_levelLoadStatus = result.error.empty()
                    ? "LevelData.xml import failed."
                    : std::move(result.error);
                return;
            }

            lts::editor::EditorSceneSnapshot snapshot =
                g_sceneDocument.CreateSnapshot();
            snapshot.entities.erase(
                std::remove_if(
                    snapshot.entities.begin(),
                    snapshot.entities.end(),
                    [](const lts::editor::EditorSceneEntity& entity)
                    {
                        return
                            !entity.terrain.has_value() &&
                            !entity.environment.has_value() &&
                            !entity.directionalLight.has_value();
                    }),
                snapshot.entities.end());

            lts::editor::EditorEntityId nextId = 1U;

            for (const lts::editor::EditorSceneEntity& entity : snapshot.entities)
            {
                nextId = (std::max)(nextId, entity.id + 1U);
            }

            snapshot.entities.reserve(
                snapshot.entities.size() + result.entities.size());

            for (lts::editor::EditorSceneEntity& entity : result.entities)
            {
                entity.id = nextId++;
                snapshot.entities.push_back(std::move(entity));
            }

            snapshot.nextEntityId = nextId;
            snapshot.selectedIndex = snapshot.entities.empty()
                ? lts::editor::InvalidEditorEntityIndex
                : 0U;
            snapshot.selectedEntityId = snapshot.entities.empty()
                ? 0U
                : snapshot.entities.front().id;
            snapshot.selectedEntityIds.clear();

            if (snapshot.selectedEntityId != 0U)
            {
                snapshot.selectedEntityIds.push_back(snapshot.selectedEntityId);
            }

            snapshot.selectionAnchorId = snapshot.selectedEntityId;
            snapshot.dirty = false;
            g_sceneDocument.RestoreSnapshot(snapshot, false);
            g_sceneDocument.MarkSaved();
            g_levelLoadStats = result.stats;
            g_managedLevelObjectIndices = std::move(result.managedObjectIndices);
            g_settingsSaveStatus = "Map is ready for saving.";
            g_loadedMapName = loadingMapName.empty() ? "Level" : loadingMapName;
            g_levelLoadStatus =
                g_loadedMapName +
                " loaded: " +
                std::to_string(
                    result.stats.buildingObjects) +
                " buildings, " +
                std::to_string(
                    result.stats.roadObjects) +
                " roads, " +
                std::to_string(
                    result.stats.waterPlaneObjects) +
                " water planes; " +
                std::to_string(
                    result.stats.staticMeshObjects) +
                " visible.";

            if (!result.warning.empty())
            {
                g_levelLoadStatus += "\n";
                g_levelLoadStatus += result.warning;
            }
        }

        void PollLegacyLevelLoad()
        {
            if (!g_levelLoadFuture.valid())
            {
                return;
            }

            if (g_levelLoadFuture.wait_for(std::chrono::seconds(0)) !=
                std::future_status::ready)
            {
                return;
            }

            ApplyLegacyLevelResult(g_levelLoadFuture.get());
        }

        [[nodiscard]]
        bool LoadTerrainMap(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& levelRoot,
            const std::string& displayName)
        {
            if (IsLevelLoading())
            {
                return false;
            }

            const std::filesystem::path workspace =
                FindWorkspaceRoot();

            const std::filesystem::path terrainPath =
                levelRoot /
                L"Terrain" /
                L"Terrain.terrain";

            const std::filesystem::path levelDataPath =
                levelRoot /
                L"LevelData.xml";

            std::error_code levelDataError;

            if (workspace.empty() ||
                !std::filesystem::is_regular_file(
                    levelDataPath,
                    levelDataError) ||
                levelDataError)
            {
                g_levelLoadStatus =
                    displayName +
                    ": LevelData.xml was not found.";

                return false;
            }

            std::error_code terrainError;

            if (!std::filesystem::is_regular_file(
                    terrainPath,
                    terrainError) ||
                terrainError ||
                !LoadTerrainAsset(device, terrainPath))
            {
                g_levelLoadStatus =
                    displayName +
                    ": Terrain/Terrain.terrain could not be loaded.";

                return false;
            }

            g_levelLoadStats = {};
            g_loadedMapName.clear();
            g_loadingMapName = displayName;

            g_levelLoadStatus =
                "Loading " +
                displayName +
                ". Converting SCB assets...";

            g_loadedLevelDataPath =
                levelDataPath.lexically_normal();

            g_managedLevelObjectIndices.clear();
            g_settingsSaveStatus.clear();

            const std::wstring mapFolderName =
                levelRoot.filename().wstring();

            try
            {
                g_levelLoadFuture = std::async(
                    std::launch::async,
                    [
                        workspace,
                        levelDataPath,
                        mapFolderName
                    ]()
                    {
                        return LoadLegacyLevelData(
                            workspace,
                            levelDataPath,
                            mapFolderName);
                    });
            }
            catch (...)
            {
                g_loadedLevelDataPath.clear();
                g_managedLevelObjectIndices.clear();
                g_loadingMapName.clear();

                g_levelLoadStatus =
                    "Cannot start background loader for " +
                    displayName +
                    ".";

                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool LoadColoradoMap(
            engine::graphics::RenderDevice& device)
        {
            const std::filesystem::path workspace =
                FindWorkspaceRoot();

            return LoadTerrainMap(
                device,
                workspace /
                    L"bin" /
                    L"Levels" /
                    L"WZ_Colorado",
                "Colorado");
        }
        
        void SaveLoadedMap() noexcept
        {
            if (
                g_loadedLevelDataPath.empty() ||
                g_loadedMapName.empty() ||
                IsLevelLoading())
            {
                g_settingsSaveStatus =
                    "No loaded map is available for saving.";

                return;
            }

            LegacyLevelSaveResult result =
                SaveLegacyLevelData(
                    g_loadedLevelDataPath,
                    g_sceneDocument.GetEntities(),
                    g_managedLevelObjectIndices);

            if (!result.succeeded)
            {
                g_settingsSaveStatus =
                    result.error.empty()
                        ? "Map save failed."
                        : std::move(result.error);

                return;
            }

            for (
                const LegacyLevelSavedIdentity& identity :
                result.identities)
            {
                lts::editor::EditorSceneEntity* const entity =
                    g_sceneDocument.FindEntityMutable(
                        identity.entityId);

                if (
                    entity == nullptr ||
                    !entity->staticMesh.has_value())
                {
                    continue;
                }

                std::wstring name =
                    std::filesystem::path(
                        entity->staticMesh->assetPath).
                        stem().
                        wstring();

                name +=
                    L" #" +
                    std::to_wstring(
                        identity.objectIndex);

                entity->name =
                    std::move(name);

                entity->editorFolder =
                    L"LevelData/obj_Building";
            }

            g_managedLevelObjectIndices =
                std::move(
                    result.managedObjectIndices);

            g_sceneDocument.MarkSaved();

            g_settingsSaveStatus =
                "Map saved. Updated: " +
                std::to_string(
                    result.updatedObjects) +
                ", added: " +
                std::to_string(
                    result.addedObjects) +
                ", removed: " +
                std::to_string(
                    result.removedObjects) +
                ".";
        }

        [[nodiscard]]
        bool LoadTerrainAsset(
            engine::graphics::RenderDevice& device,
            const std::filesystem::path& path)
        {
            engine::assets::TerrainAsset terrain;

            if (
                engine::assets::Failed(
                    engine::assets::TerrainAsset::Load(
                        path,
                        terrain)) ||
                !terrain.IsValid())
            {
                return false;
            }

            /*
             * R16 importer С…СЂР°РЅРёС‚ РІРµСЂС‚РёРєР°Р»СЊРЅС‹Р№ С†РµРЅС‚СЂ
             * Рё РёС‚РѕРіРѕРІС‹Р№ actor scale РІ Terrain.ini.
             *
             * Р‘РµР· РІРѕСЃСЃС‚Р°РЅРѕРІР»РµРЅРёСЏ СЌС‚РѕРіРѕ transform
             * Terrain Р·Р°РіСЂСѓР¶Р°РµС‚СЃСЏ РІ position 0 Рё scale 1,
             * РёР·-Р·Р° С‡РµРіРѕ РїРµСЂРµСЃС‚Р°С‘С‚ СЃРѕРІРїР°РґР°С‚СЊ СЃ РѕР±СЉРµРєС‚Р°РјРё
             * РёР· LevelData.xml.
             */
            lts::editor::EditorTransform transform{};

            if (!LoadTerrainTransformFromIni(
                    path,
                    transform))
            {
                return false;
            }

            if (!g_terrainRenderer.LoadTerrain(
                    device,
                    path))
            {
                return false;
            }

            g_sceneDocument.Clear();

            if (!g_sceneDocument.CreateTerrainEntity(
                    path.stem().wstring(),
                    path.generic_wstring(),
                    transform))
            {
                return false;
            }

            std::vector<
                engine::scene::TerrainComponent::
                    LayerOverride> layers;

            layers.reserve(
                terrain.layers.size());

            for (
                const engine::assets::TerrainLayer& source :
                terrain.layers)
            {
                engine::scene::TerrainComponent::
                    LayerOverride layer;

                layer.name =
                    source.name;

                layer.diffusePath =
                    source.diffusePath;

                layer.normalPath =
                    source.normalPath;

                layer.scaleU =
                    source.scaleU;

                layer.scaleV =
                    source.scaleV;

                layers.push_back(
                    std::move(layer));
            }

            if (!g_sceneDocument.SetSelectedTerrainLayers(
                    std::move(layers)))
            {
                return false;
            }

            EnsureEnvironmentEntities();

            const float localWidth =
                static_cast<float>(
                    terrain.width - 1U) *
                terrain.tileSize;

            const float localDepth =
                static_cast<float>(
                    terrain.height - 1U) *
                terrain.tileSize;

            const float worldWidth =
                localWidth *
                std::fabs(
                    transform.scale[0]);

            const float worldDepth =
                localDepth *
                std::fabs(
                    transform.scale[2]);

            const float terrainAmplitude =
                (std::max)(
                    std::fabs(
                        terrain.heightOffset),

                    std::fabs(
                        terrain.heightOffset +
                        terrain.heightScale));

            const DirectX::XMFLOAT3 target
            {
                transform.position[0] +
                    localWidth *
                    transform.scale[0] *
                    0.5F,

                transform.position[1] +
                    terrainAmplitude *
                    std::fabs(
                        transform.scale[1]) *
                    0.35F,

                transform.position[2] +
                    localDepth *
                    transform.scale[2] *
                    0.5F
            };

            g_cameraController.FocusOn(
                target,
                (std::max)(
                    worldWidth,
                    worldDepth) *
                0.64F);

            g_loadedTerrainPath =
                path.lexically_normal();

            g_activeLayer = 0U;

            return true;
        }

        void LoadCommandLineTerrain(engine::graphics::RenderDevice& device)
        {
            int argumentCount = 0;
            LPWSTR* arguments = CommandLineToArgvW(
                GetCommandLineW(),
                &argumentCount);

            if (arguments == nullptr)
            {
                return;
            }

            for (int index = 1; index + 1 < argumentCount; ++index)
            {
                if (_wcsicmp(arguments[index], L"-terrain") == 0)
                {
                    static_cast<void>(LoadTerrainAsset(device, arguments[index + 1]));
                    break;
                }

                if (
                    _wcsicmp(arguments[index], L"-map") == 0 &&
                    _wcsicmp(arguments[index + 1], L"Colorado") == 0)
                {
                    static_cast<void>(LoadColoradoMap(device));
                    break;
                }

                if (_wcsicmp(arguments[index], L"-reimport-colorado-r16") == 0)
                {
                    const std::filesystem::path sourcePath = arguments[index + 1];
                    lts::editor::R16TerrainImportSettings settings;
                    float terrainCenterHeight = 0.0F;
                    std::string importStatus;

                    if (lts::editor::DetectR16TerrainImportSettings(
                            sourcePath,
                            settings,
                            terrainCenterHeight,
                            importStatus))
                    {
                        settings.destinationPath =
                            FindWorkspaceRoot() / L"bin" / L"Levels" /
                            L"WZ_Colorado" / L"Terrain" / L"Terrain.terrain";

                        if (lts::editor::WriteR16TerrainAsset(settings, importStatus))
                        {
                            static_cast<void>(LoadColoradoMap(device));
                        }
                    }

                    g_levelLoadStatus = std::move(importStatus);
                    break;
                }
            }

            LocalFree(arguments);
        }

        [[nodiscard]]
        const char* GetGraphicsQualityName(
            const EditorGraphicsQuality quality) noexcept
        {
            switch (quality)
            {
            case EditorGraphicsQuality::Low:
                return "Low";

            case EditorGraphicsQuality::Medium:
                return "Medium";

            case EditorGraphicsQuality::High:
                return "High";

            case EditorGraphicsQuality::Custom:
                return "Custom";

            default:
                return "Unknown";
            }
        }

        void DrawGraphicsQualityButton(
            const char* const label,
            const EditorGraphicsQuality quality) noexcept
        {
            const bool active =
                g_graphicsQuality == quality;

            if (active)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImGui::GetStyleColorVec4(
                        ImGuiCol_ButtonActive));
            }

            if (ImGui::Button(
                    label,
                    ImVec2(
                        ImGui::GetContentRegionAvail().x,
                        28.0F)))
            {
                g_graphicsQuality = quality;
            }

            if (active)
            {
                ImGui::PopStyleColor();
            }
        }

        void DrawSystemSettingsPage() noexcept
        {
            const bool mapLoaded =
                !g_loadedMapName.empty() &&
                !IsLevelLoading();

            ImGui::TextUnformatted(
                "System Settings");

            ImGui::Separator();

            ImGui::SeparatorText("Current Map");

            if (mapLoaded)
            {
                ImGui::Text(
                    "Map: %s",
                    g_loadedMapName.c_str());

                ImGui::Text(
                    "Scene: %s",
                    g_sceneDocument.IsDirty()
                        ? "Modified"
                        : "Saved");
            }
            else
            {
                ImGui::TextDisabled(
                    "No map loaded.");
            }

            ImGui::Spacing();

            const bool canSave =
                mapLoaded &&
                !g_loadedLevelDataPath.empty();

            ImGui::BeginDisabled(!canSave);

            if (ImGui::Button(
                    "Save Map",
                    ImVec2(
                        ImGui::GetContentRegionAvail().x,
                        30.0F)))
            {
                SaveLoadedMap();
            }

            ImGui::EndDisabled();

            if (!g_settingsSaveStatus.empty())
            {
                ImGui::TextWrapped(
                    "%s",
                    g_settingsSaveStatus.c_str());
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Day / Night");

            ImGui::BeginDisabled(!mapLoaded);

            ImGui::Checkbox(
                "Simulate Day / Night",
                &g_simulateDayNight);

            ImGui::EndDisabled();

            if (mapLoaded)
            {
                const EnvironmentEntities entities =
                    ResolveEnvironmentEntities();

                if (
                    entities.environment != nullptr &&
                    entities.environment->
                        environment.has_value())
                {
                    const float time =
                        entities.environment->
                            environment->
                            timeOfDay;

                    const int totalMinutes =
                        std::clamp(
                            static_cast<int>(
                                std::round(
                                    time * 60.0F)),
                            0,
                            24 * 60);

                    ImGui::Text(
                        "Current time: %02d:%02d",
                        totalMinutes / 60,
                        totalMinutes % 60);
                }
            }

            ImGui::TextDisabled(
                "Simulation speed: "
                "1 real second = 1 game minute.");
        }

        void DrawOptionsMenuPage() noexcept
        {
            ImGui::TextUnformatted(
                "Options Menu");

            ImGui::Separator();

            ImGui::SeparatorText(
                "Graphics Quality");

            DrawGraphicsQualityButton(
                "Low",
                EditorGraphicsQuality::Low);

            DrawGraphicsQualityButton(
                "Medium",
                EditorGraphicsQuality::Medium);

            DrawGraphicsQualityButton(
                "High",
                EditorGraphicsQuality::High);

            DrawGraphicsQualityButton(
                "Custom",
                EditorGraphicsQuality::Custom);

            ImGui::Spacing();

            ImGui::Text(
                "Selected profile: %s",
                GetGraphicsQualityName(
                    g_graphicsQuality));

            ImGui::TextWrapped(
                "The profile is now part of the Studio UI state. "
                "Concrete DX11 renderer values will be connected "
                "as AO, shadows, post-processing and terrain "
                "quality settings are implemented.");
        }

        void DrawSettingsPage() noexcept
        {
            switch (g_activeSettingsPage)
            {
            case SettingsToolbarPage::SystemSettings:

                DrawSystemSettingsPage();

                break;

            case SettingsToolbarPage::OptionsMenu:

                DrawOptionsMenuPage();

                break;
            }
        }

        void DrawTerrainEditorPage() noexcept
        {
            ImGui::TextUnformatted(
            "Terrain Editor");

            ImGui::Separator();

            ImGui::TextUnformatted(
                "DX11 Terrain");

            ImGui::Spacing();
            ImGui::SeparatorText(
                "Heightmap Import");

            ImGui::TextDisabled(
                "Legacy Terrain V1 / Terrain V2 are not used.");

            ImGui::Spacing();

            if (ImGui::Button(
                    "Import .r16",
                    ImVec2(140.0F, 28.0F)))
            {
                g_terrainImporter.Open();
            }

            ImGui::SameLine();

            if (g_terrainRenderer.HasTerrain())
            {
                ImGui::TextUnformatted("Heightmap loaded");
            }
            else
            {
                ImGui::TextDisabled("No heightmap loaded");
            }

            ImGui::TextWrapped(
                "R16 source automatically detects the sidecar metadata, "
                "RGBA masks and Colorado material definitions.");

            ImGui::TextWrapped(
                "Materials: %s",
                (FindWorkspaceRoot() / L"bin" / L"Data" / L"TerrainData" /
                    L"Materials").generic_u8string().c_str());

            lts::editor::EditorSceneEntity* entity =
                g_sceneDocument.GetSelectedEntityMutable();

            if (entity == nullptr || !entity->terrain.has_value())
            {
                return;
            }

            auto& layers = entity->terrain->layers;

            if (layers.empty())
            {
                return;
            }

            g_activeLayer = (std::min)(g_activeLayer, layers.size() - 1U);

            ImGui::SeparatorText("Texture Layers");
            ImGui::Text("%zu / 18 layers (Base + painted layers)", layers.size());

            for (std::size_t layerIndex = 0U; layerIndex < layers.size(); ++layerIndex)
            {
                auto& layer = layers[layerIndex];
                ImGui::PushID(static_cast<int>(layerIndex));

                const std::string label =
                    std::to_string(layerIndex) + ". " + layer.name;

                if (ImGui::Selectable(
                        label.c_str(),
                        g_activeLayer == layerIndex))
                {
                    g_activeLayer = layerIndex;
                }

                if (g_activeLayer == layerIndex)
                {
                    bool visible = layer.visible;
                    std::array<float, 2U> scale{layer.scaleU, layer.scaleV};

                    ImGui::Checkbox("Visible", &visible);
                    ImGui::DragFloat2(
                        "Scale U / V",
                        scale.data(),
                        0.25F,
                        0.001F,
                        100000.0F,
                        "%.3f");

                    ImGui::TextWrapped(
                        "Diffuse: %s",
                        layer.diffusePath.empty()
                            ? "<fallback>"
                            : layer.diffusePath.c_str());

                    if (ImGui::Button("Browse Diffuse..."))
                    {
                        static_cast<void>(SelectTerrainTexture(layer.diffusePath));
                    }

                    ImGui::TextWrapped(
                        "Normal: %s",
                        layer.normalPath.empty()
                            ? "<flat normal>"
                            : layer.normalPath.c_str());

                    if (ImGui::Button("Browse Normal..."))
                    {
                        static_cast<void>(SelectTerrainTexture(layer.normalPath));
                    }

                    if (visible != layer.visible ||
                        scale[0] != layer.scaleU || scale[1] != layer.scaleV)
                    {
                        static_cast<void>(
                            g_sceneDocument.UpdateSelectedTerrainLayer(
                                layerIndex,
                                layer.diffusePath,
                                layer.normalPath,
                                scale[0],
                                scale[1],
                                layer.offsetU,
                                layer.offsetV,
                                visible));
                    }
                }

                ImGui::PopID();
            }
        }

        void DrawTerrainLoaderPage() noexcept
        {
            if (!g_terrainMapsScanned)
            {
                RefreshAvailableTerrainMaps();
            }

            ImGui::TextUnformatted(
                "Terrain Loader");

            ImGui::Separator();

            ImGui::TextDisabled(
                "Source: bin/Levels");

            if (ImGui::Button(
                    "Refresh Levels",
                    ImVec2(
                        ImGui::GetContentRegionAvail().x,
                        28.0F)))
            {
                RefreshAvailableTerrainMaps();
            }

            if (!g_terrainMapScanStatus.empty())
            {
                ImGui::TextWrapped(
                    "%s",
                    g_terrainMapScanStatus.c_str());
            }

            ImGui::Spacing();
            ImGui::SeparatorText(
                "Available Levels");

            if (ImGui::BeginChild(
                    "##TerrainLevelList",
                    ImVec2(0.0F, 190.0F),
                    true))
            {
                for (std::size_t index = 0U;
                     index < g_availableTerrainMaps.size();
                     ++index)
                {
                    const TerrainMapEntry& map =
                        g_availableTerrainMaps[index];

                    std::string label =
                        map.displayName;

                    if (!map.IsLoadable())
                    {
                        label += " (incomplete)";
                    }

                    label += "##TerrainMap";
                    label += std::to_string(index);

                    if (ImGui::Selectable(
                            label.c_str(),
                            g_selectedTerrainMap ==
                                static_cast<int>(index)))
                    {
                        g_selectedTerrainMap =
                            static_cast<int>(index);
                    }
                }
            }

            ImGui::EndChild();

            const bool validSelection =
                g_selectedTerrainMap >= 0 &&
                static_cast<std::size_t>(
                    g_selectedTerrainMap) <
                    g_availableTerrainMaps.size();

            if (validSelection)
            {
                const TerrainMapEntry& map =
                    g_availableTerrainMaps[
                        static_cast<std::size_t>(
                            g_selectedTerrainMap)];

                ImGui::Text(
                    "Selected: %s",
                    map.displayName.c_str());

                ImGui::TextWrapped(
                    "Folder: %s",
                    map.levelRoot.
                        filename().
                        generic_u8string().
                        c_str());

                if (!map.hasLevelData)
                {
                    ImGui::TextDisabled(
                        "Missing LevelData.xml");
                }

                if (!map.hasTerrain)
                {
                    ImGui::TextDisabled(
                        "Missing Terrain/Terrain.terrain");
                }

                const bool canLoad =
                    map.IsLoadable() &&
                    !IsLevelLoading() &&
                    g_device != nullptr;

                ImGui::BeginDisabled(!canLoad);

                if (ImGui::Button(
                        "Load Selected Map",
                        ImVec2(
                            ImGui::GetContentRegionAvail().x,
                            30.0F)) &&
                    g_device != nullptr)
                {
                    static_cast<void>(
                        LoadTerrainMap(
                            *g_device,
                            map.levelRoot,
                            map.displayName));
                }

                ImGui::EndDisabled();
            }

            if (IsLevelLoading())
            {
                ImGui::TextUnformatted(
                    "Loading...");
            }

            ImGui::TextWrapped(
                "%s",
                g_levelLoadStatus.c_str());

            if (!g_loadedMapName.empty())
            {
                ImGui::SeparatorText(
                    "Load Statistics");

                ImGui::TextWrapped(
                    "obj_Building: %zu placed | "
                    "%zu unique meshes | %zu converted | "
                    "%zu cached | %zu missing | %zu failed",
                    g_levelLoadStats.importedObjects,
                    g_levelLoadStats.uniqueMeshes,
                    g_levelLoadStats.convertedMeshes,
                    g_levelLoadStats.cachedMeshes,
                    g_levelLoadStats.missingMeshes,
                    g_levelLoadStats.failedMeshes);
            }
        }

        void DrawTerrainPage() noexcept
        {
            switch (g_activeTerrainPage)
            {
            case TerrainToolbarPage::TerrainLoader:
                DrawTerrainLoaderPage();
                break;

            case TerrainToolbarPage::TerrainEditor:
                DrawTerrainEditorPage();
                break;
            }
        }

        void DrawEnvironmentPage() noexcept
        {
            EnsureEnvironmentEntities();
            const EnvironmentEntities entities = ResolveEnvironmentEntities();

            ImGui::TextUnformatted("Environment");
            ImGui::Separator();
            ImGui::TextUnformatted("DX11 Environment System");

            if (entities.environment == nullptr ||
                !entities.environment->environment.has_value() ||
                entities.sun == nullptr ||
                !entities.sun->directionalLight.has_value())
            {
                ImGui::TextDisabled("Environment scene entities are unavailable.");
                return;
            }

            auto& environment = *entities.environment->environment;
            auto& sun = *entities.sun->directionalLight;

            ImGui::Spacing();
            ImGui::SeparatorText("Day & Night");

            float time = environment.timeOfDay;

            if (ImGui::SliderFloat(
                    "Time##DayNight",
                    &time,
                    0.0F,
                    24.0F,
                    "%.2f h",
                    ImGuiSliderFlags_AlwaysClamp))
            {
                ApplyTimeOfDay(time);
            }

            const int totalMinutes = std::clamp(
                static_cast<int>(std::round(environment.timeOfDay * 60.0F)),
                0,
                24 * 60);
            ImGui::Text(
                "Time: %02d:%02d",
                totalMinutes / 60,
                totalMinutes % 60);

            const float presetWidth =
                (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5F;

            if (ImGui::Button("Morning / 07:00", ImVec2(presetWidth, 0.0F)))
            {
                ApplyTimeOfDay(7.0F);
            }
            ImGui::SameLine();
            if (ImGui::Button("Day / 13:00", ImVec2(presetWidth, 0.0F)))
            {
                ApplyTimeOfDay(13.0F);
            }
            if (ImGui::Button("Evening / 19:00", ImVec2(presetWidth, 0.0F)))
            {
                ApplyTimeOfDay(19.0F);
            }
            ImGui::SameLine();
            if (ImGui::Button("Night / 00:00", ImVec2(presetWidth, 0.0F)))
            {
                ApplyTimeOfDay(0.0F);
            }

            if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enabled##Sun", &environment.sunEnabled);
                if (ImGui::Checkbox("Time controls sun", &environment.timeControlsSun) &&
                    environment.timeControlsSun)
                {
                    ApplyTimeOfDay(environment.timeOfDay);
                }
                ImGui::ColorEdit3("Color##Sun", sun.color.data());
                ImGui::DragFloat(
                    "Intensity##Sun",
                    &sun.intensity,
                    0.05F,
                    0.0F,
                    20.0F,
                    "%.2f");

                float elevation = -entities.sun->transform.rotationDegrees[0];
                float azimuth = entities.sun->transform.rotationDegrees[1];
                ImGui::BeginDisabled(environment.timeControlsSun);
                bool rotationChanged = ImGui::SliderFloat(
                    "Elevation",
                    &elevation,
                    -90.0F,
                    90.0F,
                    "%.1f deg");
                rotationChanged |= ImGui::SliderFloat(
                    "Azimuth",
                    &azimuth,
                    -180.0F,
                    180.0F,
                    "%.1f deg");
                ImGui::EndDisabled();

                if (rotationChanged)
                {
                    entities.sun->transform.rotationDegrees[0] = -elevation;
                    entities.sun->transform.rotationDegrees[1] = azimuth;
                }

                ImGui::DragFloat(
                    "Disk size",
                    &environment.sunDiskSizeDegrees,
                    0.02F,
                    0.01F,
                    10.0F,
                    "%.2f deg");
            }

            if (ImGui::CollapsingHeader("Fog", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enabled##Fog", &environment.fogEnabled);
                ImGui::BeginDisabled(!environment.fogEnabled);
                ImGui::ColorEdit3("Color##Fog", environment.fogColor.data());
                ImGui::DragFloat(
                    "Start distance",
                    &environment.fogStart,
                    10.0F,
                    0.0F,
                    20000.0F,
                    "%.0f m");
                ImGui::DragFloat(
                    "End distance",
                    &environment.fogEnd,
                    10.0F,
                    environment.fogStart + 1.0F,
                    40000.0F,
                    "%.0f m");
                ImGui::DragFloat(
                    "Density##Fog",
                    &environment.fogDensity,
                    0.00001F,
                    0.0F,
                    0.01F,
                    "%.6f");
                ImGui::DragFloat(
                    "Height falloff",
                    &environment.fogHeightFalloff,
                    0.00005F,
                    0.0F,
                    0.02F,
                    "%.5f");
                environment.fogEnd = (std::max)(environment.fogEnd, environment.fogStart + 1.0F);
                ImGui::EndDisabled();
            }

            if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Sun shadows", &environment.shadowsEnabled))
                {
                    sun.castShadows = environment.shadowsEnabled;
                }
                ImGui::BeginDisabled(!environment.shadowsEnabled);
                ImGui::SliderFloat(
                    "Strength##Shadows",
                    &environment.shadowStrength,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Softness##Shadows",
                    &environment.shadowSoftness,
                    0.05F,
                    4.0F,
                    "%.2f");
                ImGui::DragFloat(
                    "Distance##Shadows",
                    &environment.shadowDistance,
                    10.0F,
                    10.0F,
                    10000.0F,
                    "%.0f m");
                ImGui::EndDisabled();
            }

            if (ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Visible##Sky", &environment.visible);
                ImGui::ColorEdit3("Top", environment.topColor.data());
                ImGui::ColorEdit3("Horizon", environment.horizonColor.data());
                ImGui::ColorEdit3("Ground", environment.groundColor.data());
                ImGui::ColorEdit3("Ambient", environment.ambientColor.data());
                ImGui::DragFloat(
                    "Sky intensity",
                    &environment.skyIntensity,
                    0.02F,
                    0.0F,
                    8.0F,
                    "%.2f");
                ImGui::DragFloat(
                    "Ambient intensity",
                    &environment.ambientIntensity,
                    0.02F,
                    0.0F,
                    8.0F,
                    "%.2f");
                ImGui::DragFloat(
                    "Horizon exponent",
                    &environment.horizonExponent,
                    0.02F,
                    0.05F,
                    8.0F,
                    "%.2f");
            }

            if (ImGui::CollapsingHeader("CloudPlane", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enabled##CloudPlane", &environment.cloudPlaneEnabled);
                ImGui::BeginDisabled(!environment.cloudPlaneEnabled);
                ImGui::ColorEdit3("Color##CloudPlane", environment.cloudColor.data());
                ImGui::SliderFloat(
                    "Coverage",
                    &environment.cloudCoverage,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Density##CloudPlane",
                    &environment.cloudDensity,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::DragFloat(
                    "Scale##CloudPlane",
                    &environment.cloudScale,
                    0.00001F,
                    0.00002F,
                    0.005F,
                    "%.6f");
                ImGui::DragFloat(
                    "Height##CloudPlane",
                    &environment.cloudHeight,
                    10.0F,
                    50.0F,
                    10000.0F,
                    "%.0f m");
                ImGui::DragFloat2(
                    "Wind U / V",
                    environment.cloudSpeed.data(),
                    0.0001F,
                    -0.1F,
                    0.1F,
                    "%.4f");
                ImGui::EndDisabled();
            }
        }

        void DrawPlaceholderPage(
            const char* title) noexcept
        {
            ImGui::TextUnformatted(title);

            ImGui::Separator();

            ImGui::TextDisabled(
                "Dear ImGui migration pending.");
        }

        void ApplyColorCorrectionPreset(const int preset) noexcept
        {
            lts::editor::ColorCorrectionSettings settings;

            switch (preset)
            {
            case 0: // Neutral
                settings.exposure = 0.0F;
                settings.contrast = 1.0F;
                settings.saturation = 1.0F;
                settings.gamma = 1.0F;
                settings.vibrance = 0.0F;
                settings.temperature = 0.0F;
                settings.tint = 0.0F;
                settings.filmicStrength = 0.0F;
                settings.lift = 0.0F;
                settings.gain = 1.0F;
                settings.sharpen = 0.0F;
                settings.vignette = 0.0F;
                settings.bloomStrength = 0.0F;
                break;

            case 2: // Cinematic
                settings.exposure = -0.04F;
                settings.contrast = 1.18F;
                settings.saturation = 0.96F;
                settings.gamma = 0.98F;
                settings.vibrance = 0.13F;
                settings.temperature = 0.10F;
                settings.tint = -0.02F;
                settings.filmicStrength = 0.62F;
                settings.sharpen = 0.20F;
                settings.vignette = 0.24F;
                settings.bloomStrength = 0.10F;
                break;

            case 3: // Stalker
                settings.exposure = -0.10F;
                settings.contrast = 1.17F;
                settings.saturation = 0.90F;
                settings.gamma = 0.96F;
                settings.vibrance = 0.10F;
                settings.temperature = -0.08F;
                settings.tint = -0.035F;
                settings.filmicStrength = 0.55F;
                settings.sharpen = 0.34F;
                settings.vignette = 0.23F;
                settings.bloomStrength = 0.06F;
                settings.colorFilter[0] = 0.96F;
                settings.colorFilter[1] = 1.02F;
                settings.colorFilter[2] = 0.97F;
                break;

            case 4: // Cold
                settings.exposure = 0.02F;
                settings.contrast = 1.10F;
                settings.saturation = 1.02F;
                settings.vibrance = 0.15F;
                settings.temperature = -0.22F;
                settings.tint = 0.03F;
                settings.filmicStrength = 0.42F;
                settings.sharpen = 0.25F;
                settings.vignette = 0.16F;
                settings.bloomStrength = 0.08F;
                settings.colorFilter[0] = 0.94F;
                settings.colorFilter[1] = 1.00F;
                settings.colorFilter[2] = 1.08F;
                break;

            case 1: // Colorado Juicy
            default:
                break;
            }

            settings.enabled = g_colorCorrectionSettings.enabled;
            g_colorCorrectionSettings = settings;
        }

        void DrawColorCorrectionPage() noexcept
        {
            ImGui::TextUnformatted("DX11 Color Correction");
            ImGui::Separator();
            ImGui::Checkbox("Enabled", &g_colorCorrectionSettings.enabled);

            ImGui::TextDisabled("Presets");
            if (ImGui::Button("Neutral")) ApplyColorCorrectionPreset(0);
            ImGui::SameLine();
            if (ImGui::Button("Colorado Juicy")) ApplyColorCorrectionPreset(1);
            if (ImGui::Button("Cinematic")) ApplyColorCorrectionPreset(2);
            ImGui::SameLine();
            if (ImGui::Button("Stalker")) ApplyColorCorrectionPreset(3);
            ImGui::SameLine();
            if (ImGui::Button("Cold")) ApplyColorCorrectionPreset(4);

            ImGui::BeginDisabled(!g_colorCorrectionSettings.enabled);

            if (ImGui::CollapsingHeader(
                    "Image",
                    ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat(
                    "Exposure",
                    &g_colorCorrectionSettings.exposure,
                    -2.0F,
                    2.0F,
                    "%+.2f EV");
                ImGui::SliderFloat(
                    "Contrast",
                    &g_colorCorrectionSettings.contrast,
                    0.50F,
                    2.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Saturation",
                    &g_colorCorrectionSettings.saturation,
                    0.0F,
                    2.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Vibrance",
                    &g_colorCorrectionSettings.vibrance,
                    -1.0F,
                    1.0F,
                    "%+.2f");
                ImGui::SliderFloat(
                    "Gamma",
                    &g_colorCorrectionSettings.gamma,
                    0.50F,
                    2.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Temperature",
                    &g_colorCorrectionSettings.temperature,
                    -1.0F,
                    1.0F,
                    "%+.2f");
                ImGui::SliderFloat(
                    "Tint",
                    &g_colorCorrectionSettings.tint,
                    -1.0F,
                    1.0F,
                    "%+.2f");
                ImGui::ColorEdit3(
                    "Color filter",
                    g_colorCorrectionSettings.colorFilter,
                    ImGuiColorEditFlags_Float);
            }

            if (ImGui::CollapsingHeader(
                    "Tone",
                    ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat(
                    "Filmic strength",
                    &g_colorCorrectionSettings.filmicStrength,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Lift",
                    &g_colorCorrectionSettings.lift,
                    -0.25F,
                    0.25F,
                    "%+.3f");
                ImGui::SliderFloat(
                    "Gain",
                    &g_colorCorrectionSettings.gain,
                    0.50F,
                    2.0F,
                    "%.2f");
            }

            if (ImGui::CollapsingHeader(
                    "Lens & Detail",
                    ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat(
                    "Sharpen",
                    &g_colorCorrectionSettings.sharpen,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Vignette",
                    &g_colorCorrectionSettings.vignette,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Vignette softness",
                    &g_colorCorrectionSettings.vignetteSoftness,
                    0.05F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Soft bloom",
                    &g_colorCorrectionSettings.bloomStrength,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Bloom threshold",
                    &g_colorCorrectionSettings.bloomThreshold,
                    0.0F,
                    1.0F,
                    "%.2f");
                ImGui::SliderFloat(
                    "Bloom radius",
                    &g_colorCorrectionSettings.bloomRadius,
                    0.5F,
                    8.0F,
                    "%.1f px");
            }

            ImGui::EndDisabled();
            ImGui::Separator();
            ImGui::TextWrapped(
                "R3-inspired grading is applied to the world before the editor UI.");
        }

        [[nodiscard]]
        ObjectViewContext BuildObjectViewContext() noexcept
        {
            const ImGuiViewport* const viewport =
                ImGui::GetMainViewport();

            return
            {
                g_sceneDocument,
                g_commandHistory,
                g_cameraController,
                g_staticMeshRenderer,
                g_terrainRenderer,
                g_window,
                0,
                0,
                static_cast<std::uint32_t>(
                    (std::max)(viewport->WorkSize.x, 1.0F)),
                static_cast<std::uint32_t>(
                    (std::max)(viewport->WorkSize.y, 1.0F))
            };
        }

        void DrawActivePage(
            ObjectViewContext& objectContext) noexcept
        {
            switch (g_activePage)
            {
            case LevelEditorPage::Terrain:
                DrawTerrainPage();
                
                break;

            case LevelEditorPage::Settings:
                DrawSettingsPage();
                
                break;

            case LevelEditorPage::Objects:
                g_objectViewTab.DrawPage(objectContext);

                break;

            case LevelEditorPage::Materials:
                DrawPlaceholderPage(
                    "Materials");

                break;

            case LevelEditorPage::Environment:
                DrawEnvironmentPage();

                break;

            case LevelEditorPage::Collections:
                DrawPlaceholderPage(
                    "Collections");

                break;

            case LevelEditorPage::Decorators:
                DrawPlaceholderPage(
                    "Decorators");

                break;

            case LevelEditorPage::Roads:
                DrawPlaceholderPage(
                    "Roads");

                break;

            case LevelEditorPage::Gameplay:
                DrawPlaceholderPage(
                    "Gameplay");

                break;

            case LevelEditorPage::PostFX:
                DrawPlaceholderPage(
                    "Post FX");

                break;

            case LevelEditorPage::ColorCorrection:
                DrawColorCorrectionPage();

                break;
            }
        }

        void DrawLevelEditor() noexcept
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            
            constexpr float toolbarHeight = 42.0F;
            constexpr float secondaryToolbarHeight = 38.0F;

            const bool settingsActive =
                g_activePage ==
                    LevelEditorPage::Settings;

            const bool terrainActive =
                g_activePage ==
                    LevelEditorPage::Terrain;

            const bool objectsActive =
                g_activePage ==
                    LevelEditorPage::Objects;

            const bool secondaryToolbarVisible = settingsActive || terrainActive || objectsActive;

            const float controlsTop =
                toolbarHeight +
                (
                    secondaryToolbarVisible
                        ? secondaryToolbarHeight
                        : 0.0F
                );

            const float panelWidth =
                (std::min)(
                    375.0F,
                    viewport->WorkSize.x * 0.32F);

            const float panelHeight =
                (std::max)(
                    180.0F,
                    viewport->WorkSize.y -
                        controlsTop -
                        70.0F);

            ObjectViewContext objectContext =
                BuildObjectViewContext();

            ImGui::SetNextWindowDockID(0U, ImGuiCond_Always);
            ImGui::SetNextWindowPos(
                viewport->WorkPos,
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(viewport->WorkSize.x, toolbarHeight),
                ImGuiCond_Always);

            if (!ImGui::Begin(
                    "##WarZEditorToolbar",
                    nullptr,
                    ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoDocking |
                        ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoScrollbar |
                        ImGuiWindowFlags_NoScrollWithMouse))
            {
                ImGui::End();

                return;
            }

            g_editorToolbar.DrawMain(
                g_activePage);
            ImGui::End();

            if (secondaryToolbarVisible)
            {
                ImGui::SetNextWindowDockID(
                    0U,
                    ImGuiCond_Always);

                ImGui::SetNextWindowPos(
                    ImVec2(
                        viewport->WorkPos.x,
                        viewport->WorkPos.y +
                            toolbarHeight),
                    ImGuiCond_Always);

                ImGui::SetNextWindowSize(
                    ImVec2(
                        viewport->WorkSize.x,
                        secondaryToolbarHeight),
                    ImGuiCond_Always);

                if (ImGui::Begin(
                        "##LevelEditorSecondaryToolbar",
                        nullptr,
                        ImGuiWindowFlags_NoMove |
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoDocking |
                            ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoSavedSettings |
                            ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoScrollWithMouse))
                {
                    if (settingsActive)
                    {
                        g_editorToolbar.DrawSettings(
                            g_activeSettingsPage);
                    }
                    else if (terrainActive)
                    {
                        g_editorToolbar.DrawTerrain(
                            g_activeTerrainPage);
                    }
                    else if (objectsActive)
                    {
                        g_objectViewTab.DrawToolbar(
                            objectContext);
                    }
                }

                ImGui::End();
            }

            ImGui::SetNextWindowDockID(0U, ImGuiCond_Always);
            ImGui::SetNextWindowPos(
                ImVec2(
                    viewport->WorkPos.x + viewport->WorkSize.x - panelWidth - 5.0F,
                    viewport->WorkPos.y + controlsTop + 5.0F),
                ImGuiCond_Always);
            ImGui::SetNextWindowSize(
                ImVec2(panelWidth, panelHeight),
                ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.92F);

            if (!ImGui::Begin(
                    "##WarZEditorControls",
                    nullptr,
                    ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoDocking |
                        ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::End();

                return;
            }

            DrawActivePage(objectContext);
            ImGui::End();

            if (objectsActive)
            {
                g_objectViewTab.DrawWindows(objectContext);
                g_objectViewTab.UpdateViewport(objectContext);
            }
        }
    }

    LevelEditorPage
        GetActiveLevelEditorPage() noexcept
    {
        return g_activePage;
    }

    bool InitializeEditorUI(
        engine::graphics::RenderDevice& device,
        const engine::platform::NativeWindowHandle window) noexcept
    {
        if (g_initialized)
        {
            return true;
        }

        if (!g_skyRenderer.Initialize(device))
        {
            return false;
        }

        if (!g_gridRenderer.Initialize(device))
        {
            g_skyRenderer.Shutdown(device);
            return false;
        }

        if (!g_terrainRenderer.Initialize(device))
        {
            g_gridRenderer.Shutdown(device);
            g_skyRenderer.Shutdown(device);
            return false;
        }

        if (!g_staticMeshRenderer.Initialize(device))
        {
            g_terrainRenderer.Shutdown(device);
            g_gridRenderer.Shutdown(device);
            g_skyRenderer.Shutdown(device);
            return false;
        }

        if (!g_objectViewTab.Initialize(device, window))
        {
            g_staticMeshRenderer.Shutdown(device);
            g_terrainRenderer.Shutdown(device);
            g_gridRenderer.Shutdown(device);
            g_skyRenderer.Shutdown(device);
            return false;
        }

        if (!g_colorCorrectionRenderer.Initialize(device))
        {
            g_objectViewTab.Shutdown(device);
            g_staticMeshRenderer.Shutdown(device);
            g_terrainRenderer.Shutdown(device);
            g_gridRenderer.Shutdown(device);
            g_skyRenderer.Shutdown(device);
            return false;
        }

        g_device = &device;
        g_window = window;
        g_cameraController.SetViewportWindow(window);
        g_initialized = true;
        EnsureEnvironmentEntities();
        LoadCommandLineTerrain(device);
        return true;
    }

    void ShutdownEditorUI(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (!g_initialized)
        {
            return;
        }

        if (g_levelLoadFuture.valid())
        {
            g_levelLoadFuture.wait();
            static_cast<void>(g_levelLoadFuture.get());
        }

        g_colorCorrectionRenderer.Shutdown(device);
        g_objectViewTab.Shutdown(device);
        g_staticMeshRenderer.Shutdown(device);
        g_terrainRenderer.Shutdown(device);
        g_gridRenderer.Shutdown(device);
        g_skyRenderer.Shutdown(device);
        g_commandHistory.Clear();
        g_sceneDocument.Clear();
        g_loadedTerrainPath.clear();
        g_loadedLevelDataPath.clear();
        g_managedLevelObjectIndices.clear();
        g_settingsSaveStatus.clear();
        g_loadedMapName.clear();
        g_levelLoadStats = {};
        g_levelLoadStatus = "No map selected.";
        g_device = nullptr;
        g_window = {};
        g_activeSettingsPage = SettingsToolbarPage::SystemSettings;
        g_graphicsQuality = EditorGraphicsQuality::High;
        g_simulateDayNight = false;
        g_initialized = false;
        g_availableTerrainMaps.clear();
        g_selectedTerrainMap = -1;
        g_terrainMapsScanned = false;
        g_terrainMapScanStatus.clear();
        g_loadingMapName.clear();
        g_activeTerrainPage = TerrainToolbarPage::TerrainLoader;
    }

    engine::graphics::GraphicsResult RenderEditorWorld(
        engine::graphics::CommandContext& context,
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (!g_initialized || width == 0U || height == 0U)
        {
            return engine::graphics::GraphicsResult::Success;
        }

        DirectX::XMFLOAT4X4 viewProjection{};

        if (!g_cameraController.BuildViewProjection(
                width,
                height,
                viewProjection))
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        const engine::graphics::GraphicsResult skyResult =
            g_skyRenderer.Render(
                context,
                g_sceneDocument,
                viewProjection,
                g_cameraController.GetPosition());

        if (engine::graphics::Failed(skyResult))
        {
            return skyResult;
        }

        const engine::graphics::GraphicsResult gridResult =
            g_gridRenderer.Render(context, viewProjection);

        if (engine::graphics::Failed(gridResult))
        {
            return gridResult;
        }

        if (g_terrainRenderer.HasTerrain())
        {
            const engine::graphics::GraphicsResult terrainResult =
                g_terrainRenderer.Render(
                    context,
                    g_sceneDocument,
                    viewProjection,
                    g_cameraController.GetPosition());

            if (engine::graphics::Failed(terrainResult))
            {
                return terrainResult;
            }
        }

        const engine::graphics::GraphicsResult meshResult =
            g_staticMeshRenderer.Render(
                context,
                g_sceneDocument,
                viewProjection,
                g_cameraController.GetPosition());

        if (engine::graphics::Failed(meshResult))
        {
            return meshResult;
        }

        if (g_activePage != LevelEditorPage::Objects)
        {
            return engine::graphics::GraphicsResult::Success;
        }

        ObjectViewContext objectContext
        {
            g_sceneDocument,
            g_commandHistory,
            g_cameraController,
            g_staticMeshRenderer,
            g_terrainRenderer,
            g_window,
            0,
            0,
            width,
            height
        };

        return g_objectViewTab.Render(
            context,
            objectContext,
            viewProjection,
            g_cameraController.GetPosition());
    }

    engine::graphics::GraphicsResult RenderEditorColorCorrection(
        engine::graphics::CommandContext& context,
        const engine::graphics::TextureHandle source,
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (!g_initialized)
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        return g_colorCorrectionRenderer.Render(
            context,
            source,
            width,
            height,
            g_colorCorrectionSettings);
    }

    void DrawEditorUI() noexcept
    {
        PollLegacyLevelLoad();

        if (g_initialized)
        {
            const ImGuiIO& io = ImGui::GetIO();
            g_cameraController.Update(
                static_cast<double>(io.DeltaTime),
                io.MouseWheel);

            UpdateDayNightSimulation(
                io.DeltaTime);
        }

        DrawLevelEditor();

        if (g_initialized && g_device != nullptr)
        {
            lts::editor::TerrainImportContext importContext
            {
                *g_device,
                g_sceneDocument,
                g_commandHistory,
                g_cameraController,
                g_terrainRenderer,
                g_loadedTerrainPath,
                reinterpret_cast<void*>(g_window.Value())
            };

            g_terrainImporter.Draw(importContext);
        }
    }
}