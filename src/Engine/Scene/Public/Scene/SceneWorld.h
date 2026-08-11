#pragma once

#include "Scene/SceneTypes.h"

#include <cstddef>
#include <string>
#include <vector>
#include <cstdint>

namespace engine::scene
{
    class SceneWorld final
    {
    public:
        SceneWorld() = default;
        ~SceneWorld() noexcept = default;

        SceneWorld(const SceneWorld&) = delete;
        SceneWorld& operator=(const SceneWorld&) = delete;

        void Clear() noexcept;
        
        [[nodiscard]]
        std::uint64_t GetRevision() const noexcept;

        void MarkChanged() noexcept;

        [[nodiscard]]
        SceneEntityId CreateEntity(
            std::wstring name,
            SceneEntityKind kind,
            const SceneTransform& transform);

        [[nodiscard]]
        SceneEntityId DuplicateEntity(
            SceneEntityId sourceEntityId);

        [[nodiscard]]
        bool DeleteEntity(
            SceneEntityId entityId) noexcept;

        [[nodiscard]]
        SceneEntity* FindEntity(
            SceneEntityId entityId) noexcept;

        [[nodiscard]]
        const SceneEntity* FindEntity(
            SceneEntityId entityId) const noexcept;

        [[nodiscard]]
        std::size_t FindEntityIndex(
            SceneEntityId entityId) const noexcept;

        [[nodiscard]]
        const std::vector<SceneEntity>&
            GetEntities() const noexcept;

        [[nodiscard]] std::vector<SceneEntity>&
            GetEntitiesMutable() noexcept;

        [[nodiscard]]
        SceneEntityId GetNextEntityId() const noexcept;

        [[nodiscard]]
        SceneWorldState CreateState() const;

        void RestoreState(
            const SceneWorldState& state);

        [[nodiscard]]
        static bool IsFiniteTransform(
            const SceneTransform& transform) noexcept;

        static void EnsureDefaultComponents(
            SceneEntity& entity) noexcept;

    private:
        std::vector<SceneEntity> entities_;

        SceneEntityId nextEntityId_ = 1U;
        std::uint64_t revision_ = 1U;
    };
}
