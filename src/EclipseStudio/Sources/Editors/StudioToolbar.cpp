#include "StudioToolbar.h"

#include <imgui.h>

#include <array>
#include <cstddef>

namespace studio::editor
{
    const char* StudioToolbar::GetPageName(
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

    const char* StudioToolbar::GetTerrainEditorToolName(TerrainEditorTool tool) noexcept
    {
        switch (tool)
        {
        case TerrainEditorTool::Options:
            return "Options";

        case TerrainEditorTool::Down:
            return "Down";

        case TerrainEditorTool::Up:
            return "Up";

        case TerrainEditorTool::Level:
            return "Level";

        case TerrainEditorTool::Smooth:
            return "Smooth";

        case TerrainEditorTool::Noise:
            return "Noise";

        case TerrainEditorTool::Ramp:
            return "Ramp";

        case TerrainEditorTool::Erosion:
            return "Erosion";

        case TerrainEditorTool::Paint:
            return "Paint";

        case TerrainEditorTool::Heightmap:
            return "Heightmap";

        default:
            return "Unknown";
        }
    }

    bool StudioToolbar::DrawTab(
        const char* const label,
        const bool active,
        const float width) noexcept
    {
        if (active)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImGui::GetStyleColorVec4(
                    ImGuiCol_ButtonActive));
        }

        const bool pressed = ImGui::Button(
            label,
            ImVec2(width, 0.0F));

        if (active)
        {
            ImGui::PopStyleColor();
        }

        return pressed;
    }

    void StudioToolbar::DrawMain(
        LevelEditorPage& activePage) const noexcept
    {
        constexpr std::array<LevelEditorPage, 11U> pages
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

        for (std::size_t index = 0U;
             index < pages.size();
             ++index)
        {
            const LevelEditorPage page =
                pages[index];

            const char* const name =
                GetPageName(page);

            if (index > 0U)
            {
                const float nextWidth =
                    ImGui::CalcTextSize(name).x +
                    ImGui::GetStyle().FramePadding.x *
                        2.0F;

                if (
                    ImGui::GetCursorPosX() +
                        nextWidth <
                    ImGui::GetContentRegionMax().x)
                {
                    ImGui::SameLine();
                }
            }

            if (DrawTab(
                    name,
                    activePage == page))
            {
                activePage = page;
            }
        }
    }

    void StudioToolbar::DrawSettings(
        SettingsToolbarPage& activePage) const noexcept
    {
        constexpr float buttonWidth = 165.0F;

        if (DrawTab(
                "System Settings",
                activePage ==
                    SettingsToolbarPage::SystemSettings,
                buttonWidth))
        {
            activePage =
                SettingsToolbarPage::SystemSettings;
        }

        ImGui::SameLine();

        if (DrawTab(
                "Options Menu",
                activePage ==
                    SettingsToolbarPage::OptionsMenu,
                buttonWidth))
        {
            activePage =
                SettingsToolbarPage::OptionsMenu;
        }
    }

    void StudioToolbar::DrawTerrain(TerrainToolbarPage& activePage) const noexcept
    {
        constexpr float buttonWidth = 165.0F;

        if (DrawTab(
                "Terrain Loader",
                activePage ==
                    TerrainToolbarPage::TerrainLoader,
                buttonWidth))
        {
            activePage =
                TerrainToolbarPage::TerrainLoader;
        }

        ImGui::SameLine();

        if (DrawTab(
                "Terrain Editor",
                activePage ==
                    TerrainToolbarPage::TerrainEditor,
                buttonWidth))
        {
            activePage =
                TerrainToolbarPage::TerrainEditor;
        }
    }

    void StudioToolbar::DrawTerrainEditorTools(
        TerrainEditorTool& activeTool) const noexcept
    {
        constexpr std::array<TerrainEditorTool, 10U> tools
        {
            TerrainEditorTool::Options,
            TerrainEditorTool::Down,
            TerrainEditorTool::Up,
            TerrainEditorTool::Level,
            TerrainEditorTool::Smooth,
            TerrainEditorTool::Noise,
            TerrainEditorTool::Ramp,
            TerrainEditorTool::Erosion,
            TerrainEditorTool::Paint,
            TerrainEditorTool::Heightmap
        };

        for (std::size_t index = 0U;
             index < tools.size();
             ++index)
        {
            const TerrainEditorTool tool =
                tools[index];

            if (index > 0U)
            {
                ImGui::SameLine();
            }

            if (DrawTab(
                    GetTerrainEditorToolName(tool),
                    activeTool == tool))
            {
                activeTool = tool;
            }
        }
    }
}
