#include "Assets/LtsMeshWriter.h"
#include <cstring>
#include <limits>
namespace engine::assets
{
namespace
{
template <typename T> void Write(std::byte *b, std::size_t o, const T v) noexcept
{
    std::memcpy(b + o, &v, sizeof(v));
}
bool Mul(std::size_t a, std::size_t b, std::size_t &r) noexcept
{
    if (b && a > (std::numeric_limits<std::size_t>::max)() / b)
        return false;
    r = a * b;
    return true;
}
bool Add(std::size_t a, std::size_t b, std::size_t &r) noexcept
{
    if (a > (std::numeric_limits<std::size_t>::max)() - b)
        return false;
    r = a + b;
    return true;
}
bool Align4(std::size_t v, std::size_t &r) noexcept
{
    std::size_t x = 0;
    if (!Add(v, 3U, x))
        return false;
    r = x & ~std::size_t(3U);
    return true;
}
} // namespace
AssetResult LtsMeshWriter::Encode(const MeshAsset &asset, AssetData &out) noexcept
{
    if (!asset.IsValid() || asset.GetBounds().sphereRadius <= 0.0F)
        return AssetResult::InvalidArgument;
    std::size_t vs = 0, is = asset.GetIndexDataSize(), ss = 0, io = 0, indexEnd = 0, so = 0, total = 0;
    if (!Mul(asset.GetVertexCount(), 48U, vs) || !Mul(asset.GetSubmeshCount(), 16U, ss) || !Add(160U, vs, io) ||
        !Add(io, is, indexEnd) || !Align4(indexEnd, so) || !Add(so, ss, total))
        return AssetResult::ReferenceOverflow;
    AssetData candidate;
    const auto rr = candidate.Resize(total);
    if (Failed(rr))
        return rr;
    std::byte *b = candidate.GetData();
    std::memset(b, 0, total);
    std::memcpy(b, "LTSMESH\0", 8U);
    Write<std::uint32_t>(b, 8, 1);
    Write<std::uint32_t>(b, 12, 0x01020304);
    Write<std::uint32_t>(b, 16, 160);
    Write<std::uint32_t>(b, 20, 48);
    Write<std::uint32_t>(b, 24, asset.GetIndexFormat() == engine::graphics::IndexFormat::UInt16 ? 1U : 2U);
    Write<std::uint32_t>(b, 28, static_cast<std::uint32_t>(asset.GetVertexCount()));
    Write<std::uint32_t>(b, 32, static_cast<std::uint32_t>(asset.GetIndexCount()));
    Write<std::uint32_t>(b, 36, static_cast<std::uint32_t>(asset.GetSubmeshCount()));
    Write<std::uint32_t>(b, 40, asset.GetMaterialSlotCount());
    Write<std::uint64_t>(b, 48, 160U);
    Write<std::uint64_t>(b, 56, vs);
    Write<std::uint64_t>(b, 64, io);
    Write<std::uint64_t>(b, 72, is);
    Write<std::uint64_t>(b, 80, so);
    Write<std::uint64_t>(b, 88, ss);
    const auto &bounds = asset.GetBounds();
    for (std::size_t i = 0; i < 3; ++i)
    {
        Write<float>(b, 96 + i * 4, bounds.minimum[i]);
        Write<float>(b, 108 + i * 4, bounds.maximum[i]);
        Write<float>(b, 120 + i * 4, bounds.sphereCenter[i]);
    }
    Write<float>(b, 132, bounds.sphereRadius);
    const StaticMeshVertex *vertices = asset.GetVertexData();
    for (std::size_t n = 0; n < asset.GetVertexCount(); ++n)
    {
        const std::size_t o = 160U + n * 48U;
        for (std::size_t i = 0; i < 3; ++i)
        {
            Write<float>(b, o + i * 4, vertices[n].position[i]);
            Write<float>(b, o + 12 + i * 4, vertices[n].normal[i]);
        }
        for (std::size_t i = 0; i < 4; ++i)
            Write<float>(b, o + 24 + i * 4, vertices[n].tangent[i]);
        for (std::size_t i = 0; i < 2; ++i)
            Write<float>(b, o + 40 + i * 4, vertices[n].texcoord0[i]);
    }
    std::memcpy(b + io, asset.GetIndexData(), is);
    for (std::size_t n = 0; n < asset.GetSubmeshCount(); ++n)
    {
        const auto &s = *asset.GetSubmesh(n);
        const std::size_t o = so + n * 16;
        Write<std::uint32_t>(b, o, s.firstIndex);
        Write<std::uint32_t>(b, o + 4, s.indexCount);
        Write<std::int32_t>(b, o + 8, s.baseVertex);
        Write<std::uint32_t>(b, o + 12, s.materialSlot);
    }
    out.Swap(candidate);
    return AssetResult::Success;
}
} // namespace engine::assets
