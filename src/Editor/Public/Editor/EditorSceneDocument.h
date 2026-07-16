#pragma once

#include <Scene/SceneWorld.h>

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace lts::editor
{
    using EditorEntityId =
        engine::scene::SceneEntityId;

    using EditorEntityKind =
        engine::scene::SceneEntityKind;

    using EditorTransform =
        engine::scene::SceneTransform;

    using EditorSceneEntity =
        engine::scene::SceneEntity;

    inline constexpr std::size_t
        InvalidEditorEntityIndex =
            std::numeric_limits<
                std::size_t>::max();

    struct EditorSceneSnapshot final
    {
        std::vector<EditorSceneEntity> entities;

        EditorEntityId nextEntityId = 1U;

        std::size_t selectedIndex =
            InvalidEditorEntityIndex;

        bool dirty = false;
    };

    class EditorSceneDocument final
    {
    public:
        EditorSceneDocument() = default;
        ~EditorSceneDocument() noexcept = default;

        EditorSceneDocument(
            const EditorSceneDocument&) = delete;

        EditorSceneDocument& operator=(
            const EditorSceneDocument&) = delete;

        void CreateDefaultLevel();
        void Clear() noexcept;

        [[nodiscard]]
        EditorEntityId CreateEntity(
            std::wstring name,
            EditorEntityKind kind,
            const EditorTransform& transform);

        [[nodiscard]]
        bool CreateStaticMeshEntity(
            std::wstring name,
            std::wstring assetPath,
            const EditorTransform& transform);

        [[nodiscard]]
        const std::vector<EditorSceneEntity>&
            GetEntities() const noexcept;

        [[nodiscard]]
        const EditorSceneEntity*
            GetSelectedEntity() const noexcept;

        [[nodiscard]]
        EditorSceneEntity*
            GetSelectedEntityMutable() noexcept;

        [[nodiscard]]
        std::size_t
            GetSelectedIndex() const noexcept;

        [[nodiscard]]
        bool SelectEntityByIndex(
            std::size_t index) noexcept;

        void ClearSelection() noexcept;

        [[nodiscard]]
        bool SetSelectedTransform(
            const EditorTransform& transform) noexcept;

        [[nodiscard]]
        bool TranslateSelectedEntity(
            float translationX,
            float translationY,
            float translationZ) noexcept;

        [[nodiscard]]
        bool DuplicateSelectedEntity();

        [[nodiscard]]
        bool DeleteSelectedEntity() noexcept;

        [[nodiscard]]
        EditorSceneSnapshot
            CreateSnapshot() const;

        void RestoreSnapshot(
            const EditorSceneSnapshot& snapshot,
            bool markDirty);

        [[nodiscard]]
        bool IsDirty() const noexcept;

        void MarkSaved() noexcept;

        [[nodiscard]]
        const engine::scene::SceneWorld&
            GetSceneWorld() const noexcept;

    private:
        engine::scene::SceneWorld world_;

        std::size_t selectedIndex_ =
            InvalidEditorEntityIndex;

        bool dirty_ = false;
    };
}