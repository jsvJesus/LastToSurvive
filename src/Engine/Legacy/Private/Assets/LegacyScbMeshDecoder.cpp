#include "Legacy/Assets/LegacyScbMeshDecoder.h"

#include "Assets/MeshAssetBuilder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <unordered_map>

namespace engine::legacy::assets
{
namespace
{
constexpr std::uint32_t WeightsFlag = 1U;
constexpr std::uint32_t VertexColorsFlag = 2U;
constexpr std::uint32_t KnownFlags = WeightsFlag | VertexColorsFlag;
constexpr std::int32_t MaximumNameLength = 1024;
constexpr std::int32_t MaximumVertices = 10000000;
constexpr std::int32_t MaximumIndices = 30000000;
constexpr std::int32_t MaximumMaterials = 256;

class Reader final
{
  public:
    explicit Reader(const engine::assets::AssetData &data) noexcept : bytes_(data.GetData()), size_(data.GetSize())
    {
    }
    template <typename T> bool Read(T &value) noexcept
    {
        if (sizeof(T) > size_ - offset_)
            return false;
        std::memcpy(&value, bytes_ + offset_, sizeof(T));
        offset_ += sizeof(T);
        return true;
    }
    bool ReadBytes(void *target, const std::size_t count) noexcept
    {
        if (count > size_ - offset_)
            return false;
        if (count != 0U)
            std::memcpy(target, bytes_ + offset_, count);
        offset_ += count;
        return true;
    }
    bool Skip(const std::size_t count) noexcept
    {
        if (count > size_ - offset_)
            return false;
        offset_ += count;
        return true;
    }
    [[nodiscard]] bool AtEnd() const noexcept
    {
        return offset_ == size_;
    }

  private:
    const std::byte *bytes_ = nullptr;
    std::size_t size_ = 0U;
    std::size_t offset_ = 0U;
};

bool ReadString(Reader &reader, std::string &output) noexcept
{
    std::int32_t length = 0;
    if (!reader.Read(length) || length < 0 || length > MaximumNameLength)
        return false;
    try
    {
        output.resize(static_cast<std::size_t>(length));
    }
    catch (...)
    {
        return false;
    }
    return reader.ReadBytes(output.data(), output.size()) && output.find('\0') == std::string::npos;
}

bool Finite(const float value) noexcept
{
    return std::isfinite(value);
}
} // namespace

void LegacyStaticMeshData::Clear() noexcept
{
    mesh.Clear();
    sourceName.clear();
    pivot = {};
    materialSlotNames.clear();
}
bool LegacyStaticMeshData::IsEmpty() const noexcept
{
    return !mesh.IsValid() && sourceName.empty() && materialSlotNames.empty();
}

engine::assets::AssetResult LegacyScbMeshDecoder::Decode(const engine::assets::AssetData &source,
                                                         LegacyStaticMeshData &outData) noexcept
{
    outData.Clear();
    Reader reader(source);
    std::uint32_t version = 0U, flags = 0U;
    if (!reader.Read(version) || version != SupportedVersion)
        return engine::assets::AssetResult::UnsupportedFormat;
    if (!reader.Read(flags) || (flags & ~KnownFlags) != 0U)
        return engine::assets::AssetResult::UnsupportedFeature;
    if ((flags & WeightsFlag) != 0U)
        return engine::assets::AssetResult::UnsupportedFeature;

    LegacyStaticMeshData candidate;
    if (!ReadString(reader, candidate.sourceName) || candidate.sourceName.empty())
        return engine::assets::AssetResult::CorruptData;
    for (float &value : candidate.pivot)
        if (!reader.Read(value) || !Finite(value))
            return engine::assets::AssetResult::CorruptData;

    std::int32_t vertexCountSigned = 0;
    if (!reader.Read(vertexCountSigned) || vertexCountSigned <= 0 || vertexCountSigned > MaximumVertices)
        return engine::assets::AssetResult::CorruptData;
    const std::size_t vertexCount = static_cast<std::size_t>(vertexCountSigned);
    std::vector<engine::assets::StaticMeshVertex> vertices;
    std::vector<std::array<float, 3U>> tangents;
    try
    {
        vertices.resize(vertexCount);
        tangents.resize(vertexCount);
    }
    catch (const std::bad_alloc &)
    {
        return engine::assets::AssetResult::OutOfMemory;
    }
    catch (...)
    {
        return engine::assets::AssetResult::InternalError;
    }

    for (auto &vertex : vertices)
        for (float &v : vertex.position)
            if (!reader.Read(v) || !Finite(v))
                return engine::assets::AssetResult::CorruptData;
    for (auto &vertex : vertices)
        for (float &v : vertex.texcoord0)
            if (!reader.Read(v) || !Finite(v))
                return engine::assets::AssetResult::CorruptData;
    for (auto &vertex : vertices)
    {
        float lengthSquared = 0.0F;
        for (float &v : vertex.normal)
        {
            if (!reader.Read(v) || !Finite(v))
                return engine::assets::AssetResult::CorruptData;
            lengthSquared += v * v;
        }
        if (!Finite(lengthSquared) || lengthSquared <= 1.0e-12F)
            return engine::assets::AssetResult::CorruptData;
        const float inverseLength = 1.0F / std::sqrt(lengthSquared);
        for (float &v : vertex.normal)
            v *= inverseLength;
    }
    for (auto &tangent : tangents)
        for (float &v : tangent)
            if (!reader.Read(v) || !Finite(v))
                return engine::assets::AssetResult::CorruptData;
    for (std::size_t index = 0U; index < vertexCount; ++index)
    {
        std::int8_t sign = 0;
        if (!reader.Read(sign))
            return engine::assets::AssetResult::CorruptData;
        auto& tangent = tangents[index];
        const auto& normal = vertices[index].normal;
        const float dot = tangent[0] * normal[0] + tangent[1] * normal[1] + tangent[2] * normal[2];
        tangent[0] -= dot * normal[0];
        tangent[1] -= dot * normal[1];
        tangent[2] -= dot * normal[2];
        const float lengthSquared = tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2];
        if (!Finite(lengthSquared) || lengthSquared <= 1.0e-12F)
            return engine::assets::AssetResult::CorruptData;
        const float inverseLength = 1.0F / std::sqrt(lengthSquared);
        vertices[index].tangent = {tangent[0] * inverseLength, tangent[1] * inverseLength,
                                   tangent[2] * inverseLength, sign > 0 ? 1.0F : -1.0F};
    }

