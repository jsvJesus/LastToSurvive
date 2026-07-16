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
        bool TransformsEqual(
            const EditorTransform& left,
            const EditorTransform& right) noexcept
        {
            return
                left.position == right.position &&
                left.rotationDegrees ==
                    right.rotationDegrees &&
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
            GetEntities().empty()
                ? InvalidEditorEntityIndex
                : 0U;

        dirty_ = false;
    }

    void EditorSceneDocument::Clear() noexcept
    {
        world_.Clear();

        selectedIndex_ =
            InvalidEditorEntityIndex;

        dirty_ = false;
    }

    EditorEntityId EditorSceneDocument::CreateEntity(
        std::wstring name,
        const EditorEntityKind kind,
        const EditorTransform& transform)
    {
        const EditorEntityId entityId =
            world_.CreateEntity(
                std::move(name),
                kind,
                transform);

        if (
            selectedIndex_ ==
            InvalidEditorEntityIndex)
        {
            selectedIndex_ = 0U;
        }

        dirty_ = true;
        return entityId;
    }

    bool EditorSceneDocument::CreateStaticMeshEntity(
    std::wstring name,
    std::wstring assetPath,
    const EditorTransform& transform)
    {
        if (assetPath.empty())
        {
            return false;
        }

        const EditorEntityId entityId =
            world_.CreateEntity(
                name.empty()
                    ? L"Static Mesh"
                    : std::move(name),
                EditorEntityKind::Empty,
                transform);

        EditorSceneEntity* const entity =
            world_.FindEntity(entityId);

        if (entity == nullptr)
        {
            return false;
        }

        entity->staticMesh.emplace();

        entity->staticMesh->assetPath =
            std::move(assetPath);

        entity->staticMesh->visible = true;
        entity->staticMesh->castShadows = true;

        selectedIndex_ =
            world_.FindEntityIndex(entityId);

        dirty_ = true;
        return true;
    }

    const std::vector<EditorSceneEntity>&
        EditorSceneDocument::GetEntities() const noexcept
    {
        return world_.GetEntities();
    }

    const EditorSceneEntity*
        EditorSceneDocument::
            GetSelectedEntity() const noexcept
    {
        const auto& entities =
            world_.GetEntities();

        if (selectedIndex_ >= entities.size())
        {
            return nullptr;
        }

        return &entities[selectedIndex_];
    }

    EditorSceneEntity*
        EditorSceneDocument::
            GetSelectedEntityMutable() noexcept
    {
        const EditorSceneEntity* const selected =
            GetSelectedEntity();

        if (selected == nullptr)
        {
            return nullptr;
        }

        return world_.FindEntity(
            selected->id);
    }

    std::size_t
        EditorSceneDocument::
            GetSelectedIndex() const noexcept
    {
        return selectedIndex_;
    }

    bool EditorSceneDocument::SelectEntityByIndex(
        const std::size_t index) noexcept
    {
        if (index >= GetEntities().size())
        {
            return false;
        }

        selectedIndex_ = index;
        return true;
    }

    void EditorSceneDocument::ClearSelection() noexcept
    {
        selectedIndex_ =
            InvalidEditorEntityIndex;
    }

    bool EditorSceneDocument::SetSelectedTransform(
        const EditorTransform& transform) noexcept
    {
        EditorSceneEntity* const entity =
            GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !engine::scene::SceneWorld::
                IsFiniteTransform(transform) ||
            TransformsEqual(
                entity->transform,
                transform))
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

        EditorTransform transform =
            entity->transform;

        transform.position[0] += translationX;
        transform.position[1] += translationY;
        transform.position[2] += translationZ;

        return SetSelectedTransform(transform);
    }

    bool EditorSceneDocument::
        DuplicateSelectedEntity()
    {
        const EditorSceneEntity* const selected =
            GetSelectedEntity();

        if (selected == nullptr)
        {
            return false;
        }

        const EditorEntityId duplicatedId =
            world_.DuplicateEntity(
                selected->id);

        if (duplicatedId == 0U)
        {
            return false;
        }

        selectedIndex_ =
            world_.FindEntityIndex(
                duplicatedId);

        dirty_ = true;
        return true;
    }

    bool EditorSceneDocument::
        DeleteSelectedEntity() noexcept
    {
        const EditorSceneEntity* const selected =
            GetSelectedEntity();

        if (selected == nullptr)
        {
            return false;
        }

        if (!world_.DeleteEntity(selected->id))
        {
            return false;
        }

        const auto& entities =
            world_.GetEntities();

        if (entities.empty())
        {
            selectedIndex_ =
                InvalidEditorEntityIndex;
        }
        else if (
            selectedIndex_ >=
            entities.size())
        {
            selectedIndex_ =
                entities.size() - 1U;
        }

        dirty_ = true;
        return true;
    }

    EditorSceneSnapshot
        EditorSceneDocument::
            CreateSnapshot() const
    {
        EditorSceneSnapshot snapshot;

        const engine::scene::SceneWorldState state =
            world_.CreateState();

        snapshot.entities = state.entities;
        snapshot.nextEntityId =
            state.nextEntityId;

        snapshot.selectedIndex =
            selectedIndex_;

        snapshot.dirty = dirty_;

        return snapshot;
    }

    void EditorSceneDocument::RestoreSnapshot(
        const EditorSceneSnapshot& snapshot,
        const bool markDirty)
    {
        engine::scene::SceneWorldState state;

        state.entities = snapshot.entities;
        state.nextEntityId =
            snapshot.nextEntityId;

        world_.RestoreState(state);

        selectedIndex_ =
            snapshot.selectedIndex;

        const auto& entities =
            world_.GetEntities();

        if (selectedIndex_ >= entities.size())
        {
            selectedIndex_ =
                entities.empty()
                    ? InvalidEditorEntityIndex
                    : entities.size() - 1U;
        }

        dirty_ =
            markDirty
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

    const engine::scene::SceneWorld&
        EditorSceneDocument::
            GetSceneWorld() const noexcept
    {
        return world_;
    }
}