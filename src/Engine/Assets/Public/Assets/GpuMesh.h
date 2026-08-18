#pragma once

#include "Assets/MeshAsset.h"
#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"

#include <vector>

namespace engine::assets
{
    class GpuMesh final
    {
    public:
        [[nodiscard]] engine::graphics::GraphicsResult Upload(engine::graphics::RenderDevice&, const MeshAsset&) noexcept;
        [[nodiscard]] engine::graphics::GraphicsResult Replace(engine::graphics::RenderDevice&, const MeshAsset&) noexcept;
        [[nodiscard]] engine::graphics::GraphicsResult Release(engine::graphics::RenderDevice&) noexcept;
        void Abandon() noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] engine::graphics::BufferHandle GetVertexBuffer() const noexcept { return vertexBuffer_; }
        [[nodiscard]] engine::graphics::BufferHandle GetIndexBuffer() const noexcept { return indexBuffer_; }
        [[nodiscard]] std::uint32_t GetVertexStride() const noexcept { return vertexStride_; }
        [[nodiscard]] engine::graphics::IndexFormat GetIndexFormat() const noexcept { return indexFormat_; }
        [[nodiscard]] std::size_t GetSubmeshCount() const noexcept { return submeshes_.size(); }
        [[nodiscard]] const MeshSubmesh* GetSubmesh(std::size_t i) const noexcept { return i < submeshes_.size() ? &submeshes_[i] : nullptr; }
        [[nodiscard]] engine::graphics::GraphicsBackend GetBackend() const noexcept { return backend_; }
        [[nodiscard]] const MeshBounds& GetBounds() const noexcept { return bounds_; }
    private:
        void Clear() noexcept;
        engine::graphics::BufferHandle vertexBuffer_, indexBuffer_;
        std::uint32_t vertexStride_ = 0U;
        engine::graphics::IndexFormat indexFormat_ = engine::graphics::IndexFormat::None;
        std::uint32_t indexCount_ = 0U;
        std::vector<MeshSubmesh> submeshes_;
        engine::graphics::GraphicsBackend backend_ = engine::graphics::GraphicsBackend::None;
        MeshBounds bounds_;
    };
}
