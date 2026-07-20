#include "Assets/LegacyTerrain2Importer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <system_error>
#include <vector>

namespace engine::assets
{
    namespace
    {
        /*
         * Оригинальный WarZ:
         *
         * #define TERRAIN2_SIGNATURE '2RET'
         *
         * Для little-endian файла это 0x32524554.
         */
        constexpr std::uint32_t LegacySignature =
            0x32524554U;

        constexpr std::uint32_t LegacyVersion101 =
            101U;

        constexpr std::uint32_t LegacyVersion103 =
            103U;

        constexpr std::uint32_t TerrainSignature =
            0x5453544CU;

        constexpr std::uint32_t TerrainVersion =
            1U;

        constexpr std::uint32_t MaximumLayerCount =
            18U;

        constexpr std::uint32_t MaximumMaskCount =
            6U;

        constexpr std::uint64_t MaximumSampleCount =
            268435456ULL;

        constexpr std::uint64_t
            MaximumEmbeddedTextureSize =
                512ULL * 1024ULL * 1024ULL;

        struct Layer final
        {
            std::string name;
            std::string diffuse;
            std::string normal;
            std::string materialType;

            float scaleU = 1.0F;
            float scaleV = 1.0F;
            float specular = 0.0F;
        };

        struct Description final
        {
            std::uint32_t width = 0U;
            std::uint32_t height = 0U;

            std::uint32_t splatWidth = 0U;
            std::uint32_t splatHeight = 0U;

            float tileSize = 0.0F;
            float heightOffset = 0.0F;
            float heightScale = 0.0F;

            /*
             * layers[0] — base layer.
             * Остальные — painted layers.
             */
            std::vector<Layer> layers;
        };

        [[nodiscard]]
        std::string Trim(
            std::string value)
        {
            const auto isWhitespace =
                [](const unsigned char character)
                {
                    return
                        std::isspace(character) != 0;
                };

            value.erase(
                value.begin(),
                std::find_if_not(
                    value.begin(),
                    value.end(),
                    isWhitespace));

            value.erase(
                std::find_if_not(
                    value.rbegin(),
                    value.rend(),
                    isWhitespace).base(),
                value.end());

            if (value.size() >= 2U &&
                value.front() == '"' &&
                value.back() == '"')
            {
                value =
                    value.substr(
                        1U,
                        value.size() - 2U);
            }

            return value;
        }

        [[nodiscard]]
        bool ReadProperty(
            const std::string& line,
            std::string& key,
            std::string& value)
        {
            const std::size_t separator =
                line.find(':');

            if (separator ==
                std::string::npos)
            {
                return false;
            }

            key =
                Trim(
                    line.substr(
                        0U,
                        separator));

            value =
                Trim(
                    line.substr(
                        separator + 1U));

            return !key.empty();
        }

        [[nodiscard]]
        bool ParseUnsigned(
            const std::string& text,
            std::uint32_t& output)
        {
            std::size_t parsedCharacters = 0U;

            const unsigned long value =
                std::stoul(
                    text,
                    &parsedCharacters,
                    10);

            if (parsedCharacters !=
                    text.size() ||
                value >
                    static_cast<unsigned long>(
                        (std::numeric_limits<
                            std::uint32_t>::max)()))
            {
                return false;
            }

            output =
                static_cast<std::uint32_t>(
                    value);

            return true;
        }

        [[nodiscard]]
        bool ParseFloat(
            const std::string& text,
            float& output)
        {
            std::size_t parsedCharacters = 0U;

            const float value =
                std::stof(
                    text,
                    &parsedCharacters);

            if (parsedCharacters !=
                    text.size() ||
                !std::isfinite(value))
            {
                return false;
            }

            output = value;
            return true;
        }

