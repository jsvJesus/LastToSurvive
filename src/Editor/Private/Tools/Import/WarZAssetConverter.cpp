#include "Editor/Tools/Import/WarZAssetConverter.h"

#include <Assets/AssetData.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/LtsMaterialWriter.h>
#include <Assets/MaterialAsset.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint32_t AssetEndianMarker =
            0x01020304U;

        class BinaryWriter final
        {
        public:
            template<typename T>
            bool Write(const T& value)
            {
                static_assert(
                    std::is_trivially_copyable_v<T>);

                return WriteBytes(
                    &value,
                    sizeof(value));
            }

            bool WriteBytes(
                const void* const data,
                const std::size_t size)
            {
                if (
                    !valid_ ||
                    data == nullptr ||
                    size >
                        (std::numeric_limits<
                            std::size_t>::max)() -
                        bytes_.size())
                {
                    valid_ = false;
                    return false;
                }

                try
                {
                    const std::size_t offset =
                        bytes_.size();

                    bytes_.resize(offset + size);

                    std::memcpy(
                        bytes_.data() + offset,
                        data,
                        size);

                    return true;
                }
                catch (...)
                {
                    valid_ = false;
                    return false;
                }
            }

            bool WriteString(
                const std::string_view value)
            {
                if (
                    value.size() >
                    (std::numeric_limits<
                        std::uint32_t>::max)())
                {
                    valid_ = false;
                    return false;
                }

                const std::uint32_t length =
                    static_cast<std::uint32_t>(
                        value.size());

                if (!Write(length))
                {
                    return false;
                }

                if (value.empty())
                {
                    return true;
                }

                return WriteBytes(
                    value.data(),
                    value.size());
            }

            [[nodiscard]]
            const std::vector<std::uint8_t>&
                GetBytes() const noexcept
            {
                return bytes_;
            }

            [[nodiscard]]
            bool IsValid() const noexcept
            {
                return valid_;
            }

        private:
            std::vector<std::uint8_t> bytes_;
            bool valid_ = true;
        };

        [[nodiscard]]
        std::string ToLowerAscii(
            std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        std::wstring ToLowerWide(
            std::wstring value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            return value;
        }

        [[nodiscard]]
        bool HasPathComponent(
            const std::filesystem::path& path,
            const std::wstring_view expected)
        {
            for (const auto& component : path)
            {
                if (
                    ToLowerWide(
                        component.wstring()) ==
                    expected)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        std::filesystem::path
            MakeCharacterRelativePath(
                const std::filesystem::path& path)
        {
            std::filesystem::path result;
            bool foundCharacters = false;

            for (const auto& component : path)
            {
                if (
                    ToLowerWide(
                        component.wstring()) ==
                    L"characters")
                {
                    foundCharacters = true;
                }

                if (foundCharacters)
                {
                    result /= component;
                }
            }

            if (result.empty())
            {
                result =
                    std::filesystem::path(
                        L"Characters") /
                    path.filename();
            }

            return result.lexically_normal();
        }

        [[nodiscard]]
        std::string SanitizeFileStem(
            const std::string_view source,
            const std::string_view fallback)
        {
            std::string result;
            result.reserve(source.size());

            for (const unsigned char character :
                 source)
            {
                const bool valid =
                    (
                        character >= 'a' &&
                        character <= 'z'
                    ) ||
                    (
                        character >= 'A' &&
                        character <= 'Z'
                    ) ||
                    (
                        character >= '0' &&
                        character <= '9'
                    ) ||
                    character == '_' ||
                    character == '-';

                if (valid)
                {
                    result.push_back(
                        static_cast<char>(
                            character));
                }
                else if (
                    result.empty() ||
                    result.back() != '_')
                {
                    result.push_back('_');
                }
            }

            while (
                !result.empty() &&
                result.back() == '_')
            {
                result.pop_back();
            }

            return
                result.empty()
                    ? std::string(fallback)
                    : result;
        }

        [[nodiscard]]
        std::uint64_t HashText(
            const std::string_view value) noexcept
        {
            std::uint64_t hash =
                1469598103934665603ULL;

            for (const unsigned char character :
                 value)
            {
                hash ^= character;
                hash *= 1099511628211ULL;
            }

            return hash;
        }

        [[nodiscard]]
        std::string MakeHashSuffix(
            const std::filesystem::path& path)
        {
            char buffer[24]{};

            std::snprintf(
                buffer,
                sizeof(buffer),
                "_%08llx",
                static_cast<unsigned long long>(
                    HashText(
                        path.generic_u8string()) &
                    0xffffffffULL));

            return buffer;
        }

        [[nodiscard]]
        bool CommitTemporaryFile(
            const std::filesystem::path& temporary,
            const std::filesystem::path& destination,
            std::string& error)
        {
            std::error_code filesystemError;

            std::filesystem::remove(
                destination,
                filesystemError);

            filesystemError.clear();

            std::filesystem::rename(
                temporary,
                destination,
                filesystemError);

            if (filesystemError)
            {
                std::filesystem::remove(
                    temporary,
                    filesystemError);

                error =
                    "Failed to replace output file: " +
                    destination.generic_u8string();

                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool WriteBinaryAtomic(
            const std::filesystem::path& destination,
            const std::vector<std::uint8_t>& bytes,
            std::string& error)
        {
            std::error_code filesystemError;

            std::filesystem::create_directories(
                destination.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    "Failed to create output directory: " +
                    destination.parent_path().
                        generic_u8string();

                return false;
            }

            std::filesystem::path temporary =
                destination;

            temporary += L".tmp";

            std::filesystem::remove(
                temporary,
                filesystemError);

            std::ofstream stream(
                temporary,
                std::ios::binary |
                    std::ios::trunc);

            if (!stream)
            {
                error =
                    "Failed to open temporary output: " +
                    temporary.generic_u8string();

                return false;
            }

            if (!bytes.empty())
            {
                stream.write(
                    reinterpret_cast<const char*>(
                        bytes.data()),
                    static_cast<std::streamsize>(
                        bytes.size()));
            }

            stream.flush();

            if (!stream)
            {
                stream.close();

                std::filesystem::remove(
                    temporary,
                    filesystemError);

                error =
                    "Failed to write output file: " +
                    destination.generic_u8string();

                return false;
            }

            stream.close();

            return CommitTemporaryFile(
                temporary,
                destination,
                error);
        }

        [[nodiscard]]
        bool CopyFileAtomic(
            const std::filesystem::path& source,
            const std::filesystem::path& destination,
            std::string& error)
        {
            std::error_code filesystemError;

            if (
                !std::filesystem::is_regular_file(
                    source,
                    filesystemError) ||
                filesystemError)
            {
                error =
                    "Texture source does not exist: " +
                    source.generic_u8string();

                return false;
            }

            std::filesystem::create_directories(
                destination.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    "Failed to create texture directory.";

                return false;
            }

            std::filesystem::path temporary =
                destination;

            temporary += L".tmp";

            std::filesystem::remove(
                temporary,
                filesystemError);

            filesystemError.clear();

            std::filesystem::copy_file(
                source,
                temporary,
                std::filesystem::copy_options::
                    overwrite_existing,
                filesystemError);

            if (filesystemError)
            {
                error =
                    "Failed to copy texture: " +
                    source.generic_u8string();

                return false;
            }

            return CommitTemporaryFile(
                temporary,
                destination,
                error);
        }

        struct TextureOutputState final
        {
            std::unordered_map<std::string, std::string>
                sourceToAssetPath;

            std::unordered_map<std::string, std::string>
                outputNameToSource;
        };

        [[nodiscard]]
        bool ResolveTextureOutput(
            const LegacyMaterialTexture& texture,
            const std::filesystem::path& dataRoot,
            const bool copyTexture,
            TextureOutputState& state,
            std::string& assetPath,
            std::size_t& copiedTextureCount,
            std::string& error)
        {
            assetPath.clear();

            if (
                texture.dds.path.empty() ||
                !texture.dds.valid)
            {
                return true;
            }

            const std::string sourceKey =
                ToLowerAscii(
                    texture.dds.path.
                        lexically_normal().
                        generic_u8string());

            const auto existingSource =
                state.sourceToAssetPath.find(
                    sourceKey);

            if (
                existingSource !=
                state.sourceToAssetPath.end())
            {
                assetPath =
                    existingSource->second;

                return true;
            }

            std::filesystem::path filename =
                texture.dds.path.filename();

            if (filename.extension().empty())
            {
                filename += L".dds";
            }
            else
            {
                filename.replace_extension(
                    L".dds");
            }

            std::string outputNameKey =
                ToLowerAscii(
                    filename.generic_u8string());

            const auto existingName =
                state.outputNameToSource.find(
                    outputNameKey);

            if (
                existingName !=
                    state.outputNameToSource.end() &&
                existingName->second != sourceKey)
            {
                std::filesystem::path uniqueName =
                    filename.stem();

                uniqueName +=
                    std::filesystem::u8path(
                        MakeHashSuffix(
                            texture.dds.path));

                uniqueName += L".dds";

                filename = std::move(uniqueName);

                outputNameKey =
                    ToLowerAscii(
                        filename.generic_u8string());
            }

            const std::filesystem::path relativePath =
                std::filesystem::path(
                    L"Textures") /
                L"Characters" /
                filename;

            const std::filesystem::path destination =
                dataRoot /
                relativePath;

            if (
                copyTexture &&
                !CopyFileAtomic(
                    texture.dds.path,
                    destination,
                    error))
            {
                return false;
            }

            assetPath =
                relativePath.generic_u8string();

            state.sourceToAssetPath.emplace(
                sourceKey,
                assetPath);

            state.outputNameToSource.emplace(
                outputNameKey,
                sourceKey);

            if (copyTexture)
            {
                ++copiedTextureCount;
            }

            return true;
        }

        [[nodiscard]]
        bool WriteSkeleton(
            const LegacySkeletonData& skeleton,
            const std::filesystem::path& destination,
            std::string& error)
        {
            BinaryWriter writer;

            constexpr std::array<char, 8U> magic
            {
                'S', 'K', 'E', 'L',
                'E', 'T', 'O', 'N'
            };

            writer.WriteBytes(
                magic.data(),
                magic.size());

            writer.Write<std::uint32_t>(1U);
            writer.Write(AssetEndianMarker);
            writer.Write(skeleton.skeletonId);

            if (
                skeleton.bones.size() >
                (std::numeric_limits<
                    std::uint32_t>::max)())
            {
                error =
                    "Skeleton contains too many bones.";

                return false;
            }

            writer.Write(
                static_cast<std::uint32_t>(
                    skeleton.bones.size()));

            for (const LegacyBone& bone :
                 skeleton.bones)
            {
                writer.WriteString(bone.name);
                writer.Write(bone.parentIndex);
                writer.Write(bone.length);

                for (const float value :
                     bone.absoluteBindMatrix)
                {
                    writer.Write(value);
                }
            }

            if (!writer.IsValid())
            {
                error =
                    "Failed to serialize skeleton.";

                return false;
            }

            return WriteBinaryAtomic(
                destination,
                writer.GetBytes(),
                error);
        }

        [[nodiscard]]
        bool WriteAnimation(
            const LegacyAnimationData& animation,
            const std::string_view skeletonAssetPath,
            const std::filesystem::path& destination,
            std::string& error)
        {
            std::size_t mappedTrackCount = 0U;

            for (const LegacyAnimationTrack& track :
                 animation.tracks)
            {
                if (track.skeletonBoneIndex >= 0)
                {
                    ++mappedTrackCount;
                }
            }

            if (
                mappedTrackCount == 0U ||
                mappedTrackCount >
                (std::numeric_limits<
                    std::uint32_t>::max)())
            {
                error =
                    "Animation has no mapped tracks.";

                return false;
            }

            BinaryWriter writer;

            constexpr std::array<char, 8U> magic
            {
                'A', 'N', 'I', 'M',
                'C', 'L', 'I', 'P'
            };

            writer.WriteBytes(
                magic.data(),
                magic.size());

            writer.Write<std::uint32_t>(1U);
            writer.Write(AssetEndianMarker);

            writer.WriteString(
                skeletonAssetPath);

            writer.Write(animation.skeletonId);
            writer.Write(animation.frameCount);
            writer.Write(animation.frameRate);
            writer.Write(animation.durationSeconds);

            writer.Write(
                static_cast<std::uint32_t>(
                    mappedTrackCount));

            for (const LegacyAnimationTrack& track :
                 animation.tracks)
            {
                if (track.skeletonBoneIndex < 0)
                {
                    continue;
                }

                if (
                    track.keys.size() >
                    (std::numeric_limits<
                        std::uint32_t>::max)())
                {
                    error =
                        "Animation track contains too many keys.";

                    return false;
                }

                writer.WriteString(track.boneName);
                writer.Write(
                    track.skeletonBoneIndex);
                writer.Write(track.flags);

                writer.Write(
                    static_cast<std::uint32_t>(
                        track.keys.size()));

                for (const LegacyAnimationKey& key :
                     track.keys)
                {
                    for (const float value :
                         key.rotation)
                    {
                        writer.Write(value);
                    }

                    for (const float value :
                         key.translation)
                    {
                        writer.Write(value);
                    }
                }
            }

            if (!writer.IsValid())
            {
                error =
                    "Failed to serialize animation.";

                return false;
            }

            return WriteBinaryAtomic(
                destination,
                writer.GetBytes(),
                error);
        }

        [[nodiscard]]
        bool WriteMaterial(
            const LegacyMaterialData& material,
            const std::array<
                std::string,
                static_cast<std::size_t>(
                    LegacyTextureSlot::Count)>&
                        texturePaths,
            const std::filesystem::path& destination,
            std::string& error)
        {
            const auto sanitizeFloat =
                [](
                    const float value,
                    const float fallback,
                    const float minimum,
                    const float maximum) noexcept
                {
                    if (!std::isfinite(value))
                    {
                        return fallback;
                    }

                    return std::clamp(
                        value,
                        minimum,
                        maximum);
                };

            engine::assets::MaterialAssetDesc
                description;

            description.baseColorFactor =
            {
                sanitizeFloat(
                    material.diffuseColor[0],
                    1.0F,
                    0.0F,
                    1.0F),

                sanitizeFloat(
                    material.diffuseColor[1],
                    1.0F,
                    0.0F,
                    1.0F),

                sanitizeFloat(
                    material.diffuseColor[2],
                    1.0F,
                    0.0F,
                    1.0F),

                1.0F
            };

            description.metallicFactor =
                sanitizeFloat(
                    material.lowQualityMetalness,
                    0.0F,
                    0.0F,
                    1.0F);

            /*
             * Пока полноценная конверсия WarZ gloss
             * в PBR roughness не подключена.
             *
             * Roughness texture, если она существует,
             * всё равно сохраняется ниже.
             */
            description.roughnessFactor = 1.0F;

            description.alphaCutoff = 0.5F;

            /*
             * ForceAlpha обычно используется для
             * растительности, волос и cutout-материалов.
             */
            if (material.forceAlpha)
            {
                description.alphaMode =
                    engine::assets::
                        MaterialAlphaMode::Mask;
            }
            else if (material.transparent)
            {
                description.alphaMode =
                    engine::assets::
                        MaterialAlphaMode::Blend;
            }
            else
            {
                description.alphaMode =
                    engine::assets::
                        MaterialAlphaMode::Opaque;
            }

            description.doubleSided =
                material.doubleSided;

            description.normalScale = 1.0F;

            const bool hasSpecularTexture =
                !texturePaths[
                    static_cast<std::size_t>(
                        LegacyTextureSlot::
                            Specular)]
                    .empty();

            description.specularIntensity =
                hasSpecularTexture ||
                material.specularPower > 0.0F
                    ? 1.0F
                    : 0.0F;

            description.specularPower =
                sanitizeFloat(
                    material.specularPower,
                    32.0F,
                    1.0F,
                    8192.0F);

            description.reflectionFactor =
                sanitizeFloat(
                    material.reflectionPower,
                    0.0F,
                    0.0F,
                    16.0F);

            description.emissiveStrength =
                sanitizeFloat(
                    material.
                        selfIlluminationMultiplier,
                    0.0F,
                    0.0F,
                    64.0F);

            const bool hasEmissiveTexture =
                !texturePaths[
                    static_cast<std::size_t>(
                        LegacyTextureSlot::Glow)]
                    .empty();

            if (
                hasEmissiveTexture ||
                description.emissiveStrength >
                    0.0F)
            {
                description.emissiveFactor =
                {
                    1.0F,
                    1.0F,
                    1.0F
                };
            }

            description.sampler.filter =
                engine::graphics::
                    TextureFilter::Anisotropic;

            description.sampler.addressU =
                engine::graphics::
                    TextureAddressMode::Wrap;

            description.sampler.addressV =
                engine::graphics::
                    TextureAddressMode::Wrap;

            description.sampler.addressW =
                engine::graphics::
                    TextureAddressMode::Wrap;

            description.sampler.
                maximumAnisotropy = 8U;

            description.debugName =
                destination.filename().
                    generic_u8string();

            const auto assignTexture =
                [&texturePaths, &error](
                    const LegacyTextureSlot slot,
                    std::optional<
                        engine::assets::AssetPath>&
                            destinationPath)
                {
                    const std::string& sourcePath =
                        texturePaths[
                            static_cast<std::size_t>(
                                slot)];

                    if (sourcePath.empty())
                    {
                        return true;
                    }

                    engine::assets::AssetPath path;

                    const engine::assets::
                        AssetResult result =
                            engine::assets::
                                AssetPath::TryCreate(
                                    sourcePath,
                                    path);

                    if (engine::assets::Failed(
                            result))
                    {
                        error =
                            "Invalid material texture "
                            "asset path '" +
                            sourcePath +
                            "': " +
                            engine::assets::
                                ToString(result);

                        return false;
                    }

                    destinationPath =
                        std::move(path);

                    return true;
                };

            if (
                !assignTexture(
                    LegacyTextureSlot::Diffuse,
                    description.
                        baseColorTexture) ||
                !assignTexture(
                    LegacyTextureSlot::Normal,
                    description.
                        normalTexture) ||
                !assignTexture(
                    LegacyTextureSlot::Specular,
                    description.
                        specularGlossTexture) ||
                !assignTexture(
                    LegacyTextureSlot::Roughness,
                    description.
                        roughnessTexture) ||
                !assignTexture(
                    LegacyTextureSlot::Glow,
                    description.
                        emissiveTexture) ||
                !assignTexture(
                    LegacyTextureSlot::
                        SpecularPower,
                    description.
                        specularPowerTexture))
            {
                return false;
            }

            engine::assets::MaterialAsset
                materialAsset;

            engine::assets::AssetResult result =
                materialAsset.Initialize(
                    std::move(description));

            if (engine::assets::Failed(result))
            {
                error =
                    "Failed to initialize LTS "
                    "material '" +
                    destination.
                        generic_u8string() +
                    "': " +
                    engine::assets::
                        ToString(result);

                return false;
            }

            engine::assets::AssetData encoded;

            result =
                engine::assets::
                    LtsMaterialWriter::Encode(
                        materialAsset,
                        encoded);

            if (engine::assets::Failed(result))
            {
                error =
                    "Failed to encode LTS "
                    "material '" +
                    destination.
                        generic_u8string() +
                    "': " +
                    engine::assets::
                        ToString(result);

                return false;
            }

            if (
                encoded.IsEmpty() ||
                encoded.GetData() == nullptr)
            {
                error =
                    "LTS material writer produced "
                    "empty data.";

                return false;
            }

            std::vector<std::uint8_t> bytes;

            try
            {
                bytes.resize(
                    encoded.GetSize());

                std::memcpy(
                    bytes.data(),
                    encoded.GetData(),
                    encoded.GetSize());
            }
            catch (const std::bad_alloc&)
            {
                error =
                    "Not enough memory to write "
                    "the LTS material.";

                return false;
            }
            catch (...)
            {
                error =
                    "Unexpected failure while "
                    "preparing LTS material data.";

                return false;
            }

            return WriteBinaryAtomic(
                destination,
                bytes,
                error);
        }

        [[nodiscard]]
        bool WriteSkeletalMesh(
            const LegacyMeshData& mesh,
            const LegacyWeightData& weights,
            const std::string_view skeletonAssetPath,
            const std::vector<std::string>&
                materialAssetPaths,
            const std::filesystem::path& destination,
            std::string& error)
        {
            if (
                mesh.vertices.empty() ||
                mesh.indices.empty() ||
                mesh.vertices.size() !=
                    weights.vertices.size())
            {
                error =
                    "Invalid skeletal mesh geometry or weights.";

                return false;
            }

            if (
                mesh.vertices.size() >
                    (std::numeric_limits<
                        std::uint32_t>::max)() ||
                mesh.indices.size() >
                    (std::numeric_limits<
                        std::uint32_t>::max)() ||
                mesh.materialChunks.size() >
                    (std::numeric_limits<
                        std::uint32_t>::max)())
            {
                error =
                    "Skeletal mesh is too large.";

                return false;
            }

            std::array<float, 3U> minimum =
                mesh.vertices.front().position;

            std::array<float, 3U> maximum =
                mesh.vertices.front().position;

            for (const LegacyMeshVertex& vertex :
                 mesh.vertices)
            {
                for (
                    std::size_t axis = 0U;
                    axis < 3U;
                    ++axis)
                {
                    minimum[axis] =
                        (std::min)(
                            minimum[axis],
                            vertex.position[axis]);

                    maximum[axis] =
                        (std::max)(
                            maximum[axis],
                            vertex.position[axis]);
                }
            }

            BinaryWriter writer;

            constexpr std::array<char, 8U> magic
            {
                'S', 'K', 'M', 'E',
                'S', 'H', '\0', '\0'
            };

            writer.WriteBytes(
                magic.data(),
                magic.size());

            writer.Write<std::uint32_t>(1U);
            writer.Write(AssetEndianMarker);

            writer.WriteString(
                skeletonAssetPath);

            writer.Write(
                static_cast<std::uint32_t>(
                    mesh.vertices.size()));

            writer.Write(
                static_cast<std::uint32_t>(
                    mesh.indices.size()));

            writer.Write(
                static_cast<std::uint32_t>(
                    mesh.materialChunks.size()));

            for (const float value : mesh.pivot)
            {
                writer.Write(value);
            }

            for (const float value : minimum)
            {
                writer.Write(value);
            }

            for (const float value : maximum)
            {
                writer.Write(value);
            }

            for (
                std::size_t index = 0U;
                index < mesh.vertices.size();
                ++index)
            {
                const LegacyMeshVertex& vertex =
                    mesh.vertices[index];

                const LegacySkinVertex& skin =
                    weights.vertices[index];

                for (const float value :
                     vertex.position)
                {
                    writer.Write(value);
                }

                for (const float value :
                     vertex.normal)
                {
                    writer.Write(value);
                }

                for (const float value :
                     vertex.tangent)
                {
                    writer.Write(value);
                }

                writer.Write(vertex.tangentSign);

                for (const float value : vertex.uv)
                {
                    writer.Write(value);
                }

                writer.WriteBytes(
                    skin.boneIndices.data(),
                    skin.boneIndices.size());

                for (const float value :
                     skin.weights)
                {
                    writer.Write(value);
                }
            }

            for (const std::uint32_t index :
                 mesh.indices)
            {
                writer.Write(index);
            }

            for (
                std::size_t sectionIndex = 0U;
                sectionIndex <
                    mesh.materialChunks.size();
                ++sectionIndex)
            {
                const LegacyMaterialChunk& section =
                    mesh.materialChunks[
                        sectionIndex];

                writer.Write(section.firstIndex);
                writer.Write(section.indexCount);

                writer.Write(
                    static_cast<std::uint32_t>(
                        sectionIndex));

                const std::string materialPath =
                    sectionIndex <
                        materialAssetPaths.size()
                            ? materialAssetPaths[
                                sectionIndex]
                            : std::string{};

                writer.WriteString(materialPath);
            }

            if (!writer.IsValid())
            {
                error =
                    "Failed to serialize skeletal mesh.";

                return false;
            }

            return WriteBinaryAtomic(
                destination,
                writer.GetBytes(),
                error);
        }
    }

    std::filesystem::path
    WarZAssetConverter::BuildSkeletonRelativePath(
        const std::filesystem::path&
            sourceSkeletonPath)
    {
        const std::wstring lowerFilename =
            ToLowerWide(
                sourceSkeletonPath.
                    filename().
                    wstring());

        const bool characterSkeleton =
            lowerFilename ==
                L"properscale_andbiped_new.skl";

        const bool weaponSkeleton =
            HasPathComponent(
                sourceSkeletonPath,
                L"weapons");

        std::filesystem::path filename;

        if (characterSkeleton)
        {
            filename =
                L"CH_Skeletal.sk";
        }
        else
        {
            filename =
                sourceSkeletonPath.stem();

            filename += L".sk";
        }

        return
            std::filesystem::path(
                L"Skeletons") /
            (
                weaponSkeleton
                    ? L"Weapons"
                    : L"Characters"
            ) /
            filename;
    }

    bool WarZAssetConverter::Convert(
        const WarZConversionRequest& request,
        WarZConversionResult& result) noexcept
    {
        result = {};

        try
        {
            if (
                request.dataRoot.empty() ||
                request.mesh == nullptr ||
                request.skeleton == nullptr ||
                request.weights == nullptr)
            {
                result.error =
                    "Invalid conversion request.";

                return false;
            }

            if (
                request.mesh->vertices.size() !=
                request.weights->vertices.size())
            {
                result.error =
                    "Mesh vertex count does not match weight count.";

                return false;
            }

            const std::filesystem::path
                skeletonRelativePath =
                    BuildSkeletonRelativePath(
                        request.sourceSkeletonPath);

            const std::string skeletonAssetPath =
                skeletonRelativePath.
                    generic_u8string();

            result.skeletonPath =
                request.dataRoot /
                skeletonRelativePath;

            if (
                request.writeSkeleton &&
                !WriteSkeleton(
                    *request.skeleton,
                    result.skeletonPath,
                    result.error))
            {
                return false;
            }

            TextureOutputState textureState;

            std::unordered_map<
                std::string,
                std::string>
                    materialPathsByName;

            std::unordered_set<std::string>
                usedMaterialFilenames;

            if (
                request.writeMaterials &&
                request.materials != nullptr)
            {
                std::size_t materialIndex = 0U;

                for (
                    const LegacyMaterialData& material :
                    request.materials->materials)
                {
                    std::string filenameStem =
                        SanitizeFileStem(
                            material.name,
                            "material_" +
                                std::to_string(
                                    materialIndex));

                    std::string filenameKey =
                        ToLowerAscii(filenameStem);

                    if (
                        !usedMaterialFilenames.
                            insert(filenameKey).
                            second)
                    {
                        filenameStem +=
                            MakeHashSuffix(
                                material.sourcePath);

                        filenameKey =
                            ToLowerAscii(
                                filenameStem);

                        usedMaterialFilenames.insert(
                            filenameKey);
                    }

                    const std::filesystem::path
                        materialRelativePath =
                            std::filesystem::path(
                                L"Materials") /
                            L"Characters" /
                            std::filesystem::u8path(
                                filenameStem +
                                ".material");

                    std::array<
                        std::string,
                        static_cast<std::size_t>(
                            LegacyTextureSlot::Count)>
                            texturePaths{};

                    for (
                        std::size_t textureIndex = 0U;
                        textureIndex <
                            material.textures.size();
                        ++textureIndex)
                    {
                        std::string textureError;

                        if (!ResolveTextureOutput(
                                material.textures[
                                    textureIndex],
                                request.dataRoot,
                                request.writeTextures,
                                textureState,
                                texturePaths[
                                    textureIndex],
                                result.textureCount,
                                textureError))
                        {
                            result.warnings.push_back(
                                std::move(
                                    textureError));
                        }
                    }

                    if (!WriteMaterial(
                            material,
                            texturePaths,
                            request.dataRoot /
                                materialRelativePath,
                            result.error))
                    {
                        return false;
                    }

                    materialPathsByName.emplace(
                        ToLowerAscii(material.name),
                        materialRelativePath.
                            generic_u8string());

                    ++result.materialCount;
                    ++materialIndex;
                }
            }

            std::vector<std::string>
                sectionMaterialPaths;

            sectionMaterialPaths.reserve(
                request.mesh->
                    materialChunks.size());

            for (
                const LegacyMaterialChunk& chunk :
                request.mesh->materialChunks)
            {
                const auto material =
                    materialPathsByName.find(
                        ToLowerAscii(
                            chunk.materialName));

                if (
                    material ==
                    materialPathsByName.end())
                {
                    sectionMaterialPaths.
                        emplace_back();

                    result.warnings.push_back(
                        "Material was not converted: " +
                        chunk.materialName);
                }
                else
                {
                    sectionMaterialPaths.push_back(
                        material->second);
                }
            }

            std::filesystem::path
                meshRelativePath =
                    std::filesystem::path(
                        L"SkeletalMeshes") /
                    MakeCharacterRelativePath(
                        request.
                            packageRelativePath);

            meshRelativePath.replace_extension(
                L".skm");

            result.skeletalMeshPath =
                request.dataRoot /
                meshRelativePath;

            if (
                request.writeSkeletalMesh &&
                !WriteSkeletalMesh(
                    *request.mesh,
                    *request.weights,
                    skeletonAssetPath,
                    sectionMaterialPaths,
                    result.skeletalMeshPath,
                    result.error))
            {
                return false;
            }

            if (
                request.writeAnimations &&
                request.animationPaths != nullptr)
            {
                const std::filesystem::path
                    animationsRoot =
                        request.sourceRoot /
                        L"Animations";

                for (
                    const std::filesystem::path&
                        animationPath :
                    *request.animationPaths)
                {
                    LegacyAnimationData animation;

                    if (!LegacyAnimationReader::Read(
                            animationPath,
                            request.skeleton,
                            animation))
                    {
                        result.warnings.push_back(
                            "Animation skipped: " +
                            animationPath.
                                filename().
                                generic_u8string() +
                            " — " +
                            animation.error);

                        continue;
                    }

                    if (!animation.IsCompatible())
                    {
                        result.warnings.push_back(
                            "Animation is not compatible: " +
                            animationPath.
                                filename().
                                generic_u8string());

                        continue;
                    }

                    std::error_code relativeError;

                    std::filesystem::path
                        animationRelativeSource =
                            std::filesystem::relative(
                                animationPath,
                                animationsRoot,
                                relativeError);

                    if (
                        relativeError ||
                        animationRelativeSource.empty())
                    {
                        animationRelativeSource =
                            animationPath.filename();
                    }

                    std::filesystem::path
                        animationRelativePath =
                            std::filesystem::path(
                                L"Animations") /
                            MakeCharacterRelativePath(
                                animationRelativeSource);

                    animationRelativePath.
                        replace_extension(
                            L".anim");

                    std::string animationError;

                    if (!WriteAnimation(
                            animation,
                            skeletonAssetPath,
                            request.dataRoot /
                                animationRelativePath,
                            animationError))
                    {
                        result.warnings.push_back(
                            "Animation write failed: " +
                            animationPath.
                                filename().
                                generic_u8string() +
                            " — " +
                            animationError);

                        continue;
                    }

                    ++result.animationCount;
                }
            }

            result.success = true;
            return true;
        }
        catch (const std::exception& exception)
        {
            result.error =
                "Conversion failed: " +
                std::string(
                    exception.what());

            return false;
        }
        catch (...)
        {
            result.error =
                "Conversion failed with an unknown error.";

            return false;
        }
    }
}