#include "Editor/EditorCommandHistory.h"

#include <algorithm>
#include <utility>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        bool EntityEquals(
            const EditorSceneEntity& left,
            const EditorSceneEntity& right) noexcept
        {
            return
                left.id == right.id &&
                left.kind == right.kind &&
                left.name == right.name &&
                left.transform.position == right.transform.position &&
                left.transform.rotationDegrees ==
                    right.transform.rotationDegrees &&
                left.transform.scale == right.transform.scale;
        }

        [[nodiscard]]
        bool SnapshotEquals(
            const EditorSceneSnapshot& left,
            const EditorSceneSnapshot& right) noexcept
        {
            if (
                left.nextEntityId != right.nextEntityId ||
                left.selectedIndex != right.selectedIndex ||
                left.entities.size() != right.entities.size())
            {
                return false;
            }

            for (
                std::size_t index = 0U;
                index < left.entities.size();
                ++index)
            {
                if (!EntityEquals(
                        left.entities[index],
                        right.entities[index]))
                {
                    return false;
                }
            }

            return true;
        }
    }

    void EditorCommandHistory::Clear() noexcept
    {
        entries_.clear();
        cursor_ = 0U;
    }

    bool EditorCommandHistory::Push(
        const EditorSceneSnapshot& before,
        const EditorSceneSnapshot& after)
    {
        if (SnapshotEquals(before, after))
        {
            return false;
        }

        if (cursor_ < entries_.size())
        {
            entries_.erase(
                entries_.begin() +
                    static_cast<std::ptrdiff_t>(cursor_),
                entries_.end());
        }

        Entry entry;
        entry.before = before;
        entry.after = after;

        entries_.push_back(
            std::move(entry));

        if (entries_.size() > MaximumEntries)
        {
            entries_.erase(entries_.begin());
        }

        cursor_ = entries_.size();
        return true;
    }

    bool EditorCommandHistory::Undo(
        EditorSceneDocument& document)
    {
        if (!CanUndo())
        {
            return false;
        }

        --cursor_;

        document.RestoreSnapshot(
            entries_[cursor_].before,
            true);

        return true;
    }

    bool EditorCommandHistory::Redo(
        EditorSceneDocument& document)
    {
        if (!CanRedo())
        {
            return false;
        }

        document.RestoreSnapshot(
            entries_[cursor_].after,
            true);

        ++cursor_;
        return true;
    }

    bool EditorCommandHistory::CanUndo() const noexcept
    {
        return cursor_ > 0U;
    }

    bool EditorCommandHistory::CanRedo() const noexcept
    {
        return cursor_ < entries_.size();
    }
}
