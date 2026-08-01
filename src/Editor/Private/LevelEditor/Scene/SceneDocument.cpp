#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Documents/LevelSerializer.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <filesystem>

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

        struct SkyPresetValues final
        {
            std::array<float, 3U> topColor
            {
                0.055F, 0.200F, 0.550F
            };

            std::array<float, 3U> horizonColor
            {
                0.450F, 0.680F, 0.920F
            };

            std::array<float, 3U> groundColor
            {
                0.080F, 0.075F, 0.070F
            };

            std::array<float, 3U> ambientColor
            {
                0.280F, 0.310F, 0.360F
            };

            float skyIntensity = 1.0F;
            float ambientIntensity = 1.0F;
            float horizonExponent = 0.55F;
            float sunDiskSizeDegrees = 1.25F;

            std::array<float, 3U> sunRotation
            {
                -50.0F, 35.0F, 0.0F
            };

            std::array<float, 3U> sunColor
            {
                1.0F, 0.94F, 0.82F
            };

            float sunIntensity = 4.0F;
        };

        [[nodiscard]]
        SkyPresetValues GetSkyPresetValues(
            const engine::scene::SkyPreset preset) noexcept
        {
            SkyPresetValues value;

            switch (preset)
            {
                case engine::scene::SkyPreset::ClearDay:
                    break;

                case engine::scene::SkyPreset::Cloudy:
                    value.topColor = {0.170F, 0.205F, 0.250F};
                    value.horizonColor = {0.410F, 0.440F, 0.470F};
                    value.groundColor = {0.080F, 0.080F, 0.085F};
                    value.ambientColor = {0.340F, 0.365F, 0.400F};
                    value.skyIntensity = 0.75F;
                    value.ambientIntensity = 1.15F;
                    value.horizonExponent = 0.85F;
                    value.sunDiskSizeDegrees = 0.35F;
                    value.sunRotation = {-60.0F, 20.0F, 0.0F};
                    value.sunColor = {0.78F, 0.84F, 0.92F};
                    value.sunIntensity = 1.60F;
                    break;

                case engine::scene::SkyPreset::Sunrise:
                    value.topColor = {0.025F, 0.060F, 0.170F};
                    value.horizonColor = {0.950F, 0.360F, 0.120F};
                    value.groundColor = {0.070F, 0.030F, 0.025F};
                    value.ambientColor = {0.300F, 0.180F, 0.200F};
                    value.skyIntensity = 0.90F;
                    value.ambientIntensity = 0.80F;
                    value.horizonExponent = 0.42F;
                    value.sunDiskSizeDegrees = 2.00F;
                    value.sunRotation = {-8.0F, -75.0F, 0.0F};
                    value.sunColor = {1.00F, 0.39F, 0.13F};
                    value.sunIntensity = 3.20F;
                    break;

                case engine::scene::SkyPreset::Sunset:
                    value.topColor = {0.035F, 0.045F, 0.140F};
                    value.horizonColor = {1.000F, 0.240F, 0.070F};
                    value.groundColor = {0.055F, 0.020F, 0.018F};
                    value.ambientColor = {0.270F, 0.135F, 0.180F};
                    value.skyIntensity = 0.85F;
                    value.ambientIntensity = 0.72F;
                    value.horizonExponent = 0.38F;
                    value.sunDiskSizeDegrees = 2.20F;
                    value.sunRotation = {-6.0F, 78.0F, 0.0F};
                    value.sunColor = {1.00F, 0.24F, 0.07F};
                    value.sunIntensity = 3.00F;
                    break;

                case engine::scene::SkyPreset::Night:
                    value.topColor = {0.002F, 0.006F, 0.025F};
                    value.horizonColor = {0.018F, 0.030F, 0.070F};
                    value.groundColor = {0.003F, 0.004F, 0.008F};
                    value.ambientColor = {0.030F, 0.045F, 0.095F};
                    value.skyIntensity = 0.32F;
                    value.ambientIntensity = 0.38F;
                    value.horizonExponent = 0.70F;
                    value.sunDiskSizeDegrees = 0.50F;
                    value.sunRotation = {-42.0F, -20.0F, 0.0F};
                    value.sunColor = {0.30F, 0.42F, 0.70F};
                    value.sunIntensity = 0.35F;
                    break;

                case engine::scene::SkyPreset::Storm:
                    value.topColor = {0.012F, 0.016F, 0.022F};
                    value.horizonColor = {0.105F, 0.125F, 0.145F};
                    value.groundColor = {0.020F, 0.022F, 0.025F};
                    value.ambientColor = {0.105F, 0.125F, 0.150F};
                    value.skyIntensity = 0.55F;
                    value.ambientIntensity = 0.66F;
                    value.horizonExponent = 1.20F;
                    value.sunDiskSizeDegrees = 0.12F;
                    value.sunRotation = {-65.0F, 40.0F, 0.0F};
                    value.sunColor = {0.55F, 0.62F, 0.72F};
                    value.sunIntensity = 0.55F;
                    break;

                case engine::scene::SkyPreset::Custom:
                    break;
            }

            return value;
        }

        [[nodiscard]]
        bool IsValidSkyColor(
            const std::array<float, 3U>& color) noexcept
        {
            for (const float component : color)
            {
                if (!std::isfinite(component) ||
                    component < 0.0F ||
                    component > 100.0F)
                {
                    return false;
                }
            }

            return true;
        }
    }

    void SceneDocument::CreateDefaultLevel()
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
        if (selectedIndex_ != InvalidEditorEntityIndex)
            static_cast<void>(SelectEntityByIndex(selectedIndex_));

        dirty_ = false;
    }

    void SceneDocument::Clear() noexcept
    {
        world_.Clear();

        selectedIndex_ =
            InvalidEditorEntityIndex;
        selectedEntityIds_.clear();
        selectionAnchorId_ = 0U;

        dirty_ = false;
    }

    EditorEntityId SceneDocument::CreateEntity(
        std::wstring name,
        const EditorEntityKind kind,
        const EditorTransform& transform)
    {
        name = MakeUniqueName(std::move(name));
        const EditorEntityId entityId =
            world_.CreateEntity(
                std::move(name),
                kind,
                transform);

        selectedIndex_ =
            world_.FindEntityIndex(
                entityId);

        selectedEntityIds_.assign(
            1U,
            entityId);

        selectionAnchorId_ =
            entityId;

        if (
            kind ==
                EditorEntityKind::Character)
        {
            std::wstring ignoredError;

            static_cast<void>(
                ReloadCharacterAnimationProfile(
                    entityId,
                    ignoredError));
        }

        dirty_ = true;
        return entityId;
    }

    bool SceneDocument::CreateStaticMeshEntity(
    std::wstring name,
    std::wstring assetPath,
    const EditorTransform& transform)
    {
        if (assetPath.empty())
        {
            return false;
        }

        name = MakeUniqueName(name.empty() ? L"Static Mesh" : std::move(name));
        const EditorEntityId entityId =
            world_.CreateEntity(
                std::move(name),
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
        selectedEntityIds_.assign(1U, entityId);
        selectionAnchorId_ = entityId;

        dirty_ = true;
        return true;
    }

    bool SceneDocument::CreateTerrainEntity(
        std::wstring name, std::wstring assetPath,
        const EditorTransform& transform)
    {
        if (assetPath.empty()) return false;
        for (EditorSceneEntity& existing : world_.GetEntitiesMutable())
        {
            if (!existing.terrain.has_value()) continue;
            existing.terrain->assetPath = std::move(assetPath);
            existing.transform = transform;
            selectedIndex_ = world_.FindEntityIndex(existing.id);
            selectedEntityIds_.assign(1U, existing.id);
            selectionAnchorId_ = existing.id;
            dirty_ = true;
            return true;
        }
        name = MakeUniqueName(name.empty() ? L"Terrain" : std::move(name));
        const EditorEntityId id = world_.CreateEntity(
            std::move(name), EditorEntityKind::Terrain, transform);
        EditorSceneEntity* entity = world_.FindEntity(id);
        if (entity == nullptr) return false;
        entity->terrain.emplace();
        entity->terrain->assetPath = std::move(assetPath);
        entity->terrain->visible = true;
        entity->terrain->castShadows = true;
        selectedIndex_ = world_.FindEntityIndex(id);
        selectedEntityIds_.assign(1U, id);
        selectionAnchorId_ = id;
        dirty_ = true;
        return true;
    }

    bool SceneDocument::UpdateSelectedSkeletalMesh(engine::scene::SkeletalMeshComponent component)
    {
        EditorSceneEntity* const entity =
            GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !entity->skeletalMesh.has_value())
        {
            return false;
        }

        entity->skeletalMesh =
            std::move(component);

        dirty_ = true;
        return true;
    }

    bool SceneDocument::UpdateSelectedCharacterAnimation(engine::scene::CharacterAnimationComponent component)
    {
        EditorSceneEntity* const entity =
            GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !entity->characterAnimation.has_value())
        {
            return false;
        }

        /*
         * Runtime не переносим из Inspector.
         * После изменения набора клипов state machine
         * должна начать с чистого состояния.
         */
        component.runtime.Reset();

        entity->characterAnimation =
            std::move(component);

        dirty_ = true;
        return true;
    }

    bool SceneDocument::
        SetSelectedCharacterAnimationProfile(
            std::wstring profilePath,
            std::wstring& error)
    {
        error.clear();

        EditorSceneEntity* const entity =
            GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !entity->characterAnimation.has_value())
        {
            error =
                L"The selected entity has no "
                L"CharacterAnimation component.";

            return false;
        }

        if (profilePath.empty())
        {
            error =
                L"Character animation profile "
                L"path is empty.";

            return false;
        }

        profilePath =
            std::filesystem::path(
                profilePath).
                lexically_normal().
                generic_wstring();

        engine::scene::
            CharacterAnimationComponent&
                component =
                    *entity->characterAnimation;

        if (
            component.profilePath ==
            profilePath)
        {
            return false;
        }

        component.profilePath =
            std::move(profilePath);

        component.animationSet = {};
        component.runtime.Reset();

        component.profileLoaded = false;
        component.profileError.clear();

        std::wstring loadError;

        static_cast<void>(
            ReloadCharacterAnimationProfile(
                entity->id,
                loadError));

        error =
            std::move(loadError);

        dirty_ = true;
        return true;
    }

    bool SceneDocument::
        ReloadSelectedCharacterAnimationProfile(
            std::wstring& error)
    {
        const EditorSceneEntity* const entity =
            GetSelectedEntity();

        if (entity == nullptr)
        {
            error =
                L"No entity is selected.";

            return false;
        }

        return ReloadCharacterAnimationProfile(
            entity->id,
            error);
    }

    bool SceneDocument::
        ReloadCharacterAnimationProfile(
            const EditorEntityId entityId,
            std::wstring& error)
    {
        error.clear();

        EditorSceneEntity* const entity =
            world_.FindEntity(
                entityId);

        if (
            entity == nullptr ||
            !entity->characterAnimation.has_value())
        {
            error =
                L"The entity has no "
                L"CharacterAnimation component.";

            return false;
        }

        engine::scene::
            CharacterAnimationComponent&
                component =
                    *entity->characterAnimation;

        engine::scene::CharacterAnimationSet
            loadedSet;

        if (!LevelSerializer::
                LoadCharacterAnimationProfile(
                    component.profilePath,
                    loadedSet,
                    error))
        {
            /*
             * Последний валидный set сохраняется.
             *
             * Это также сохраняет compatibility
             * с inline Animation Set версии 5.
             */
            component.runtime.Reset();

            component.profileLoaded = false;
            component.profileError = error;

            return false;
        }

        component.animationSet =
            std::move(loadedSet);

        component.runtime.Reset();

        component.profileLoaded = true;
        component.profileError.clear();

        return true;
    }

    void SceneDocument::
        ReloadCharacterAnimationProfiles()
    {
        for (
            EditorSceneEntity& entity :
            world_.GetEntitiesMutable())
        {
            if (!entity.characterAnimation.has_value())
            {
                continue;
            }

            std::wstring ignoredError;

            static_cast<void>(
                ReloadCharacterAnimationProfile(
                    entity.id,
                    ignoredError));
        }
    }

    bool SceneDocument::SetSelectedTerrainLayers(
        std::vector<engine::scene::TerrainComponent::LayerOverride> layers)
    {
        EditorSceneEntity* entity = GetSelectedEntityMutable();
        if (entity == nullptr || !entity->terrain.has_value()) return false;
        entity->terrain->layers = std::move(layers);
        dirty_ = true;
        return true;
    }

    bool SceneDocument::UpdateSelectedTerrainLayer(
        const std::size_t index, std::string diffusePath, std::string normalPath,
        const float scaleU, const float scaleV, const float offsetU,
        const float offsetV, const bool visible) noexcept
    {
        EditorSceneEntity* entity = GetSelectedEntityMutable();
        if (entity == nullptr || !entity->terrain.has_value() ||
            index >= entity->terrain->layers.size() ||
            !std::isfinite(scaleU) || !std::isfinite(scaleV) ||
            !std::isfinite(offsetU) || !std::isfinite(offsetV) ||
            scaleU <= 0.0F || scaleV <= 0.0F) return false;
        auto& layer = entity->terrain->layers[index];
        if (layer.scaleU == scaleU && layer.scaleV == scaleV &&
            layer.offsetU == offsetU && layer.offsetV == offsetV &&
            layer.diffusePath == diffusePath && layer.normalPath == normalPath &&
            layer.visible == visible) return false;
        layer.diffusePath = std::move(diffusePath); layer.normalPath = std::move(normalPath);
        layer.scaleU = scaleU; layer.scaleV = scaleV;
        layer.offsetU = offsetU; layer.offsetV = offsetV; layer.visible = visible;
        dirty_ = true; return true;
    }

    bool SceneDocument::AddSelectedTerrainLayer(std::string name)
    {
        EditorSceneEntity* entity = GetSelectedEntityMutable();

        if (entity == nullptr ||
            !entity->terrain.has_value() ||
            entity->terrain->layers.size() >= 18U)
        {
            return false;
        }

        auto& layers = entity->terrain->layers;

        if (name.empty())
        {
            name = "Layer " + std::to_string(layers.size());
        }

        const std::string baseName = name;
        std::uint32_t suffix = 2U;

        const auto nameExists = [&layers](const std::string& candidate)
        {
            return std::any_of(
                layers.begin(),
                layers.end(),
                [&candidate](const auto& layer)
                {
                    return layer.name == candidate;
                });
        };

        while (nameExists(name))
        {
            name = baseName + " " + std::to_string(suffix++);
        }

        engine::scene::TerrainComponent::LayerOverride layer;
        layer.name = std::move(name);
        layer.scaleU = 16.0F;
        layer.scaleV = 16.0F;
        layer.visible = true;

        layers.push_back(std::move(layer));
        dirty_ = true;

        return true;
    }

    bool SceneDocument::RemoveSelectedTerrainLayer(std::size_t index) noexcept
    {
        EditorSceneEntity* entity = GetSelectedEntityMutable();

        if (entity == nullptr ||
            !entity->terrain.has_value() ||
            index == 0U ||
            index >= entity->terrain->layers.size())
        {
            return false;
        }

        auto& layers = entity->terrain->layers;
        layers.erase(layers.begin() + static_cast<std::ptrdiff_t>(index));

        dirty_ = true;
        return true;
    }

    bool SceneDocument::UpdateSelectedDirectionalLight(const std::array<float, 3>& color, float intensity,
        bool castShadows) noexcept
    {
        EditorSceneEntity* entity = GetSelectedEntityMutable();

        if (entity == nullptr ||
            !entity->directionalLight.has_value() ||
            !std::isfinite(intensity) ||
            intensity < 0.0F ||
            intensity > 1000.0F)
        {
            return false;
        }

        for (const float component : color)
        {
            if (!std::isfinite(component) ||
                component < 0.0F ||
                component > 100.0F)
            {
                return false;
            }
        }

        auto& light = *entity->directionalLight;

        if (light.color == color &&
            light.intensity == intensity &&
            light.castShadows == castShadows)
        {
            return false;
        }

        light.color = color;
        light.intensity = intensity;
        light.castShadows = castShadows;

        dirty_ = true;
        return true;
    }

    bool SceneDocument::UpdateSelectedEnvironment(const std::array<float, 3>& topColor,
        const std::array<float, 3>& horizonColor, const std::array<float, 3>& groundColor,
        const std::array<float, 3>& ambientColor, float skyIntensity, float ambientIntensity, float horizonExponent,
        float sunDiskSizeDegrees, bool visible, bool linkSun) noexcept
    {
        EditorSceneEntity* entity = GetSelectedEntityMutable();

        if (entity == nullptr ||
            !entity->environment.has_value() ||
            !IsValidSkyColor(topColor) ||
            !IsValidSkyColor(horizonColor) ||
            !IsValidSkyColor(groundColor) ||
            !IsValidSkyColor(ambientColor) ||
            !std::isfinite(skyIntensity) ||
            !std::isfinite(ambientIntensity) ||
            !std::isfinite(horizonExponent) ||
            !std::isfinite(sunDiskSizeDegrees) ||
            skyIntensity < 0.0F ||
            skyIntensity > 100.0F ||
            ambientIntensity < 0.0F ||
            ambientIntensity > 100.0F ||
            horizonExponent < 0.05F ||
            horizonExponent > 8.0F ||
            sunDiskSizeDegrees < 0.01F ||
            sunDiskSizeDegrees > 20.0F)
        {
            return false;
        }

        auto& environment = *entity->environment;

        if (environment.topColor == topColor &&
            environment.horizonColor == horizonColor &&
            environment.groundColor == groundColor &&
            environment.ambientColor == ambientColor &&
            environment.skyIntensity == skyIntensity &&
            environment.ambientIntensity == ambientIntensity &&
            environment.horizonExponent == horizonExponent &&
            environment.sunDiskSizeDegrees == sunDiskSizeDegrees &&
            environment.visible == visible &&
            environment.linkSun == linkSun)
        {
            return false;
        }

        environment.preset = engine::scene::SkyPreset::Custom;
        environment.topColor = topColor;
        environment.horizonColor = horizonColor;
        environment.groundColor = groundColor;
        environment.ambientColor = ambientColor;
        environment.skyIntensity = skyIntensity;
        environment.ambientIntensity = ambientIntensity;
        environment.horizonExponent = horizonExponent;
        environment.sunDiskSizeDegrees = sunDiskSizeDegrees;
        environment.visible = visible;
        environment.linkSun = linkSun;

        dirty_ = true;
        return true;
    }

    bool SceneDocument::ApplySelectedSkyPreset(engine::scene::SkyPreset preset) noexcept
    {
        if (preset == engine::scene::SkyPreset::Custom)
        {
            return false;
        }

        EditorSceneEntity* entity = GetSelectedEntityMutable();

        if (entity == nullptr || !entity->environment.has_value())
        {
            return false;
        }

        const SkyPresetValues values = GetSkyPresetValues(preset);

        auto& environment = *entity->environment;
        environment.preset = preset;
        environment.topColor = values.topColor;
        environment.horizonColor = values.horizonColor;
        environment.groundColor = values.groundColor;
        environment.ambientColor = values.ambientColor;
        environment.skyIntensity = values.skyIntensity;
        environment.ambientIntensity = values.ambientIntensity;
        environment.horizonExponent = values.horizonExponent;
        environment.sunDiskSizeDegrees = values.sunDiskSizeDegrees;

        if (environment.linkSun)
        {
            for (EditorSceneEntity& candidate :
                 world_.GetEntitiesMutable())
            {
                if (!candidate.directionalLight.has_value())
                {
                    continue;
                }

                candidate.transform.rotationDegrees =
                    values.sunRotation;

                candidate.directionalLight->color =
                    values.sunColor;

                candidate.directionalLight->intensity =
                    values.sunIntensity;

                break;
            }
        }

        dirty_ = true;
        return true;
    }

    const std::vector<EditorSceneEntity>&
        SceneDocument::GetEntities() const noexcept
    {
        return world_.GetEntities();
    }

    const EditorSceneEntity* SceneDocument::FindEntity(EditorEntityId entityId) const noexcept
    {
        return world_.FindEntity(
            entityId);
    }

    EditorSceneEntity* SceneDocument::FindEntityMutable(EditorEntityId entityId) noexcept
    {
        return world_.FindEntity(
            entityId);
    }

    const EditorSceneEntity*
        SceneDocument::
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
        SceneDocument::
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
        SceneDocument::
            GetSelectedIndex() const noexcept
    {
        return selectedIndex_;
    }

    bool SceneDocument::SelectEntityByIndex(
        const std::size_t index) noexcept
    {
        return SelectEntityByIndex(index, EditorSelectionMode::Replace);
    }

    bool SceneDocument::SelectEntityByIndex(
        const std::size_t index,
        const EditorSelectionMode mode) noexcept
    {
        const std::vector<EditorSceneEntity>& entities = GetEntities();
        if (index >= entities.size())
        {
            return false;
        }

        const EditorEntityId entityId = entities[index].id;
        if (mode == EditorSelectionMode::Toggle)
        {
            const auto iterator = std::find(
                selectedEntityIds_.begin(), selectedEntityIds_.end(), entityId);
            if (iterator != selectedEntityIds_.end())
            {
                selectedEntityIds_.erase(iterator);
                if (selectedEntityIds_.empty())
                {
                    ClearSelection();
                    return true;
                }
                selectedIndex_ = world_.FindEntityIndex(selectedEntityIds_.back());
                return true;
            }
            selectedEntityIds_.push_back(entityId);
        }
        else if (mode == EditorSelectionMode::Range && selectionAnchorId_ != 0U)
        {
            const std::size_t anchorIndex = world_.FindEntityIndex(selectionAnchorId_);
            if (anchorIndex != InvalidEditorEntityIndex)
            {
                selectedEntityIds_.clear();
                const std::size_t first = std::min(anchorIndex, index);
                const std::size_t last = std::max(anchorIndex, index);
                for (std::size_t selectionIndex = first;
                     selectionIndex <= last; ++selectionIndex)
                {
                    selectedEntityIds_.push_back(entities[selectionIndex].id);
                }
            }
        }
        else
        {
            selectedEntityIds_.assign(1U, entityId);
            selectionAnchorId_ = entityId;
        }

        if (selectedEntityIds_.empty()) selectedEntityIds_.push_back(entityId);
        selectedIndex_ = index;
        if (mode != EditorSelectionMode::Range) selectionAnchorId_ = entityId;
        return true;
    }

    bool SceneDocument::IsEntitySelected(
        const EditorEntityId entityId) const noexcept
    {
        return std::find(selectedEntityIds_.begin(), selectedEntityIds_.end(), entityId) !=
            selectedEntityIds_.end();
    }

    const std::vector<EditorEntityId>&
        SceneDocument::GetSelectedEntityIds() const noexcept
    {
        return selectedEntityIds_;
    }

    void SceneDocument::ClearSelection() noexcept
    {
        selectedIndex_ =
            InvalidEditorEntityIndex;
        selectedEntityIds_.clear();
        selectionAnchorId_ = 0U;
    }

    bool SceneDocument::SetSelectedTransform(
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

    std::wstring SceneDocument::MakeUniqueName(
        std::wstring baseName,
        const EditorEntityId ignoredEntityId) const
    {
        if (baseName.empty())
        {
            baseName = L"Entity";
        }
        const auto exists = [this, ignoredEntityId](const std::wstring& candidate)
        {
            return std::any_of(GetEntities().begin(), GetEntities().end(),
                [&candidate, ignoredEntityId](const EditorSceneEntity& entity)
                {
                    return entity.id != ignoredEntityId && entity.name == candidate;
                });
        };
        if (!exists(baseName))
        {
            return baseName;
        }
        for (std::uint32_t suffix = 2U; suffix < 1000000U; ++suffix)
        {
            std::wstring candidate = baseName + L"_" + std::to_wstring(suffix);
            if (!exists(candidate))
            {
                return candidate;
            }
        }
        return baseName + L"_Copy";
    }

    bool SceneDocument::RenameSelectedEntity(std::wstring name)
    {
        EditorSceneEntity* const entity = GetSelectedEntityMutable();
        if (entity == nullptr)
        {
            return false;
        }
        name = MakeUniqueName(std::move(name), entity->id);
        if (entity->name == name)
        {
            return false;
        }
        entity->name = std::move(name);
        dirty_ = true;
        return true;
    }

    bool SceneDocument::TranslateSelectedEntity(
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

    bool SceneDocument::
        DuplicateSelectedEntity()
    {
        if (selectedEntityIds_.empty())
        {
            return false;
        }

        const std::vector<EditorEntityId> sourceIds = selectedEntityIds_;
        std::vector<EditorEntityId> duplicatedIds;
        duplicatedIds.reserve(sourceIds.size());
        for (const EditorEntityId sourceId : sourceIds)
        {
            const EditorEntityId duplicatedId = world_.DuplicateEntity(sourceId);
            if (duplicatedId == 0U) continue;
            if (EditorSceneEntity* const duplicated = world_.FindEntity(duplicatedId))
            {
                duplicated->name = MakeUniqueName(duplicated->name, duplicated->id);
            }
            duplicatedIds.push_back(duplicatedId);
        }

        if (duplicatedIds.empty())
        {
            return false;
        }
        selectedEntityIds_ = std::move(duplicatedIds);
        selectionAnchorId_ = selectedEntityIds_.front();
        selectedIndex_ = world_.FindEntityIndex(selectedEntityIds_.back());

        dirty_ = true;
        return true;
    }

    bool SceneDocument::
        DeleteSelectedEntity() noexcept
    {
        if (selectedEntityIds_.empty())
        {
            return false;
        }

        const std::size_t previousIndex = selectedIndex_;
        const std::vector<EditorEntityId> ids = selectedEntityIds_;
        for (const EditorSceneEntity& entity : world_.GetEntities())
        {
            if (std::find(ids.begin(), ids.end(), entity.parentId) != ids.end())
            {
                if (EditorSceneEntity* const mutableEntity = world_.FindEntity(entity.id))
                    mutableEntity->parentId = 0U;
            }
        }
        bool deleted = false;
        for (const EditorEntityId entityId : ids)
        {
            deleted = world_.DeleteEntity(entityId) || deleted;
        }
        if (!deleted) return false;

        const auto& entities =
            world_.GetEntities();
        selectedEntityIds_.clear();
        selectionAnchorId_ = 0U;

        if (entities.empty())
        {
            selectedIndex_ =
                InvalidEditorEntityIndex;
        }
        else if (
            previousIndex >=
            entities.size())
        {
            selectedIndex_ =
                entities.size() - 1U;
        }
        else
        {
            selectedIndex_ = previousIndex;
        }
        if (selectedIndex_ != InvalidEditorEntityIndex)
        {
            selectedEntityIds_.push_back(entities[selectedIndex_].id);
            selectionAnchorId_ = entities[selectedIndex_].id;
        }

        dirty_ = true;
        return true;
    }

    bool SceneDocument::SetEntityParent(
        const EditorEntityId entityId,
        const EditorEntityId parentId) noexcept
    {
        EditorSceneEntity* const entity = world_.FindEntity(entityId);
        if (entity == nullptr || entityId == parentId) return false;
        if (parentId != 0U && world_.FindEntity(parentId) == nullptr) return false;

        EditorEntityId ancestorId = parentId;
        while (ancestorId != 0U)
        {
            if (ancestorId == entityId) return false;
            const EditorSceneEntity* const ancestor = world_.FindEntity(ancestorId);
            if (ancestor == nullptr) break;
            ancestorId = ancestor->parentId;
        }
        if (entity->parentId == parentId) return false;
        entity->parentId = parentId;
        dirty_ = true;
        return true;
    }

    bool SceneDocument::MoveSelectionToFolder(std::wstring folder)
    {
        bool changed = false;
        for (const EditorEntityId entityId : selectedEntityIds_)
        {
            EditorSceneEntity* const entity = world_.FindEntity(entityId);
            if (entity != nullptr && entity->editorFolder != folder)
            {
                entity->editorFolder = folder;
                changed = true;
            }
        }
        dirty_ = dirty_ || changed;
        return changed;
    }

    bool SceneDocument::RenameFolder(
        const std::wstring& oldName, std::wstring newName)
    {
        if (oldName.empty() || oldName == newName) return false;
        bool changed = false;
        for (const EditorSceneEntity& entity : world_.GetEntities())
        {
            if (entity.editorFolder != oldName) continue;
            if (EditorSceneEntity* const mutableEntity = world_.FindEntity(entity.id))
            {
                mutableEntity->editorFolder = newName;
                changed = true;
            }
        }
        dirty_ = dirty_ || changed;
        return changed;
    }

    bool SceneDocument::ApplySelectionTransformDelta(
        const std::array<float, 3U>& positionDelta,
        const std::array<float, 3U>& rotationDelta,
        const std::array<float, 3U>& scaleRatio) noexcept
    {
        bool changed = false;
        for (const EditorEntityId entityId : selectedEntityIds_)
        {
            EditorSceneEntity* const entity = world_.FindEntity(entityId);
            if (entity == nullptr) continue;
            EditorTransform transform = entity->transform;
            for (std::size_t axis = 0U; axis < 3U; ++axis)
            {
                transform.position[axis] += positionDelta[axis];
                transform.rotationDegrees[axis] += rotationDelta[axis];
                transform.scale[axis] = std::max(
                    0.001F, transform.scale[axis] * scaleRatio[axis]);
            }
            if (!TransformsEqual(transform, entity->transform))
            {
                entity->transform = transform;
                changed = true;
            }
        }
        dirty_ = dirty_ || changed;
        return changed;
    }

    bool SceneDocument::SetSelectionTransform(
        const EditorTransform& transform) noexcept
    {
        if (!engine::scene::SceneWorld::IsFiniteTransform(transform)) return false;
        bool changed = false;
        for (const EditorEntityId entityId : selectedEntityIds_)
        {
            EditorSceneEntity* const entity = world_.FindEntity(entityId);
            if (entity != nullptr && !TransformsEqual(entity->transform, transform))
            {
                entity->transform = transform;
                changed = true;
            }
        }
        dirty_ = dirty_ || changed;
        return changed;
    }

    EditorSceneSnapshot
        SceneDocument::
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
        if (const EditorSceneEntity* const selected = GetSelectedEntity())
        {
            snapshot.selectedEntityId = selected->id;
        }
        snapshot.selectedEntityIds = selectedEntityIds_;
        snapshot.selectionAnchorId = selectionAnchorId_;

        snapshot.dirty = dirty_;

        return snapshot;
    }

    void SceneDocument::RestoreSnapshot(
        const EditorSceneSnapshot& snapshot,
        const bool markDirty)
    {
        engine::scene::SceneWorldState state;

        state.entities = snapshot.entities;
        state.nextEntityId =
            snapshot.nextEntityId;

        world_.RestoreState(state);

        /*
         * animationSet не хранится в level.
         * После восстановления сцены загружаем
         * его из внешнего profilePath.
         */
        ReloadCharacterAnimationProfiles();

        selectedEntityIds_.clear();
        for (const EditorEntityId entityId : snapshot.selectedEntityIds)
        {
            if (world_.FindEntity(entityId) != nullptr)
                selectedEntityIds_.push_back(entityId);
        }
        selectionAnchorId_ = world_.FindEntity(snapshot.selectionAnchorId) != nullptr
            ? snapshot.selectionAnchorId : 0U;

        selectedIndex_ = InvalidEditorEntityIndex;
        if (snapshot.selectedEntityId != 0U)
        {
            selectedIndex_ = world_.FindEntityIndex(snapshot.selectedEntityId);
        }
        if (selectedIndex_ == InvalidEditorEntityIndex)
        {
            selectedIndex_ = snapshot.selectedIndex;
        }

        const auto& entities =
            world_.GetEntities();

        if (selectedIndex_ >= entities.size())
        {
            selectedIndex_ =
                entities.empty()
                    ? InvalidEditorEntityIndex
                    : entities.size() - 1U;
        }
        if (selectedEntityIds_.empty() && selectedIndex_ != InvalidEditorEntityIndex)
        {
            selectedEntityIds_.push_back(entities[selectedIndex_].id);
        }

        dirty_ =
            markDirty
                ? true
                : snapshot.dirty;
    }

    bool SceneDocument::IsDirty() const noexcept
    {
        return dirty_;
    }

    void SceneDocument::MarkSaved() noexcept
    {
        dirty_ = false;
    }

    const engine::scene::SceneWorld&
        SceneDocument::
            GetSceneWorld() const noexcept
    {
        return world_;
    }
}
