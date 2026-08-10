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
        LootContainer,
        Terrain
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

    enum class SkyPreset : std::uint8_t
    {
        ClearDay = 0,
        Cloudy,
        Sunrise,
        Sunset,
        Night,
        Storm,
        Custom
    };

    struct EnvironmentComponent final
    {
        std::wstring environmentAsset;

        SkyPreset preset =
            SkyPreset::ClearDay;

        std::array<float, 3U> topColor
        {
            0.055F,
            0.200F,
            0.550F
        };

        std::array<float, 3U> horizonColor
        {
            0.450F,
            0.680F,
            0.920F
        };

        std::array<float, 3U> groundColor
        {
            0.080F,
            0.075F,
            0.070F
        };

        std::array<float, 3U> ambientColor
        {
            0.280F,
            0.310F,
            0.360F
        };

        float skyIntensity = 1.0F;
        float ambientIntensity = 1.0F;
        float horizonExponent = 0.55F;
        float sunDiskSizeDegrees = 1.25F;

        // Independent DX11 Studio day/night environment state.
        float timeOfDay = 12.0F;
        bool timeControlsSun = true;
        bool sunEnabled = true;

        bool fogEnabled = true;
        std::array<float, 3U> fogColor
        {
            0.45F,
            0.62F,
            0.78F
        };
        float fogStart = 450.0F;
        float fogEnd = 5000.0F;
        float fogDensity = 0.00018F;
        float fogHeightFalloff = 0.0015F;

        bool shadowsEnabled = true;
        float shadowStrength = 0.82F;
        float shadowSoftness = 1.25F;
        float shadowDistance = 1800.0F;

        bool cloudPlaneEnabled = true;
        std::array<float, 3U> cloudColor
        {
            0.92F,
            0.95F,
            1.0F
        };
        float cloudCoverage = 0.48F;
        float cloudDensity = 0.72F;
        float cloudScale = 0.00032F;
        float cloudHeight = 1400.0F;
        std::array<float, 2U> cloudSpeed
        {
            0.0035F,
            0.0012F
        };

        bool visible = true;
        bool linkSun = true;
    };

    /*
     * Generic static mesh component.
     *
     * assetPath должен ссылаться только на новый
     * ресурс, созданный FbxStaticMeshImporter.
     *
     * Никакой WarZ geometry/skeleton логики здесь нет.
     */
    struct StaticMeshComponent final
    {
        std::wstring assetPath;

        bool visible = true;
        bool castShadows = true;
    };

    struct TerrainComponent final
    {
        struct LayerOverride final
        {
            std::string name;

            std::string diffusePath;
            std::string normalPath;

            float scaleU = 1.0F;
            float scaleV = 1.0F;

            float offsetU = 0.0F;
            float offsetV = 0.0F;

            bool visible = true;
        };

        std::wstring assetPath;

        bool visible = true;
        bool castShadows = true;

        std::vector<LayerOverride> layers;
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
        std::wstring spawnTag =
            L"Player";
    };

    struct AnomalyComponent final
    {
        std::wstring anomalyType =
            L"Default";

        float radius = 4.0F;
        float damagePerSecond = 0.0F;
    };

    struct LootContainerComponent final
    {
        std::wstring lootTable;

        float respawnSeconds = 0.0F;
    };

    struct SceneEntity final :
        NameComponent,
        TransformComponent
    {
        SceneEntityId id = 0U;

        SceneEntityKind kind =
            SceneEntityKind::Empty;

        SceneEntityId parentId = 0U;

        std::wstring editorFolder;

        std::optional<EnvironmentComponent>
            environment;

        std::optional<StaticMeshComponent>
            staticMesh;

        std::optional<TerrainComponent>
            terrain;

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
