#pragma once

#include "Graphics/Buffer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace engine::assets
{
    struct StaticMeshVertex final
    {
        std::array<float, 3U> position{};
        std::array<float, 3U> normal{};
        std::array<float, 4U> tangent{};
        std::array<float, 2U> texcoord0{};
    };

    static_assert(sizeof(StaticMeshVertex) == 48U);

    struct MeshBounds final
    {
        std::array<float, 3U> minimum{};
        std::array<float, 3U> maximum{};
        std::array<float, 3U> sphereCenter{};
        float sphereRadius = 0.0F;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct MeshSubmesh final
    {
        std::uint32_t firstIndex = 0U;
        std::uint32_t indexCount = 0U;
        std::int32_t baseVertex = 0;
        std::uint32_t materialSlot = 0U;
    };

    class MeshAsset final
    {
    public:
        MeshAsset() = default;
        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;
        MeshAsset(MeshAsset&&) noexcept = default;
        MeshAsset& operator=(MeshAsset&&) noexcept = default;

        void Clear() noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] std::size_t GetVertexCount() const noexcept;
        [[nodiscard]] std::size_t GetIndexCount() const noexcept;
        [[nodiscard]] engine::graphics::IndexFormat GetIndexFormat() const noexcept;
        [[nodiscard]] std::size_t GetSubmeshCount() const noexcept;
        [[nodiscard]] const MeshSubmesh* GetSubmesh(std::size_t index) const noexcept;
        [[nodiscard]] const MeshBounds& GetBounds() const noexcept;
        [[nodiscard]] const StaticMeshVertex* GetVertexData() const noexcept;
        [[nodiscard]] const std::byte* GetIndexData() const noexcept;
        [[nodiscard]] std::size_t GetIndexDataSize() const noexcept;
        [[nodiscard]] std::uint32_t GetMaterialSlotCount() const noexcept;
        [[nodiscard]] const std::string& GetDebugName() const noexcept;

    private:
        friend class MeshAssetBuilder;
        friend class MeshAssetLoader;
        std::vector<StaticMeshVertex> vertices_;
        std::vector<std::byte> indices_;
        std::vector<MeshSubmesh> submeshes_;
        engine::graphics::IndexFormat indexFormat_ = engine::graphics::IndexFormat::None;
        std::uint32_t materialSlotCount_ = 0U;
        MeshBounds bounds_;
        std::string debugName_;
    };
}
