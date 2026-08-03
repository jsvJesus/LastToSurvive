#pragma once

#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"

#include <filesystem>
#include <string>
#include <vector>

namespace engine::assets
{
    struct StaticMeshPrefabPart final
    {
        std::string name;
        AssetPath meshPath;
    };

    struct StaticMeshPrefab final
    {
        std::string name;

        std::vector<StaticMeshPrefabPart>
            parts;

        void Clear() noexcept;

        [[nodiscard]]
        bool IsValid() const noexcept;
    };

    /*
     * Editor authoring format:
     *
     * LTS_PREFAB 1
     * name "Truck_02"
     * part "Body" "Data/Meshes/.../Body.sm"
     * part "Door" "Data/Meshes/.../Door.sm"
     *
     * Prefab разворачивается в обычные SceneEntity.
     * Runtime loader для prefab пока не требуется.
     */
    class StaticMeshPrefabCodec final
    {
    public:
        StaticMeshPrefabCodec() = delete;

        [[nodiscard]]
        static AssetResult Save(
            const std::filesystem::path& path,
            const StaticMeshPrefab& prefab,
            bool overwriteExisting,
            std::wstring& error) noexcept;

        [[nodiscard]]
        static AssetResult Load(
            const std::filesystem::path& path,
            StaticMeshPrefab& prefab,
            std::wstring& error) noexcept;
    };
}