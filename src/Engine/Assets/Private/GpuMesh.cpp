#include "Assets/GpuMesh.h"

#include <new>

namespace engine::assets
{
    namespace
    {
        bool Destroyed(engine::graphics::GraphicsResult r) noexcept { return r == engine::graphics::GraphicsResult::Success || r == engine::graphics::GraphicsResult::NotFound || r == engine::graphics::GraphicsResult::DeviceRemoved; }
        engine::graphics::GraphicsResult Create(engine::graphics::RenderDevice& d, const MeshAsset& m, engine::graphics::BufferHandle& vb, engine::graphics::BufferHandle& ib) noexcept
        {
            vb = {}; ib = {};
            if (!d.IsReady()) return engine::graphics::GraphicsResult::InvalidState;
            if (!m.IsValid()) return engine::graphics::GraphicsResult::InvalidArgument;
            engine::graphics::BufferDesc vd{}; vd.byteSize = m.GetVertexCount() * sizeof(StaticMeshVertex); vd.stride = sizeof(StaticMeshVertex); vd.usage = engine::graphics::ResourceUsage::Immutable; vd.bindFlags = engine::graphics::BufferBindFlags::Vertex;
            engine::graphics::BufferInitialData vi{reinterpret_cast<const std::byte*>(m.GetVertexData()), vd.byteSize};
            auto r = d.CreateBuffer(vd, &vi, vb); if (engine::graphics::Failed(r)) return r;
            engine::graphics::BufferDesc id{}; id.byteSize = m.GetIndexDataSize(); id.stride = m.GetIndexFormat() == engine::graphics::IndexFormat::UInt16 ? 2U : 4U; id.usage = engine::graphics::ResourceUsage::Immutable; id.bindFlags = engine::graphics::BufferBindFlags::Index; id.indexFormat = m.GetIndexFormat();
            engine::graphics::BufferInitialData ii{m.GetIndexData(), id.byteSize};
            r = d.CreateBuffer(id, &ii, ib); if (engine::graphics::Failed(r)) { (void)d.DestroyBuffer(vb); vb = {}; ib = {}; return r; }
            return engine::graphics::GraphicsResult::Success;
        }
    }
    engine::graphics::GraphicsResult GpuMesh::Upload(engine::graphics::RenderDevice& d, const MeshAsset& m) noexcept
    {
        if (vertexBuffer_.IsValid() || indexBuffer_.IsValid()) return engine::graphics::GraphicsResult::InvalidState;
        engine::graphics::BufferHandle vb, ib; auto r = Create(d, m, vb, ib); if (engine::graphics::Failed(r)) return r;
        try { submeshes_.clear(); for (std::size_t i=0;i<m.GetSubmeshCount();++i) submeshes_.push_back(*m.GetSubmesh(i)); }
        catch (...) { (void)d.DestroyBuffer(ib); (void)d.DestroyBuffer(vb); return engine::graphics::GraphicsResult::OutOfMemory; }
        vertexBuffer_=vb; indexBuffer_=ib; vertexStride_=sizeof(StaticMeshVertex); indexFormat_=m.GetIndexFormat(); indexCount_=static_cast<std::uint32_t>(m.GetIndexCount()); backend_=d.GetBackend(); bounds_=m.GetBounds(); return engine::graphics::GraphicsResult::Success;
    }
    engine::graphics::GraphicsResult GpuMesh::Replace(engine::graphics::RenderDevice& d, const MeshAsset& m) noexcept
    {
        if (!IsValid() || d.GetBackend()!=backend_) return engine::graphics::GraphicsResult::InvalidState;
        GpuMesh replacement; auto r=replacement.Upload(d,m); if(engine::graphics::Failed(r)) return r;
        r=Release(d); if(engine::graphics::Failed(r)) { (void)replacement.Release(d); return r; }
        vertexBuffer_=replacement.vertexBuffer_; indexBuffer_=replacement.indexBuffer_; vertexStride_=replacement.vertexStride_; indexFormat_=replacement.indexFormat_; indexCount_=replacement.indexCount_; submeshes_=std::move(replacement.submeshes_); backend_=replacement.backend_; bounds_=replacement.bounds_; replacement.Clear(); return engine::graphics::GraphicsResult::Success;
    }
    engine::graphics::GraphicsResult GpuMesh::Release(engine::graphics::RenderDevice& d) noexcept
    {
        if (!vertexBuffer_.IsValid() && !indexBuffer_.IsValid()) { Clear(); return engine::graphics::GraphicsResult::Success; }
        if (d.GetBackend()!=backend_) return engine::graphics::GraphicsResult::InvalidArgument;
        auto ir = indexBuffer_.IsValid()?d.DestroyBuffer(indexBuffer_):engine::graphics::GraphicsResult::Success; if(Destroyed(ir)) indexBuffer_={};
        auto vr = vertexBuffer_.IsValid()?d.DestroyBuffer(vertexBuffer_):engine::graphics::GraphicsResult::Success; if(Destroyed(vr)) vertexBuffer_={};
        if (!vertexBuffer_.IsValid() && !indexBuffer_.IsValid()) Clear();
        return engine::graphics::Failed(ir) && !Destroyed(ir) ? ir : vr;
    }
    void GpuMesh::Abandon() noexcept { Clear(); }
    bool GpuMesh::IsValid() const noexcept { return vertexBuffer_.IsValid()&&indexBuffer_.IsValid()&&backend_!=engine::graphics::GraphicsBackend::None&&!submeshes_.empty(); }
    void GpuMesh::Clear() noexcept { vertexBuffer_={};indexBuffer_={};vertexStride_=0;indexFormat_=engine::graphics::IndexFormat::None;indexCount_=0;submeshes_.clear();backend_=engine::graphics::GraphicsBackend::None;bounds_={}; }
}
