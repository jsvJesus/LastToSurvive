#include "StudioEditorUI.h"

#include <imgui.h>

namespace studio::editor
{
    void DrawEditorUI() noexcept
    {
        ImGui::DockSpaceOverViewport(
            0,
            nullptr,
            ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::SetNextWindowSize(
            ImVec2(360.0F, 160.0F),
            ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Renderer"))
        {
            ImGui::TextUnformatted(
                "Renderer: Direct3D 11");

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
        }

        ImGui::End();
    }
}