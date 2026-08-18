#include "Assets/MaterialAssetWriter.h"

#include <array>
#include <cstring>
#include <limits>

namespace engine::assets
{
    namespace
    {
        constexpr std::array<char, 8U> MaterialMagic
        {
            'M', 'A', 'T', 'B', 'I', 'N', '\0', '\0'
        };

        constexpr std::size_t HeaderSize = 192U;
        constexpr std::size_t EntrySize = 24U;

        template<typename ValueType>
        void Write(
            std::byte* const bytes,
            const std::size_t offset,
            const ValueType value) noexcept
        {
            std::memcpy(
                bytes + offset,
                &value,
                sizeof(value));
        }
    }

    AssetResult MaterialAssetWriter::Encode(
        const MaterialAsset& asset,
        AssetData& output) noexcept
    {
        if (!asset.IsValid())
        {
            return AssetResult::InvalidArgument;
        }

        const MaterialAssetDesc& description =
            asset.GetDesc();

        const std::array<
            const std::optional<AssetPath>*,
            6U> paths
        {
            &description.baseColorTexture,
            &description.normalTexture,
            &description.specularGlossTexture,
            &description.roughnessTexture,
            &description.emissiveTexture,
            &description.specularPowerTexture
        };

        std::size_t pathCount = 0U;
        std::size_t pathBytes = 0U;

        for (const auto* const path : paths)
        {
            if (!path->has_value())
            {
                continue;
            }

            ++pathCount;

            const std::size_t size =
                path->value().View().size();

            if (
                size >
                (std::numeric_limits<std::size_t>::max)() -
                    pathBytes)
            {
                return AssetResult::FileTooLarge;
            }

            pathBytes += size;
        }

        if (
            pathCount >
            (
                (std::numeric_limits<std::size_t>::max)() -
                HeaderSize) /
                EntrySize)
        {
            return AssetResult::FileTooLarge;
        }

        const std::size_t tableOffset =
            HeaderSize;

        const std::size_t pathsOffset =
            tableOffset +
            pathCount * EntrySize;

        if (
            pathBytes >
            (std::numeric_limits<std::size_t>::max)() -
                pathsOffset)
        {
            return AssetResult::FileTooLarge;
        }

        AssetData encoded;

        const AssetResult resizeResult =
            encoded.Resize(
                pathsOffset + pathBytes);

        if (Failed(resizeResult))
        {
            return resizeResult;
        }

        std::byte* const bytes =
            encoded.GetData();

        std::memset(
            bytes,
            0,
            encoded.GetSize());

        std::memcpy(
            bytes,
            MaterialMagic.data(),
            MaterialMagic.size());

        Write<std::uint32_t>(
            bytes,
            8U,
            2U);

        Write<std::uint32_t>(
            bytes,
            12U,
            0x01020304U);

        Write<std::uint32_t>(
            bytes,
            16U,
            static_cast<std::uint32_t>(
                HeaderSize));

        Write<std::uint32_t>(
            bytes,
            20U,
            static_cast<std::uint32_t>(
                description.alphaMode));

        Write<std::uint32_t>(
            bytes,
            24U,
            description.doubleSided
                ? 1U
                : 0U);

        for (std::size_t index = 0U; index < 4U; ++index)
        {
            Write<float>(
                bytes,
                28U + index * sizeof(float),
                description.baseColorFactor[index]);
        }

        for (std::size_t index = 0U; index < 3U; ++index)
        {
            Write<float>(
                bytes,
                44U + index * sizeof(float),
                description.emissiveFactor[index]);
        }

        Write<float>(
            bytes,
            56U,
            description.metallicFactor);

        Write<float>(
            bytes,
            60U,
            description.roughnessFactor);

        Write<float>(
            bytes,
            64U,
            description.alphaCutoff);

        Write<std::uint32_t>(
            bytes,
            68U,
            static_cast<std::uint32_t>(
                description.sampler.filter));

        Write<std::uint32_t>(
            bytes,
            72U,
            static_cast<std::uint32_t>(
                description.sampler.addressU));

        Write<std::uint32_t>(
            bytes,
            76U,
            static_cast<std::uint32_t>(
                description.sampler.addressV));

        Write<std::uint32_t>(
            bytes,
            80U,
            static_cast<std::uint32_t>(
                description.sampler.addressW));

        Write<float>(
            bytes,
            84U,
            description.sampler.mipLodBias);

        Write<std::uint32_t>(
            bytes,
            88U,
            description.sampler.maximumAnisotropy);

        Write<std::uint32_t>(
            bytes,
            92U,
            static_cast<std::uint32_t>(
                description.sampler.comparisonFunction));

        for (std::size_t index = 0U; index < 4U; ++index)
        {
            Write<float>(
                bytes,
                96U + index * sizeof(float),
                description.sampler.borderColor[index]);
        }

        Write<float>(
            bytes,
            112U,
            description.sampler.minimumLod);

        Write<float>(
            bytes,
            116U,
            description.sampler.maximumLod);

        Write<float>(
            bytes,
            120U,
            description.normalScale);

        Write<float>(
            bytes,
            124U,
            description.specularIntensity);

        Write<float>(
            bytes,
            128U,
            description.specularPower);

        Write<float>(
            bytes,
            132U,
            description.reflectionFactor);

        Write<float>(
            bytes,
            136U,
            description.emissiveStrength);

        Write<std::uint32_t>(
            bytes,
            140U,
            static_cast<std::uint32_t>(
                pathCount));

        Write<std::uint64_t>(
            bytes,
            144U,
            pathCount != 0U
                ? static_cast<std::uint64_t>(
                    tableOffset)
                : 0U);

        std::size_t entryIndex = 0U;
        std::size_t pathCursor = pathsOffset;

        for (
            std::size_t semantic = 0U;
            semantic < paths.size();
            ++semantic)
        {
            if (!paths[semantic]->has_value())
            {
                continue;
            }

            const std::string_view value =
                paths[semantic]->value().View();

            const std::size_t entryOffset =
                tableOffset +
                entryIndex * EntrySize;

            Write<std::uint32_t>(
                bytes,
                entryOffset,
                static_cast<std::uint32_t>(
                    semantic));

            Write<std::uint64_t>(
                bytes,
                entryOffset + 8U,
                static_cast<std::uint64_t>(
                    pathCursor));

            Write<std::uint32_t>(
                bytes,
                entryOffset + 16U,
                static_cast<std::uint32_t>(
                    value.size()));

            std::memcpy(
                bytes + pathCursor,
                value.data(),
                value.size());

            pathCursor += value.size();
            ++entryIndex;
        }

        output.Swap(encoded);

        return AssetResult::Success;
    }
}