#pragma once

#include "Assets/SkeletalMeshAsset.h"

#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace engine::assets
{
    class GpuSkeletalMesh final
    {
    public:
        GpuSkeletalMesh() = default;
        ~GpuSkeletalMesh() noexcept = default;

        GpuSkeletalMesh(
            const GpuSkeletalMesh&) = delete;

        GpuSkeletalMesh& operator=(
            const GpuSkeletalMesh&) = delete;

        [[nodiscard]]
        engine::graphics::GraphicsResult Upload(
            engine::graphics::RenderDevice& device,
            const SkeletalMeshAsset& mesh) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Replace(
            engine::graphics::RenderDevice& device,
            const SkeletalMeshAsset& mesh) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Release(
            engine::graphics::RenderDevice& device) noexcept;

        void Abandon() noexcept;

        [[nodiscard]]
        bool IsValid() const noexcept;

        [[nodiscard]]
        engine::graphics::BufferHandle
            GetVertexBuffer() const noexcept
        {
            return vertexBuffer_;
        }

        [[nodiscard]]
        engine::graphics::BufferHandle
            GetIndexBuffer() const noexcept
        {
            return indexBuffer_;
        }

        [[nodiscard]]
        std::uint32_t
            GetVertexStride() const noexcept
        {
            return vertexStride_;
        }

        [[nodiscard]]
        engine::graphics::IndexFormat
            GetIndexFormat() const noexcept
        {
            return indexFormat_;
        }

        [[nodiscard]]
        std::size_t
            GetSectionCount() const noexcept
        {
            return sections_.size();
        }

        [[nodiscard]]
        const SkeletalMeshSection*
            GetSection(
                std::size_t index) const noexcept
        {
            return index < sections_.size()
                ? &sections_[index]
                : nullptr;
        }

        [[nodiscard]]
        engine::graphics::GraphicsBackend
            GetBackend() const noexcept
        {
            return backend_;
        }

        [[nodiscard]]
        const MeshBounds&
            GetBounds() const noexcept
        {
            return bounds_;
        }

    private:
        void Clear() noexcept;

        engine::graphics::BufferHandle
            vertexBuffer_;

        engine::graphics::BufferHandle
            indexBuffer_;

        std::uint32_t vertexStride_ = 0U;

        engine::graphics::IndexFormat
            indexFormat_ =
                engine::graphics::IndexFormat::None;

        std::vector<SkeletalMeshSection>
            sections_;

        engine::graphics::GraphicsBackend
            backend_ =
                engine::graphics::GraphicsBackend::None;

        MeshBounds bounds_;
    };
}