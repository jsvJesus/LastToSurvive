#include "StudioEditorUI.h"
#include "LegacyLevelDataLoader.h"

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
        std::filesystem::path g_loadedTerrainPath;
        std::future<LegacyLevelLoadResult> g_levelLoadFuture;
        LegacyLevelLoadStats g_levelLoadStats;
        std::string g_levelLoadStatus = "No map selected.";
        std::string g_loadedMapName;
        struct ObjectDepotModel final
        {
            std::string relativePath;
            std::string lowercasePath;
            std::string category;
        };
        std::vector<ObjectDepotModel> g_objectDepotModels;
        std::vector<std::string> g_objectDepotCategories;
        std::array<char, 192U> g_objectDepotFilter{};
        std::size_t g_selectedDepotModel =
            std::numeric_limits<std::size_t>::max();
        int g_selectedDepotCategory = -1;
        bool g_objectDepotScanned = false;
        std::string g_objectDepotStatus;
        engine::platform::NativeWindowHandle g_window;
        std::size_t g_activeLayer = 0U;
        bool g_initialized = false;

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
                 * Старые .terrain могут не иметь Terrain.ini.
                 * Для них сохраняем прежний transform:
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
                 * Terrain.ini, созданный новым R16 importer,
                 * обязан содержать полный transform.
                 *
                 * Если в старом Terrain.ini нет ни одного
                 * transform-поля, используем default transform.
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

        void RefreshObjectDepot() noexcept
        {
            try
            {
                g_objectDepotModels.clear();
                g_objectDepotCategories.clear();
                g_selectedDepotModel = std::numeric_limits<std::size_t>::max();
                g_selectedDepotCategory = -1;
                const std::filesystem::path depotRoot =
                    FindWorkspaceRoot() / L"bin" / L"Data" / L"ObjectsDepot";
                std::error_code error;

                if (!std::filesystem::is_directory(depotRoot, error) || error)
                {
                    g_objectDepotStatus = "ObjectsDepot directory was not found.";
                    g_objectDepotScanned = true;
                    return;
                }

                for (std::filesystem::recursive_directory_iterator iterator(
                         depotRoot,
                         std::filesystem::directory_options::skip_permission_denied,
                         error),
                     end;
                     !error && iterator != end;
                     iterator.increment(error))
                {
                    if (!iterator->is_regular_file(error) || error)
                    {
                        error.clear();
                        continue;
                    }

                    const std::string extension = LowercaseAscii(
                        iterator->path().extension().u8string());

                    if (extension != ".sco" && extension != ".scb")
                    {
                        continue;
                    }

                    const std::filesystem::path relative =
                        std::filesystem::relative(iterator->path(), depotRoot, error);

                    if (error)
                    {
                        error.clear();
                        continue;
                    }

                    ObjectDepotModel model;
                    model.relativePath = relative.generic_u8string();
                    model.lowercasePath = LowercaseAscii(model.relativePath);
                    const std::filesystem::path parent = relative.parent_path();

                    if (parent.empty())
                    {
                        model.category = "(Root)";
                    }
                    else
                    {
                        model.category = parent.begin()->u8string();
                    }

                    g_objectDepotModels.push_back(std::move(model));
                }

                std::sort(
                    g_objectDepotModels.begin(),
                    g_objectDepotModels.end(),
                    [](const ObjectDepotModel& left, const ObjectDepotModel& right)
                    {
                        return left.lowercasePath < right.lowercasePath;
                    });

                for (const ObjectDepotModel& model : g_objectDepotModels)
                {
                    g_objectDepotCategories.push_back(model.category);
                }

                std::sort(
                    g_objectDepotCategories.begin(),
                    g_objectDepotCategories.end());
                g_objectDepotCategories.erase(
                    std::unique(
                        g_objectDepotCategories.begin(),
                        g_objectDepotCategories.end()),
                    g_objectDepotCategories.end());
                g_objectDepotStatus =
                    std::to_string(g_objectDepotModels.size()) +
                    " models found.";
                g_objectDepotScanned = true;
            }
            catch (...)
            {
                g_objectDepotModels.clear();
                g_objectDepotCategories.clear();
                g_objectDepotStatus = "ObjectsDepot scan failed.";
                g_objectDepotScanned = true;
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
            if (!result.succeeded)
            {
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
            
            g_levelLoadStatus = "Colorado loaded: " +
                std::to_string(result.stats.importedObjects) +
                " obj_Building entries, " +
                std::to_string(result.stats.staticMeshObjects) +
                " visible.";

            if (!result.warning.empty())
            {
                g_levelLoadStatus += "\n";
                g_levelLoadStatus += result.warning;
            }
            g_loadedMapName = "Colorado";
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

        [[nodiscard]] bool LoadColoradoMap(
            engine::graphics::RenderDevice& device)
        {
            if (IsLevelLoading())
            {
                return false;
            }

            const std::filesystem::path workspace = FindWorkspaceRoot();
            const std::filesystem::path levelRoot =
                workspace / L"bin" / L"Levels" / L"WZ_Colorado";
            const std::filesystem::path terrainPath =
                levelRoot / L"Terrain" / L"Terrain.terrain";
            const std::filesystem::path levelDataPath =
                levelRoot / L"LevelData.xml";
            std::error_code error;

            if (
                workspace.empty() ||
                !std::filesystem::is_regular_file(levelDataPath, error) ||
                error ||
                !LoadTerrainAsset(device, terrainPath))
            {
                g_levelLoadStatus =
                    "Cannot load Colorado terrain or LevelData.xml.";
                return false;
            }

            g_levelLoadStats = {};
            g_loadedMapName.clear();
            g_levelLoadStatus = "Converting SCB assets to Data/StaticMeshes/*.mesh...";

            try
            {
                g_levelLoadFuture = std::async(
                    std::launch::async,
                    [workspace, levelDataPath]()
                    {
                        return LoadLegacyLevelData(
                            workspace,
                            levelDataPath,
                            L"Colorado");
                    });
            }
            catch (...)
            {
                g_levelLoadStatus = "Cannot start Colorado background loader.";
                return false;
            }

            return true;
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
             * R16 importer хранит вертикальный центр
             * и итоговый actor scale в Terrain.ini.
             *
             * Без восстановления этого transform
             * Terrain загружается в position 0 и scale 1,
             * из-за чего перестаёт совпадать с объектами
             * из LevelData.xml.
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

        const char* GetPageName(
            const LevelEditorPage page) noexcept
        {
            switch (page)
            {
            case LevelEditorPage::Settings:
                return "Settings";

            case LevelEditorPage::Terrain:
                return "Terrain";

            case LevelEditorPage::Objects:
                return "Objects";

            case LevelEditorPage::Materials:
                return "Materials";

            case LevelEditorPage::Environment:
                return "Environment";

            case LevelEditorPage::Collections:
                return "Collections";

            case LevelEditorPage::Decorators:
                return "Decorators";

            case LevelEditorPage::Roads:
                return "Roads";

            case LevelEditorPage::Gameplay:
                return "Gameplay";

            case LevelEditorPage::PostFX:
                return "Post FX";

            case LevelEditorPage::ColorCorrection:
                return "Color Correction";

            default:
                return "Unknown";
            }
        }

        void DrawPageButton(
            const LevelEditorPage page) noexcept
        {
            const bool active =
                g_activePage == page;

            if (active)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImGui::GetStyleColorVec4(
                        ImGuiCol_ButtonActive));
            }

            if (ImGui::Button(
                    GetPageName(page)))
            {
                g_activePage = page;
            }

            if (active)
            {
                ImGui::PopStyleColor();
            }
        }

        void DrawLevelEditorToolbar() noexcept
        {
            constexpr std::array pages
            {
                LevelEditorPage::Settings,
                LevelEditorPage::Terrain,
                LevelEditorPage::Objects,
                LevelEditorPage::Materials,
                LevelEditorPage::Environment,
                LevelEditorPage::Collections,
                LevelEditorPage::Decorators,
                LevelEditorPage::Roads,
                LevelEditorPage::Gameplay,
                LevelEditorPage::PostFX,
                LevelEditorPage::ColorCorrection
            };

            for (std::size_t index = 0U; index < pages.size(); ++index)
            {
                if (index > 0U)
                {
                    const float nextWidth =
                        ImGui::CalcTextSize(GetPageName(pages[index])).x +
                        ImGui::GetStyle().FramePadding.x * 2.0F;

                    if (ImGui::GetCursorPosX() + nextWidth <
                        ImGui::GetContentRegionMax().x)
                    {
                        ImGui::SameLine();
                    }
                }

                DrawPageButton(pages[index]);
            }
        }

        void DrawTerrainPage() noexcept
        {
            ImGui::TextUnformatted(
                "Terrain");

            ImGui::Separator();

            ImGui::TextUnformatted(
                "DX11 Terrain");

            ImGui::Spacing();

            ImGui::SeparatorText("Map");
            constexpr const char* maps[]
            {
                "Colorado"
            };
            static int selectedMap = 0;
            ImGui::Combo(
                "Map",
                &selectedMap,
                maps,
                IM_ARRAYSIZE(maps));
            ImGui::BeginDisabled(IsLevelLoading() || g_device == nullptr);

            if (ImGui::Button("Load Colorado", ImVec2(140.0F, 28.0F)) &&
                g_device != nullptr)
            {
                static_cast<void>(LoadColoradoMap(*g_device));
            }

            ImGui::EndDisabled();

            if (IsLevelLoading())
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("Loading...");
            }

            ImGui::TextWrapped("%s", g_levelLoadStatus.c_str());

            if (!g_loadedMapName.empty())
            {
                ImGui::Text(
                    "obj_Building: %zu placed | %zu unique meshes | "
                    "%zu converted | %zu cached | %zu missing | %zu failed",
                    g_levelLoadStats.importedObjects,
                    g_levelLoadStats.uniqueMeshes,
                    g_levelLoadStats.convertedMeshes,
                    g_levelLoadStats.cachedMeshes,
                    g_levelLoadStats.missingMeshes,
                    g_levelLoadStats.failedMeshes);
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Heightmap Import");

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

        void DrawObjectsPage() noexcept
        {
            if (!g_objectDepotScanned)
            {
                RefreshObjectDepot();
            }

            ImGui::TextUnformatted("Objects Viewer");
            ImGui::Separator();
            ImGui::TextWrapped(
                "Source: %s",
                (FindWorkspaceRoot() / L"bin" / L"Data" / L"ObjectsDepot").
                    generic_u8string().c_str());
            ImGui::Text("Models: %zu", g_objectDepotModels.size());

            if (IsLevelLoading())
            {
                ImGui::TextDisabled("Colorado obj_Building placement is loading...");
            }
            else if (!g_loadedMapName.empty())
            {
                ImGui::TextDisabled(
                    "Colorado: %zu obj_Building entries placed",
                    g_levelLoadStats.importedObjects);
            }

            if (ImGui::Button("Refresh depot"))
            {
                RefreshObjectDepot();
            }

            ImGui::SameLine();
            ImGui::TextDisabled("%s", g_objectDepotStatus.c_str());
            ImGui::InputTextWithHint(
                "##ObjectDepotSearch",
                "Search model...",
                g_objectDepotFilter.data(),
                g_objectDepotFilter.size());

            const char* currentCategory = "All folders";

            if (
                g_selectedDepotCategory >= 0 &&
                static_cast<std::size_t>(g_selectedDepotCategory) <
                    g_objectDepotCategories.size())
            {
                currentCategory = g_objectDepotCategories[
                    static_cast<std::size_t>(g_selectedDepotCategory)].c_str();
            }

            if (ImGui::BeginCombo("Folder", currentCategory))
            {
                if (ImGui::Selectable(
                        "All folders",
                        g_selectedDepotCategory < 0))
                {
                    g_selectedDepotCategory = -1;
                }

                for (std::size_t categoryIndex = 0U;
                     categoryIndex < g_objectDepotCategories.size();
                     ++categoryIndex)
                {
                    const bool selected =
                        g_selectedDepotCategory ==
                        static_cast<int>(categoryIndex);

                    if (ImGui::Selectable(
                            g_objectDepotCategories[categoryIndex].c_str(),
                            selected))
                    {
                        g_selectedDepotCategory =
                            static_cast<int>(categoryIndex);
                    }
                }

                ImGui::EndCombo();
            }

            const std::string filter = LowercaseAscii(g_objectDepotFilter.data());
            std::vector<std::size_t> visibleModels;
            visibleModels.reserve(g_objectDepotModels.size());

            for (std::size_t modelIndex = 0U;
                 modelIndex < g_objectDepotModels.size();
                 ++modelIndex)
            {
                const ObjectDepotModel& model = g_objectDepotModels[modelIndex];

                if (
                    g_selectedDepotCategory >= 0 &&
                    model.category != g_objectDepotCategories[
                        static_cast<std::size_t>(g_selectedDepotCategory)])
                {
                    continue;
                }

                if (
                    !filter.empty() &&
                    model.lowercasePath.find(filter) == std::string::npos)
                {
                    continue;
                }

                visibleModels.push_back(modelIndex);
            }

            ImGui::SeparatorText("ObjectsDepot Models");
            ImGui::TextDisabled("Showing %zu", visibleModels.size());
            ImGui::BeginChild(
                "##ObjectsDepotModels",
                ImVec2(0.0F, 0.0F),
                true,
                ImGuiWindowFlags_HorizontalScrollbar);
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(visibleModels.size()));

            while (clipper.Step())
            {
                for (int index = clipper.DisplayStart;
                     index < clipper.DisplayEnd;
                     ++index)
                {
                    const std::size_t modelIndex = visibleModels[
                        static_cast<std::size_t>(index)];
                    const ObjectDepotModel& model =
                        g_objectDepotModels[modelIndex];
                    ImGui::PushID(index);

                    if (ImGui::Selectable(
                            model.relativePath.c_str(),
                            g_selectedDepotModel == modelIndex))
                    {
                        g_selectedDepotModel = modelIndex;
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip(
                            "Data/ObjectsDepot/%s",
                            model.relativePath.c_str());
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndChild();
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

        void DrawActivePage() noexcept
        {
            switch (g_activePage)
            {
            case LevelEditorPage::Terrain:

                DrawTerrainPage();

                break;

            case LevelEditorPage::Settings:

                DrawPlaceholderPage(
                    "Settings");

                break;

            case LevelEditorPage::Objects:

                DrawObjectsPage();

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
            const float panelWidth = (std::min)(
                375.0F,
                viewport->WorkSize.x * 0.32F);
            const float panelHeight = (std::max)(
                180.0F,
                viewport->WorkSize.y - toolbarHeight - 70.0F);

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

            DrawLevelEditorToolbar();
            ImGui::End();

            ImGui::SetNextWindowDockID(0U, ImGuiCond_Always);
            ImGui::SetNextWindowPos(
                ImVec2(
                    viewport->WorkPos.x + viewport->WorkSize.x - panelWidth - 5.0F,
                    viewport->WorkPos.y + toolbarHeight + 5.0F),
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

            DrawActivePage();
            ImGui::End();
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

        if (!g_colorCorrectionRenderer.Initialize(device))
        {
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
        RefreshObjectDepot();
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
        g_staticMeshRenderer.Shutdown(device);
        g_terrainRenderer.Shutdown(device);
        g_gridRenderer.Shutdown(device);
        g_skyRenderer.Shutdown(device);
        g_commandHistory.Clear();
        g_sceneDocument.Clear();
        g_loadedTerrainPath.clear();
        g_loadedMapName.clear();
        g_levelLoadStats = {};
        g_levelLoadStatus = "No map selected.";
        g_device = nullptr;
        g_window = {};
        g_initialized = false;
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

        return g_staticMeshRenderer.Render(
            context,
            g_sceneDocument,
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
