#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <cstddef>
#include <vector>

namespace lts::editor
{
    class CommandHistory final
    {
    public:
        CommandHistory() = default;
        ~CommandHistory() noexcept = default;

        CommandHistory(const CommandHistory&) = delete;
        CommandHistory& operator=(const CommandHistory&) = delete;

        void Clear() noexcept;

        [[nodiscard]]
        bool Push(
            const EditorSceneSnapshot& before,
            const EditorSceneSnapshot& after);

        [[nodiscard]]
        bool Undo(SceneDocument& document);

        [[nodiscard]]
        bool Redo(SceneDocument& document);

        [[nodiscard]]
        bool CanUndo() const noexcept;

        [[nodiscard]]
        bool CanRedo() const noexcept;

    private:
        struct Entry final
        {
            EditorSceneSnapshot before;
            EditorSceneSnapshot after;
        };

        static constexpr std::size_t MaximumEntries = 128U;

        std::vector<Entry> entries_;
        std::size_t cursor_ = 0U;
    };
}
