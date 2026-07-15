#include "Assets/MeshAssetLoader.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace engine::assets
{
namespace
{
constexpr std::size_t HeaderSize = 160U;
constexpr std::uint32_t EndianMarker = 0x01020304U;
constexpr std::uint32_t MaximumVertexCount = 10000000U;
constexpr std::uint32_t MaximumIndexCount = 30000000U;
constexpr std::uint32_t MaximumSubmeshCount = 65536U;

template <typename T> [[nodiscard]] T Read(const std::byte *bytes, const std::size_t offset) noexcept
{
    T value{};
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

[[nodiscard]] bool IsZero(const std::byte *bytes, std::size_t offset, const std::size_t end) noexcept
{
    for (; offset < end; ++offset)
        if (bytes[offset] != std::byte{})
            return false;
    return true;
}

[[nodiscard]] bool CheckedMultiply(const std::uint64_t left, const std::uint64_t right, std::uint64_t &result) noexcept
{
    if (right != 0U && left > (std::numeric_limits<std::uint64_t>::max)() / right)
        return false;
    result = left * right;
    return true;
}

[[nodiscard]] bool CheckedAdd(const std::uint64_t left, const std::uint64_t right, std::uint64_t &result) noexcept
{
    if (left > (std::numeric_limits<std::uint64_t>::max)() - right)
        return false;
    result = left + right;
    return true;
}

[[nodiscard]] bool ValidateRegion(const std::uint64_t offset, const std::uint64_t size, const std::size_t fileSize,
                                  std::uint64_t &end) noexcept
{
    return CheckedAdd(offset, size, end) && end <= fileSize;
}
} // namespace

AssetResult MeshAssetLoader::Load(const AssetMetadata &metadata, const AssetData &source,
                                  std::unique_ptr<LoadedAsset> &outAsset) noexcept
{
    outAsset.reset();
    if (!metadata.IsValid())
        return AssetResult::InvalidMetadata;
    if (metadata.type != AssetType::Mesh)
        return AssetResult::TypeMismatch;
    if (source.GetSize() < HeaderSize)
        return AssetResult::CorruptData;

    const std::byte *const bytes = source.GetData();
    constexpr char Magic[8] = {'L', 'T', 'S', 'M', 'E', 'S', 'H', '\0'};
    if (std::memcmp(bytes, Magic, sizeof(Magic)) != 0)
        return AssetResult::UnsupportedFormat;
    if (Read<std::uint32_t>(bytes, 8U) != 1U)
        return AssetResult::UnsupportedFormat;
    if (Read<std::uint32_t>(bytes, 12U) != EndianMarker || Read<std::uint32_t>(bytes, 16U) != HeaderSize ||
        Read<std::uint32_t>(bytes, 44U) != 0U || !IsZero(bytes, 136U, HeaderSize))
        return AssetResult::CorruptData;

    const std::uint32_t stride = Read<std::uint32_t>(bytes, 20U);
    const std::uint32_t fileIndexFormat = Read<std::uint32_t>(bytes, 24U);
    const std::uint32_t vertexCount = Read<std::uint32_t>(bytes, 28U);
    const std::uint32_t indexCount = Read<std::uint32_t>(bytes, 32U);
    const std::uint32_t submeshCount = Read<std::uint32_t>(bytes, 36U);
    const std::uint32_t materialCount = Read<std::uint32_t>(bytes, 40U);
    if (stride != sizeof(StaticMeshVertex) || (fileIndexFormat != 1U && fileIndexFormat != 2U) || vertexCount == 0U ||
        vertexCount > MaximumVertexCount || indexCount == 0U || indexCount > MaximumIndexCount || submeshCount == 0U ||
        submeshCount > MaximumSubmeshCount || materialCount == 0U || materialCount > MaximumSubmeshCount)
        return AssetResult::CorruptData;

    const std::uint64_t vertexOffset = Read<std::uint64_t>(bytes, 48U);
    const std::uint64_t vertexSize = Read<std::uint64_t>(bytes, 56U);
    const std::uint64_t indexOffset = Read<std::uint64_t>(bytes, 64U);
    const std::uint64_t indexSize = Read<std::uint64_t>(bytes, 72U);
    const std::uint64_t submeshOffset = Read<std::uint64_t>(bytes, 80U);
    const std::uint64_t submeshSize = Read<std::uint64_t>(bytes, 88U);
    const std::uint64_t indexStride = fileIndexFormat == 1U ? 2U : 4U;
    std::uint64_t expectedVertexSize = 0U, expectedIndexSize = 0U, expectedSubmeshSize = 0U;
    std::uint64_t vertexEnd = 0U, indexEnd = 0U, submeshEnd = 0U;
    if (!CheckedMultiply(vertexCount, stride, expectedVertexSize) ||
        !CheckedMultiply(indexCount, indexStride, expectedIndexSize) ||
        !CheckedMultiply(submeshCount, 16U, expectedSubmeshSize) || vertexSize != expectedVertexSize ||
        indexSize != expectedIndexSize || submeshSize != expectedSubmeshSize || vertexOffset < HeaderSize ||
        vertexOffset % 4U != 0U || indexOffset % indexStride != 0U || submeshOffset % 4U != 0U ||
        !ValidateRegion(vertexOffset, vertexSize, source.GetSize(), vertexEnd) ||
        !ValidateRegion(indexOffset, indexSize, source.GetSize(), indexEnd) ||
        !ValidateRegion(submeshOffset, submeshSize, source.GetSize(), submeshEnd) || indexOffset < vertexEnd ||
        submeshOffset < indexEnd || submeshEnd != source.GetSize())
        return AssetResult::CorruptData;

    MeshAsset mesh;
    try
    {
        mesh.vertices_.resize(vertexCount);
        mesh.indices_.resize(static_cast<std::size_t>(indexSize));
        mesh.submeshes_.resize(submeshCount);
        mesh.debugName_ = metadata.path.String();
    }
    catch (const std::bad_alloc &)
    {
        return AssetResult::OutOfMemory;
    }
    catch (...)
    {
        return AssetResult::InternalError;
    }

    std::memcpy(mesh.vertices_.data(), bytes + static_cast<std::size_t>(vertexOffset),
                static_cast<std::size_t>(vertexSize));
    std::memcpy(mesh.indices_.data(), bytes + static_cast<std::size_t>(indexOffset),
                static_cast<std::size_t>(indexSize));
    mesh.indexFormat_ =
        fileIndexFormat == 1U ? engine::graphics::IndexFormat::UInt16 : engine::graphics::IndexFormat::UInt32;
    mesh.materialSlotCount_ = materialCount;
    for (std::size_t axis = 0U; axis < 3U; ++axis)
    {
        mesh.bounds_.minimum[axis] = Read<float>(bytes, 96U + axis * 4U);
        mesh.bounds_.maximum[axis] = Read<float>(bytes, 108U + axis * 4U);
        mesh.bounds_.sphereCenter[axis] = Read<float>(bytes, 120U + axis * 4U);
    }
    mesh.bounds_.sphereRadius = Read<float>(bytes, 132U);
    for (std::size_t index = 0U; index < submeshCount; ++index)
    {
        const std::size_t offset = static_cast<std::size_t>(submeshOffset) + index * 16U;
        mesh.submeshes_[index] = {Read<std::uint32_t>(bytes, offset), Read<std::uint32_t>(bytes, offset + 4U),
                                  Read<std::int32_t>(bytes, offset + 8U), Read<std::uint32_t>(bytes, offset + 12U)};
    }
    if (!mesh.IsValid() || mesh.bounds_.sphereRadius <= 0.0F)
        return AssetResult::CorruptData;
    try
    {
        outAsset = std::make_unique<MeshLoadedAsset>(std::move(mesh));
    }
    catch (const std::bad_alloc &)
    {
        return AssetResult::OutOfMemory;
    }
    catch (...)
    {
        return AssetResult::InternalError;
    }
    return AssetResult::Success;
}
} // namespace engine::assets
