#include "Navigation/NavigationMesh.h"

#include "Platform/File.h"

#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace engine::navigation
{
    namespace
    {
        constexpr std::uint32_t NavigationFileMagic = 0x4C4E5453U; // "STNL"
        constexpr std::uint32_t NavigationFileVersion = 2U;
        constexpr unsigned short WalkableAreaFlags = 1U;
        constexpr int MaximumPathPolygons = 2048;
        constexpr int MaximumStraightPathPoints = 2048;

        struct NavigationFileHeader final
        {
            std::uint32_t magic = NavigationFileMagic;
            std::uint32_t version = NavigationFileVersion;
            std::uint32_t tileCount = 0;
            dtNavMeshParams parameters{};
        };

        struct NavigationTileHeader final
        {
            dtTileRef tileReference = 0;
            std::uint32_t dataSize = 0;
        };

        template<typename Type>
        bool WriteValue(platform::File& file, const Type& value) noexcept
        {
            const platform::FileIoResult result = file.Write(&value, sizeof(value));
            return result.success && result.bytesTransferred == sizeof(value);
        }

        template<typename Type>
        bool ReadValue(platform::File& file, Type& value) noexcept
        {
            const platform::FileIoResult result = file.Read(&value, sizeof(value));
            return result.success && result.bytesTransferred == sizeof(value);
        }

        bool IsBuildInputValid(
            const TriangleMeshView& geometry,
            const BuildConfig& config) noexcept
        {
            return
                geometry.vertices != nullptr &&
                geometry.indices != nullptr &&
                geometry.vertexCount >= 3U &&
                geometry.vertexCount <= static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
                geometry.indexCount >= 3U &&
                geometry.indexCount % 3U == 0U &&
                geometry.indexCount <= static_cast<std::size_t>(std::numeric_limits<int>::max()) &&
                config.cellSize > 0.0f &&
                config.cellHeight > 0.0f &&
                config.agentHeight > 0.0f &&
                config.agentRadius >= 0.0f &&
                config.agentMaxClimb >= 0.0f &&
                config.verticesPerPolygon >= 3 &&
                config.verticesPerPolygon <= DT_VERTS_PER_POLYGON &&
                config.tileSizeCells >= 16;
        }

        unsigned NextPowerOfTwo(unsigned value) noexcept
        {
            --value;
            value |= value >> 1U;
            value |= value >> 2U;
            value |= value >> 4U;
            value |= value >> 8U;
            value |= value >> 16U;
            return ++value;
        }

        bool BuildNavigationTile(
            const TriangleMeshView& geometry,
            const BuildConfig& config,
            const std::vector<int>& tileIndices,
            const int tileX,
            const int tileY,
            const float* tileBoundsMinimum,
            const float* tileBoundsMaximum,
            unsigned char*& navigationData,
            int& navigationDataSize)
        {
            navigationData = nullptr;
            navigationDataSize = 0;

            rcConfig recastConfig{};
            recastConfig.cs = config.cellSize;
            recastConfig.ch = config.cellHeight;
            recastConfig.walkableSlopeAngle = config.agentMaxSlopeDegrees;
            recastConfig.walkableHeight = static_cast<int>(std::ceil(config.agentHeight / recastConfig.ch));
            recastConfig.walkableClimb = static_cast<int>(std::floor(config.agentMaxClimb / recastConfig.ch));
            recastConfig.walkableRadius = static_cast<int>(std::ceil(config.agentRadius / recastConfig.cs));
            recastConfig.maxEdgeLen = static_cast<int>(config.edgeMaximumLength / recastConfig.cs);
            recastConfig.maxSimplificationError = config.edgeMaximumError;
            recastConfig.minRegionArea = static_cast<int>(config.regionMinimumSize * config.regionMinimumSize);
            recastConfig.mergeRegionArea = static_cast<int>(config.regionMergeSize * config.regionMergeSize);
            recastConfig.maxVertsPerPoly = config.verticesPerPolygon;
            recastConfig.tileSize = config.tileSizeCells;
            recastConfig.borderSize = recastConfig.walkableRadius + 3;
            recastConfig.width = recastConfig.tileSize + recastConfig.borderSize * 2;
            recastConfig.height = recastConfig.tileSize + recastConfig.borderSize * 2;
            recastConfig.detailSampleDist = config.detailSampleDistance < 0.9f
                ? 0.0f
                : recastConfig.cs * config.detailSampleDistance;
            recastConfig.detailSampleMaxError = recastConfig.ch * config.detailSampleMaximumError;
            rcVcopy(recastConfig.bmin, tileBoundsMinimum);
            rcVcopy(recastConfig.bmax, tileBoundsMaximum);
            recastConfig.bmin[0] -= static_cast<float>(recastConfig.borderSize) * recastConfig.cs;
            recastConfig.bmin[2] -= static_cast<float>(recastConfig.borderSize) * recastConfig.cs;
            recastConfig.bmax[0] += static_cast<float>(recastConfig.borderSize) * recastConfig.cs;
            recastConfig.bmax[2] += static_cast<float>(recastConfig.borderSize) * recastConfig.cs;

            rcContext context{};
            rcHeightfield* heightField = rcAllocHeightfield();
            rcCompactHeightfield* compactHeightField = nullptr;
            rcContourSet* contours = nullptr;
            rcPolyMesh* polygonMesh = nullptr;
            rcPolyMeshDetail* detailMesh = nullptr;
            const auto releaseBuildData = [&]() noexcept
            {
                rcFreePolyMeshDetail(detailMesh);
                rcFreePolyMesh(polygonMesh);
                rcFreeContourSet(contours);
                rcFreeCompactHeightfield(compactHeightField);
                rcFreeHeightField(heightField);
            };

            if (!heightField ||
                !rcCreateHeightfield(&context, *heightField, recastConfig.width, recastConfig.height,
                    recastConfig.bmin, recastConfig.bmax, recastConfig.cs, recastConfig.ch))
            {
                releaseBuildData();
                return false;
            }

            const int triangleCount = static_cast<int>(tileIndices.size() / 3U);
            std::vector<unsigned char> triangleAreas(static_cast<std::size_t>(triangleCount), RC_NULL_AREA);
            const float* vertices = geometry.vertices[0].Data();
            rcMarkWalkableTriangles(&context, recastConfig.walkableSlopeAngle, vertices,
                static_cast<int>(geometry.vertexCount), tileIndices.data(), triangleCount, triangleAreas.data());
            if (!rcRasterizeTriangles(&context, vertices, static_cast<int>(geometry.vertexCount), tileIndices.data(),
                    triangleAreas.data(), triangleCount, *heightField, recastConfig.walkableClimb))
            {
                releaseBuildData();
                return false;
            }

            rcFilterLowHangingWalkableObstacles(&context, recastConfig.walkableClimb, *heightField);
            rcFilterLedgeSpans(&context, recastConfig.walkableHeight, recastConfig.walkableClimb, *heightField);
            rcFilterWalkableLowHeightSpans(&context, recastConfig.walkableHeight, *heightField);

            compactHeightField = rcAllocCompactHeightfield();
            if (!compactHeightField ||
                !rcBuildCompactHeightfield(&context, recastConfig.walkableHeight, recastConfig.walkableClimb,
                    *heightField, *compactHeightField) ||
                !rcErodeWalkableArea(&context, recastConfig.walkableRadius, *compactHeightField) ||
                !rcBuildDistanceField(&context, *compactHeightField) ||
                !rcBuildRegions(&context, *compactHeightField, recastConfig.borderSize,
                    recastConfig.minRegionArea, recastConfig.mergeRegionArea))
            {
                releaseBuildData();
                return false;
            }

            contours = rcAllocContourSet();
            if (!contours ||
                !rcBuildContours(&context, *compactHeightField, recastConfig.maxSimplificationError,
                    recastConfig.maxEdgeLen, *contours))
            {
                releaseBuildData();
                return false;
            }
            if (contours->nconts == 0)
            {
                releaseBuildData();
                return true;
            }

            polygonMesh = rcAllocPolyMesh();
            if (!polygonMesh ||
                !rcBuildPolyMesh(&context, *contours, recastConfig.maxVertsPerPoly, *polygonMesh))
            {
                releaseBuildData();
                return false;
            }
            if (polygonMesh->npolys == 0)
            {
                releaseBuildData();
                return true;
            }

            detailMesh = rcAllocPolyMeshDetail();
            if (!detailMesh ||
                !rcBuildPolyMeshDetail(&context, *polygonMesh, *compactHeightField,
                    recastConfig.detailSampleDist, recastConfig.detailSampleMaxError, *detailMesh))
            {
                releaseBuildData();
                return false;
            }

            for (int polygon = 0; polygon < polygonMesh->npolys; ++polygon)
            {
                if (polygonMesh->areas[polygon] == RC_WALKABLE_AREA)
                {
                    polygonMesh->areas[polygon] = 0;
                    polygonMesh->flags[polygon] = WalkableAreaFlags;
                }
            }

            dtNavMeshCreateParams createParameters{};
            createParameters.verts = polygonMesh->verts;
            createParameters.vertCount = polygonMesh->nverts;
            createParameters.polys = polygonMesh->polys;
            createParameters.polyAreas = polygonMesh->areas;
            createParameters.polyFlags = polygonMesh->flags;
            createParameters.polyCount = polygonMesh->npolys;
            createParameters.nvp = polygonMesh->nvp;
            createParameters.detailMeshes = detailMesh->meshes;
            createParameters.detailVerts = detailMesh->verts;
            createParameters.detailVertsCount = detailMesh->nverts;
            createParameters.detailTris = detailMesh->tris;
            createParameters.detailTriCount = detailMesh->ntris;
            createParameters.walkableHeight = config.agentHeight;
            createParameters.walkableRadius = config.agentRadius;
            createParameters.walkableClimb = config.agentMaxClimb;
            createParameters.tileX = tileX;
            createParameters.tileY = tileY;
            createParameters.tileLayer = 0;
            rcVcopy(createParameters.bmin, polygonMesh->bmin);
            rcVcopy(createParameters.bmax, polygonMesh->bmax);
            createParameters.cs = recastConfig.cs;
            createParameters.ch = recastConfig.ch;
            createParameters.buildBvTree = true;

            const bool created = dtCreateNavMeshData(
                &createParameters, &navigationData, &navigationDataSize);
            releaseBuildData();
            return created;
        }
    }

    struct NavigationMesh::Implementation final
    {
        dtNavMesh* mesh = nullptr;
        dtNavMeshQuery* query = nullptr;

        ~Implementation() noexcept
        {
            Reset();
        }

        void Reset() noexcept
        {
            dtFreeNavMeshQuery(query);
            dtFreeNavMesh(mesh);
            query = nullptr;
            mesh = nullptr;
        }

        [[nodiscard]] bool InitializeQuery() noexcept
        {
            query = dtAllocNavMeshQuery();
            return query && dtStatusSucceed(query->init(mesh, 4096));
        }
    };

    NavigationMesh::NavigationMesh() noexcept
        : implementation_(std::make_unique<Implementation>())
    {
    }

    NavigationMesh::~NavigationMesh() noexcept = default;
    NavigationMesh::NavigationMesh(NavigationMesh&&) noexcept = default;
    NavigationMesh& NavigationMesh::operator=(NavigationMesh&&) noexcept = default;

    bool NavigationMesh::Build(
        const TriangleMeshView& geometry,
        const BuildConfig& config)
    {
        Clear();
        if (!IsBuildInputValid(geometry, config))
        {
            return false;
        }

        std::vector<int> indices;
        indices.reserve(geometry.indexCount);
        for (std::size_t index = 0; index < geometry.indexCount; ++index)
        {
            if (geometry.indices[index] >= geometry.vertexCount ||
                geometry.indices[index] > static_cast<std::uint32_t>(std::numeric_limits<int>::max()))
            {
                return false;
            }
            indices.push_back(static_cast<int>(geometry.indices[index]));
        }

        std::array<float, 3> boundsMinimum{
            geometry.vertices[geometry.indices[0]].x,
            geometry.vertices[geometry.indices[0]].y,
            geometry.vertices[geometry.indices[0]].z};
        std::array<float, 3> boundsMaximum = boundsMinimum;
        for (std::size_t index = 0; index < geometry.indexCount; ++index)
        {
            const std::uint32_t vertexIndex = geometry.indices[index];
            const math::Vector3& vertex = geometry.vertices[vertexIndex];
            boundsMinimum[0] = std::min(boundsMinimum[0], vertex.x);
            boundsMinimum[1] = std::min(boundsMinimum[1], vertex.y);
            boundsMinimum[2] = std::min(boundsMinimum[2], vertex.z);
            boundsMaximum[0] = std::max(boundsMaximum[0], vertex.x);
            boundsMaximum[1] = std::max(boundsMaximum[1], vertex.y);
            boundsMaximum[2] = std::max(boundsMaximum[2], vertex.z);
        }

        int gridWidth = 0;
        int gridHeight = 0;
        rcCalcGridSize(boundsMinimum.data(), boundsMaximum.data(), config.cellSize, &gridWidth, &gridHeight);
        const int tileWidth = (gridWidth + config.tileSizeCells - 1) / config.tileSizeCells;
        const int tileHeight = (gridHeight + config.tileSizeCells - 1) / config.tileSizeCells;
        if (tileWidth <= 0 || tileHeight <= 0 ||
            static_cast<std::uint64_t>(tileWidth) * static_cast<std::uint64_t>(tileHeight) > 1'048'576ULL)
        {
            return false;
        }

        const float tileWorldSize = static_cast<float>(config.tileSizeCells) * config.cellSize;
        const float tileBorderSize = static_cast<float>(
            static_cast<int>(std::ceil(config.agentRadius / config.cellSize)) + 3) * config.cellSize;
        std::vector<std::vector<int>> tileTriangles(
            static_cast<std::size_t>(tileWidth) * static_cast<std::size_t>(tileHeight));
        for (std::size_t triangle = 0; triangle < indices.size(); triangle += 3U)
        {
            const math::Vector3& first = geometry.vertices[static_cast<std::size_t>(indices[triangle])];
            const math::Vector3& second = geometry.vertices[static_cast<std::size_t>(indices[triangle + 1U])];
            const math::Vector3& third = geometry.vertices[static_cast<std::size_t>(indices[triangle + 2U])];
            const float triangleMinimumX = std::min({first.x, second.x, third.x});
            const float triangleMaximumX = std::max({first.x, second.x, third.x});
            const float triangleMinimumZ = std::min({first.z, second.z, third.z});
            const float triangleMaximumZ = std::max({first.z, second.z, third.z});
            const int minimumTileX = std::clamp(
                static_cast<int>(std::floor(
                    (triangleMinimumX - tileBorderSize - boundsMinimum[0]) / tileWorldSize)), 0, tileWidth - 1);
            const int maximumTileX = std::clamp(
                static_cast<int>(std::floor(
                    (triangleMaximumX + tileBorderSize - boundsMinimum[0]) / tileWorldSize)), 0, tileWidth - 1);
            const int minimumTileY = std::clamp(
                static_cast<int>(std::floor(
                    (triangleMinimumZ - tileBorderSize - boundsMinimum[2]) / tileWorldSize)), 0, tileHeight - 1);
            const int maximumTileY = std::clamp(
                static_cast<int>(std::floor(
                    (triangleMaximumZ + tileBorderSize - boundsMinimum[2]) / tileWorldSize)), 0, tileHeight - 1);

            for (int tileY = minimumTileY; tileY <= maximumTileY; ++tileY)
            {
                for (int tileX = minimumTileX; tileX <= maximumTileX; ++tileX)
                {
                    std::vector<int>& bucket = tileTriangles[
                        static_cast<std::size_t>(tileY) * static_cast<std::size_t>(tileWidth) +
                        static_cast<std::size_t>(tileX)];
                    bucket.push_back(indices[triangle]);
                    bucket.push_back(indices[triangle + 1U]);
                    bucket.push_back(indices[triangle + 2U]);
                }
            }
        }

        const std::size_t populatedTileCount = static_cast<std::size_t>(std::count_if(
            tileTriangles.begin(), tileTriangles.end(),
            [](const std::vector<int>& triangles) { return !triangles.empty(); }));
        if (populatedTileCount == 0U || populatedTileCount > 1'048'576U)
        {
            return false;
        }

        dtNavMeshParams parameters{};
        rcVcopy(parameters.orig, boundsMinimum.data());
        parameters.tileWidth = tileWorldSize;
        parameters.tileHeight = tileWorldSize;
        parameters.maxTiles = static_cast<int>(NextPowerOfTwo(static_cast<unsigned>(populatedTileCount)));
        parameters.maxPolys = 1 << 20;
        implementation_->mesh = dtAllocNavMesh();
        if (!implementation_->mesh || dtStatusFailed(implementation_->mesh->init(&parameters)))
        {
            Clear();
            return false;
        }

        std::size_t builtTileCount = 0;
        for (std::size_t tileIndex = 0; tileIndex < tileTriangles.size(); ++tileIndex)
        {
            if (tileTriangles[tileIndex].empty())
            {
                continue;
            }

            const int tileX = static_cast<int>(tileIndex % static_cast<std::size_t>(tileWidth));
            const int tileY = static_cast<int>(tileIndex / static_cast<std::size_t>(tileWidth));
            const std::array<float, 3> tileBoundsMinimum{
                boundsMinimum[0] + static_cast<float>(tileX) * tileWorldSize,
                boundsMinimum[1],
                boundsMinimum[2] + static_cast<float>(tileY) * tileWorldSize};
            const std::array<float, 3> tileBoundsMaximum{
                boundsMinimum[0] + static_cast<float>(tileX + 1) * tileWorldSize,
                boundsMaximum[1],
                boundsMinimum[2] + static_cast<float>(tileY + 1) * tileWorldSize};

            unsigned char* navigationData = nullptr;
            int navigationDataSize = 0;
            if (!BuildNavigationTile(geometry, config, tileTriangles[tileIndex], tileX, tileY,
                    tileBoundsMinimum.data(), tileBoundsMaximum.data(), navigationData, navigationDataSize))
            {
                Clear();
                return false;
            }
            if (!navigationData)
            {
                continue;
            }

            if (dtStatusFailed(implementation_->mesh->addTile(
                    navigationData, navigationDataSize, DT_TILE_FREE_DATA, 0, nullptr)))
            {
                dtFree(navigationData);
                Clear();
                return false;
            }
            ++builtTileCount;
        }

        if (builtTileCount == 0U || !implementation_->InitializeQuery())
        {
            Clear();
            return false;
        }
        return true;
    }

    bool NavigationMesh::Load(const char* const filePath)
    {
        Clear();
        if (!filePath || filePath[0] == '\0')
        {
            return false;
        }

        platform::File file{platform::Path(filePath)};
        NavigationFileHeader header{};
        if (!file || !ReadValue(file, header) ||
            header.magic != NavigationFileMagic ||
            header.version != NavigationFileVersion ||
            header.tileCount == 0U)
        {
            return false;
        }

        implementation_->mesh = dtAllocNavMesh();
        if (!implementation_->mesh || dtStatusFailed(implementation_->mesh->init(&header.parameters)))
        {
            Clear();
            return false;
        }

        for (std::uint32_t tileIndex = 0; tileIndex < header.tileCount; ++tileIndex)
        {
            NavigationTileHeader tileHeader{};
            if (!ReadValue(file, tileHeader) || tileHeader.tileReference == 0 || tileHeader.dataSize == 0U)
            {
                Clear();
                return false;
            }

            unsigned char* tileData = static_cast<unsigned char*>(dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM));
            if (!tileData)
            {
                Clear();
                return false;
            }

            const platform::FileIoResult readResult = file.Read(tileData, tileHeader.dataSize);
            if (!readResult.success || readResult.bytesTransferred != tileHeader.dataSize ||
                dtStatusFailed(implementation_->mesh->addTile(tileData, static_cast<int>(tileHeader.dataSize),
                    DT_TILE_FREE_DATA, tileHeader.tileReference, nullptr)))
            {
                dtFree(tileData);
                Clear();
                return false;
            }
        }

        if (!implementation_->InitializeQuery())
        {
            Clear();
            return false;
        }
        return true;
    }

    bool NavigationMesh::Save(const char* const filePath) const
    {
        if (!IsReady() || !filePath || filePath[0] == '\0')
        {
            return false;
        }

        NavigationFileHeader header{};
        header.parameters = *implementation_->mesh->getParams();
        const dtNavMesh* const mesh = implementation_->mesh;
        for (int tileIndex = 0; tileIndex < mesh->getMaxTiles(); ++tileIndex)
        {
            const dtMeshTile* tile = mesh->getTile(tileIndex);
            if (tile && tile->header && tile->dataSize > 0)
            {
                ++header.tileCount;
            }
        }

        platform::File file(
            platform::Path(filePath),
            platform::FileAccess::Write,
            platform::FileCreation::CreateAlways);
        if (!file || !WriteValue(file, header))
        {
            return false;
        }

        for (int tileIndex = 0; tileIndex < mesh->getMaxTiles(); ++tileIndex)
        {
            const dtMeshTile* tile = mesh->getTile(tileIndex);
            if (!tile || !tile->header || tile->dataSize <= 0)
            {
                continue;
            }

            NavigationTileHeader tileHeader{};
            tileHeader.tileReference = mesh->getTileRef(tile);
            tileHeader.dataSize = static_cast<std::uint32_t>(tile->dataSize);
            if (!WriteValue(file, tileHeader))
            {
                return false;
            }
            const platform::FileIoResult writeResult = file.Write(tile->data, tileHeader.dataSize);
            if (
                !writeResult.success || writeResult.bytesTransferred != tileHeader.dataSize)
            {
                return false;
            }
        }
        return file.Flush();
    }

    void NavigationMesh::Clear() noexcept
    {
        if (implementation_)
        {
            implementation_->Reset();
        }
    }

    bool NavigationMesh::IsReady() const noexcept
    {
        return implementation_ && implementation_->mesh && implementation_->query;
    }

    bool NavigationMesh::FindClosestPoint(
        const math::Vector3& point,
        const math::Vector3& searchExtents,
        math::Vector3& closestPoint) const noexcept
    {
        if (!IsReady())
        {
            return false;
        }

        const dtQueryFilter filter{};
        dtPolyRef polygon = 0;
        float result[3]{};
        const dtStatus status = implementation_->query->findNearestPoly(
            point.Data(), searchExtents.Data(), &filter, &polygon, result);
        if (dtStatusFailed(status) || polygon == 0)
        {
            return false;
        }

        closestPoint = {result[0], result[1], result[2]};
        return true;
    }

    bool NavigationMesh::IsPointOnMesh(
        const math::Vector3& point,
        const math::Vector3& searchExtents) const noexcept
    {
        math::Vector3 closest{};
        return FindClosestPoint(point, searchExtents, closest);
    }

    bool NavigationMesh::FindPath(
        const math::Vector3& start,
        const math::Vector3& destination,
        std::vector<math::Vector3>& path,
        const math::Vector3& searchExtents) const
    {
        path.clear();
        if (!IsReady())
        {
            return false;
        }

        const dtQueryFilter filter{};
        dtPolyRef startPolygon = 0;
        dtPolyRef destinationPolygon = 0;
        float nearestStart[3]{};
        float nearestDestination[3]{};
        if (dtStatusFailed(implementation_->query->findNearestPoly(start.Data(), searchExtents.Data(),
                &filter, &startPolygon, nearestStart)) || startPolygon == 0 ||
            dtStatusFailed(implementation_->query->findNearestPoly(destination.Data(), searchExtents.Data(),
                &filter, &destinationPolygon, nearestDestination)) || destinationPolygon == 0)
        {
            return false;
        }

        std::array<dtPolyRef, MaximumPathPolygons> polygons{};
        int polygonCount = 0;
        if (dtStatusFailed(implementation_->query->findPath(startPolygon, destinationPolygon,
                nearestStart, nearestDestination, &filter, polygons.data(), &polygonCount,
                static_cast<int>(polygons.size()))) || polygonCount <= 0)
        {
            return false;
        }

        std::array<float, MaximumStraightPathPoints * 3> points{};
        int pointCount = 0;
        if (dtStatusFailed(implementation_->query->findStraightPath(nearestStart, nearestDestination,
                polygons.data(), polygonCount, points.data(), nullptr, nullptr, &pointCount,
                MaximumStraightPathPoints)) || pointCount <= 0)
        {
            return false;
        }

        path.reserve(static_cast<std::size_t>(pointCount));
        for (int pointIndex = 0; pointIndex < pointCount; ++pointIndex)
        {
            const float* value = &points[static_cast<std::size_t>(pointIndex) * 3U];
            path.emplace_back(value[0], value[1], value[2]);
        }
        return true;
    }
}
