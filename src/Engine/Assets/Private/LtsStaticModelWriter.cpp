#include "Assets/LtsStaticModelWriter.h"

#include <cstring>
#include <limits>

namespace engine::assets
{
namespace
{
constexpr std::size_t HeaderSize = 64U;
constexpr std::size_t EntrySize = 16U;
template<typename T> void Write(std::byte* bytes, const std::size_t offset, const T value) noexcept
{
    std::memcpy(bytes + offset, &value, sizeof(value));
}
bool Add(std::size_t& value, const std::size_t amount) noexcept
{
    if (amount > std::numeric_limits<std::size_t>::max() - value)
        return false;
    value += amount;
    return true;
}
}

AssetResult LtsStaticModelWriter::Encode(const StaticModelAsset& asset, AssetData& outData) noexcept
{
    if (!asset.IsValid())
        return AssetResult::InvalidArgument;
    const std::size_t count = asset.GetMaterialCount();
    std::size_t size = HeaderSize;
    if (count > (std::numeric_limits<std::size_t>::max() - size) / EntrySize)
        return AssetResult::FileTooLarge;
    size += count * EntrySize;
    if (!Add(size, asset.GetMeshPath().View().size()))
        return AssetResult::FileTooLarge;
    for (std::size_t index = 0U; index < count; ++index)
        if (!Add(size, asset.GetMaterialPath(index).View().size()))
            return AssetResult::FileTooLarge;
    if (!Add(size, asset.GetDebugName().size()))
        return AssetResult::FileTooLarge;

    AssetData candidate;
    const AssetResult resizeResult = candidate.Resize(size);
    if (Failed(resizeResult))
        return resizeResult;
    std::byte* const bytes = candidate.GetData();
    std::memset(bytes, 0, size);
    std::memcpy(bytes, "LTSMODEL", 8U);
    Write<std::uint32_t>(bytes, 8U, 1U);
    Write<std::uint32_t>(bytes, 12U, 0x01020304U);
    Write<std::uint32_t>(bytes, 16U, static_cast<std::uint32_t>(HeaderSize));
    Write<std::uint32_t>(bytes, 20U, static_cast<std::uint32_t>(count));
    Write<std::uint64_t>(bytes, 40U, static_cast<std::uint64_t>(HeaderSize));

    std::size_t cursor = HeaderSize + count * EntrySize;
    Write<std::uint64_t>(bytes, 24U, static_cast<std::uint64_t>(cursor));
    Write<std::uint32_t>(bytes, 32U, static_cast<std::uint32_t>(asset.GetMeshPath().View().size()));
    std::memcpy(bytes + cursor, asset.GetMeshPath().View().data(), asset.GetMeshPath().View().size());
    cursor += asset.GetMeshPath().View().size();
    for (std::size_t index = 0U; index < count; ++index)
    {
        const auto path = asset.GetMaterialPath(index).View();
        Write<std::uint64_t>(bytes, HeaderSize + index * EntrySize, static_cast<std::uint64_t>(cursor));
        Write<std::uint32_t>(bytes, HeaderSize + index * EntrySize + 8U, static_cast<std::uint32_t>(path.size()));
        std::memcpy(bytes + cursor, path.data(), path.size());
        cursor += path.size();
    }
    if (!asset.GetDebugName().empty())
    {
        Write<std::uint64_t>(bytes, 48U, static_cast<std::uint64_t>(cursor));
        Write<std::uint32_t>(bytes, 56U, static_cast<std::uint32_t>(asset.GetDebugName().size()));
        std::memcpy(bytes + cursor, asset.GetDebugName().data(), asset.GetDebugName().size());
    }
    outData.Swap(candidate);
    return AssetResult::Success;
}
}
