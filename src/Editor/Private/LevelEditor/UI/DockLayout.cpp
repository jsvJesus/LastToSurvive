#include "Editor/LevelEditor/UI/DockLayout.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace lts::editor
{
    std::uint32_t DockLayout::DrawDockSpace() noexcept
    {
        const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(
            0,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode);

        if (resetRequested_ || ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
        {
            BuildDefault(dockspaceId);
            resetRequested_ = false;
        }

        return dockspaceId;
    }

    void DockLayout::RequestReset() noexcept
    {
        resetRequested_ = true;
    }

    void DockLayout::BuildDefault(
        const std::uint32_t dockspaceId) noexcept
    {
        ImGuiViewport* const viewport = ImGui::GetMainViewport();
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(
            dockspaceId,
            ImGuiDockNodeFlags_DockSpace |
                ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        ImGuiID center = dockspaceId;
        const ImGuiID toolbar = ImGui::DockBuilderSplitNode(
            center, ImGuiDir_Up, 0.055F, nullptr, &center);
        const ImGuiID left = ImGui::DockBuilderSplitNode(
            center, ImGuiDir_Left, 0.18F, nullptr, &center);
        const ImGuiID right = ImGui::DockBuilderSplitNode(
            center, ImGuiDir_Right, 0.23F, nullptr, &center);
        const ImGuiID bottom = ImGui::DockBuilderSplitNode(
            center, ImGuiDir_Down, 0.26F, nullptr, &center);
        ImGuiID rightTop = right;
        const ImGuiID rightBottom = ImGui::DockBuilderSplitNode(
            rightTop, ImGuiDir_Down, 0.48F, nullptr, &rightTop);

        ImGui::DockBuilderDockWindow("Toolbar", toolbar);
        ImGui::DockBuilderDockWindow("Place Actors", left);
        ImGui::DockBuilderDockWindow("World Outliner", left);
        ImGui::DockBuilderDockWindow("Viewport", center);
        ImGui::DockBuilderDockWindow("FBX Importer", center);
        ImGui::DockBuilderDockWindow("WarZ Importer", center);
        ImGui::DockBuilderDockWindow("Inspector", rightTop);
        ImGui::DockBuilderDockWindow("World Settings", rightBottom);
        ImGui::DockBuilderDockWindow("Content Browser", bottom);
        ImGui::DockBuilderDockWindow("Console", bottom);
        ImGui::DockBuilderDockWindow("Level Statistics", bottom);
        ImGui::DockBuilderFinish(dockspaceId);
    }
}
