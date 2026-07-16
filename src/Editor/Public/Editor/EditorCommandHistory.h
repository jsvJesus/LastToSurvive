#pragma once

#include "Editor/EditorSceneDocument.h"

#include <cstddef>
#include <vector>

namespace lts::editor
{
    class EditorCommandHistory final
    {
    public:
        EditorCommandHistory() = default;
        ~EditorCommandHistory() noexcept = default;

        EditorCommandHistory(const EditorCommandHistory&) = delete;
        EditorCommandHistory& operator=(const EditorCommandHistory&) = delete;

        void Clear() noexcept;

        [[nodiscard]]
        bool Push(
            const EditorSceneSnapshot& before,
            const EditorSceneSnapshot& after);

        [[nodiscard]]
        bool Undo(EditorSceneDocument& document);

        [[nodiscard]]
        bool Redo(EditorSceneDocument& document);

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
