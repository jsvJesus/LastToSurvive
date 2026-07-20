#include "Assets/TerrainAsset.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <new>
#include <utility>

namespace engine::assets
{
    namespace
    {
        constexpr std::uint32_t Signature = 0x5453544CU;
        constexpr std::uint32_t Version = 1U;

        constexpr std::uint32_t MaximumLayerCount = 18U;
        constexpr std::uint32_t MaximumMaskCount = 6U;

        constexpr std::uint64_t MaximumSampleCount =
            268435456ULL;

        constexpr std::uint64_t MaximumEmbeddedTextureSize =
            512ULL * 1024ULL * 1024ULL;

        template<typename T>
        [[nodiscard]]
        bool Read(
            std::ifstream& stream,
            T& value)
        {
            return static_cast<bool>(
                stream.read(
                    reinterpret_cast<char*>(&value),
                    sizeof(value)));
        }

        [[nodiscard]]
        std::uint32_t ExpectedMaskCount(
            const std::uint32_t layerCount) noexcept
        {
            /*
             * Первый слой — base layer.
             * Он не хранится в Mat-Splat textures.
             */
            if (layerCount <= 1U)
            {
                return 0U;
            }

            const std::uint32_t paintedLayerCount =
                layerCount - 1U;

            return
                (paintedLayerCount + 2U) /
                3U;
        }

        [[nodiscard]]
        bool ReadString(
            std::ifstream& stream,
            std::string& value)
        {
            std::uint32_t length = 0U;

            if (!Read(stream, length) ||
                length > 4096U)
            {
                return false;
            }

            value.resize(length);

            return
                length == 0U ||
                static_cast<bool>(
                    stream.read(
                        value.data(),
                        static_cast<std::streamsize>(
                            length)));
        }

        [[nodiscard]]
        bool ReadTexture(
            std::ifstream& stream,
            TerrainEmbeddedTexture& texture)
        {
            if (!Read(stream, texture.size) ||
                texture.size == 0U ||
                texture.size >
                    MaximumEmbeddedTextureSize)
            {
                return false;
            }

            const std::streampos position =
                stream.tellg();

            if (position < 0)
            {
                return false;
            }

            texture.offset =
                static_cast<std::uint64_t>(
                    position);

            stream.seekg(
                static_cast<std::streamoff>(
                    texture.size),
                std::ios::cur);

            return static_cast<bool>(stream);
        }

        [[nodiscard]]
        bool IsFinitePositive(
            const float value) noexcept
        {
            return
                std::isfinite(value) &&
                value > 0.0F;
        }
    }

