#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

namespace lts::editor
{
    class TerrainRenderer;

    struct GrassAssetEntry final
    {
        std::filesystem::path sourcePath;
        std::filesystem::path relativePath;
        std::wstring displayName;
        std::wstring logicalMeshPath;
    };

    struct GrassRenderInstance final
    {
        std::filesystem::path relativeSrtPath;

        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;

        float yawDegrees = 0.0F;
        float scale = 1.0F;
        float modulation = 1.0F;
    };

    class GrassEditor final
    {
    public:
        GrassEditor() noexcept;

        [[nodiscard]] bool RefreshAssets(
            const std::filesystem::path& workspaceRoot,
            std::string& status) noexcept;

        [[nodiscard]] bool Load(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelRoot,
            std::string& status) noexcept;

        [[nodiscard]] bool Save(
            const std::filesystem::path& levelRoot,
            std::string& status) noexcept;

        void Reset() noexcept;

        void BeginStroke() noexcept;

        [[nodiscard]] bool Stamp(
            const std::filesystem::path& workspaceRoot,
            const TerrainRenderer& terrainRenderer,
            const SceneDocument& levelDocument,
            float worldX,
            float worldZ,
            float radius,
            bool erase,
            std::string& status) noexcept;

        [[nodiscard]] bool EndStroke() noexcept;

        [[nodiscard]] bool RecalculateHeights(
            const TerrainRenderer& terrainRenderer,
            const SceneDocument& levelDocument,
            std::string& status) noexcept;

        [[nodiscard]] bool UpdateModulation(
            std::string& status) noexcept;

        [[nodiscard]] bool ClearAll(
            std::string& status) noexcept;

        [[nodiscard]] bool Optimize(
            std::string& status) noexcept;

        void PrepareRenderDocument(
            const SceneDocument& levelDocument,
            const DirectX::XMFLOAT3& cameraPosition) noexcept;

        [[nodiscard]] const SceneDocument& GetRenderDocument() const noexcept;

        [[nodiscard]] const std::vector<GrassAssetEntry>&
            GetAssets() const noexcept;

        [[nodiscard]] std::size_t
            GetSelectedAssetIndex() const noexcept;

        void SetSelectedAssetIndex(
            std::size_t index) noexcept;

        [[nodiscard]] std::size_t
            GetInstanceCount() const noexcept;

        [[nodiscard]]
        const std::vector<GrassRenderInstance>&
            GetRenderInstances() const noexcept;

        [[nodiscard]] bool
            IsDirty() const noexcept;

        [[nodiscard]] float
            GetViewDistance() const noexcept;

        void SetViewDistance(
            float value) noexcept;

    private:
        using Instance = GrassRenderInstance;

        [[nodiscard]] bool EnsureSelectedAssetCooked(
            const std::filesystem::path& workspaceRoot,
            std::string& status) noexcept;

        void MarkChanged() noexcept;

        std::vector<GrassAssetEntry> assets_;
        std::vector<Instance> instances_;

        SceneDocument renderDocument_;

        std::mt19937 random_;

        std::size_t selectedAssetIndex_ = 0U;

        float viewDistance_ = 600.0F;

        float lastStampX_ = 0.0F;
        float lastStampZ_ = 0.0F;

        float lastCameraX_ = 0.0F;
        float lastCameraZ_ = 0.0F;

        std::uint64_t lastLevelRevision_ = 0U;

        bool strokeActive_ = false;
        bool strokeChanged_ = false;
        bool hasLastStamp_ = false;
        bool hasLastCamera_ = false;

        bool dirty_ = false;
        bool renderDocumentDirty_ = true;
    };
}