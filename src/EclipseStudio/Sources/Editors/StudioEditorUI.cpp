#include "StudioEditorUI.h"

#include <imgui.h>

namespace studio::editor
{
    namespace
    {
        EditorWorkspace g_activeWorkspace =
            EditorWorkspace::Settings;

        bool g_showRendererWindow = true;

        const char* GetWorkspaceName(
            const EditorWorkspace workspace) noexcept
        {
            switch (workspace)
            {
            case EditorWorkspace::Settings:
                return "Settings";

            case EditorWorkspace::Objects:
                return "Objects";

            case EditorWorkspace::Materials:
                return "Materials";

            case EditorWorkspace::Environment:
                return "Environment";

            case EditorWorkspace::Collections:
                return "Collections";

            case EditorWorkspace::Decorators:
                return "Decorators";

            case EditorWorkspace::Roads:
                return "Roads";

            case EditorWorkspace::Gameplay:
                return "Gameplay";

            case EditorWorkspace::PostFX:
                return "Post FX";

            case EditorWorkspace::ColorCorrection:
                return "Color Correction";

            default:
                return "Unknown";
            }
        }

        void DrawMainMenu() noexcept
        {
            if (!ImGui::BeginMenuBar())
            {
                return;
            }

            if (ImGui::BeginMenu("File"))
            {
                ImGui::TextDisabled(
                    "DX11 editor commands will be migrated here");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                ImGui::TextDisabled(
                    "Undo / Redo migration pending");

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem(
                    "Renderer",
                    nullptr,
                    &g_showRendererWindow);

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        void DrawWorkspaceButton(
            const EditorWorkspace workspace) noexcept
        {
            const bool selected =
                g_activeWorkspace == workspace;

            if (selected)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImGui::GetStyleColorVec4(
                        ImGuiCol_ButtonActive));
            }

            if (ImGui::Button(
                    GetWorkspaceName(workspace)))
            {
                g_activeWorkspace = workspace;
            }

            if (selected)
            {
                ImGui::PopStyleColor();
            }
        }

        void DrawWorkspaceBar() noexcept
        {
            DrawWorkspaceButton(
                EditorWorkspace::Settings);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Objects);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Materials);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Environment);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Collections);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Decorators);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Roads);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::Gameplay);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::PostFX);

            ImGui::SameLine();

            DrawWorkspaceButton(
                EditorWorkspace::ColorCorrection);
        }

        void DrawDockSpace() noexcept
        {
            const ImGuiViewport* viewport =
                ImGui::GetMainViewport();

            ImGui::SetNextWindowPos(
                viewport->WorkPos);

            ImGui::SetNextWindowSize(
                viewport->WorkSize);

            ImGui::SetNextWindowViewport(
                viewport->ID);

            ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus;

            ImGui::PushStyleVar(
                ImGuiStyleVar_WindowRounding,
                0.0F);

            ImGui::PushStyleVar(
                ImGuiStyleVar_WindowBorderSize,
                0.0F);

            ImGui::PushStyleVar(
                ImGuiStyleVar_WindowPadding,
                ImVec2(0.0F, 0.0F));

            ImGui::Begin(
                "EditorDockSpace",
                nullptr,
                windowFlags);

            ImGui::PopStyleVar(3);

            DrawMainMenu();

            const ImGuiID dockSpaceId =
                ImGui::GetID(
                    "EditorMainDockSpace");

            ImGui::DockSpace(
                dockSpaceId,
                ImVec2(0.0F, 0.0F),
                ImGuiDockNodeFlags_PassthruCentralNode);

            ImGui::End();
        }

        void DrawWorkspaceWindow() noexcept
        {
            ImGui::SetNextWindowSize(
                ImVec2(720.0F, 90.0F),
                ImGuiCond_FirstUseEver);

            if (!ImGui::Begin("Editor"))
            {
                ImGui::End();
                return;
            }

            DrawWorkspaceBar();

            ImGui::Separator();

            ImGui::Text(
                "Workspace: %s",
                GetWorkspaceName(
                    g_activeWorkspace));

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

            ImGui::Text(
                "Display: %.0f x %.0f",
                io.DisplaySize.x,
                io.DisplaySize.y);

            ImGui::End();
        }
    }

    EditorWorkspace
        GetActiveWorkspace() noexcept
    {
        return g_activeWorkspace;
    }

    void DrawEditorUI() noexcept
    {
        DrawDockSpace();
        DrawWorkspaceWindow();
        DrawRendererWindow();
    }
}