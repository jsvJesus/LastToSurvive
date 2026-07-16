#include "Editor/EditorSceneDocument.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        EditorTransform MakeTransform(
            const float positionX,
            const float positionY,
            const float positionZ,
            const float rotationX = 0.0F,
            const float rotationY = 0.0F,
            const float rotationZ = 0.0F,
            const float scaleX = 1.0F,
            const float scaleY = 1.0F,
            const float scaleZ = 1.0F) noexcept
        {
            EditorTransform transform;

            transform.position =
            {
                positionX,
                positionY,
                positionZ
            };

            transform.rotationDegrees =
            {
                rotationX,
                rotationY,
                rotationZ
            };

            transform.scale =
            {
                scaleX,
                scaleY,
                scaleZ
            };

            return transform;
        }

        [[nodiscard]]
        bool IsFiniteTransform(
            const EditorTransform& transform) noexcept
        {
            for (const float value : transform.position)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            for (const float value : transform.rotationDegrees)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            for (const float value : transform.scale)
            {
                if (
                    !std::isfinite(value) ||
                    std::abs(value) < 0.001F)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool TransformsEqual(
            const EditorTransform& left,
            const EditorTransform& right) noexcept
        {
            return
                left.position == right.position &&
                left.rotationDegrees == right.rotationDegrees &&
                left.scale == right.scale;
        }
    }

    void EditorSceneDocument::CreateDefaultLevel()
    {
        Clear();

        static_cast<void>(
            CreateEntity(
                L"Environment",
                EditorEntityKind::Environment,
                MakeTransform(
                    0.0F,
                    0.0F,
                    0.0F)));

        static_cast<void>(
            CreateEntity(
                L"Sun",
                EditorEntityKind::DirectionalLight,
                MakeTransform(
                    0.0F,
                    10.0F,
                    0.0F,
                    -50.0F,
                    35.0F,
                    0.0F)));

        static_cast<void>(
            CreateEntity(
                L"Player Start",
                EditorEntityKind::SpawnPoint,
                MakeTransform(
                    0.0F,
                    1.0F,
                    0.0F)));

        static_cast<void>(
            CreateEntity(
                L"Anomaly Field",
                EditorEntityKind::Anomaly,
                MakeTransform(
                    8.0F,
                    0.0F,
                    5.0F,
                    0.0F,
                    0.0F,
                    0.0F,
                    4.0F,
                    1.0F,
                    4.0F)));

        static_cast<void>(
            CreateEntity(
                L"Loot Container",
                EditorEntityKind::LootContainer,
                MakeTransform(
                    -4.0F,
                    0.0F,
                    3.0F)));

        selectedIndex_ =
            entities_.empty()
                ? InvalidEditorEntityIndex
                : 0U;

        dirty_ = false;
    }

    void EditorSceneDocument::Clear() noexcept
    {
        entities_.clear();

        nextEntityId_ = 1U;
        selectedIndex_ = InvalidEditorEntityIndex;
        dirty_ = false;
    }

    EditorEntityId EditorSceneDocument::CreateEntity(
        std::wstring name,
        const EditorEntityKind kind,
        const EditorTransform& transform)
    {
        if (name.empty())
        {
            name = L"Entity";
        }

        EditorSceneEntity entity;

        entity.id = nextEntityId_++;
        entity.kind = kind;
        entity.name = std::move(name);
        entity.transform = transform;

        entities_.push_back(
            std::move(entity));

        if (selectedIndex_ == InvalidEditorEntityIndex)
        {
            selectedIndex_ = 0U;
        }

        dirty_ = true;

        return entities_.back().id;
    }

    const std::vector<EditorSceneEntity>&
        EditorSceneDocument::GetEntities() const noexcept
    {
        return entities_;
    }

    const EditorSceneEntity*
        EditorSceneDocument::GetSelectedEntity() const noexcept
    {
        if (selectedIndex_ >= entities_.size())
        {
            return nullptr;
        }

        return &entities_[selectedIndex_];
    }

    EditorSceneEntity*
        EditorSceneDocument::GetSelectedEntityMutable() noexcept
    {
        if (selectedIndex_ >= entities_.size())
        {
            return nullptr;
        }

        return &entities_[selectedIndex_];
    }

    std::size_t
        EditorSceneDocument::GetSelectedIndex() const noexcept
    {
        return selectedIndex_;
    }

    bool EditorSceneDocument::SelectEntityByIndex(
        const std::size_t index) noexcept
    {
        if (index >= entities_.size())
        {
            return false;
        }

        selectedIndex_ = index;
        return true;
    }

    void EditorSceneDocument::ClearSelection() noexcept
    {
        selectedIndex_ = InvalidEditorEntityIndex;
    }

    bool EditorSceneDocument::SetSelectedTransform(
        const EditorTransform& transform) noexcept
    {
        EditorSceneEntity* const entity =
            GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !IsFiniteTransform(transform) ||
            TransformsEqual(entity->transform, transform))
        {
            return false;
        }

        entity->transform = transform;
        dirty_ = true;

        return true;
    }

    bool EditorSceneDocument::TranslateSelectedEntity(
        const float translationX,
        const float translationY,
        const float translationZ) noexcept
    {
        const EditorSceneEntity* const entity =
            GetSelectedEntity();

        if (
            entity == nullptr ||
            !std::isfinite(translationX) ||
            !std::isfinite(translationY) ||
            !std::isfinite(translationZ))
        {
            return false;
        }

        EditorTransform transform = entity->transform;

        transform.position[0] += translationX;
        transform.position[1] += translationY;
        transform.position[2] += translationZ;

        return SetSelectedTransform(transform);
    }

    bool EditorSceneDocument::DuplicateSelectedEntity()
    {
        const EditorSceneEntity* const selected =
            GetSelectedEntity();

        if (selected == nullptr)
        {
            return false;
        }

        EditorSceneEntity duplicate = *selected;

        duplicate.id = nextEntityId_++;
        duplicate.name += L" Copy";
        duplicate.transform.position[0] += 1.0F;

        entities_.push_back(
            std::move(duplicate));

        selectedIndex_ = entities_.size() - 1U;
        dirty_ = true;

        return true;
    }

    bool EditorSceneDocument::DeleteSelectedEntity() noexcept
    {
        if (selectedIndex_ >= entities_.size())
        {
            return false;
        }

        entities_.erase(
            entities_.begin() +
            static_cast<std::ptrdiff_t>(selectedIndex_));

        if (entities_.empty())
        {
            selectedIndex_ = InvalidEditorEntityIndex;
        }
        else if (selectedIndex_ >= entities_.size())
        {
            selectedIndex_ = entities_.size() - 1U;
        }

        dirty_ = true;
        return true;
    }

    EditorSceneSnapshot
        EditorSceneDocument::CreateSnapshot() const
    {
        EditorSceneSnapshot snapshot;

        snapshot.entities = entities_;
        snapshot.nextEntityId = nextEntityId_;
        snapshot.selectedIndex = selectedIndex_;
        snapshot.dirty = dirty_;

        return snapshot;
    }

    void EditorSceneDocument::RestoreSnapshot(
        const EditorSceneSnapshot& snapshot,
        const bool markDirty)
    {
        entities_ = snapshot.entities;
        nextEntityId_ = snapshot.nextEntityId;
        selectedIndex_ = snapshot.selectedIndex;

        if (selectedIndex_ >= entities_.size())
        {
            selectedIndex_ = entities_.empty()
                ? InvalidEditorEntityIndex
                : entities_.size() - 1U;
        }

        dirty_ = markDirty
            ? true
            : snapshot.dirty;
    }

    bool EditorSceneDocument::IsDirty() const noexcept
    {
        return dirty_;
    }

    void EditorSceneDocument::MarkSaved() noexcept
    {
        dirty_ = false;
    }
}
