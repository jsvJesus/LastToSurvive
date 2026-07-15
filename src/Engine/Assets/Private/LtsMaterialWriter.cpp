#include "Assets/LtsMaterialWriter.h"
#include <cstring>
namespace engine::assets
{
namespace
{
template <typename T> void Write(std::byte *b, std::size_t o, T v) noexcept
{
    std::memcpy(b + o, &v, sizeof(v));
}
} // namespace
AssetResult LtsMaterialWriter::Encode(const MaterialAsset &asset, AssetData &out) noexcept
{
    if (!asset.IsValid())
        return AssetResult::InvalidArgument;
    const auto &d = asset.GetDesc();
    const std::size_t ps = d.baseColorTexture ? d.baseColorTexture->View().size() : 0U;
    AssetData candidate;
    const auto r = candidate.Resize(160U + ps);
    if (Failed(r))
        return r;
    auto *b = candidate.GetData();
    std::memset(b, 0, 160U + ps);
    std::memcpy(b, "LTSMAT\0\0", 8U);
    Write<std::uint32_t>(b, 8, 1);
    Write<std::uint32_t>(b, 12, 0x01020304);
    Write<std::uint32_t>(b, 16, 160);
    Write<std::uint32_t>(b, 20, static_cast<std::uint32_t>(d.alphaMode));
    Write<std::uint32_t>(b, 24, (d.doubleSided ? 1U : 0U) | (ps ? 2U : 0U));
    for (std::size_t i = 0; i < 4; ++i)
        Write<float>(b, 28 + i * 4, d.baseColorFactor[i]);
    for (std::size_t i = 0; i < 3; ++i)
        Write<float>(b, 44 + i * 4, d.emissiveFactor[i]);
    Write<float>(b, 56, d.metallicFactor);
    Write<float>(b, 60, d.roughnessFactor);
    Write<float>(b, 64, d.alphaCutoff);
    Write<std::uint32_t>(b, 68, static_cast<std::uint32_t>(d.sampler.filter));
    Write<std::uint32_t>(b, 72, static_cast<std::uint32_t>(d.sampler.addressU));
    Write<std::uint32_t>(b, 76, static_cast<std::uint32_t>(d.sampler.addressV));
    Write<std::uint32_t>(b, 80, static_cast<std::uint32_t>(d.sampler.addressW));
    Write<float>(b, 84, d.sampler.mipLodBias);
    Write<std::uint32_t>(b, 88, d.sampler.maximumAnisotropy);
    Write<std::uint32_t>(b, 92, static_cast<std::uint32_t>(d.sampler.comparisonFunction));
    for (std::size_t i = 0; i < 4; ++i)
        Write<float>(b, 96 + i * 4, d.sampler.borderColor[i]);
    Write<float>(b, 112, d.sampler.minimumLod);
    Write<float>(b, 116, d.sampler.maximumLod);
    Write<std::uint64_t>(b, 120, ps ? 160U : 0U);
    Write<std::uint32_t>(b, 128, static_cast<std::uint32_t>(ps));
    if (ps)
        std::memcpy(b + 160, d.baseColorTexture->View().data(), ps);
    out.Swap(candidate);
    return AssetResult::Success;
}
} // namespace engine::assets