    std::int32_t indexCountSigned = 0;
    if (!reader.Read(indexCountSigned) || indexCountSigned <= 0 || indexCountSigned > MaximumIndices ||
        indexCountSigned % 3 != 0)
        return engine::assets::AssetResult::CorruptData;
    std::vector<std::uint32_t> indices;
    try
    {
        indices.resize(static_cast<std::size_t>(indexCountSigned));
    }
    catch (const std::bad_alloc &)
    {
        return engine::assets::AssetResult::OutOfMemory;
    }
    catch (...)
    {
        return engine::assets::AssetResult::InternalError;
    }
    for (std::uint32_t &index : indices)
        if (!reader.Read(index) || index >= vertexCount)
            return engine::assets::AssetResult::CorruptData;

    std::int32_t materialCount = 0;
    if (!reader.Read(materialCount) || materialCount <= 0 || materialCount > MaximumMaterials)
        return engine::assets::AssetResult::CorruptData;
    std::vector<engine::assets::MeshSubmesh> submeshes;
    std::unordered_map<std::string, std::uint32_t> slots;
    try
    {
        submeshes.reserve(static_cast<std::size_t>(materialCount));
        candidate.materialSlotNames.reserve(static_cast<std::size_t>(materialCount));
    }
    catch (...)
    {
        return engine::assets::AssetResult::OutOfMemory;
    }
    std::int32_t previousEnd = 0;
    for (std::int32_t chunk = 0; chunk < materialCount; ++chunk)
    {
        std::int32_t start = 0, end = 0;
        std::string name;
        if (!reader.Read(start) || !reader.Read(end) || !ReadString(reader, name) || name.empty() || start < 0 ||
            end <= start || end > indexCountSigned || start != previousEnd || (end - start) % 3 != 0)
            return engine::assets::AssetResult::CorruptData;
        previousEnd = end;
        std::uint32_t slot = 0U;
        const auto found = slots.find(name);
        if (found == slots.end())
        {
            slot = static_cast<std::uint32_t>(candidate.materialSlotNames.size());
            try
            {
                slots.emplace(name, slot);
                candidate.materialSlotNames.push_back(name);
            }
            catch (...)
            {
                return engine::assets::AssetResult::OutOfMemory;
            }
        }
        else
            slot = found->second;
        submeshes.push_back({static_cast<std::uint32_t>(start), static_cast<std::uint32_t>(end - start), 0, slot});
    }
    if (previousEnd != indexCountSigned)
        return engine::assets::AssetResult::CorruptData;
    if ((flags & VertexColorsFlag) != 0U && !reader.Skip(vertexCount * 4U))
        return engine::assets::AssetResult::CorruptData;
    if (!reader.AtEnd())
        return engine::assets::AssetResult::CorruptData;

    const auto result = engine::assets::MeshAssetBuilder::Build(
        vertices.data(), vertices.size(), indices.data(), indices.size(), submeshes.data(), submeshes.size(),
        static_cast<std::uint32_t>(candidate.materialSlotNames.size()), candidate.sourceName, candidate.mesh);
    if (engine::assets::Failed(result))
        return result;
    outData = std::move(candidate);
    return engine::assets::AssetResult::Success;
}
} // namespace engine::legacy::assets
