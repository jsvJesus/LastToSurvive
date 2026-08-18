#pragma once

#include "Math/Vector3.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace engine::navigation
{
    struct BuildConfig final
    {
        float cellSize = 0.3f;
        float cellHeight = 0.2f;
        float agentHeight = 1.8f;
        float agentRadius = 0.4f;
        float agentMaxClimb = 0.5f;
        float agentMaxSlopeDegrees = 45.0f;
        float regionMinimumSize = 8.0f;
        float regionMergeSize = 20.0f;
        float edgeMaximumLength = 12.0f;
        float edgeMaximumError = 1.3f;
        float detailSampleDistance = 6.0f;
        float detailSampleMaximumError = 1.0f;
        int verticesPerPolygon = 6;
        int tileSizeCells = 256;
    };

    struct TriangleMeshView final
    {
        const math::Vector3* vertices = nullptr;
        std::size_t vertexCount = 0;
        const std::uint32_t* indices = nullptr;
        std::size_t indexCount = 0;
    };

    class NavigationMesh final
    {
    public:
        NavigationMesh() noexcept;
        ~NavigationMesh() noexcept;

        NavigationMesh(const NavigationMesh&) = delete;
        NavigationMesh& operator=(const NavigationMesh&) = delete;
        NavigationMesh(NavigationMesh&&) noexcept;
        NavigationMesh& operator=(NavigationMesh&&) noexcept;

        [[nodiscard]] bool Build(
            const TriangleMeshView& geometry,
            const BuildConfig& config = {});

        [[nodiscard]] bool Load(const char* filePath);
        [[nodiscard]] bool Save(const char* filePath) const;
        void Clear() noexcept;

        [[nodiscard]] bool IsReady() const noexcept;

        [[nodiscard]] bool FindClosestPoint(
            const math::Vector3& point,
            const math::Vector3& searchExtents,
            math::Vector3& closestPoint) const noexcept;

        [[nodiscard]] bool IsPointOnMesh(
            const math::Vector3& point,
            const math::Vector3& searchExtents = {0.2f, 1.0f, 0.2f}) const noexcept;

        [[nodiscard]] bool FindPath(
            const math::Vector3& start,
            const math::Vector3& destination,
            std::vector<math::Vector3>& path,
            const math::Vector3& searchExtents = {2.0f, 4.0f, 2.0f}) const;

    private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}
