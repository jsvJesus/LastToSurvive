#pragma once

#include "Scene/SceneTypes.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <unordered_map>
#include <vector>

namespace engine::scene
{
    class SpatialGrid final
    {
    public:
        explicit SpatialGrid(
            const float cellSize = 128.0F) noexcept
            : cellSize_(
                std::isfinite(cellSize) &&
                cellSize > 0.001F
                    ? cellSize
                    : 128.0F)
        {
        }

        void Clear() noexcept
        {
            cells_.clear();
        }

        [[nodiscard]]
        bool Rebuild(
            const std::vector<SceneEntity>& entities) noexcept
        {
            try
            {
                cells_.clear();

                if (entities.empty())
                {
                    return true;
                }

                cells_.reserve(
                    entities.size() / 4U + 1U);

                for (std::size_t entityIndex = 0U;
                     entityIndex < entities.size();
                     ++entityIndex)
                {
                    const SceneEntity& entity =
                        entities[entityIndex];

                    const float x =
                        entity.transform.position[0];

                    const float z =
                        entity.transform.position[2];

                    if (!std::isfinite(x) ||
                        !std::isfinite(z))
                    {
                        continue;
                    }

                    const CellKey key =
                        PositionToCell(x, z);

                    cells_[key].push_back(
                        entityIndex);
                }

                return true;
            }
            catch (...)
            {
                cells_.clear();
                return false;
            }
        }

        [[nodiscard]]
        bool QueryRadius(
            const float centerX,
            const float centerZ,
            const float radius,
            std::vector<std::size_t>& output) const noexcept
        {
            output.clear();

            if (!std::isfinite(centerX) ||
                !std::isfinite(centerZ) ||
                !std::isfinite(radius) ||
                radius <= 0.0F)
            {
                return false;
            }

            try
            {
                const CellKey minimum =
                    PositionToCell(
                        centerX - radius,
                        centerZ - radius);

                const CellKey maximum =
                    PositionToCell(
                        centerX + radius,
                        centerZ + radius);

                for (std::int64_t cellZ = minimum.z;; ++cellZ)
                {
                    for (std::int64_t cellX = minimum.x;; ++cellX)
                    {
                        const CellKey key
                        {
                            cellX,
                            cellZ
                        };

                        const auto found =
                            cells_.find(key);

                        if (found != cells_.end())
                        {
                            output.insert(
                                output.end(),
                                found->second.begin(),
                                found->second.end());
                        }

                        if (cellX == maximum.x)
                        {
                            break;
                        }
                    }

                    if (cellZ == maximum.z)
                    {
                        break;
                    }
                }

                return true;
            }
            catch (...)
            {
                output.clear();
                return false;
            }
        }

        [[nodiscard]]
        float GetCellSize() const noexcept
        {
            return cellSize_;
        }

        [[nodiscard]]
        std::size_t GetCellCount() const noexcept
        {
            return cells_.size();
        }

    private:
        struct CellKey final
        {
            std::int64_t x = 0;
            std::int64_t z = 0;

            [[nodiscard]]
            bool operator==(
                const CellKey& other) const noexcept
            {
                return
                    x == other.x &&
                    z == other.z;
            }
        };

        struct CellKeyHash final
        {
            [[nodiscard]]
            std::size_t operator()(
                const CellKey& key) const noexcept
            {
                const std::size_t hashX =
                    std::hash<std::int64_t>{}(key.x);

                const std::size_t hashZ =
                    std::hash<std::int64_t>{}(key.z);

                const std::size_t magic =
                    static_cast<std::size_t>(
                        0x9E3779B97F4A7C15ULL);

                return
                    hashX ^
                    (
                        hashZ +
                        magic +
                        (hashX << 6U) +
                        (hashX >> 2U)
                    );
            }
        };

        [[nodiscard]]
        static std::int64_t ToCellCoordinate(
            const float coordinate,
            const float cellSize) noexcept
        {
            const long double scaled =
                std::floor(
                    static_cast<long double>(coordinate) /
                    static_cast<long double>(cellSize));

            const long double minimum =
                static_cast<long double>(
                    (std::numeric_limits<std::int64_t>::min)());

            const long double maximum =
                static_cast<long double>(
                    (std::numeric_limits<std::int64_t>::max)());

            if (scaled <= minimum)
            {
                return
                    (std::numeric_limits<std::int64_t>::min)();
            }

            if (scaled >= maximum)
            {
                return
                    (std::numeric_limits<std::int64_t>::max)();
            }

            return static_cast<std::int64_t>(
                scaled);
        }

        [[nodiscard]]
        CellKey PositionToCell(
            const float x,
            const float z) const noexcept
        {
            return
            {
                ToCellCoordinate(
                    x,
                    cellSize_),

                ToCellCoordinate(
                    z,
                    cellSize_)
            };
        }

        float cellSize_ = 128.0F;

        std::unordered_map<
            CellKey,
            std::vector<std::size_t>,
            CellKeyHash> cells_;
    };
}