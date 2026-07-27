#include "Editor/Tools/ToolWindowManager.h"

#include <imgui.h>
#include <iterator>

#include <initializer_list>

namespace lts::editor
{
    namespace
    {
        void DrawPlannedFields(
            const std::initializer_list<const char*> fields) noexcept
        {
            for (const char* const field : fields)
            {
                ImGui::BulletText("%s", field);
            }
        }

        void DrawDevelopmentNotice() noexcept
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled(
                "This tool window is connected to LTS.Editor.");
            ImGui::TextDisabled(
                "Asset logic will be implemented in the next stages.");
        }
    }

    EditorToolAction
    ToolWindowManager::DrawToolsMenu() noexcept
    {
        EditorToolAction action =
            EditorToolAction::None;

        if (!ImGui::BeginMenu("Tools"))
        {
            return action;
        }

        if (ImGui::MenuItem(
                "Test Game",
                "F5"))
        {
            action =
                EditorToolAction::TestGame;
        }

        ImGui::Separator();

        ImGui::MenuItem(
            "Character Editor",
            nullptr,
            &characterEditorOpen_);

        ImGui::MenuItem(
            "Physics Editor",
            nullptr,
            &physicsEditorOpen_);

        ImGui::MenuItem(
            "FBX Importer",
            nullptr,
            &fbxImporterOpen_);

        const bool warZImporterOpen = warZImporterWindow_.IsOpen();
        if (ImGui::MenuItem(
                "WarZ Importer",
                nullptr,
                warZImporterOpen))
        {
            warZImporterWindow_.SetOpen(
                !warZImporterOpen);
        }

        ImGui::MenuItem(
            "Icon Generator",
            nullptr,
            &iconGeneratorOpen_);

        ImGui::EndMenu();

        return action;
    }

    void ToolWindowManager::
        DrawOpenWindows() noexcept
    {
        DrawCharacterEditor();
        DrawPhysicsEditor();
        DrawFbxImporter();
        DrawIconGenerator();

        warZImporterWindow_.Draw();
    }

    void ToolWindowManager::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) noexcept
    {
        warZImporterWindow_.Initialize(device, context);
    }

    void ToolWindowManager::Shutdown() noexcept
    {
        warZImporterWindow_.Shutdown();
    }

    void ToolWindowManager::
        DrawCharacterEditor() noexcept
    {
        if (!characterEditorOpen_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(840.0F, 620.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "Character Editor",
                &characterEditorOpen_,
                ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Character asset editor and preview");
        ImGui::Separator();

        if (ImGui::BeginTable(
                "CharacterEditorLayout",
                2,
                ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(
                "Settings",
                ImGuiTableColumnFlags_WidthFixed,
                300.0F);

            ImGui::TableSetupColumn(
                "Preview",
                ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Character");

            DrawPlannedFields(
                {
                    "Character Asset",
                    "Skeletal Mesh",
                    "Skeleton",
                    "Materials",
                    "Animation Set",
                    "First-Person Arms"
                });

            ImGui::SeparatorText("Equipment");

            DrawPlannedFields(
                {
                    "Equipment Sockets",
                    "Weapon Attachments",
                    "Backpack Socket",
                    "Vest Socket"
                });

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Character Preview");
            ImGui::TextDisabled(
                "3D character preview render target");
            ImGui::Dummy(ImVec2(1.0F, 400.0F));

            ImGui::EndTable();
        }

        DrawDevelopmentNotice();
        ImGui::End();
    }

    void ToolWindowManager::
        DrawPhysicsEditor() noexcept
    {
        if (!physicsEditorOpen_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(820.0F, 600.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "Physics Editor",
                &physicsEditorOpen_,
                ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Physics and collision editor");
        ImGui::Separator();

        if (ImGui::BeginTable(
                "PhysicsEditorLayout",
                2,
                ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(
                "Physics Settings",
                ImGuiTableColumnFlags_WidthFixed,
                320.0F);

            ImGui::TableSetupColumn(
                "Physics Preview",
                ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Asset");

            DrawPlannedFields(
                {
                    "Selected Asset",
                    "Static Mesh",
                    "Skeletal Mesh"
                });

            ImGui::SeparatorText("Collision");

            DrawPlannedFields(
                {
                    "Box Shape",
                    "Sphere Shape",
                    "Capsule Shape",
                    "Convex Shape",
                    "Mesh Collision"
                });

            ImGui::SeparatorText("Properties");

            DrawPlannedFields(
                {
                    "Mass",
                    "Friction",
                    "Restitution",
                    "Center of Mass",
                    "Constraints"
                });

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Physics Preview");
            ImGui::TextDisabled(
                "Physics simulation viewport");
            ImGui::Dummy(ImVec2(1.0F, 400.0F));

            ImGui::EndTable();
        }

        DrawDevelopmentNotice();
        ImGui::End();
    }

    void ToolWindowManager::
        DrawFbxImporter() noexcept
    {
        if (!fbxImporterOpen_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(760.0F, 580.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "FBX Importer",
                &fbxImporterOpen_,
                ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Import FBX assets into LTS asset formats");
        ImGui::Separator();

        ImGui::SeparatorText("Source");
        ImGui::TextDisabled("Source FBX: Not selected");
        ImGui::TextDisabled("Destination: Data/Meshes");

        ImGui::SeparatorText("Asset Type");

        static int assetType = 0;

        ImGui::RadioButton(
            "Static Mesh",
            &assetType,
            0);

        ImGui::SameLine();

        ImGui::RadioButton(
            "Skeletal Mesh",
            &assetType,
            1);

        ImGui::SeparatorText("Import Settings");

        static float importScale = 1.0F;
        static bool importMaterials = true;
        static bool importAnimations = true;
        static bool generateCollision = true;
        static bool recalculateNormals = false;
        static bool recalculateTangents = true;

        ImGui::DragFloat(
            "Import Scale",
            &importScale,
            0.01F,
            0.001F,
            1000.0F);

        ImGui::Checkbox(
            "Import Materials",
            &importMaterials);

        ImGui::Checkbox(
            "Import Animations",
            &importAnimations);

        ImGui::Checkbox(
            "Generate Collision",
            &generateCollision);

        ImGui::Checkbox(
            "Recalculate Normals",
            &recalculateNormals);

        ImGui::Checkbox(
            "Recalculate Tangents",
            &recalculateTangents);

        ImGui::Spacing();

        ImGui::BeginDisabled();
        ImGui::Button(
            "Import FBX",
            ImVec2(160.0F, 32.0F));
        ImGui::EndDisabled();

        ImGui::TextDisabled(
            "The import button will be connected to "
            "FbxStaticMeshImporter later.");

        ImGui::End();
    }

    void ToolWindowManager::
        DrawIconGenerator() noexcept
    {
        if (!iconGeneratorOpen_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(820.0F, 600.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "Icon Generator",
                &iconGeneratorOpen_,
                ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Generate inventory icons from item assets");
        ImGui::Separator();

        if (ImGui::BeginTable(
                "IconGeneratorLayout",
                2,
                ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(
                "Icon Settings",
                ImGuiTableColumnFlags_WidthFixed,
                310.0F);

            ImGui::TableSetupColumn(
                "Preview",
                ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Item");
            ImGui::TextDisabled("Item Asset: Not selected");
            ImGui::TextDisabled("Mesh Asset: Not selected");

            ImGui::SeparatorText("Rendering");

            static int resolutionIndex = 2;
            constexpr const char* resolutions[]
            {
                "64 x 64",
                "128 x 128",
                "256 x 256",
                "512 x 512"
            };

            ImGui::Combo(
                "Resolution",
                &resolutionIndex,
                resolutions,
                static_cast<int>(
                    std::size(resolutions)));

            static float fieldOfView = 30.0F;
            static float padding = 0.08F;
            static bool transparentBackground = true;

            ImGui::DragFloat(
                "Field of View",
                &fieldOfView,
                0.1F,
                5.0F,
                120.0F);

            ImGui::DragFloat(
                "Padding",
                &padding,
                0.005F,
                0.0F,
                0.5F);

            ImGui::Checkbox(
                "Transparent Background",
                &transparentBackground);

            ImGui::SeparatorText("Output");
            ImGui::TextDisabled(
                "Output: Data/UI/ItemIcons");

            ImGui::BeginDisabled();
            ImGui::Button(
                "Generate Icon",
                ImVec2(160.0F, 32.0F));
            ImGui::EndDisabled();

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Icon Preview");
            ImGui::TextDisabled(
                "Off-screen item render preview");
            ImGui::Dummy(ImVec2(1.0F, 400.0F));

            ImGui::EndTable();
        }

        ImGui::End();
    }
}