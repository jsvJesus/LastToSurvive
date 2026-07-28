#pragma once

#include <array>
#include <cstddef>
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
        Terrain,
        Character
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

        SkyPreset preset = SkyPreset::ClearDay;

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

        bool visible = true;
        bool linkSun = true;
    };

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
     * Компонент отображаемого скелетного меша.
     *
     * Пути намеренно пустые. Конкретный персонаж, NPC или оружие
     * должны назначать свои ресурсы через Editor, prefab или loader.
     */
    enum class CharacterMeshSlot : std::uint8_t
    {
        Hair = 0,
        Head,
        Body,
        Legs,
        Shoes,
        FirstPersonBody,
        Count
    };

    inline constexpr std::size_t CharacterMeshSlotCount =
        static_cast<std::size_t>(
            CharacterMeshSlot::Count);

    struct SkeletalMeshPart final
    {
        std::wstring assetPath;

        bool visible = true;
    };

    /*
     * Один модульный персонаж.
     *
     * Все части используют один Skeleton и одну текущую
     * анимационную позу.
     */
    struct SkeletalMeshComponent final
    {
        /*
         * Например:
         *
         * char_lms
         * char_male_01
         * skies_survivor1
         */
        std::wstring characterFamily;

        std::wstring skeletonPath;

        std::array<
            SkeletalMeshPart,
            CharacterMeshSlotCount>
            parts;

        std::wstring idleAnimation;
        std::wstring walkAnimation;
        std::wstring runAnimation;
        std::wstring jumpAnimation;

        bool visible = true;
        bool castShadows = true;

        /*
         * При смене body_01 редактор пытается автоматически
         * назначить bodyfps01 из того же семейства.
         */
        bool autoFirstPersonBody = true;

        [[nodiscard]]
        SkeletalMeshPart& GetPart(
            const CharacterMeshSlot slot) noexcept
        {
            return parts[
                static_cast<std::size_t>(
                    slot)];
        }

        [[nodiscard]]
        const SkeletalMeshPart& GetPart(
            const CharacterMeshSlot slot) const noexcept
        {
            return parts[
                static_cast<std::size_t>(
                    slot)];
        }
    };

    struct CharacterControllerComponent final
    {
        float capsuleRadius = 0.35F;
        float capsuleHeight = 1.80F;

        float walkSpeed = 2.5F;
        float runSpeed = 5.5F;
        float acceleration = 18.0F;
        float deceleration = 22.0F;

        float rotationSpeedDegrees = 540.0F;
        float jumpVelocity = 5.0F;
        float gravity = 9.81F;

        bool playerControlled = true;
        bool useRootMotion = false;
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

        // Editor hierarchy metadata. Runtime systems may safely ignore it.
        SceneEntityId parentId = 0U;
        std::wstring editorFolder;

        std::optional<SkeletalMeshComponent>
            skeletalMesh;

        std::optional<CharacterControllerComponent>
            characterController;

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
