#pragma once

#include <Scene/SceneWorld.h>

#include <cstddef>
#include <array>
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

    enum class EditorSelectionMode
    {
        Replace,
        Toggle,
        Range
    };

    struct EditorSceneSnapshot final
    {
        std::vector<EditorSceneEntity> entities;

        EditorEntityId nextEntityId = 1U;

        std::size_t selectedIndex =
            InvalidEditorEntityIndex;

        EditorEntityId selectedEntityId = 0U;

        std::vector<EditorEntityId> selectedEntityIds;
        EditorEntityId selectionAnchorId = 0U;

        bool dirty = false;
    };

    class SceneDocument final
    {
    public:
        SceneDocument() = default;
        ~SceneDocument() noexcept = default;

        SceneDocument(
            const SceneDocument&) = delete;

        SceneDocument& operator=(
            const SceneDocument&) = delete;

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

        [[nodiscard]] bool CreateTerrainEntity(
            std::wstring name,
            std::wstring assetPath,
            const EditorTransform& transform);

        [[nodiscard]]
        bool UpdateSelectedSkeletalMesh(
            engine::scene::SkeletalMeshComponent component);

        [[nodiscard]] bool SetSelectedTerrainLayers(
            std::vector<engine::scene::TerrainComponent::LayerOverride> layers);
        
        [[nodiscard]] bool UpdateSelectedTerrainLayer(
            std::size_t index, std::string diffusePath, std::string normalPath,
            float scaleU, float scaleV, float offsetU, float offsetV,
            bool visible) noexcept;

        [[nodiscard]] bool AddSelectedTerrainLayer(std::string name);
        [[nodiscard]] bool RemoveSelectedTerrainLayer(std::size_t index) noexcept;

        [[nodiscard]] bool UpdateSelectedDirectionalLight(
            const std::array<float, 3U>& color,
            float intensity,
            bool castShadows) noexcept;

        [[nodiscard]] bool UpdateSelectedEnvironment(
            const std::array<float, 3U>& topColor,
            const std::array<float, 3U>& horizonColor,
            const std::array<float, 3U>& groundColor,
            const std::array<float, 3U>& ambientColor,
            float skyIntensity,
            float ambientIntensity,
            float horizonExponent,
            float sunDiskSizeDegrees,
            bool visible,
            bool linkSun) noexcept;

        [[nodiscard]] bool ApplySelectedSkyPreset(
            engine::scene::SkyPreset preset) noexcept;

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

        [[nodiscard]] bool SelectEntityByIndex(
            std::size_t index,
            EditorSelectionMode mode) noexcept;

        [[nodiscard]] bool IsEntitySelected(
            EditorEntityId entityId) const noexcept;

        [[nodiscard]] const std::vector<EditorEntityId>&
            GetSelectedEntityIds() const noexcept;

        void ClearSelection() noexcept;

        [[nodiscard]]
        bool SetSelectedTransform(
            const EditorTransform& transform) noexcept;

        [[nodiscard]] bool RenameSelectedEntity(
            std::wstring name);

        [[nodiscard]]
        bool TranslateSelectedEntity(
            float translationX,
            float translationY,
            float translationZ) noexcept;

        [[nodiscard]]
        bool DuplicateSelectedEntity();

        [[nodiscard]]
        bool DeleteSelectedEntity() noexcept;

        [[nodiscard]] bool SetEntityParent(
            EditorEntityId entityId,
            EditorEntityId parentId) noexcept;

        [[nodiscard]] bool MoveSelectionToFolder(std::wstring folder);
        [[nodiscard]] bool RenameFolder(
            const std::wstring& oldName,
            std::wstring newName);

        [[nodiscard]] bool ApplySelectionTransformDelta(
            const std::array<float, 3U>& positionDelta,
            const std::array<float, 3U>& rotationDelta,
            const std::array<float, 3U>& scaleRatio) noexcept;

        [[nodiscard]] bool SetSelectionTransform(
            const EditorTransform& transform) noexcept;

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
        [[nodiscard]] std::wstring MakeUniqueName(
            std::wstring baseName,
            EditorEntityId ignoredEntityId = 0U) const;

        engine::scene::SceneWorld world_;

        std::size_t selectedIndex_ =
            InvalidEditorEntityIndex;

        std::vector<EditorEntityId> selectedEntityIds_;
        EditorEntityId selectionAnchorId_ = 0U;

        bool dirty_ = false;
    };
}
