#pragma once

#include "StudioEditorUI.h"

#include <cstdint>

namespace studio::editor
{
    enum class SettingsToolbarPage : std::uint8_t
    {
        SystemSettings = 0,
        OptionsMenu
    };

    enum class TerrainToolbarPage : std::uint8_t
    {
        TerrainLoader = 0,
        TerrainEditor
    };

    enum class TerrainEditorTool : std::uint8_t
    {
        Options = 0,
        Down,
        Up,
        Level,
        Smooth,
        Noise,
        Ramp,
        Erosion,
        Paint,
        Heightmap
    };

    enum class EnvironmentToolbarPage : std::uint8_t
    {
        LightSetup = 0,
        Atmosphere,
        CloudPlane,
        Grass,
        WaterPlanes,
        Decals,
        Rain,
        Weather
    };

    enum class EnvironmentLightTool : std::uint8_t
    {
        SunSetup = 0,
        MoonSetup,
        SkySetup
    };

    class StudioToolbar final
    {
    public:
        StudioToolbar() noexcept = default;
        ~StudioToolbar() noexcept = default;

        StudioToolbar(
            const StudioToolbar&) = delete;

        StudioToolbar& operator=(
            const StudioToolbar&) = delete;

        void DrawMain(
            LevelEditorPage& activePage) const noexcept;

        void DrawSettings(
            SettingsToolbarPage& activePage) const noexcept;

        void DrawTerrain(
            TerrainToolbarPage& activePage) const noexcept;

        void DrawTerrainEditorTools(
            TerrainEditorTool& activeTool) const noexcept;

        void DrawEnvironment(
            EnvironmentToolbarPage& activePage) const noexcept;

        void DrawEnvironmentLightTools(
            EnvironmentLightTool& activeTool) const noexcept;

    private:
        [[nodiscard]]
        static const char* GetPageName(
            LevelEditorPage page) noexcept;

        [[nodiscard]]
        static const char* GetTerrainEditorToolName(
            TerrainEditorTool tool) noexcept;

        [[nodiscard]]
        static const char* GetEnvironmentPageName(
            EnvironmentToolbarPage page) noexcept;

        [[nodiscard]]
        static const char* GetEnvironmentLightToolName(
            EnvironmentLightTool tool) noexcept;

        [[nodiscard]]
        static bool DrawTab(
            const char* label,
            bool active,
            float width = 0.0F) noexcept;
    };
}