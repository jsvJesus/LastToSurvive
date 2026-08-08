#include "StudioEditorUI.h"

#include <imgui.h>

namespace studio::editor
{
    namespace
    {
        LevelEditorPage g_activePage =
            LevelEditorPage::Settings;

        bool g_showRendererWindow = true;

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
            DrawPageButton(
                LevelEditorPage::Settings);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Terrain);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Objects);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Materials);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Environment);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Collections);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Decorators);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Roads);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::Gameplay);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::PostFX);

            ImGui::SameLine();

            DrawPageButton(
                LevelEditorPage::ColorCorrection);
        }

        void DrawTerrainPage() noexcept
        {
            ImGui::TextUnformatted(
                "Terrain");

            ImGui::Separator();

            ImGui::TextUnformatted(
                "DX11 Terrain");

            ImGui::Spacing();

            ImGui::TextDisabled(
                "Legacy Terrain V1 / Terrain V2 are not used.");

            ImGui::Spacing();

            if (ImGui::Button(
                    "Import .r16",
                    ImVec2(140.0F, 28.0F)))
            {
                // R16 importer will be connected here.
            }

            ImGui::SameLine();

            ImGui::TextDisabled(
                "No heightmap loaded");
        }

        void DrawPlaceholderPage(
            const char* title) noexcept
        {
            ImGui::TextUnformatted(title);

            ImGui::Separator();

            ImGui::TextDisabled(
                "Dear ImGui migration pending.");
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

                DrawPlaceholderPage(
                    "Objects");

                break;

            case LevelEditorPage::Materials:

                DrawPlaceholderPage(
                    "Materials");

                break;

            case LevelEditorPage::Environment:

                DrawPlaceholderPage(
                    "Environment");

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

                DrawPlaceholderPage(
                    "Color Correction");

                break;
            }
        }

        void DrawLevelEditor() noexcept
        {
            ImGui::SetNextWindowSize(
                ImVec2(1000.0F, 600.0F),
                ImGuiCond_FirstUseEver);

            if (!ImGui::Begin(
                    "Level Editor"))
            {
                ImGui::End();

                return;
            }

            DrawLevelEditorToolbar();

            ImGui::Separator();

            DrawActivePage();

            ImGui::End();
        }

        void DrawRendererWindow() noexcept
        {
            if (!g_showRendererWindow)
            {
                return;
            }

            ImGui::SetNextWindowSize(
                ImVec2(360.0F, 180.0F),
                ImGuiCond_FirstUseEver);

            if (!ImGui::Begin(
                    "Renderer",
                    &g_showRendererWindow))
            {
                ImGui::End();

                return;
            }

            ImGui::TextUnformatted(
                "Renderer backend: Direct3D 11");

            ImGui::TextUnformatted(
                "Editor UI: Dear ImGui");

            ImGui::Separator();

            const ImGuiIO& io =
                ImGui::GetIO();

            const float frameTimeMs =
                io.Framerate > 0.0F
                    ? 1000.0F / io.Framerate
                    : 0.0F;

            ImGui::Text(
                "Frame: %.3f ms",
                frameTimeMs);

            ImGui::Text(
                "FPS: %.1f",
                io.Framerate);

            ImGui::End();
        }
    }

    LevelEditorPage
        GetActiveLevelEditorPage() noexcept
    {
        return g_activePage;
    }

    void DrawEditorUI() noexcept
    {
        ImGui::DockSpaceOverViewport(
            0,
            nullptr,
            ImGuiDockNodeFlags_PassthruCentralNode);

        DrawLevelEditor();
        DrawRendererWindow();
    }
}