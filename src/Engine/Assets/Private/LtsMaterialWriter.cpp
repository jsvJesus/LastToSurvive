#include "Assets/LtsMaterialWriter.h"

#include <array>
#include <cstring>
#include <limits>

namespace engine::assets
{
    namespace
    {
        constexpr std::size_t HeaderSize = 192U;
        constexpr std::size_t EntrySize = 24U;
        template <typename T> void Write(std::byte* bytes, const std::size_t offset, const T value) noexcept
        {
            std::memcpy(bytes + offset, &value, sizeof(value));
        }
    }

    AssetResult LtsMaterialWriter::Encode(const MaterialAsset& asset, AssetData& out) noexcept
    {
        if (!asset.IsValid()) return AssetResult::InvalidArgument;
        const auto& d = asset.GetDesc();
        const std::array<const std::optional<AssetPath>*, 6U> paths = {&d.baseColorTexture, &d.normalTexture,
            &d.specularGlossTexture, &d.roughnessTexture, &d.emissiveTexture, &d.specularPowerTexture};

        std::size_t count = 0U, pathBytes = 0U;
        for (const auto* path : paths) if (*path)
        {
            ++count;
            if ((*path)->View().size() > (std::numeric_limits<std::size_t>::max)() - pathBytes) return AssetResult::FileTooLarge;
            pathBytes += (*path)->View().size();
        }
        
        if (count > ((std::numeric_limits<std::size_t>::max)() - HeaderSize) / EntrySize) return AssetResult::FileTooLarge;
        const std::size_t tableOffset = HeaderSize;
        const std::size_t pathsOffset = tableOffset + count * EntrySize;

        if (pathBytes > (std::numeric_limits<std::size_t>::max)() - pathsOffset) return AssetResult::FileTooLarge;
        AssetData candidate;
        const AssetResult resize = candidate.Resize(pathsOffset + pathBytes);

        if (Failed(resize)) return resize;
        std::byte* const b = candidate.GetData();
        std::memset(b, 0, candidate.GetSize());
        std::memcpy(b, "LTSMAT\0\0", 8U);

        Write<std::uint32_t>(b, 8U, 2U); Write<std::uint32_t>(b, 12U, 0x01020304U);
        Write<std::uint32_t>(b, 16U, static_cast<std::uint32_t>(HeaderSize));
        Write<std::uint32_t>(b, 20U, static_cast<std::uint32_t>(d.alphaMode));
        Write<std::uint32_t>(b, 24U, d.doubleSided ? 1U : 0U);

        for (std::size_t i=0;i<4;++i) Write<float>(b,28U+i*4U,d.baseColorFactor[i]);
        for (std::size_t i=0;i<3;++i) Write<float>(b,44U+i*4U,d.emissiveFactor[i]);

        Write<float>(b,56U,d.metallicFactor); Write<float>(b,60U,d.roughnessFactor); Write<float>(b,64U,d.alphaCutoff);
        Write<std::uint32_t>(b,68U,static_cast<std::uint32_t>(d.sampler.filter));
        Write<std::uint32_t>(b,72U,static_cast<std::uint32_t>(d.sampler.addressU));
        Write<std::uint32_t>(b,76U,static_cast<std::uint32_t>(d.sampler.addressV));
        Write<std::uint32_t>(b,80U,static_cast<std::uint32_t>(d.sampler.addressW));
        Write<float>(b,84U,d.sampler.mipLodBias); Write<std::uint32_t>(b,88U,d.sampler.maximumAnisotropy);
        Write<std::uint32_t>(b,92U,static_cast<std::uint32_t>(d.sampler.comparisonFunction));

        for (std::size_t i=0;i<4;++i) Write<float>(b,96U+i*4U,d.sampler.borderColor[i]);

        Write<float>(b,112U,d.sampler.minimumLod); Write<float>(b,116U,d.sampler.maximumLod);
        Write<float>(b,120U,d.normalScale); Write<float>(b,124U,d.specularIntensity);
        Write<float>(b,128U,d.specularPower); Write<float>(b,132U,d.reflectionFactor); Write<float>(b,136U,d.emissiveStrength);
        Write<std::uint32_t>(b,140U,static_cast<std::uint32_t>(count));
        Write<std::uint64_t>(b,144U,count ? static_cast<std::uint64_t>(tableOffset) : 0U);

        std::size_t entry = 0U, cursor = pathsOffset;
        for (std::size_t semantic=0U; semantic<paths.size(); ++semantic) if (*paths[semantic])
        {
            const auto view = (*paths[semantic])->View();
            const std::size_t offset = tableOffset + entry * EntrySize;
            Write<std::uint32_t>(b,offset,static_cast<std::uint32_t>(semantic));
            Write<std::uint64_t>(b,offset+8U,static_cast<std::uint64_t>(cursor));
            Write<std::uint32_t>(b,offset+16U,static_cast<std::uint32_t>(view.size()));
            std::memcpy(b+cursor,view.data(),view.size()); cursor += view.size(); ++entry;
        }
        out.Swap(candidate);
        return AssetResult::Success;
    }
}
