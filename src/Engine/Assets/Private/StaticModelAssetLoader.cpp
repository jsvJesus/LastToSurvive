#include "Assets/StaticModelAssetLoader.h"

#include <cstring>
#include <new>
#include <string_view>

namespace engine::assets
{
namespace
{
constexpr std::size_t HeaderSize = 64U;
constexpr std::size_t EntrySize = 16U;
template<typename T> T Read(const std::byte* bytes, const std::size_t offset) noexcept
{
    T value{};
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}
bool Zero(const std::byte* bytes, const std::size_t begin, const std::size_t end) noexcept
{
    for (std::size_t index = begin; index < end; ++index)
        if (bytes[index] != std::byte{}) return false;
    return true;
}
bool Region(const std::size_t offset, const std::size_t length, const std::size_t size) noexcept
{
    return offset <= size && length <= size - offset;
}
AssetResult ReadPath(const std::byte* bytes, const std::size_t size, const std::size_t offset,
                     const std::size_t length, AssetPath& path) noexcept
{
    if (length == 0U || length > AssetPath::MaximumLength || !Region(offset, length, size))
        return AssetResult::CorruptData;
    return AssetPath::TryCreate(
        std::string_view(reinterpret_cast<const char*>(bytes + offset), length), path);
}
}

AssetResult StaticModelAssetLoader::Load(const AssetMetadata& metadata, const AssetData& source,
                                         std::unique_ptr<LoadedAsset>& outAsset) noexcept
{
    outAsset.reset();
    if (!metadata.IsValid()) return AssetResult::InvalidMetadata;
    if (metadata.type != AssetType::StaticModel) return AssetResult::TypeMismatch;
    if (source.GetSize() < HeaderSize) return AssetResult::CorruptData;
    const std::byte* const bytes = source.GetData();
    if (std::memcmp(bytes, "LTSMODEL", 8U) != 0) return AssetResult::UnsupportedFormat;
    if (Read<std::uint32_t>(bytes, 8U) != 1U) return AssetResult::UnsupportedFormat;
    if (Read<std::uint32_t>(bytes, 12U) != 0x01020304U ||
        Read<std::uint32_t>(bytes, 16U) != HeaderSize || !Zero(bytes, 36U, 40U) || !Zero(bytes, 60U, 64U))
        return AssetResult::CorruptData;
    const std::size_t count = Read<std::uint32_t>(bytes, 20U);
    if (count == 0U || count > StaticModelAsset::MaximumMaterialCount ||
        count > (source.GetSize() - HeaderSize) / EntrySize || Read<std::uint64_t>(bytes, 40U) != HeaderSize)
        return AssetResult::CorruptData;
    const std::size_t stringsBegin = HeaderSize + count * EntrySize;
    std::size_t expected = stringsBegin;
    const std::size_t meshOffset = static_cast<std::size_t>(Read<std::uint64_t>(bytes, 24U));
    const std::size_t meshLength = Read<std::uint32_t>(bytes, 32U);
    if (meshOffset != expected || !Region(meshOffset, meshLength, source.GetSize())) return AssetResult::CorruptData;
    AssetPath meshPath;
    AssetResult result = ReadPath(bytes, source.GetSize(), meshOffset, meshLength, meshPath);
    if (Failed(result)) return result;
    expected += meshLength;
    std::vector<AssetPath> materials;
    try { materials.reserve(count); }
    catch (...) { return AssetResult::OutOfMemory; }
    for (std::size_t index = 0U; index < count; ++index)
    {
        const std::size_t entry = HeaderSize + index * EntrySize;
        if (!Zero(bytes, entry + 12U, entry + EntrySize)) return AssetResult::CorruptData;
        const std::size_t offset = static_cast<std::size_t>(Read<std::uint64_t>(bytes, entry));
        const std::size_t length = Read<std::uint32_t>(bytes, entry + 8U);
        if (offset != expected || !Region(offset, length, source.GetSize())) return AssetResult::CorruptData;
        AssetPath path;
        result = ReadPath(bytes, source.GetSize(), offset, length, path);
        if (Failed(result)) return result;
        try { materials.push_back(std::move(path)); }
        catch (...) { return AssetResult::OutOfMemory; }
        expected += length;
    }
    const std::size_t debugOffset = static_cast<std::size_t>(Read<std::uint64_t>(bytes, 48U));
    const std::size_t debugLength = Read<std::uint32_t>(bytes, 56U);
    if (debugLength > StaticModelAsset::MaximumDebugNameLength ||
        (debugLength == 0U ? debugOffset != 0U : debugOffset != expected) ||
        !Region(debugOffset, debugLength, source.GetSize()) || expected + debugLength != source.GetSize())
        return AssetResult::CorruptData;
    std::string debugName;
    try { debugName.assign(reinterpret_cast<const char*>(bytes + debugOffset), debugLength); }
    catch (...) { return AssetResult::OutOfMemory; }
    if (debugName.find('\0') != std::string::npos) return AssetResult::CorruptData;
    StaticModelAsset model;
    result = model.Initialize(std::move(meshPath), std::move(materials), std::move(debugName));
    if (Failed(result)) return AssetResult::CorruptData;
    try { outAsset = std::make_unique<StaticModelLoadedAsset>(std::move(model)); }
    catch (...) { return AssetResult::OutOfMemory; }
    return AssetResult::Success;
}
}
