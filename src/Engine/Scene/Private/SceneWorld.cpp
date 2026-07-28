#include "Scene/SceneWorld.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace engine::scene
{
    namespace
    {
        constexpr std::size_t InvalidSceneEntityIndex =
            std::numeric_limits<std::size_t>::max();
    }

    void SceneWorld::Clear() noexcept
    {
        entities_.clear();
        nextEntityId_ = 1U;
    }

    SceneEntityId SceneWorld::CreateEntity(
        std::wstring name,
        const SceneEntityKind kind,
        const SceneTransform& transform)
    {
        SceneEntity entity;

        entity.id = nextEntityId_++;
        entity.kind = kind;

        entity.name =
            name.empty()
                ? L"Entity"
                : std::move(name);

        entity.transform =
            IsFiniteTransform(transform)
                ? transform
                : SceneTransform{};

        EnsureDefaultComponents(entity);

        entities_.push_back(
            std::move(entity));

        return entities_.back().id;
    }

    SceneEntityId SceneWorld::DuplicateEntity(
        const SceneEntityId sourceEntityId)
    {
        const SceneEntity* const source =
            FindEntity(sourceEntityId);

        if (source == nullptr)
        {
            return 0U;
        }

        SceneEntity duplicate = *source;

        duplicate.id = nextEntityId_++;
        duplicate.name += L" Copy";
        duplicate.transform.position[0] += 1.0F;

        EnsureDefaultComponents(duplicate);

        entities_.push_back(
            std::move(duplicate));

        return entities_.back().id;
    }

    bool SceneWorld::DeleteEntity(
        const SceneEntityId entityId) noexcept
    {
        const auto iterator =
            std::find_if(
                entities_.begin(),
                entities_.end(),
                [entityId](
                    const SceneEntity& entity)
                {
                    return entity.id == entityId;
                });

        if (iterator == entities_.end())
        {
            return false;
        }

        entities_.erase(iterator);
        return true;
    }

    SceneEntity* SceneWorld::FindEntity(
        const SceneEntityId entityId) noexcept
    {
        const std::size_t index =
            FindEntityIndex(entityId);

        if (index >= entities_.size())
        {
            return nullptr;
        }

        return &entities_[index];
    }

    const SceneEntity* SceneWorld::FindEntity(
        const SceneEntityId entityId) const noexcept
    {
        const std::size_t index =
            FindEntityIndex(entityId);

        if (index >= entities_.size())
        {
            return nullptr;
        }

        return &entities_[index];
    }

    std::size_t SceneWorld::FindEntityIndex(
        const SceneEntityId entityId) const noexcept
    {
        for (
            std::size_t index = 0U;
            index < entities_.size();
            ++index)
        {
            if (entities_[index].id == entityId)
            {
                return index;
            }
        }

        return InvalidSceneEntityIndex;
    }

    const std::vector<SceneEntity>&
        SceneWorld::GetEntities() const noexcept
    {
        return entities_;
    }

    std::vector<SceneEntity>& SceneWorld::GetEntitiesMutable() noexcept
    {
        return entities_;
    }

    SceneEntityId
        SceneWorld::GetNextEntityId() const noexcept
    {
        return nextEntityId_;
    }

    SceneWorldState SceneWorld::CreateState() const
    {
        SceneWorldState state;

        state.entities = entities_;
        state.nextEntityId = nextEntityId_;

        return state;
    }

    void SceneWorld::RestoreState(
        const SceneWorldState& state)
    {
        entities_ = state.entities;

        SceneEntityId maximumEntityId = 0U;

        for (SceneEntity& entity : entities_)
        {
            if (entity.id == 0U)
            {
                entity.id =
                    maximumEntityId + 1U;
            }

            maximumEntityId =
                std::max(
                    maximumEntityId,
                    entity.id);

            if (entity.name.empty())
            {
                entity.name = L"Entity";
            }

            if (!IsFiniteTransform(entity.transform))
            {
                entity.transform =
                    SceneTransform{};
            }

            EnsureDefaultComponents(entity);
        }

        nextEntityId_ =
            std::max(
                state.nextEntityId,
                maximumEntityId + 1U);

        if (nextEntityId_ == 0U)
        {
            nextEntityId_ = 1U;
        }
    }

    bool SceneWorld::IsFiniteTransform(
        const SceneTransform& transform) noexcept
    {
        for (const float value : transform.position)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
        }

        for (
            const float value :
            transform.rotationDegrees)
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

    void SceneWorld::EnsureDefaultComponents(
        SceneEntity& entity) noexcept
    {
        switch (entity.kind)
        {
            case SceneEntityKind::Environment:
                if (!entity.environment.has_value())
                {
                    entity.environment.emplace();
                }
                break;

            case SceneEntityKind::DirectionalLight:
                if (!entity.directionalLight.has_value())
                {
                    entity.directionalLight.emplace();
                }
                break;

            case SceneEntityKind::SpawnPoint:
                if (!entity.spawnPoint.has_value())
                {
                    entity.spawnPoint.emplace();
                }
                break;

            case SceneEntityKind::Anomaly:
                if (!entity.anomaly.has_value())
                {
                    entity.anomaly.emplace();
                }
                break;

            case SceneEntityKind::LootContainer:
                if (!entity.lootContainer.has_value())
                {
                    entity.lootContainer.emplace();
                }
                break;

            case SceneEntityKind::Terrain:
                if (!entity.terrain.has_value())
                {
                    entity.terrain.emplace();
                }
                break;

            case SceneEntityKind::Character:
                if (!entity.skeletalMesh.has_value())
                {
                    entity.skeletalMesh.emplace();
                }

                if (!entity.characterController.has_value())
                {
                    entity.characterController.emplace();
                }
                break;

            case SceneEntityKind::Empty:
            default:
                break;
        }
    }
}