        [[nodiscard]]
        bool ParseDescription(
            const std::filesystem::path& file,
            Description& output)
        {
            std::ifstream stream(file);

            if (!stream)
            {
                return false;
            }

            Description result;

            Layer* currentLayer = nullptr;
            bool baseLayerFound = false;

            std::string line;

            while (std::getline(
                stream,
                line))
            {
                line = Trim(std::move(line));

                if (line.empty() ||
                    line.rfind("//", 0U) == 0U ||
                    line.rfind("#", 0U) == 0U)
                {
                    continue;
                }

                if (line == "base_layer")
                {
                    if (baseLayerFound ||
                        !result.layers.empty())
                    {
                        return false;
                    }

                    result.layers.emplace_back();

                    result.layers.back().name =
                        "Base Layer";

                    currentLayer =
                        &result.layers.back();

                    baseLayerFound = true;
                    continue;
                }

                if (line == "layer")
                {
                    if (!baseLayerFound ||
                        result.layers.size() >=
                            MaximumLayerCount)
                    {
                        return false;
                    }

                    result.layers.emplace_back();

                    result.layers.back().name =
                        "Layer " +
                        std::to_string(
                            result.layers.size() -
                            1U);

                    currentLayer =
                        &result.layers.back();

                    continue;
                }

                if (line == "{")
                {
                    continue;
                }

                if (line == "}")
                {
                    currentLayer = nullptr;
                    continue;
                }

                std::string key;
                std::string value;

                if (!ReadProperty(
                        line,
                        key,
                        value))
                {
                    continue;
                }

                try
                {
                    if (currentLayer != nullptr)
                    {
                        if (key == "name")
                        {
                            currentLayer->name =
                                value;
                        }
                        else if (
                            key == "map_diffuse")
                        {
                            currentLayer->diffuse =
                                value;
                        }
                        else if (
                            key == "map_normal")
                        {
                            currentLayer->normal =
                                value;
                        }
                        else if (
                            key == "mat_type")
                        {
                            currentLayer->
                                materialType =
                                    value;
                        }
                        else if (
                            key == "scale_u")
                        {
                            if (!ParseFloat(
                                    value,
                                    currentLayer->
                                        scaleU))
                            {
                                return false;
                            }
                        }
                        else if (
                            key == "scale_v")
                        {
                            if (!ParseFloat(
                                    value,
                                    currentLayer->
                                        scaleV))
                            {
                                return false;
                            }
                        }
                        else if (
                            key == "specular")
                        {
                            if (!ParseFloat(
                                    value,
                                    currentLayer->
                                        specular))
                            {
                                return false;
                            }
                        }

                        continue;
                    }

                    if (key == "vert_count_x")
                    {
                        if (!ParseUnsigned(
                                value,
                                result.width))
                        {
                            return false;
                        }
                    }
                    else if (
                        key == "vert_count_z")
                    {
                        if (!ParseUnsigned(
                                value,
                                result.height))
                        {
                            return false;
                        }
                    }
                    else if (
                        key == "splat_res_u")
                    {
                        if (!ParseUnsigned(
                                value,
                                result.splatWidth))
                        {
                            return false;
                        }
                    }
                    else if (
                        key == "splat_res_v")
                    {
                        if (!ParseUnsigned(
                                value,
                                result.splatHeight))
                        {
                            return false;
                        }
                    }
                    else if (
                        key == "tile_unit_size")
                    {
                        if (!ParseFloat(
                                value,
                                result.tileSize))
                        {
                            return false;
                        }
                    }
                    else if (
                        key == "height_offset")
                    {
                        if (!ParseFloat(
                                value,
                                result.heightOffset))
                        {
                            return false;
                        }
                    }
                    else if (
                        key == "height_scale")
                    {
                        if (!ParseFloat(
                                value,
                                result.heightScale))
                        {
                            return false;
                        }
                    }
                }
                catch (...)
                {
                    return false;
                }
            }

            /*
             * Оригинальный Terrain2 при нулевом
             * splat resolution использует размер
             * heightfield.
             */
            if (result.splatWidth == 0U)
            {
                result.splatWidth =
                    result.width;
            }

            if (result.splatHeight == 0U)
            {
                result.splatHeight =
                    result.height;
            }

            const std::uint64_t sampleCount =
                static_cast<std::uint64_t>(
                    result.width) *
                static_cast<std::uint64_t>(
                    result.height);

            if (!baseLayerFound ||
                result.width < 2U ||
                result.height < 2U ||
                result.splatWidth == 0U ||
                result.splatHeight == 0U ||
                sampleCount == 0U ||
                sampleCount >
                    MaximumSampleCount ||
                !std::isfinite(
                    result.tileSize) ||
                result.tileSize <= 0.0F ||
                !std::isfinite(
                    result.heightOffset) ||
                !std::isfinite(
                    result.heightScale) ||
                result.heightScale <= 0.0F ||
                result.layers.empty() ||
                result.layers.size() >
                    MaximumLayerCount)
            {
                return false;
            }

            for (Layer& layer :
                 result.layers)
            {
                if (!std::isfinite(
                        layer.scaleU) ||
                    !std::isfinite(
                        layer.scaleV) ||
                    !std::isfinite(
                        layer.specular))
                {
                    return false;
                }

                /*
                 * Старые карты иногда содержат
                 * нулевой ScaleU/ScaleV.
                 */
                if (layer.scaleU <= 0.0F)
                {
                    layer.scaleU = 1.0F;
                }

                if (layer.scaleV <= 0.0F)
                {
                    layer.scaleV = 1.0F;
                }
            }

            output = std::move(result);
            return true;
        }

