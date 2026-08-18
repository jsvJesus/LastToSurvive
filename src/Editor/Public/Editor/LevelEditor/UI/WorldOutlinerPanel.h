#pragma once

#include "Editor/Commands/CommandHistory.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <array>
#include <string>
#include <unordered_set>

namespace lts::editor
{
    class WorldOutlinerPanel final
    {
    public:
        void Draw(SceneDocument& document, CommandHistory& history);

    private:
        void DrawEntity(EditorEntityId entityId, SceneDocument& document,
            CommandHistory& history,
            const std::unordered_set<EditorEntityId>& visibleIds);
        void HandleEntityDrop(EditorEntityId targetId,
            SceneDocument& document, CommandHistory& history);

        std::array<char, 128U> search_{};
        std::array<char, 128U> folderRename_{};
        std::wstring renamingFolder_;
    };
}
