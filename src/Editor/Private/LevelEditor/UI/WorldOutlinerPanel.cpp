#include "Editor/LevelEditor/UI/WorldOutlinerPanel.h"

#include <Windows.h>
#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>

namespace lts::editor
{
    namespace
    {
        constexpr const char* EntityPayload = "LTS_OUTLINER_ENTITY";

        std::string ToUtf8(const std::wstring& text)
        {
            if (text.empty()) return {};
            const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
            std::string result(static_cast<std::size_t>(std::max(size, 0)), '\0');
            if (size > 0) WideCharToMultiByte(CP_UTF8, 0, text.data(),
                static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
            return result;
        }

        std::wstring FromUtf8(const char* text)
        {
            if (text == nullptr || *text == '\0') return {};
            const int length = static_cast<int>(std::strlen(text));
            const int size = MultiByteToWideChar(CP_UTF8, 0, text, length, nullptr, 0);
            std::wstring result(static_cast<std::size_t>(std::max(size, 0)), L'\0');
            if (size > 0) MultiByteToWideChar(CP_UTF8, 0, text, length,
                result.data(), size);
            return result;
        }

        std::string Lower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(),
                [](const unsigned char character)
                { return static_cast<char>(std::tolower(character)); });
            return value;
        }
    }

    void WorldOutlinerPanel::Draw(
        SceneDocument& document, CommandHistory& history)
    {
        ImGui::SetNextItemWidth(-1.0F);
        ImGui::InputTextWithHint("##OutlinerSearch", "Search objects...",
            search_.data(), search_.size());

        const auto& entities = document.GetEntities();
        std::unordered_set<EditorEntityId> visibleIds;
        const std::string query = Lower(search_.data());
        for (const EditorSceneEntity& entity : entities)
        {
            if (query.empty() || Lower(ToUtf8(entity.name)).find(query) != std::string::npos ||
                Lower(ToUtf8(entity.editorFolder)).find(query) != std::string::npos)
            {
                EditorEntityId current = entity.id;
                while (current != 0U && visibleIds.insert(current).second)
                {
                    const EditorSceneEntity* ancestor =
                        document.GetSceneWorld().FindEntity(current);
                    current = ancestor != nullptr ? ancestor->parentId : 0U;
                }
            }
        }

        if (ImGui::Button("New Folder") && !document.GetSelectedEntityIds().empty())
        {
            std::set<std::wstring> folders;
            for (const EditorSceneEntity& entity : entities)
                if (!entity.editorFolder.empty()) folders.insert(entity.editorFolder);
            std::uint32_t suffix = 1U;
            std::wstring folder;
            do { folder = L"Folder " + std::to_wstring(suffix++); }
            while (folders.find(folder) != folders.end());
            const EditorSceneSnapshot before = document.CreateSnapshot();
            if (document.MoveSelectionToFolder(folder))
                static_cast<void>(history.Push(before, document.CreateSnapshot()));
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Ctrl: toggle  Shift: range");

        ImGui::Separator();
        ImGui::Selectable("Scene Root", false, ImGuiSelectableFlags_AllowOverlap);
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EntityPayload))
            {
                if (payload->IsDelivery() && payload->DataSize == sizeof(EditorEntityId))
                {
                    const EditorEntityId id = *static_cast<const EditorEntityId*>(payload->Data);
                    const EditorSceneSnapshot before = document.CreateSnapshot();
                    bool changed = false;
                    const std::vector<EditorEntityId> draggedIds =
                        document.IsEntitySelected(id)
                            ? document.GetSelectedEntityIds()
                            : std::vector<EditorEntityId>{id};
                    for (const EditorEntityId draggedId : draggedIds)
                        changed = document.SetEntityParent(draggedId, 0U) || changed;
                    if (changed)
                        static_cast<void>(history.Push(before, document.CreateSnapshot()));
                }
            }
            ImGui::EndDragDropTarget();
        }

        std::set<std::wstring> folders;
        for (const EditorSceneEntity& entity : entities)
            if (!entity.editorFolder.empty() &&
                visibleIds.find(entity.id) != visibleIds.end())
                folders.insert(entity.editorFolder);

        for (const std::wstring& folder : folders)
        {
            ImGui::PushID(ToUtf8(folder).c_str());
            const bool open = ImGui::TreeNodeEx("Folder", ImGuiTreeNodeFlags_SpanAvailWidth,
                "%s", ToUtf8(folder).c_str());
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EntityPayload))
                {
                    if (payload->IsDelivery())
                    {
                        const EditorEntityId id = *static_cast<const EditorEntityId*>(payload->Data);
                        const EditorSceneSnapshot before = document.CreateSnapshot();
                        if (!document.IsEntitySelected(id))
                        {
                            const std::size_t index =
                                document.GetSceneWorld().FindEntityIndex(id);
                            if (index != InvalidEditorEntityIndex)
                            static_cast<void>(document.SelectEntityByIndex(index));
                        }
                        if (document.MoveSelectionToFolder(folder))
                            static_cast<void>(history.Push(before, document.CreateSnapshot()));
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::BeginPopupContextItem("FolderMenu"))
            {
                if (ImGui::MenuItem("Rename"))
                {
                    renamingFolder_ = folder;
                    folderRename_.fill('\0');
                    const std::string name = ToUtf8(folder);
                    std::memcpy(folderRename_.data(), name.data(),
                        std::min(name.size(), folderRename_.size() - 1U));
                    ImGui::OpenPopup("Rename Folder");
                }
                if (ImGui::MenuItem("Remove Folder"))
                {
                    const EditorSceneSnapshot before = document.CreateSnapshot();
                    if (document.RenameFolder(folder, L""))
                        static_cast<void>(history.Push(before, document.CreateSnapshot()));
                }
                ImGui::EndPopup();
            }
            if (open)
            {
                for (const EditorSceneEntity& entity : entities)
                    if (entity.parentId == 0U && entity.editorFolder == folder &&
                        visibleIds.find(entity.id) != visibleIds.end())
                        DrawEntity(entity.id, document, history, visibleIds);
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        for (const EditorSceneEntity& entity : entities)
            if (entity.parentId == 0U && entity.editorFolder.empty() &&
                visibleIds.find(entity.id) != visibleIds.end())
                DrawEntity(entity.id, document, history, visibleIds);

        if (!renamingFolder_.empty()) ImGui::OpenPopup("Rename Folder");
        if (ImGui::BeginPopupModal("Rename Folder", nullptr,
                ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Name", folderRename_.data(), folderRename_.size());
            if (ImGui::Button("Apply"))
            {
                const EditorSceneSnapshot before = document.CreateSnapshot();
                if (document.RenameFolder(renamingFolder_, FromUtf8(folderRename_.data())))
                    static_cast<void>(history.Push(before, document.CreateSnapshot()));
                renamingFolder_.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                renamingFolder_.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void WorldOutlinerPanel::DrawEntity(
        const EditorEntityId entityId, SceneDocument& document,
        CommandHistory& history,
        const std::unordered_set<EditorEntityId>& visibleIds)
    {
        const EditorSceneEntity* entity = document.GetSceneWorld().FindEntity(entityId);
        if (entity == nullptr) return;
        bool hasChildren = false;
        for (const EditorSceneEntity& candidate : document.GetEntities())
            if (candidate.parentId == entityId &&
                visibleIds.find(candidate.id) != visibleIds.end())
                hasChildren = true;

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth |
            ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (document.IsEntitySelected(entityId)) flags |= ImGuiTreeNodeFlags_Selected;
        const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(entityId), flags,
            "%s", ToUtf8(entity->name).c_str());
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            const ImGuiIO& io = ImGui::GetIO();
            const EditorSelectionMode mode = io.KeyShift ? EditorSelectionMode::Range :
                (io.KeyCtrl ? EditorSelectionMode::Toggle : EditorSelectionMode::Replace);
            static_cast<void>(document.SelectEntityByIndex(
                document.GetSceneWorld().FindEntityIndex(entityId), mode));
        }
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(EntityPayload, &entityId, sizeof(entityId));
            ImGui::Text("%s", ToUtf8(entity->name).c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            HandleEntityDrop(entityId, document, history);
            ImGui::EndDragDropTarget();
        }
        if (hasChildren && open)
        {
            for (const EditorSceneEntity& child : document.GetEntities())
                if (child.parentId == entityId &&
                    visibleIds.find(child.id) != visibleIds.end())
                    DrawEntity(child.id, document, history, visibleIds);
            ImGui::TreePop();
        }
    }

    void WorldOutlinerPanel::HandleEntityDrop(
        const EditorEntityId targetId, SceneDocument& document,
        CommandHistory& history)
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EntityPayload);
        if (payload == nullptr || !payload->IsDelivery() ||
            payload->DataSize != sizeof(EditorEntityId)) return;
        const EditorEntityId draggedId = *static_cast<const EditorEntityId*>(payload->Data);
        const EditorSceneSnapshot before = document.CreateSnapshot();
        bool changed = false;
        const std::vector<EditorEntityId> draggedIds =
            document.IsEntitySelected(draggedId)
                ? document.GetSelectedEntityIds()
                : std::vector<EditorEntityId>{draggedId};
        for (const EditorEntityId id : draggedIds)
            changed = document.SetEntityParent(id, targetId) || changed;
        if (changed)
            static_cast<void>(history.Push(before, document.CreateSnapshot()));
    }
}
