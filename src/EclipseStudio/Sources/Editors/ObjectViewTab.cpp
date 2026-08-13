#include "ObjectViewTab.h"

#include <Editor/Commands/CommandHistory.h>
#include <Editor/LevelEditor/Rendering/SceneRenderer.h>
#include <Editor/LevelEditor/Rendering/StaticMeshRenderer.h>
#include <Editor/LevelEditor/Scene/SceneDocument.h>
#include <Editor/LevelEditor/Scene/ScenePicker.h>
#include <Editor/LevelEditor/Terrain/TerrainRenderer.h>
#include <Editor/LevelEditor/UI/MaterialInspector.h>
#include <Editor/LevelEditor/Viewport/CameraController.h>
#include <Editor/LevelEditor/Viewport/TransformController.h>

#include <Graphics/CommandContext.h>
#include <Graphics/GraphicsBackend.h>
#include <Graphics/RenderDevice.h>
#include <Graphics/Texture.h>
#include <Graphics/Viewport.h>
#include <GraphicsDX11/D3D11Device.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cwctype>
#include <cmath>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace studio::editor
{
    namespace
    {
        constexpr std::size_t InvalidModelIndex =
            std::numeric_limits<std::size_t>::max();

        constexpr std::uint32_t MinimumPreviewSize = 64U;
        constexpr std::uint32_t MaximumPreviewSize = 2048U;

        struct StaticMeshEntry final
        {
            std::filesystem::path physicalPath;
            std::wstring assetPath;
            std::string relativePath;
            std::string lowercasePath;
            std::string category;
        };

        [[nodiscard]]
        std::string LowercaseAscii(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        std::wstring LowercaseWide(std::wstring value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            return value;
        }

        [[nodiscard]]
        std::filesystem::path FindWorkspaceRoot() noexcept
        {
            try
            {
                std::error_code error;
                std::filesystem::path current =
                    std::filesystem::absolute(
                        std::filesystem::current_path(error),
                        error).lexically_normal();

                if (error)
                {
                    return {};
                }

                while (!current.empty())
                {
                    error.clear();

                    if (std::filesystem::is_directory(
                            current / L"bin" / L"Data" / L"StaticMeshes",
                            error) &&
                        !error)
                    {
                        return current;
                    }

                    const std::filesystem::path parent =
                        current.parent_path();

                    if (parent.empty() || parent == current)
                    {
                        break;
                    }

                    current = parent;
                }
            }
            catch (...)
            {
            }

            return {};
        }

        [[nodiscard]]
        bool IsInsideViewport(
            const ImVec2 mousePosition,
            const ObjectViewContext& context) noexcept
        {
            return
                mousePosition.x >= static_cast<float>(context.viewportX) &&
                mousePosition.y >= static_cast<float>(context.viewportY) &&
                mousePosition.x <
                    static_cast<float>(context.viewportX) +
                        static_cast<float>(context.viewportWidth) &&
                mousePosition.y <
                    static_cast<float>(context.viewportY) +
                        static_cast<float>(context.viewportHeight);
        }

        [[nodiscard]]
        lts::editor::ViewportClick BuildViewportClick(
            const ImVec2 mousePosition,
            const ObjectViewContext& context) noexcept
        {
            const float localX =
                mousePosition.x -
                static_cast<float>(context.viewportX);

            const float localY =
                mousePosition.y -
                static_cast<float>(context.viewportY);

            return
            {
                static_cast<std::uint32_t>(
                    std::clamp(
                        localX,
                        0.0F,
                        static_cast<float>(context.viewportWidth - 1U))),

                static_cast<std::uint32_t>(
                    std::clamp(
                        localY,
                        0.0F,
                        static_cast<float>(context.viewportHeight - 1U)))
            };
        }

        [[nodiscard]]
        bool DrawToolButton(
            const char* label,
            const bool active) noexcept
        {
            if (active)
            {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImGui::GetStyleColorVec4(
                        ImGuiCol_ButtonActive));
            }

            const bool pressed = ImGui::Button(label);

            if (active)
            {
                ImGui::PopStyleColor();
            }

            return pressed;
        }
    }

    class ObjectViewTab::Impl final
    {
    public:
        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device,
            const engine::platform::NativeWindowHandle window) noexcept
        {
            if (initialized_)
            {
                return device_ == &device;
            }

            if (!sceneRenderer_.Initialize(device))
            {
                return false;
            }

            device_ = &device;
            window_ = window;
            
            transformController_.SetViewportWindow(window);
            transformController_.SetKeyboardShortcutsEnabled(false);
            transformController_.SetEditorRoadPickingEnabled(false);
            
            initialized_ = true;

            Refresh();
            return true;
        }

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept
        {
            if (!initialized_)
            {
                return;
            }

            DestroyPreviewTargets(device);
            sceneRenderer_.Shutdown(device);
            transformController_.SetViewportWindow({});
            materialInspector_.Reset();
            previewDocument_.Clear();
            models_.clear();
            categories_.clear();
            device_ = nullptr;
            window_ = {};
            initialized_ = false;
        }

        void Refresh() noexcept
        {
            models_.clear();
            categories_.clear();
            selectedModel_ = InvalidModelIndex;
            selectedCategory_ = -1;
            filter_.fill('\0');
            previewAssetPath_.clear();
            inspectedAssetPath_.clear();
            previewDocument_.Clear();
            materialInspector_.Reset();

            try
            {
                workspaceRoot_ = FindWorkspaceRoot();
                staticMeshesRoot_ =
                    workspaceRoot_ /
                    L"bin" /
                    L"Data" /
                    L"StaticMeshes";

                std::error_code error;

                if (workspaceRoot_.empty() ||
                    !std::filesystem::is_directory(
                        staticMeshesRoot_,
                        error) ||
                    error)
                {
                    status_ =
                        "bin/Data/StaticMeshes directory was not found.";
                    scanned_ = true;
                    return;
                }

                for (std::filesystem::recursive_directory_iterator iterator(
                         staticMeshesRoot_,
                         std::filesystem::directory_options::
                             skip_permission_denied,
                         error),
                     end;
                     iterator != end;
                     iterator.increment(error))
                {
                    if (error)
                    {
                        error.clear();
                        continue;
                    }

                    if (!iterator->is_regular_file(error) || error)
                    {
                        error.clear();
                        continue;
                    }

                    if (LowercaseWide(
                            iterator->path().extension().wstring()) !=
                        L".mesh")
                    {
                        continue;
                    }

                    const std::filesystem::path relative =
                        std::filesystem::relative(
                            iterator->path(),
                            staticMeshesRoot_,
                            error);

                    if (error)
                    {
                        error.clear();
                        continue;
                    }

                    StaticMeshEntry entry;
                    entry.physicalPath =
                        iterator->path().lexically_normal();

                    entry.relativePath =
                        relative.generic_u8string();

                    entry.lowercasePath =
                        LowercaseAscii(entry.relativePath);

                    entry.assetPath =
                        (std::filesystem::path(L"Data") /
                         L"StaticMeshes" /
                         relative).generic_wstring();

                    const std::filesystem::path parent =
                        relative.parent_path();

                    entry.category =
                        parent.empty()
                            ? "(Root)"
                            : parent.begin()->u8string();

                    models_.push_back(std::move(entry));
                }

                std::sort(
                    models_.begin(),
                    models_.end(),
                    [](const StaticMeshEntry& left,
                       const StaticMeshEntry& right)
                    {
                        return left.lowercasePath < right.lowercasePath;
                    });

                for (const StaticMeshEntry& model : models_)
                {
                    categories_.push_back(model.category);
                }

                std::sort(
                    categories_.begin(),
                    categories_.end());

                categories_.erase(
                    std::unique(
                        categories_.begin(),
                        categories_.end()),
                    categories_.end());

                status_ =
                    std::to_string(models_.size()) +
                    " .mesh models found.";

                scanned_ = true;
            }
            catch (...)
            {
                models_.clear();
                categories_.clear();
                status_ = "StaticMeshes scan failed.";
                scanned_ = true;
            }
        }

        void DrawToolbar(
            ObjectViewContext& context) noexcept
        {
            const auto operation =
                transformController_.GetVisualState().operation;

            if (DrawToolButton(
                    "Gizmo: Move",
                    operation ==
                        lts::editor::EditorTransformOperation::Move))
            {
                transformController_.SetOperation(
                    lts::editor::EditorTransformOperation::Move);
            }

            ImGui::SameLine();

            if (DrawToolButton(
                    "Gizmo: Rotate",
                    operation ==
                        lts::editor::EditorTransformOperation::Rotate))
            {
                transformController_.SetOperation(
                    lts::editor::EditorTransformOperation::Rotate);
            }

            ImGui::SameLine();

            if (DrawToolButton(
                    "Gizmo: Transform",
                    operation ==
                        lts::editor::EditorTransformOperation::Scale))
            {
                transformController_.SetOperation(
                    lts::editor::EditorTransformOperation::Scale);
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(
                    "Scale operation (R). Universal gizmo will be added later.");
            }

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            if (DrawToolButton(
                    scenePickerEnabled_
                        ? "Scene Picker: On"
                        : "Scene Picker: Off",
                    scenePickerEnabled_))
            {
                scenePickerEnabled_ = !scenePickerEnabled_;
            }

            ImGui::SameLine();

            if (DrawToolButton(
                    "Scene Objects View",
                    sceneObjectsWindowOpen_))
            {
                sceneObjectsWindowOpen_ =
                    !sceneObjectsWindowOpen_;
            }

            ImGui::SameLine();

            if (DrawToolButton(
                    "Physics Editor",
                    physicsWindowOpen_))
            {
                physicsWindowOpen_ = !physicsWindowOpen_;
            }

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            const bool canDelete =
                CanDeleteSelection(
                    context.sceneDocument);

            ImGui::BeginDisabled(!canDelete);

            if (ImGui::Button("Delete"))
            {
                DeleteSelectedObjects(context);
            }

            ImGui::EndDisabled();

            if (ImGui::IsItemHovered(
                    ImGuiHoveredFlags_AllowWhenDisabled))
            {
                ImGui::SetTooltip(
                    canDelete
                        ? "Delete selected object"
                        : "Select an obj_Building or placed StaticMesh first");
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(
                !context.commandHistory.CanUndo());

            if (ImGui::Button("Undo"))
            {
                if (context.commandHistory.Undo(
                        context.sceneDocument))
                {
                    ResetInspectedObject();
                    status_ =
                        "Last object operation was undone.";
                }
            }

            ImGui::EndDisabled();

            ImGui::SameLine();

            ImGui::BeginDisabled(
                !context.commandHistory.CanRedo());

            if (ImGui::Button("Redo"))
            {
                if (context.commandHistory.Redo(
                        context.sceneDocument))
                {
                    ResetInspectedObject();
                    status_ =
                        "Last object operation was restored.";
                }
            }

            ImGui::EndDisabled();

            const ImGuiIO& io =
                ImGui::GetIO();

            if (
                canDelete &&
                !io.WantTextInput &&
                ImGui::IsKeyPressed(
                    ImGuiKey_Delete,
                    false))
            {
                DeleteSelectedObjects(context);
            }
        }

        void DrawPage(
            ObjectViewContext&) noexcept
        {
            if (!scanned_)
            {
                Refresh();
            }

            ImGui::TextUnformatted("Objects Viewer");
            ImGui::Separator();

            ImGui::TextWrapped(
                "Source: %s",
                staticMeshesRoot_.generic_u8string().c_str());

            ImGui::Text("Models: %zu", models_.size());

            if (ImGui::Button("Refresh StaticMeshes"))
            {
                Refresh();
            }

            ImGui::SameLine();
            ImGui::TextDisabled("%s", status_.c_str());

            ImGui::InputTextWithHint(
                "##StaticMeshSearch",
                "Search model...",
                filter_.data(),
                filter_.size());

            const char* currentCategory = "All folders";

            if (selectedCategory_ >= 0 &&
                static_cast<std::size_t>(selectedCategory_) <
                    categories_.size())
            {
                currentCategory =
                    categories_[
                        static_cast<std::size_t>(selectedCategory_)].c_str();
            }

            if (ImGui::BeginCombo("Folder", currentCategory))
            {
                if (ImGui::Selectable(
                        "All folders",
                        selectedCategory_ < 0))
                {
                    selectedCategory_ = -1;
                }

                for (std::size_t index = 0U;
                     index < categories_.size();
                     ++index)
                {
                    const bool selected =
                        selectedCategory_ ==
                        static_cast<int>(index);

                    if (ImGui::Selectable(
                            categories_[index].c_str(),
                            selected))
                    {
                        selectedCategory_ =
                            static_cast<int>(index);
                    }
                }

                ImGui::EndCombo();
            }

            const std::string filter =
                LowercaseAscii(filter_.data());

            std::vector<std::size_t> visibleModels;
            visibleModels.reserve(models_.size());

            for (std::size_t index = 0U;
                 index < models_.size();
                 ++index)
            {
                const StaticMeshEntry& model = models_[index];

                if (selectedCategory_ >= 0 &&
                    model.category !=
                        categories_[
                            static_cast<std::size_t>(selectedCategory_)])
                {
                    continue;
                }

                if (!filter.empty() &&
                    model.lowercasePath.find(filter) ==
                        std::string::npos)
                {
                    continue;
                }

                visibleModels.push_back(index);
            }

            ImGui::SeparatorText("StaticMeshes Models");
            ImGui::TextDisabled(
                "Showing %zu | Select, then Ctrl+LMB in viewport",
                visibleModels.size());

            ImGui::BeginChild(
                "##StaticMeshModels",
                ImVec2(0.0F, 0.0F),
                true,
                ImGuiWindowFlags_HorizontalScrollbar);

            ImGuiListClipper clipper;
            clipper.Begin(
                static_cast<int>(visibleModels.size()));

            while (clipper.Step())
            {
                for (int visibleIndex = clipper.DisplayStart;
                     visibleIndex < clipper.DisplayEnd;
                     ++visibleIndex)
                {
                    const std::size_t modelIndex =
                        visibleModels[
                            static_cast<std::size_t>(visibleIndex)];

                    const StaticMeshEntry& model =
                        models_[modelIndex];

                    ImGui::PushID(
                        static_cast<int>(modelIndex));

                    if (ImGui::Selectable(
                            model.relativePath.c_str(),
                            selectedModel_ == modelIndex))
                    {
                        SelectModel(modelIndex);
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip(
                            "Data/StaticMeshes/%s",
                            model.relativePath.c_str());
                    }

                    ImGui::PopID();
                }
            }

            ImGui::EndChild();
        }

        void DrawWindows(
            ObjectViewContext& context) noexcept
        {
            if (sceneObjectsWindowOpen_)
            {
                DrawSceneObjectsWindow(context);
            }

            if (materialsWindowOpen_)
            {
                DrawMaterialsWindow(context);
            }

            if (physicsWindowOpen_)
            {
                ImGui::SetNextWindowSize(
                    ImVec2(420.0F, 220.0F),
                    ImGuiCond_FirstUseEver);

                if (ImGui::Begin(
                        "Physics Editor",
                        &physicsWindowOpen_))
                {
                    ImGui::TextUnformatted("Physics Editor");
                    ImGui::Separator();
                    ImGui::TextDisabled(
                        "Stub. Collision shapes and rigid-body settings "
                        "will be implemented in the next stage.");
                }

                ImGui::End();
            }
        }

        void UpdateViewport(
            ObjectViewContext& context) noexcept
        {
            if (!initialized_ || !scenePickerEnabled_)
            {
                return;
            }

            context.viewportWidth =
                (std::max)(context.viewportWidth, 1U);

            context.viewportHeight =
                (std::max)(context.viewportHeight, 1U);

            transformController_.SetViewportRegion(
                context.viewportX,
                context.viewportY,
                context.viewportWidth,
                context.viewportHeight);

            const ImGuiIO& io = ImGui::GetIO();
            const ImGuiViewport* const mainViewport =
                ImGui::GetMainViewport();

            const ImVec2 mousePosition
            {
                io.MousePos.x - mainViewport->Pos.x,
                io.MousePos.y - mainViewport->Pos.y
            };

            const bool insideViewport =
                IsInsideViewport(mousePosition, context);

            const bool uiCapturesMouse = io.WantCaptureMouse;

            const bool leftClicked =
                insideViewport &&
                !uiCapturesMouse &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            const bool rightClicked =
                insideViewport &&
                !uiCapturesMouse &&
                ImGui::IsMouseClicked(ImGuiMouseButton_Right);

            const lts::editor::ViewportClick click =
                BuildViewportClick(mousePosition, context);

            if (leftClicked && io.KeyCtrl)
            {
                static_cast<void>(
                    PlaceSelectedModel(click, context));
            }
            else
            {
                const auto transformResult =
                    transformController_.Update(
                        context.sceneDocument,
                        context.commandHistory,
                        context.cameraController,
                        engine::platform::WindowSize
                        {
                            context.viewportWidth,
                            context.viewportHeight
                        },
                        leftClicked ? &click : nullptr,
                        &context.staticMeshRenderer);

                if (transformResult.documentChanged)
                {
                    KeepSelectedMeshOnTerrain(context);
                }
            }

            if (rightClicked)
            {
                SelectSceneObject(click, context);
            }
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            const ObjectViewContext& objectContext,
            const DirectX::XMFLOAT4X4& viewProjection,
            const DirectX::XMFLOAT3& cameraPosition) noexcept
        {
            if (!initialized_)
            {
                return engine::graphics::GraphicsResult::InvalidState;
            }

            auto result = sceneRenderer_.Render(
                context,
                objectContext.sceneDocument,
                viewProjection,
                cameraPosition,
                transformController_.GetVisualState(),
                &objectContext.staticMeshRenderer);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            if (sceneObjectsWindowOpen_ &&
                !previewAssetPath_.empty() &&
                desiredPreviewWidth_ > 0U &&
                desiredPreviewHeight_ > 0U)
            {
                const auto previewResult = RenderPreview(
                    context,
                    objectContext.staticMeshRenderer);

                engine::graphics::Viewport viewport;
                viewport.width = static_cast<float>(
                    objectContext.viewportWidth);
                viewport.height = static_cast<float>(
                    objectContext.viewportHeight);
                viewport.minDepth = 0.0F;
                viewport.maxDepth = 1.0F;

                result = context.SetViewport(viewport);

                if (engine::graphics::Failed(result))
                {
                    return result;
                }

                if (engine::graphics::Failed(previewResult))
                {
                    status_ =
                        "Scene object preview render failed; "
                        "world rendering is still active.";
                }
            }

            return result;
        }

    private:
        [[nodiscard]]
        static bool IsDeletableObject(
            const lts::editor::EditorSceneEntity& entity) noexcept
        {
            if (!entity.staticMesh.has_value())
            {
                return false;
            }

            /*
             * Roads и WaterPlane пока управляются своими
             * отдельными редакторами.
             */
            if (
                entity.editorFolder ==
                    L"LevelData/obj_Road" ||
                entity.editorFolder ==
                    L"LevelData/obj_WaterPlane")
            {
                return false;
            }

            /*
             * Разрешаем:
             * 1. obj_Building, загруженные из LevelData.xml;
             * 2. новые StaticMesh, поставленные через Ctrl+LMB.
             */
            return
                entity.editorFolder.empty() ||
                entity.editorFolder ==
                    L"LevelData/obj_Building";
        }

        [[nodiscard]]
        static bool CanDeleteSelection(
            const lts::editor::SceneDocument& document) noexcept
        {
            const auto& selectedIds =
                document.GetSelectedEntityIds();

            if (selectedIds.empty())
            {
                return false;
            }

            for (
                const lts::editor::EditorEntityId entityId :
                selectedIds)
            {
                const lts::editor::EditorSceneEntity* const entity =
                    document.FindEntity(entityId);

                if (
                    entity == nullptr ||
                    !IsDeletableObject(*entity))
                {
                    return false;
                }
            }

            return true;
        }

        void ResetInspectedObject() noexcept
        {
            inspectedAssetPath_.clear();
            previewAssetPath_.clear();
            previewBoundsValid_ = false;

            previewDocument_.Clear();
            materialInspector_.Reset();
        }

        void DeleteSelectedObjects(
            ObjectViewContext& context) noexcept
        {
            if (!CanDeleteSelection(
                    context.sceneDocument))
            {
                status_ =
                    "Only obj_Building and placed StaticMesh "
                    "objects can be deleted here.";

                return;
            }

            const std::size_t selectedCount =
                context.sceneDocument.
                    GetSelectedEntityIds().
                    size();

            const lts::editor::EditorSceneSnapshot before =
                context.sceneDocument.CreateSnapshot();

            if (!context.sceneDocument.DeleteSelectedEntity())
            {
                status_ =
                    "Could not delete the selected object.";

                return;
            }

            const lts::editor::EditorSceneSnapshot after =
                context.sceneDocument.CreateSnapshot();

            if (!context.commandHistory.Push(
                    before,
                    after))
            {
                context.sceneDocument.RestoreSnapshot(
                    before,
                    false);

                status_ =
                    "Could not register delete command.";

                return;
            }

            ResetInspectedObject();

            status_ =
                std::to_string(selectedCount) +
                (
                    selectedCount == 1U
                        ? " object deleted. Use Undo to restore it."
                        : " objects deleted. Use Undo to restore them."
                );
        }
        
        void SelectModel(
            const std::size_t modelIndex) noexcept
        {
            if (modelIndex >= models_.size())
            {
                return;
            }

            selectedModel_ = modelIndex;
            previewAssetPath_ = models_[modelIndex].assetPath;
            inspectedAssetPath_ = previewAssetPath_;
            sceneObjectsWindowOpen_ = true;
            previewBoundsValid_ = false;

            previewDocument_.Clear();
            lts::editor::EditorTransform transform;

            static_cast<void>(
                previewDocument_.CreateStaticMeshEntity(
                    models_[modelIndex].physicalPath.stem().wstring(),
                    previewAssetPath_,
                    transform));
        }

        [[nodiscard]]
        bool PlaceSelectedModel(
            const lts::editor::ViewportClick& click,
            ObjectViewContext& context) noexcept
        {
            if (selectedModel_ >= models_.size())
            {
                status_ =
                    "Select a StaticMeshes model before placement.";
                return false;
            }

            lts::editor::EditorPickRay ray;

            if (!context.cameraController.BuildPickRay(
                    click.x,
                    click.y,
                    context.viewportWidth,
                    context.viewportHeight,
                    ray))
            {
                status_ = "Could not build the placement ray.";
                return false;
            }

            float distance = 10.0F;

            if (std::abs(ray.direction.y) > 0.00001F)
            {
                const float planeDistance =
                    -ray.origin.y / ray.direction.y;

                if (planeDistance >= 0.0F)
                {
                    distance = planeDistance;
                }

                float terrainHeight = 0.0F;

                for (std::uint32_t iteration = 0U;
                     iteration < 8U;
                     ++iteration)
                {
                    const float worldX =
                        ray.origin.x + ray.direction.x * distance;

                    const float worldZ =
                        ray.origin.z + ray.direction.z * distance;

                    if (!context.terrainRenderer.TryGetSurfaceHeight(
                            context.sceneDocument,
                            worldX,
                            worldZ,
                            terrainHeight))
                    {
                        break;
                    }

                    const float refinedDistance =
                        (terrainHeight - ray.origin.y) /
                        ray.direction.y;

                    if (refinedDistance < 0.0F)
                    {
                        break;
                    }

                    distance = refinedDistance;
                }
            }

            lts::editor::EditorTransform transform;
            transform.position =
            {
                ray.origin.x + ray.direction.x * distance,
                ray.origin.y + ray.direction.y * distance,
                ray.origin.z + ray.direction.z * distance
            };

            float terrainHeight = 0.0F;

            if (context.terrainRenderer.TryGetSurfaceHeight(
                    context.sceneDocument,
                    transform.position[0],
                    transform.position[2],
                    terrainHeight))
            {
                transform.position[1] = terrainHeight;
            }

            const StaticMeshEntry& model =
                models_[selectedModel_];

            DirectX::XMFLOAT3 boundsMinimum{};
            DirectX::XMFLOAT3 boundsMaximum{};

            if (context.staticMeshRenderer.TryGetMeshBounds(
                    model.assetPath,
                    boundsMinimum,
                    boundsMaximum))
            {
                transform.position[1] -= boundsMinimum.y;
            }

            const lts::editor::EditorSceneSnapshot before =
                context.sceneDocument.CreateSnapshot();

            if (!context.sceneDocument.CreateStaticMeshEntity(
                    model.physicalPath.stem().wstring(),
                    model.assetPath,
                    transform))
            {
                status_ = "Failed to place the selected model.";
                return false;
            }

            static_cast<void>(
                context.commandHistory.Push(
                    before,
                    context.sceneDocument.CreateSnapshot()));

            inspectedAssetPath_ = model.assetPath;
            status_ = "Object placed. Scene document is dirty.";
            return true;
        }

        void SelectSceneObject(
            const lts::editor::ViewportClick& click,
            ObjectViewContext& context) noexcept
        {
            lts::editor::EditorPickRay ray;

            if (!context.cameraController.BuildPickRay(
                    click.x,
                    click.y,
                    context.viewportWidth,
                    context.viewportHeight,
                    ray))
            {
                return;
            }

            std::size_t entityIndex =
                lts::editor::InvalidEditorEntityIndex;

            float distance = 0.0F;

            if (!lts::editor::ScenePicker::Pick(
                context.sceneDocument,
                ray,
                entityIndex,
                distance,
                &context.staticMeshRenderer,
                false) ||
            !context.sceneDocument.SelectEntityByIndex(entityIndex))
            {
                return;
            }

            const lts::editor::EditorSceneEntity* const entity =
                context.sceneDocument.GetSelectedEntity();

            if (entity == nullptr || !entity->staticMesh.has_value())
            {
                return;
            }

            inspectedAssetPath_ = entity->staticMesh->assetPath;
            materialsWindowOpen_ = true;
            SetPreviewFromSceneObject(*entity);
        }

        void SetPreviewFromSceneObject(
            const lts::editor::EditorSceneEntity& entity) noexcept
        {
            if (!entity.staticMesh.has_value())
            {
                return;
            }

            previewAssetPath_ = entity.staticMesh->assetPath;
            previewBoundsValid_ = false;
            sceneObjectsWindowOpen_ = true;
            previewDocument_.Clear();

            lts::editor::EditorTransform transform;

            static_cast<void>(
                previewDocument_.CreateStaticMeshEntity(
                    entity.name,
                    previewAssetPath_,
                    transform));
        }

        void KeepSelectedMeshOnTerrain(
            ObjectViewContext& context) noexcept
        {
            const auto& transformState =
                transformController_.GetVisualState();

            if (transformState.operation !=
                    lts::editor::EditorTransformOperation::Move ||
                (transformState.activeAxis !=
                     lts::editor::EditorTransformAxis::X &&
                 transformState.activeAxis !=
                     lts::editor::EditorTransformAxis::Z))
            {
                return;
            }

            lts::editor::EditorSceneEntity* const entity =
                context.sceneDocument.GetSelectedEntityMutable();

            if (entity == nullptr || !entity->staticMesh.has_value())
            {
                return;
            }

            float terrainHeight = 0.0F;

            if (!context.terrainRenderer.TryGetSurfaceHeight(
                    context.sceneDocument,
                    entity->transform.position[0],
                    entity->transform.position[2],
                    terrainHeight))
            {
                return;
            }

            DirectX::XMFLOAT3 boundsMinimum{};
            DirectX::XMFLOAT3 boundsMaximum{};
            float bottomOffset = 0.0F;

            if (context.staticMeshRenderer.TryGetMeshBounds(
                    entity->staticMesh->assetPath,
                    boundsMinimum,
                    boundsMaximum))
            {
                bottomOffset =
                    boundsMinimum.y *
                    entity->transform.scale[1];
            }

            entity->transform.position[1] =
                terrainHeight - bottomOffset;
        }

        void DrawSceneObjectsWindow(
            ObjectViewContext& context) noexcept
        {
            ImGui::SetNextWindowSize(
                ImVec2(540.0F, 620.0F),
                ImGuiCond_FirstUseEver);

            if (!ImGui::Begin(
                    "Scene Objects View",
                    &sceneObjectsWindowOpen_))
            {
                ImGui::End();
                return;
            }

            if (previewAssetPath_.empty())
            {
                ImGui::TextDisabled(
                    "Select a model in StaticMeshes or RMB a scene object.");
                ImGui::End();
                return;
            }

            ImGui::TextWrapped(
                "Asset: %s",
                std::filesystem::path(previewAssetPath_).
                    generic_u8string().c_str());

            if (ImGui::Button("Edit Materials"))
            {
                inspectedAssetPath_ = previewAssetPath_;
                materialsWindowOpen_ = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Focus Scene Object"))
            {
                const lts::editor::EditorSceneEntity* const entity =
                    context.sceneDocument.GetSelectedEntity();

                if (entity != nullptr)
                {
                    context.cameraController.FocusOn(
                        DirectX::XMFLOAT3
                        {
                            entity->transform.position[0],
                            entity->transform.position[1],
                            entity->transform.position[2]
                        },
                        10.0F);
                }
            }

            const ImVec2 available =
                ImGui::GetContentRegionAvail();

            const float previewHeight =
                std::clamp(
                    available.y * 0.62F,
                    180.0F,
                    520.0F);

            desiredPreviewWidth_ =
                static_cast<std::uint32_t>(
                    std::clamp(
                        available.x,
                        static_cast<float>(MinimumPreviewSize),
                        static_cast<float>(MaximumPreviewSize)));

            desiredPreviewHeight_ =
                static_cast<std::uint32_t>(
                    std::clamp(
                        previewHeight,
                        static_cast<float>(MinimumPreviewSize),
                        static_cast<float>(MaximumPreviewSize)));

            void* const textureId = GetPreviewTextureId();

            if (textureId != nullptr)
            {
                ImGui::Image(
                    reinterpret_cast<ImTextureID>(textureId),
                    ImVec2(
                        static_cast<float>(desiredPreviewWidth_),
                        static_cast<float>(desiredPreviewHeight_)),
                    ImVec2(0.0F, 0.0F),
                    ImVec2(1.0F, 1.0F));
            }
            else
            {
                ImGui::Dummy(
                    ImVec2(
                        static_cast<float>(desiredPreviewWidth_),
                        static_cast<float>(desiredPreviewHeight_)));

                ImGui::TextDisabled(
                    "Preview render target is being prepared...");
            }

            ImGui::TextDisabled(
                "Materials are previewed immediately and saved on edit release.");

            ImGui::End();
        }

        void DrawMaterialsWindow(
            ObjectViewContext& context) noexcept
        {
            ImGui::SetNextWindowSize(
                ImVec2(520.0F, 680.0F),
                ImGuiCond_FirstUseEver);

            if (ImGui::Begin(
                    "Object Materials",
                    &materialsWindowOpen_))
            {
                if (inspectedAssetPath_.empty())
                {
                    ImGui::TextDisabled(
                        "RMB a static-mesh object or select a model first.");
                    materialInspector_.Reset();
                }
                else
                {
                    materialInspector_.Draw(
                        inspectedAssetPath_,
                        context.staticMeshRenderer);
                }
            }

            ImGui::End();
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult EnsurePreviewTargets() noexcept
        {
            if (device_ == nullptr ||
                desiredPreviewWidth_ == 0U ||
                desiredPreviewHeight_ == 0U)
            {
                return engine::graphics::GraphicsResult::InvalidState;
            }

            const std::uint32_t width =
                std::clamp(
                    desiredPreviewWidth_,
                    MinimumPreviewSize,
                    MaximumPreviewSize);

            const std::uint32_t height =
                std::clamp(
                    desiredPreviewHeight_,
                    MinimumPreviewSize,
                    MaximumPreviewSize);

            if (previewColor_.IsValid() &&
                previewDepth_.IsValid() &&
                previewWidth_ == width &&
                previewHeight_ == height)
            {
                return engine::graphics::GraphicsResult::Success;
            }

            DestroyPreviewTargets(*device_);

            engine::graphics::TextureDesc colorDescription;
            colorDescription.width = width;
            colorDescription.height = height;
            colorDescription.format =
                engine::graphics::Format::R8G8B8A8UNorm;
            colorDescription.bindFlags =
                engine::graphics::TextureBindFlags::RenderTarget |
                engine::graphics::TextureBindFlags::ShaderResource;

            auto result = device_->CreateTexture(
                colorDescription,
                nullptr,
                0U,
                previewColor_);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            engine::graphics::TextureDesc depthDescription;
            depthDescription.width = width;
            depthDescription.height = height;
            depthDescription.format =
                engine::graphics::Format::D32Float;
            depthDescription.bindFlags =
                engine::graphics::TextureBindFlags::DepthStencil;

            result = device_->CreateTexture(
                depthDescription,
                nullptr,
                0U,
                previewDepth_);

            if (engine::graphics::Failed(result))
            {
                static_cast<void>(
                    device_->DestroyTexture(previewColor_));
                previewColor_ = {};
                return result;
            }

            previewWidth_ = width;
            previewHeight_ = height;
            return engine::graphics::GraphicsResult::Success;
        }

        void DestroyPreviewTargets(
            engine::graphics::RenderDevice& device) noexcept
        {
            if (previewColor_.IsValid())
            {
                static_cast<void>(
                    device.DestroyTexture(previewColor_));
            }

            if (previewDepth_.IsValid())
            {
                static_cast<void>(
                    device.DestroyTexture(previewDepth_));
            }

            previewColor_ = {};
            previewDepth_ = {};
            previewWidth_ = 0U;
            previewHeight_ = 0U;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult RenderPreview(
            engine::graphics::CommandContext& context,
            lts::editor::StaticMeshRenderer& renderer) noexcept
        {
            auto result = EnsurePreviewTargets();

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            result = context.SetRenderTargets(
                &previewColor_,
                1U,
                previewDepth_);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            const auto finish = [&context]() noexcept
            {
                context.UnbindRenderTargets();
            };

            engine::graphics::Viewport viewport;
            viewport.width = static_cast<float>(previewWidth_);
            viewport.height = static_cast<float>(previewHeight_);
            viewport.minDepth = 0.0F;
            viewport.maxDepth = 1.0F;

            result = context.SetViewport(viewport);

            if (engine::graphics::Failed(result))
            {
                finish();
                return result;
            }

            engine::graphics::ClearColor clearColor;
            clearColor.red = 0.035F;
            clearColor.green = 0.040F;
            clearColor.blue = 0.048F;
            clearColor.alpha = 1.0F;

            result = context.ClearColorTarget(
                previewColor_,
                clearColor);

            if (!engine::graphics::Failed(result))
            {
                result = context.ClearDepthStencilTarget(
                    previewDepth_,
                    engine::graphics::ClearDepthStencilFlags::Depth,
                    0.0F,
                    0U);
            }

            if (engine::graphics::Failed(result))
            {
                finish();
                return result;
            }

            UpdatePreviewBounds(renderer);

            const float radius =
                previewBoundsValid_
                    ? previewRadius_
                    : 2.0F;

            const DirectX::XMFLOAT3 center =
                previewBoundsValid_
                    ? previewCenter_
                    : DirectX::XMFLOAT3{};

            const float distance =
                (std::max)(radius * 2.6F, 2.0F);

            const DirectX::XMVECTOR target =
                DirectX::XMLoadFloat3(&center);

            const DirectX::XMVECTOR eye =
                DirectX::XMVectorAdd(
                    target,
                    DirectX::XMVectorSet(
                        distance * 0.70F,
                        distance * 0.45F,
                        -distance,
                        0.0F));

            const DirectX::XMMATRIX view =
                DirectX::XMMatrixLookAtLH(
                    eye,
                    target,
                    DirectX::XMVectorSet(
                        0.0F,
                        1.0F,
                        0.0F,
                        0.0F));

            const float aspect =
                static_cast<float>(previewWidth_) /
                static_cast<float>(previewHeight_);

            const float physicalNear =
                (std::max)(radius * 0.005F, 0.01F);

            const float physicalFar =
                (std::max)(distance + radius * 20.0F, 100.0F);

            const DirectX::XMMATRIX projection =
                DirectX::XMMatrixPerspectiveFovLH(
                    DirectX::XMConvertToRadians(45.0F),
                    aspect,
                    physicalFar,
                    physicalNear);

            DirectX::XMFLOAT4X4 viewProjection;
            DirectX::XMStoreFloat4x4(
                &viewProjection,
                view * projection);

            DirectX::XMFLOAT3 cameraPosition;
            DirectX::XMStoreFloat3(
                &cameraPosition,
                eye);

            result = renderer.Render(
                context,
                previewDocument_,
                viewProjection,
                cameraPosition);

            finish();
            return result;
        }

        void UpdatePreviewBounds(
            lts::editor::StaticMeshRenderer& renderer) noexcept
        {
            if (previewBoundsValid_ || previewAssetPath_.empty())
            {
                return;
            }

            DirectX::XMFLOAT3 minimum{};
            DirectX::XMFLOAT3 maximum{};

            if (!renderer.TryGetMeshBounds(
                    previewAssetPath_,
                    minimum,
                    maximum))
            {
                return;
            }

            previewCenter_ =
            {
                (minimum.x + maximum.x) * 0.5F,
                (minimum.y + maximum.y) * 0.5F,
                (minimum.z + maximum.z) * 0.5F
            };

            const float extentX =
                (maximum.x - minimum.x) * 0.5F;

            const float extentY =
                (maximum.y - minimum.y) * 0.5F;

            const float extentZ =
                (maximum.z - minimum.z) * 0.5F;

            previewRadius_ =
                (std::max)(
                    std::sqrt(
                        extentX * extentX +
                        extentY * extentY +
                        extentZ * extentZ),
                    0.1F);

            previewBoundsValid_ = true;
        }

        [[nodiscard]]
        void* GetPreviewTextureId() const noexcept
        {
            if (device_ == nullptr ||
                !previewColor_.IsValid() ||
                device_->GetBackend() !=
                    engine::graphics::GraphicsBackend::D3D11)
            {
                return nullptr;
            }

            const auto& d3d11Device =
                static_cast<
                    const engine::graphics::d3d11::D3D11Device&>(
                        *device_);

            return reinterpret_cast<void*>(
                d3d11Device.GetTextureShaderResourceView(
                    previewColor_));
        }

        engine::graphics::RenderDevice* device_ = nullptr;
        engine::platform::NativeWindowHandle window_;

        lts::editor::SceneRenderer sceneRenderer_;
        lts::editor::TransformController transformController_;
        lts::editor::MaterialInspector materialInspector_;
        lts::editor::SceneDocument previewDocument_;

        std::filesystem::path workspaceRoot_;
        std::filesystem::path staticMeshesRoot_;

        std::vector<StaticMeshEntry> models_;
        std::vector<std::string> categories_;
        std::array<char, 192U> filter_{};

        std::size_t selectedModel_ = InvalidModelIndex;
        int selectedCategory_ = -1;

        std::wstring previewAssetPath_;
        std::wstring inspectedAssetPath_;

        engine::graphics::TextureHandle previewColor_;
        engine::graphics::TextureHandle previewDepth_;

        std::uint32_t desiredPreviewWidth_ = 480U;
        std::uint32_t desiredPreviewHeight_ = 320U;
        std::uint32_t previewWidth_ = 0U;
        std::uint32_t previewHeight_ = 0U;

        DirectX::XMFLOAT3 previewCenter_{};
        float previewRadius_ = 2.0F;

        std::string status_;

        bool initialized_ = false;
        bool scanned_ = false;
        bool scenePickerEnabled_ = true;
        bool sceneObjectsWindowOpen_ = false;
        bool materialsWindowOpen_ = false;
        bool physicsWindowOpen_ = false;
        bool previewBoundsValid_ = false;
    };

    ObjectViewTab::ObjectViewTab() noexcept = default;
    ObjectViewTab::~ObjectViewTab() noexcept = default;

    bool ObjectViewTab::Initialize(
        engine::graphics::RenderDevice& device,
        const engine::platform::NativeWindowHandle window) noexcept
    {
        if (impl_ == nullptr)
        {
            try
            {
                impl_ = std::make_unique<Impl>();
            }
            catch (...)
            {
                return false;
            }
        }

        return impl_->Initialize(device, window);
    }

    void ObjectViewTab::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->Shutdown(device);
            impl_.reset();
        }
    }

    void ObjectViewTab::Refresh() noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->Refresh();
        }
    }

    void ObjectViewTab::DrawToolbar(
        ObjectViewContext& context) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->DrawToolbar(context);
        }
    }

    void ObjectViewTab::DrawPage(
        ObjectViewContext& context) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->DrawPage(context);
        }
    }

    void ObjectViewTab::DrawWindows(
        ObjectViewContext& context) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->DrawWindows(context);
        }
    }

    void ObjectViewTab::UpdateViewport(
        ObjectViewContext& context) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->UpdateViewport(context);
        }
    }

    engine::graphics::GraphicsResult ObjectViewTab::Render(
        engine::graphics::CommandContext& context,
        const ObjectViewContext& objectContext,
        const DirectX::XMFLOAT4X4& viewProjection,
        const DirectX::XMFLOAT3& cameraPosition) noexcept
    {
        if (impl_ == nullptr)
        {
            return engine::graphics::GraphicsResult::InvalidState;
        }

        return impl_->Render(
            context,
            objectContext,
            viewProjection,
            cameraPosition);
    }
}