        template<typename T>
        [[nodiscard]]
        bool Read(
            std::ifstream& stream,
            T& value)
        {
            return static_cast<bool>(
                stream.read(
                    reinterpret_cast<char*>(
                        &value),
                    sizeof(value)));
        }

        template<typename T>
        [[nodiscard]]
        bool Write(
            std::ofstream& stream,
            const T& value)
        {
            return static_cast<bool>(
                stream.write(
                    reinterpret_cast<
                        const char*>(&value),
                    sizeof(value)));
        }

        [[nodiscard]]
        bool WriteString(
            std::ofstream& stream,
            const std::string& value)
        {
            if (value.size() > 4096U)
            {
                return false;
            }

            const std::uint32_t length =
                static_cast<std::uint32_t>(
                    value.size());

            return
                Write(stream, length) &&
                (
                    length == 0U ||
                    static_cast<bool>(
                        stream.write(
                            value.data(),
                            static_cast<
                                std::streamsize>(
                                    length)))
                );
        }

        [[nodiscard]]
        bool WriteBlob(
            std::ofstream& output,
            const std::filesystem::path& file)
        {
            std::ifstream input(
                file,
                std::ios::binary |
                    std::ios::ate);

            if (!input)
            {
                return false;
            }

            const std::streampos end =
                input.tellg();

            if (end <= 0)
            {
                return false;
            }

            const std::uint64_t size =
                static_cast<std::uint64_t>(
                    end);

            if (size >
                    MaximumEmbeddedTextureSize ||
                !Write(output, size))
            {
                return false;
            }

            input.seekg(
                0,
                std::ios::beg);

            std::vector<char> buffer(
                1024U * 1024U);

            std::uint64_t bytesRemaining =
                size;

            while (bytesRemaining > 0U)
            {
                const std::streamsize amount =
                    static_cast<
                        std::streamsize>(
                            (std::min)(
                                bytesRemaining,
                                static_cast<
                                    std::uint64_t>(
                                        buffer.size())));

                if (!input.read(
                        buffer.data(),
                        amount) ||
                    !output.write(
                        buffer.data(),
                        amount))
                {
                    return false;
                }

                bytesRemaining -=
                    static_cast<std::uint64_t>(
                        amount);
            }

            return static_cast<bool>(output);
        }

        void RemoveQuietly(
            const std::filesystem::path& path)
                noexcept
        {
            std::error_code error;

            static_cast<void>(
                std::filesystem::remove(
                    path,
                    error));
        }

        [[nodiscard]]
        AssetResult CommitTemporaryFile(
            const std::filesystem::path&
                temporary,
            const std::filesystem::path&
                destination) noexcept
        {
            std::error_code error;

            std::filesystem::path backup =
                destination;

            backup += ".bak";

            static_cast<void>(
                std::filesystem::remove(
                    backup,
                    error));

            error.clear();

            const bool destinationExists =
                std::filesystem::exists(
                    destination,
                    error);

            if (error)
            {
                return AssetResult::IoError;
            }

            if (destinationExists)
            {
                std::filesystem::rename(
                    destination,
                    backup,
                    error);

                if (error)
                {
                    return AssetResult::IoError;
                }
            }

            std::filesystem::rename(
                temporary,
                destination,
                error);

            if (!error)
            {
                RemoveQuietly(backup);
                return AssetResult::Success;
            }

            /*
             * Возвращаем старый asset,
             * если замена нового файла провалилась.
             */
            if (destinationExists)
            {
                std::error_code restoreError;

                std::filesystem::rename(
                    backup,
                    destination,
                    restoreError);
            }

            RemoveQuietly(temporary);
            return AssetResult::IoError;
        }
    }