    AssetResult TerrainAsset::Load(
        const std::filesystem::path& path,
        TerrainAsset& output) noexcept
    {
        try
        {
            TerrainAsset result;

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream)
            {
                return AssetResult::NotFound;
            }

            std::uint32_t signature = 0U;
            std::uint32_t version = 0U;
            std::uint32_t layerCount = 0U;
            std::uint32_t maskCount = 0U;

            std::uint64_t heightBytes = 0U;

            if (!Read(stream, signature) ||
                !Read(stream, version) ||
                signature != Signature ||
                version != Version ||
                !Read(stream, result.width) ||
                !Read(stream, result.height) ||
                !Read(stream, result.splatWidth) ||
                !Read(stream, result.splatHeight) ||
                !Read(stream, result.tileSize) ||
                !Read(stream, result.heightOffset) ||
                !Read(stream, result.heightScale) ||
                !Read(stream, layerCount) ||
                !Read(stream, maskCount) ||
                !Read(stream, heightBytes))
            {
                return AssetResult::CorruptData;
            }

            const std::uint64_t sampleCount =
                static_cast<std::uint64_t>(
                    result.width) *
                static_cast<std::uint64_t>(
                    result.height);

            const std::uint32_t expectedMaskCount =
                ExpectedMaskCount(layerCount);

            if (result.width < 2U ||
                result.height < 2U ||
                result.splatWidth == 0U ||
                result.splatHeight == 0U ||
                sampleCount == 0U ||
                sampleCount > MaximumSampleCount ||
                heightBytes !=
                    sampleCount *
                    sizeof(std::int16_t) ||
                !IsFinitePositive(
                    result.tileSize) ||
                !std::isfinite(
                    result.heightOffset) ||
                !IsFinitePositive(
                    result.heightScale) ||
                layerCount == 0U ||
                layerCount >
                    MaximumLayerCount ||
                maskCount !=
                    expectedMaskCount ||
                maskCount >
                    MaximumMaskCount)
            {
                return AssetResult::CorruptData;
            }

            result.heights.resize(
                static_cast<std::size_t>(
                    sampleCount));

            if (!stream.read(
                    reinterpret_cast<char*>(
                        result.heights.data()),
                    static_cast<std::streamsize>(
                        heightBytes)))
            {
                return AssetResult::CorruptData;
            }

            result.layers.resize(layerCount);

            for (TerrainLayer& layer :
                 result.layers)
            {
                if (!ReadString(
                        stream,
                        layer.name) ||
                    !ReadString(
                        stream,
                        layer.diffusePath) ||
                    !ReadString(
                        stream,
                        layer.normalPath) ||
                    !ReadString(
                        stream,
                        layer.materialType) ||
                    !Read(
                        stream,
                        layer.scaleU) ||
                    !Read(
                        stream,
                        layer.scaleV) ||
                    !Read(
                        stream,
                        layer.specular) ||
                    !IsFinitePositive(
                        layer.scaleU) ||
                    !IsFinitePositive(
                        layer.scaleV) ||
                    !std::isfinite(
                        layer.specular))
                {
                    return AssetResult::CorruptData;
                }
            }

            result.masks.resize(maskCount);

            for (TerrainEmbeddedTexture& mask :
                 result.masks)
            {
                if (!ReadTexture(
                        stream,
                        mask))
                {
                    return AssetResult::CorruptData;
                }
            }

            if (!ReadTexture(
                    stream,
                    result.colorMap) ||
                !ReadTexture(
                    stream,
                    result.normalMap))
            {
                return AssetResult::CorruptData;
            }

            result.sourcePath = path;
            output = std::move(result);

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            return AssetResult::IoError;
        }
    }

    float TerrainAsset::GetHeight(
        const std::uint32_t x,
        const std::uint32_t z) const noexcept
    {
        if (x >= width ||
            z >= height ||
            heights.empty())
        {
            return 0.0F;
        }

        /*
         * Оригинальный WarZ Terrain2 сохраняет samples
         * в row-major порядке:
         *
         *     index = z * width + x
         */
        const std::size_t index =
            static_cast<std::size_t>(z) *
                static_cast<std::size_t>(width) +
            static_cast<std::size_t>(x);

        /*
         * Совпадает с r3dTerrain2::SetupHFScale().
         */
        const float amplitude =
            (std::max)(
                std::fabs(heightOffset),
                std::fabs(
                    heightOffset +
                    heightScale));

        if (amplitude <= 0.0F)
        {
            return 0.0F;
        }

        return
            static_cast<float>(
                heights[index]) *
            (amplitude / 32767.0F);
    }

    bool TerrainAsset::IsValid() const noexcept
    {
        const std::uint64_t sampleCount =
            static_cast<std::uint64_t>(
                width) *
            static_cast<std::uint64_t>(
                height);

        return
            width > 1U &&
            height > 1U &&
            splatWidth > 0U &&
            splatHeight > 0U &&
            sampleCount <=
                MaximumSampleCount &&
            heights.size() ==
                static_cast<std::size_t>(
                    sampleCount) &&
            !layers.empty() &&
            layers.size() <=
                MaximumLayerCount &&
            masks.size() ==
                ExpectedMaskCount(
                    static_cast<std::uint32_t>(
                        layers.size())) &&
            IsFinitePositive(tileSize) &&
            std::isfinite(heightOffset) &&
            IsFinitePositive(heightScale) &&
            colorMap.size > 0U &&
            normalMap.size > 0U;
    }
}