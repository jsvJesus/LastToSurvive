#include "Editor/EditorSceneDocument.h"

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

    bool EditorSceneDocument::IsDirty() const noexcept
    {
        return dirty_;
    }

    void EditorSceneDocument::MarkSaved() noexcept
    {
        dirty_ = false;
    }
}