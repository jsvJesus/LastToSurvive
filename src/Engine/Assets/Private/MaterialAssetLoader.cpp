#include "Assets/MaterialAssetLoader.h"

#include <cstring>
#include <new>

namespace engine::assets
{
namespace
{
constexpr std::size_t HeaderSize = 160U;
template <typename T> T Read(const std::byte *bytes, std::size_t offset) noexcept
{
    T value{};
    std::memcpy(&value, bytes + offset, sizeof(value));
    return value;
}
bool IsZero(const std::byte *bytes, std::size_t first, std::size_t end) noexcept
{
    for (; first < end; ++first)
        if (bytes[first] != std::byte{})
            return false;
    return true;
}
} // namespace

AssetResult MaterialAssetLoader::Load(const AssetMetadata &metadata, const AssetData &source,
                                      std::unique_ptr<LoadedAsset> &outAsset) noexcept
{
    outAsset.reset();
    if (!metadata.IsValid())
        return AssetResult::InvalidMetadata;
    if (metadata.type != AssetType::Material)
        return AssetResult::TypeMismatch;
    if (source.GetSize() < HeaderSize)
        return AssetResult::CorruptData;
    const std::byte *const bytes = source.GetData();
    constexpr char Magic[8] = {'L', 'T', 'S', 'M', 'A', 'T', '\0', '\0'};
    if (std::memcmp(bytes, Magic, 8U) != 0)
        return AssetResult::UnsupportedFormat;
    if (Read<std::uint32_t>(bytes, 8U) != 1U)
        return AssetResult::UnsupportedFormat;
    if (Read<std::uint32_t>(bytes, 12U) != 0x01020304U || Read<std::uint32_t>(bytes, 16U) != HeaderSize ||
        !IsZero(bytes, 132U, HeaderSize))
        return AssetResult::CorruptData;

    const std::uint32_t alpha = Read<std::uint32_t>(bytes, 20U);
    const std::uint32_t flags = Read<std::uint32_t>(bytes, 24U);
    const std::uint32_t filter = Read<std::uint32_t>(bytes, 68U);
    const std::uint32_t addressU = Read<std::uint32_t>(bytes, 72U);
    const std::uint32_t addressV = Read<std::uint32_t>(bytes, 76U);
    const std::uint32_t addressW = Read<std::uint32_t>(bytes, 80U);
    const std::uint32_t comparison = Read<std::uint32_t>(bytes, 92U);
    if (alpha > static_cast<std::uint32_t>(MaterialAlphaMode::Blend) || (flags & ~3U) != 0U ||
        filter > static_cast<std::uint32_t>(engine::graphics::TextureFilter::ComparisonLinear) ||
        addressU > static_cast<std::uint32_t>(engine::graphics::TextureAddressMode::Border) ||
        addressV > static_cast<std::uint32_t>(engine::graphics::TextureAddressMode::Border) ||
        addressW > static_cast<std::uint32_t>(engine::graphics::TextureAddressMode::Border) ||
        comparison > static_cast<std::uint32_t>(engine::graphics::ComparisonFunction::Always))
        return AssetResult::CorruptData;

    MaterialAssetDesc desc;
    for (std::size_t i = 0U; i < 4U; ++i)
        desc.baseColorFactor[i] = Read<float>(bytes, 28U + i * 4U);
    for (std::size_t i = 0U; i < 3U; ++i)
        desc.emissiveFactor[i] = Read<float>(bytes, 44U + i * 4U);
    desc.metallicFactor = Read<float>(bytes, 56U);
    desc.roughnessFactor = Read<float>(bytes, 60U);
    desc.alphaCutoff = Read<float>(bytes, 64U);
    desc.alphaMode = static_cast<MaterialAlphaMode>(alpha);
    desc.doubleSided = (flags & 1U) != 0U;
    desc.sampler.filter = static_cast<engine::graphics::TextureFilter>(filter);
    desc.sampler.addressU = static_cast<engine::graphics::TextureAddressMode>(addressU);
    desc.sampler.addressV = static_cast<engine::graphics::TextureAddressMode>(addressV);
    desc.sampler.addressW = static_cast<engine::graphics::TextureAddressMode>(addressW);
    desc.sampler.mipLodBias = Read<float>(bytes, 84U);
    desc.sampler.maximumAnisotropy = Read<std::uint32_t>(bytes, 88U);
    desc.sampler.comparisonFunction = static_cast<engine::graphics::ComparisonFunction>(comparison);
    for (std::size_t i = 0U; i < 4U; ++i)
        desc.sampler.borderColor[i] = Read<float>(bytes, 96U + i * 4U);
    desc.sampler.minimumLod = Read<float>(bytes, 112U);
    desc.sampler.maximumLod = Read<float>(bytes, 116U);

    const std::uint64_t pathOffset = Read<std::uint64_t>(bytes, 120U);
    const std::uint32_t pathSize = Read<std::uint32_t>(bytes, 128U);
    const bool hasPath = (flags & 2U) != 0U;
    if (hasPath)
    {
        if (pathSize == 0U || pathSize > AssetPath::MaximumLength || pathOffset != HeaderSize ||
            pathSize > source.GetSize() - HeaderSize || pathOffset + pathSize != source.GetSize())
            return AssetResult::CorruptData;
        AssetPath path;
        const AssetResult pathResult =
            AssetPath::TryCreate(std::string_view(reinterpret_cast<const char *>(bytes + HeaderSize), pathSize), path);
        if (Failed(pathResult))
            return pathResult;
        desc.baseColorTexture = std::move(path);
    }
    else if (pathSize != 0U || (pathOffset != 0U && pathOffset != HeaderSize) || source.GetSize() != HeaderSize)
        return AssetResult::CorruptData;

    desc.debugName = metadata.path.String();
    MaterialAsset material;
    if (Failed(material.Initialize(std::move(desc))))
        return AssetResult::CorruptData;
    try
    {
        outAsset = std::make_unique<MaterialLoadedAsset>(std::move(material));
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
