#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace lts::editor
{
    using EditorEntityId = std::uint64_t;

    inline constexpr std::size_t InvalidEditorEntityIndex =
        std::numeric_limits<std::size_t>::max();

    enum class EditorEntityKind : std::uint8_t
    {
        Empty = 0,
        Environment,
        DirectionalLight,
        SpawnPoint,
        Anomaly,
        LootContainer
    };

    struct EditorTransform final
    {
        std::array<float, 3U> position
        {
            0.0F,
            0.0F,
            0.0F
        };

        std::array<float, 3U> rotationDegrees
        {
            0.0F,
            0.0F,
            0.0F
        };

        std::array<float, 3U> scale
        {
            1.0F,
            1.0F,
            1.0F
        };
    };

    struct EditorSceneEntity final
    {
        EditorEntityId id = 0U;
        EditorEntityKind kind = EditorEntityKind::Empty;

        std::wstring name;
        EditorTransform transform;
    };

    class EditorSceneDocument final
    {
    public:
        EditorSceneDocument() = default;
        ~EditorSceneDocument() noexcept = default;

        EditorSceneDocument(const EditorSceneDocument&) = delete;
        EditorSceneDocument& operator=(const EditorSceneDocument&) = delete;

        void CreateDefaultLevel();
        void Clear() noexcept;

        [[nodiscard]]
        EditorEntityId CreateEntity(
            std::wstring name,
            EditorEntityKind kind,
            const EditorTransform& transform);

        [[nodiscard]]
        const std::vector<EditorSceneEntity>& GetEntities() const noexcept;

        [[nodiscard]]
        const EditorSceneEntity* GetSelectedEntity() const noexcept;

        [[nodiscard]]
        std::size_t GetSelectedIndex() const noexcept;

        [[nodiscard]]
        bool SelectEntityByIndex(std::size_t index) noexcept;

        [[nodiscard]]
        bool IsDirty() const noexcept;

        void MarkSaved() noexcept;

    private:
        std::vector<EditorSceneEntity> entities_;

        EditorEntityId nextEntityId_ = 1U;
        std::size_t selectedIndex_ = InvalidEditorEntityIndex;

        bool dirty_ = false;
    };
}