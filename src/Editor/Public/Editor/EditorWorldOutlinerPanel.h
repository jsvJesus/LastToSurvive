#pragma once

#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorSceneDocument.h"

#include <array>
#include <string>
#include <unordered_set>

namespace lts::editor
{
    class EditorWorldOutlinerPanel final
    {
    public:
        void Draw(EditorSceneDocument& document, EditorCommandHistory& history);

    private:
        void DrawEntity(EditorEntityId entityId, EditorSceneDocument& document,
            EditorCommandHistory& history,
            const std::unordered_set<EditorEntityId>& visibleIds);
        void HandleEntityDrop(EditorEntityId targetId,
            EditorSceneDocument& document, EditorCommandHistory& history);

        std::array<char, 128U> search_{};
        std::array<char, 128U> folderRename_{};
        std::wstring renamingFolder_;
    };
}
