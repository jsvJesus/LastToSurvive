#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace engine::scene
{
    using SceneEntityId = std::uint64_t;

    enum class SceneEntityKind : std::uint8_t
    {
        Empty = 0,
        Environment,
        DirectionalLight,
        SpawnPoint,
        Anomaly,
        LootContainer
    };

    struct SceneTransform final
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

    struct NameComponent
    {
        std::wstring name;
    };

    struct TransformComponent
    {
        SceneTransform transform;
    };

    struct EnvironmentComponent final
    {
        std::wstring environmentAsset;
    };

    struct StaticMeshComponent final
    {
        std::wstring assetPath;

        bool visible = true;
        bool castShadows = true;
    };

    struct DirectionalLightComponent final
    {
        std::array<float, 3U> color
        {
            1.0F,
            1.0F,
            1.0F
        };

        float intensity = 4.0F;

        bool castShadows = true;
    };

    struct SpawnPointComponent final
    {
        std::wstring spawnTag = L"Player";
    };

    struct AnomalyComponent final
    {
        std::wstring anomalyType = L"Default";

        float radius = 4.0F;
        float damagePerSecond = 0.0F;
    };

    struct LootContainerComponent final
    {
        std::wstring lootTable;

        float respawnSeconds = 0.0F;
    };

    /*
     * Name и Transform являются обязательными компонентами.
     *
     * Наследование здесь специально сохраняет старый синтаксис:
     *
     * entity.name
     * entity.transform
     *
     * Благодаря этому Editor не приходится переписывать целиком.
     */
    struct SceneEntity final :
        NameComponent,
        TransformComponent
    {
        SceneEntityId id = 0U;

        /*
         * kind используется как основная Editor-классификация.
         * Реальные данные объекта находятся в компонентах ниже.
         */
        SceneEntityKind kind =
            SceneEntityKind::Empty;

        std::optional<EnvironmentComponent>
            environment;

        std::optional<StaticMeshComponent>
            staticMesh;

        std::optional<DirectionalLightComponent>
            directionalLight;

        std::optional<SpawnPointComponent>
            spawnPoint;

        std::optional<AnomalyComponent>
            anomaly;

        std::optional<LootContainerComponent>
            lootContainer;
    };

    struct SceneWorldState final
    {
        std::vector<SceneEntity> entities;

        SceneEntityId nextEntityId = 1U;
    };
}