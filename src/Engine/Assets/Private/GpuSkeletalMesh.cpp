#include "Assets/GpuSkeletalMesh.h"

#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]]
        bool WasDestroyed(
            const engine::graphics::GraphicsResult
                result) noexcept
        {
            return
                result ==
                    engine::graphics::
                        GraphicsResult::Success ||
                result ==
                    engine::graphics::
                        GraphicsResult::NotFound ||
                result ==
                    engine::graphics::
                        GraphicsResult::DeviceRemoved;
        }

        [[nodiscard]]
        engine::graphics::GraphicsResult
            CreateBuffers(
                engine::graphics::RenderDevice& device,
                const SkeletalMeshAsset& mesh,
                engine::graphics::BufferHandle&
                    vertexBuffer,
                engine::graphics::BufferHandle&
                    indexBuffer) noexcept
        {
            vertexBuffer = {};
            indexBuffer = {};

            if (!device.IsReady())
            {
                return engine::graphics::
                    GraphicsResult::InvalidState;
            }

            if (!mesh.IsValid())
            {
                return engine::graphics::
                    GraphicsResult::InvalidArgument;
            }

            engine::graphics::BufferDesc
                vertexDescription;

            vertexDescription.byteSize =
                mesh.GetVertexCount() *
                sizeof(SkeletalMeshVertex);

            vertexDescription.stride =
                sizeof(SkeletalMeshVertex);

            vertexDescription.usage =
                engine::graphics::
                    ResourceUsage::Immutable;

            vertexDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Vertex;

            engine::graphics::BufferInitialData
                vertexInitialData;

            vertexInitialData.data =
                reinterpret_cast<const std::byte*>(
                    mesh.GetVertexData());

            vertexInitialData.dataSize =
                vertexDescription.byteSize;

            engine::graphics::GraphicsResult result =
                device.CreateBuffer(
                    vertexDescription,
                    &vertexInitialData,
                    vertexBuffer);

            if (engine::graphics::Failed(result))
            {
                return result;
            }

            engine::graphics::BufferDesc
                indexDescription;

            indexDescription.byteSize =
                mesh.GetIndexDataSize();

            indexDescription.stride =
                sizeof(std::uint32_t);

            indexDescription.usage =
                engine::graphics::
                    ResourceUsage::Immutable;

            indexDescription.bindFlags =
                engine::graphics::
                    BufferBindFlags::Index;

            indexDescription.indexFormat =
                engine::graphics::
                    IndexFormat::UInt32;

            engine::graphics::BufferInitialData
                indexInitialData;

            indexInitialData.data =
                reinterpret_cast<const std::byte*>(
                    mesh.GetIndexData());

            indexInitialData.dataSize =
                indexDescription.byteSize;

            result =
                device.CreateBuffer(
                    indexDescription,
                    &indexInitialData,
                    indexBuffer);

            if (engine::graphics::Failed(result))
            {
                static_cast<void>(
                    device.DestroyBuffer(
                        vertexBuffer));

                vertexBuffer = {};
                indexBuffer = {};

                return result;
            }

            return engine::graphics::
                GraphicsResult::Success;
        }
    }

    engine::graphics::GraphicsResult
        GpuSkeletalMesh::Upload(
            engine::graphics::RenderDevice& device,
            const SkeletalMeshAsset& mesh) noexcept
    {
        if (
            vertexBuffer_.IsValid() ||
            indexBuffer_.IsValid())
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        engine::graphics::BufferHandle
            vertexBuffer;

        engine::graphics::BufferHandle
            indexBuffer;

        const engine::graphics::GraphicsResult
            result =
                CreateBuffers(
                    device,
                    mesh,
                    vertexBuffer,
                    indexBuffer);

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        try
        {
            sections_.clear();

            sections_.reserve(
                mesh.GetSectionCount());

            for (
                std::size_t index = 0U;
                index < mesh.GetSectionCount();
                ++index)
            {
                const SkeletalMeshSection*
                    section =
                        mesh.GetSection(index);

                if (section == nullptr)
                {
                    static_cast<void>(
                        device.DestroyBuffer(
                            indexBuffer));

                    static_cast<void>(
                        device.DestroyBuffer(
                            vertexBuffer));

                    sections_.clear();

                    return engine::graphics::
                        GraphicsResult::InvalidArgument;
                }

                sections_.push_back(*section);
            }
        }
        catch (const std::bad_alloc&)
        {
            static_cast<void>(
                device.DestroyBuffer(indexBuffer));

            static_cast<void>(
                device.DestroyBuffer(vertexBuffer));

            sections_.clear();

            return engine::graphics::
                GraphicsResult::OutOfMemory;
        }
        catch (...)
        {
            static_cast<void>(
                device.DestroyBuffer(indexBuffer));

            static_cast<void>(
                device.DestroyBuffer(vertexBuffer));

            sections_.clear();

            return engine::graphics::GraphicsResult::BackendFailure;
        }

        vertexBuffer_ = vertexBuffer;
        indexBuffer_ = indexBuffer;

        vertexStride_ =
            sizeof(SkeletalMeshVertex);

        indexFormat_ =
            engine::graphics::
                IndexFormat::UInt32;

        backend_ = device.GetBackend();
        bounds_ = mesh.GetBounds();

        return engine::graphics::
            GraphicsResult::Success;
    }

    engine::graphics::GraphicsResult
        GpuSkeletalMesh::Replace(
            engine::graphics::RenderDevice& device,
            const SkeletalMeshAsset& mesh) noexcept
    {
        if (
            !IsValid() ||
            device.GetBackend() != backend_)
        {
            return engine::graphics::
                GraphicsResult::InvalidState;
        }

        GpuSkeletalMesh replacement;

        engine::graphics::GraphicsResult result =
            replacement.Upload(
                device,
                mesh);

        if (engine::graphics::Failed(result))
        {
            return result;
        }

        result = Release(device);

        if (engine::graphics::Failed(result))
        {
            static_cast<void>(
                replacement.Release(device));

            return result;
        }

        vertexBuffer_ =
            replacement.vertexBuffer_;

        indexBuffer_ =
            replacement.indexBuffer_;

        vertexStride_ =
            replacement.vertexStride_;

        indexFormat_ =
            replacement.indexFormat_;

        sections_ =
            std::move(
                replacement.sections_);

        backend_ =
            replacement.backend_;

        bounds_ =
            replacement.bounds_;

        replacement.Clear();

        return engine::graphics::
            GraphicsResult::Success;
    }

    engine::graphics::GraphicsResult
        GpuSkeletalMesh::Release(
            engine::graphics::RenderDevice& device) noexcept
    {
        if (
            !vertexBuffer_.IsValid() &&
            !indexBuffer_.IsValid())
        {
            Clear();

            return engine::graphics::
                GraphicsResult::Success;
        }

        if (device.GetBackend() != backend_)
        {
            return engine::graphics::
                GraphicsResult::InvalidArgument;
        }

        const engine::graphics::GraphicsResult
            indexResult =
                indexBuffer_.IsValid()
                    ? device.DestroyBuffer(
                        indexBuffer_)
                    : engine::graphics::
                        GraphicsResult::Success;

        if (WasDestroyed(indexResult))
        {
            indexBuffer_ = {};
        }

        const engine::graphics::GraphicsResult
            vertexResult =
                vertexBuffer_.IsValid()
                    ? device.DestroyBuffer(
                        vertexBuffer_)
                    : engine::graphics::
                        GraphicsResult::Success;

        if (WasDestroyed(vertexResult))
        {
            vertexBuffer_ = {};
        }

        if (
            !vertexBuffer_.IsValid() &&
            !indexBuffer_.IsValid())
        {
            Clear();
        }

        if (
            engine::graphics::Failed(indexResult) &&
            !WasDestroyed(indexResult))
        {
            return indexResult;
        }

        return vertexResult;
    }

    void GpuSkeletalMesh::Abandon() noexcept
    {
        Clear();
    }

    bool GpuSkeletalMesh::IsValid() const noexcept
    {
        return
            vertexBuffer_.IsValid() &&
            indexBuffer_.IsValid() &&
            vertexStride_ ==
                sizeof(SkeletalMeshVertex) &&
            indexFormat_ ==
                engine::graphics::
                    IndexFormat::UInt32 &&
            backend_ !=
                engine::graphics::
                    GraphicsBackend::None &&
            !sections_.empty() &&
            bounds_.IsValid();
    }

    void GpuSkeletalMesh::Clear() noexcept
    {
        vertexBuffer_ = {};
        indexBuffer_ = {};

        vertexStride_ = 0U;

        indexFormat_ =
            engine::graphics::
                IndexFormat::None;

        sections_.clear();

        backend_ =
            engine::graphics::
                GraphicsBackend::None;

        bounds_ = {};
    }
}