    AssetResult LegacyTerrain2Importer::Import(
        const std::filesystem::path& source,
        const std::filesystem::path&
            destination) noexcept
    {
        try
        {
            if (source.empty() ||
                destination.empty())
            {
                return AssetResult::InvalidArgument;
            }

            const std::filesystem::path
                descriptionPath =
                    source /
                    "terrain2.ini";

            const std::filesystem::path
                binaryPath =
                    source /
                    "terrain2.bin";

            std::error_code filesystemError;

            if (!std::filesystem::is_regular_file(
                    descriptionPath,
                    filesystemError) ||
                filesystemError)
            {
                return AssetResult::NotFound;
            }

            Description description;

            if (!ParseDescription(
                    descriptionPath,
                    description))
            {
                return AssetResult::CorruptData;
            }

            const std::uint32_t layerCount =
                static_cast<std::uint32_t>(
                    description.layers.size());

            /*
             * Base layer не занимает канал
             * в Mat-Splat.
             */
            const std::uint32_t
                paintedLayerCount =
                    layerCount - 1U;

            const std::uint32_t maskCount =
                (paintedLayerCount + 2U) /
                3U;

            if (layerCount >
                    MaximumLayerCount ||
                maskCount >
                    MaximumMaskCount)
            {
                return
                    AssetResult::
                        UnsupportedFeature;
            }

            std::ifstream binary(
                binaryPath,
                std::ios::binary);

            if (!binary)
            {
                return AssetResult::NotFound;
            }

            std::uint32_t signature = 0U;
            std::uint32_t version = 0U;
            std::uint32_t sampleCount = 0U;

            if (!Read(
                    binary,
                    signature) ||
                !Read(
                    binary,
                    version) ||
                !Read(
                    binary,
                    sampleCount))
            {
                return AssetResult::CorruptData;
            }

            const std::uint64_t
                expectedSampleCount =
                    static_cast<
                        std::uint64_t>(
                            description.width) *
                    static_cast<
                        std::uint64_t>(
                            description.height);

            if (signature !=
                    LegacySignature ||
                (
                    version !=
                        LegacyVersion101 &&
                    version !=
                        LegacyVersion103
                ) ||
                sampleCount !=
                    expectedSampleCount)
            {
                return AssetResult::CorruptData;
            }

            /*
             * Не транспонируем данные здесь.
             * TerrainAsset::GetHeight использует
             * оригинальный индекс z * width + x.
             */
            std::vector<std::int16_t> heights(
                sampleCount);

            const std::uint64_t heightBytes =
                static_cast<std::uint64_t>(
                    heights.size()) *
                sizeof(heights[0]);

            if (!binary.read(
                    reinterpret_cast<char*>(
                        heights.data()),
                    static_cast<
                        std::streamsize>(
                            heightBytes)))
            {
                return AssetResult::CorruptData;
            }

            std::filesystem::
                create_directories(
                    destination.parent_path(),
                    filesystemError);

            if (filesystemError)
            {
                return AssetResult::IoError;
            }

            std::filesystem::path temporary =
                destination;

            temporary += ".tmp";

            RemoveQuietly(temporary);

            std::ofstream output(
                temporary,
                std::ios::binary |
                    std::ios::trunc);

            if (!output)
            {
                return AssetResult::IoError;
            }

            const bool headerWritten =
                Write(
                    output,
                    TerrainSignature) &&
                Write(
                    output,
                    TerrainVersion) &&
                Write(
                    output,
                    description.width) &&
                Write(
                    output,
                    description.height) &&
                Write(
                    output,
                    description.splatWidth) &&
                Write(
                    output,
                    description.splatHeight) &&
                Write(
                    output,
                    description.tileSize) &&
                Write(
                    output,
                    description.heightOffset) &&
                Write(
                    output,
                    description.heightScale) &&
                Write(
                    output,
                    layerCount) &&
                Write(
                    output,
                    maskCount) &&
                Write(
                    output,
                    heightBytes) &&
                static_cast<bool>(
                    output.write(
                        reinterpret_cast<
                            const char*>(
                                heights.data()),
                        static_cast<
                            std::streamsize>(
                                heightBytes)));

            if (!headerWritten)
            {
                output.close();
                RemoveQuietly(temporary);

                return AssetResult::IoError;
            }

            for (const Layer& layer :
                 description.layers)
            {
                if (!WriteString(
                        output,
                        layer.name) ||
                    !WriteString(
                        output,
                        layer.diffuse) ||
                    !WriteString(
                        output,
                        layer.normal) ||
                    !WriteString(
                        output,
                        layer.materialType) ||
                    !Write(
                        output,
                        layer.scaleU) ||
                    !Write(
                        output,
                        layer.scaleV) ||
                    !Write(
                        output,
                        layer.specular))
                {
                    output.close();
                    RemoveQuietly(temporary);

                    return AssetResult::IoError;
                }
            }

            for (std::uint32_t maskIndex = 0U;
                 maskIndex < maskCount;
                 ++maskIndex)
            {
                const std::filesystem::path
                    maskPath =
                        source /
                        (
                            "Mat-Splat" +
                            std::to_string(
                                maskIndex) +
                            ".dds"
                        );

                if (!WriteBlob(
                        output,
                        maskPath))
                {
                    output.close();
                    RemoveQuietly(temporary);

                    return AssetResult::NotFound;
                }
            }

            if (!WriteBlob(
                    output,
                    source / "Color.dds") ||
                !WriteBlob(
                    output,
                    source / "Normal.dds"))
            {
                output.close();
                RemoveQuietly(temporary);

                return AssetResult::NotFound;
            }

            output.close();

            if (!output)
            {
                RemoveQuietly(temporary);
                return AssetResult::IoError;
            }

            return CommitTemporaryFile(
                temporary,
                destination);
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
}