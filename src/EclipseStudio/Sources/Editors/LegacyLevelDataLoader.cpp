#include "LegacyLevelDataLoader.h"

#include <Assets/AssetResult.h>
#include <Assets/AssetData.h>
#include <Assets/LegacyMeshImporter.h>
#include <Assets/LtsMeshWriter.h>
#include <Assets/MaterialAsset.h>
#include <Assets/MaterialAssetWriter.h>
#include <Assets/MeshAssetBuilder.h>
#include <Assets/ScbMaterialConverter.h>

#include <pugixml.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace studio::editor
{
    namespace
    {
        constexpr std::uint32_t ScbSignature = 0xFADC0038U;
        constexpr std::size_t MaximumReportedErrors = 12U;
        constexpr wchar_t LevelDataMeshDirectory[] = L"LevelData";
        constexpr std::size_t MinimumWaterRegionCells = 64U;

        struct MeshReference final
        {
            bool sourceReference = false;
            bool rewriteXml = false;

            std::filesystem::path sourcePath;
            std::filesystem::path meshPath;
            std::filesystem::path logicalMeshPath;
        };

        struct CachedMesh final
        {
            bool available = false;

            std::filesystem::path path;
        };

        struct PendingObject final
        {
            pugi::xml_node node;

            std::wstring originalName;
            std::wstring editorFolder;

            std::filesystem::path meshPath;
            std::filesystem::path logicalMeshPath;

            engine::scene::SceneTransform transform;

            std::size_t objectIndex = 0U;

            bool rewriteXml = false;
            bool available = false;
            bool castShadows = true;
            bool disableDistanceCulling = false;
            std::int32_t renderOrder = 0;
        };

        [[nodiscard]]
        std::wstring ToWide(
            const char* const text)
        {
            if (
                text == nullptr ||
                *text == '\0')
            {
                return {};
            }

            return
                std::filesystem::u8path(text).
                    generic_wstring();
        }

        [[nodiscard]]
        std::wstring Lowercase(
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
        bool IsSafeRelativePath(
            const std::filesystem::path& path) noexcept
        {
            if (
                path.empty() ||
                path.is_absolute() ||
                path.has_root_path())
            {
                return false;
            }

            for (
                const std::filesystem::path& component :
                path)
            {
                if (component == L"..")
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool ExtractRelativePath(
            const std::filesystem::path& logicalPath,
            const std::wstring_view rootName,
            std::filesystem::path& relativePath)
        {
            relativePath.clear();

            if (!IsSafeRelativePath(logicalPath))
            {
                return false;
            }

            auto component =
                logicalPath.begin();

            if (
                component == logicalPath.end() ||
                Lowercase(component->wstring()) !=
                    L"data")
            {
                return false;
            }

            ++component;

            if (
                component == logicalPath.end() ||
                Lowercase(component->wstring()) !=
                    rootName)
            {
                return false;
            }

            ++component;

            for (
                ;
                component != logicalPath.end();
                ++component)
            {
                relativePath /= *component;
            }

            return
                IsSafeRelativePath(relativePath);
        }

        [[nodiscard]]
        bool BuildMeshReference(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& logicalPath,
            MeshReference& reference)
        {
            reference = {};

            const std::filesystem::path normalized =
                logicalPath.lexically_normal();

            const std::wstring extension =
                Lowercase(
                    normalized.extension().wstring());

            std::filesystem::path relativePath;
            
            if (extension == L".mesh")
            {
                if (!ExtractRelativePath(
                        normalized,
                        L"staticmeshes",
                        relativePath))
                {
                    return false;
                }

                reference.meshPath =
                    workspaceRoot /
                    L"bin" /
                    L"Data" /
                    L"StaticMeshes" /
                    relativePath;

                reference.logicalMeshPath =
                    std::filesystem::path(L"Data") /
                    L"StaticMeshes" /
                    relativePath;

                /*
                 * LevelData.xml may already contain the migrated .mesh path,
                 * while the generated .material files are still missing.
                 * Reconstruct the original SCB path so the importer can repair
                 * the complete mesh/material package on the next level load.
                 */
                std::filesystem::path sourceRelativePath =
                    relativePath;

                sourceRelativePath.replace_extension(
                    L".scb");

                reference.sourceReference = true;
                reference.rewriteXml = false;

                reference.sourcePath =
                    workspaceRoot /
                    L"bin" /
                    L"Data" /
                    L"ObjectsDepot" /
                    sourceRelativePath;

                return true;
            }

            /*
             * Р’ СЃС‚Р°СЂРѕРј XML РјРѕР¶РµС‚ Р±С‹С‚СЊ РЅР°РїРёСЃР°РЅРѕ .sco РёР»Рё .scb.
             * Р’ РѕР±РѕРёС… СЃР»СѓС‡Р°СЏС… С„РёР·РёС‡РµСЃРєРё С‡РёС‚Р°РµС‚СЃСЏ С‚РѕР»СЊРєРѕ SCB.
             */
            if (
                extension != L".sco" &&
                extension != L".scb")
            {
                return false;
            }

            if (!ExtractRelativePath(
                    normalized,
                    L"objectsdepot",
                    relativePath))
            {
                return false;
            }

            std::filesystem::path sourceRelativePath =
                relativePath;

            sourceRelativePath.replace_extension(
                L".scb");

            std::filesystem::path meshRelativePath =
                relativePath;

            meshRelativePath.replace_extension(
                L".mesh");

            reference.sourceReference = true;
            reference.rewriteXml = true;

            reference.sourcePath =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"ObjectsDepot" /
                sourceRelativePath;

            reference.meshPath =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"StaticMeshes" /
                meshRelativePath;

            reference.logicalMeshPath =
                std::filesystem::path(L"Data") /
                L"StaticMeshes" /
                meshRelativePath;

            return true;
        }

        [[nodiscard]]
        bool HasScbSignature(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::ifstream input(
                    path,
                    std::ios::binary);

                std::uint32_t signature = 0U;

                return
                    input.read(
                        reinterpret_cast<char*>(
                            &signature),
                        sizeof(signature)).
                        good() &&
                    signature == ScbSignature;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        float AttributeFloat(
            const pugi::xml_attribute& attribute,
            const float fallback) noexcept
        {
            return
                attribute
                    ? attribute.as_float()
                    : fallback;
        }

        void ReadTransform(
            const pugi::xml_node& objectNode,
            engine::scene::SceneTransform& transform) noexcept
        {
            const pugi::xml_node position =
                objectNode.child("position");

            transform.position =
            {
                AttributeFloat(
                    position.attribute("x"),
                    0.0F),

                AttributeFloat(
                    position.attribute("y"),
                    0.0F),

                AttributeFloat(
                    position.attribute("z"),
                    0.0F)
            };

            const pugi::xml_node gameObject =
                objectNode.child("gameObject");

            pugi::xml_node rotation =
                gameObject.child("rotation");

            if (!rotation)
            {
                rotation =
                    objectNode.child("rotation");
            }

            transform.rotationDegrees =
            {
                AttributeFloat(
                    rotation.attribute("x"),
                    0.0F),

                AttributeFloat(
                    rotation.attribute("y"),
                    0.0F),

                AttributeFloat(
                    rotation.attribute("z"),
                    0.0F)
            };

            pugi::xml_node scale =
                gameObject.child("scale");

            if (!scale)
            {
                scale =
                    objectNode.child("scale");
            }

            if (scale)
            {
                transform.scale =
                {
                    AttributeFloat(
                        scale.attribute("x"),
                        1.0F),

                    AttributeFloat(
                        scale.attribute("y"),
                        1.0F),

                    AttributeFloat(
                        scale.attribute("z"),
                        1.0F)
                };
            }
        }

        void AddError(
            std::vector<std::string>& errors,
            const std::string_view message,
            const std::filesystem::path& path)
        {
            if (
                errors.size() >=
                MaximumReportedErrors)
            {
                return;
            }

            std::string text(message);

            text += " ";
            text += path.generic_u8string();

            errors.push_back(
                std::move(text));
        }

        [[nodiscard]]
        std::filesystem::path FindSourceTexturesDirectory(
            const std::filesystem::path& sourceMeshPath)
        {
            std::filesystem::path directory =
                sourceMeshPath.parent_path();

            while (!directory.empty())
            {
                if (
                    Lowercase(
                        directory.filename().wstring()) ==
                    L"objectsdepot")
                {
                    break;
                }

                std::error_code error;

                const std::filesystem::path textures =
                    directory / L"Textures";

                if (
                    std::filesystem::is_directory(
                        textures,
                        error) &&
                    !error)
                {
                    return textures;
                }

                const std::filesystem::path parent =
                    directory.parent_path();

                if (parent == directory)
                {
                    break;
                }

                directory = parent;
            }

            return {};
        }

        [[nodiscard]]
        bool PreparePackage(
            const MeshReference& reference,
            std::unordered_set<std::wstring>&
                copiedTextureDirectories,
            std::vector<std::string>& errors)
        {
            std::error_code filesystemError;

            const std::filesystem::path packageDirectory =
                reference.meshPath.parent_path();

            for (
                const wchar_t* const directoryName :
                {
                    L"Materials",
                    L"Textures",
                    L"Physics"
                })
            {
                filesystemError.clear();

                std::filesystem::create_directories(
                    packageDirectory /
                        directoryName,
                    filesystemError);

                if (filesystemError)
                {
                    AddError(
                        errors,
                        "Cannot create output directory:",
                        packageDirectory /
                            directoryName);

                    return false;
                }
            }

            const std::filesystem::path sourceTextures =
                FindSourceTexturesDirectory(
                    reference.sourcePath);

            if (sourceTextures.empty())
            {
                return true;
            }

            const std::wstring textureKey =
                Lowercase(
                    sourceTextures.
                        lexically_normal().
                        wstring());

            if (
                !copiedTextureDirectories.
                    insert(textureKey).
                    second)
            {
                return true;
            }

            filesystemError.clear();

            std::filesystem::copy(
                sourceTextures,
                packageDirectory /
                    L"Textures",
                std::filesystem::copy_options::recursive |
                    std::filesystem::copy_options::update_existing,
                filesystemError);

            if (filesystemError)
            {
                AddError(
                    errors,
                    "Cannot copy original textures:",
                    sourceTextures);

                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool MeshFileExists(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::error_code error;

                if (
                    !std::filesystem::is_regular_file(
                        path,
                        error) ||
                    error)
                {
                    return false;
                }

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        path,
                        error);

                return
                    !error &&
                    fileSize >= 160U;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool ReadMeshMaterialSlotCount(
            const std::filesystem::path& meshPath,
            std::uint32_t& materialSlotCount) noexcept
        {
            materialSlotCount = 0U;

            try
            {
                constexpr std::array<char, 8U> MeshMagic
                {
                    'L', 'T', 'S', 'M', 'E', 'S', 'H', '\0'
                };

                constexpr std::size_t RequiredHeaderBytes = 44U;

                std::array<std::byte, RequiredHeaderBytes> header{};
                std::ifstream input(
                    meshPath,
                    std::ios::binary);

                if (
                    !input ||
                    !input.read(
                        reinterpret_cast<char*>(
                            header.data()),
                        static_cast<std::streamsize>(
                            header.size())).good() ||
                    std::memcmp(
                        header.data(),
                        MeshMagic.data(),
                        MeshMagic.size()) != 0)
                {
                    return false;
                }

                std::uint32_t version = 0U;
                std::uint32_t endianMarker = 0U;
                std::uint32_t headerSize = 0U;

                std::memcpy(
                    &version,
                    header.data() + 8U,
                    sizeof(version));

                std::memcpy(
                    &endianMarker,
                    header.data() + 12U,
                    sizeof(endianMarker));

                std::memcpy(
                    &headerSize,
                    header.data() + 16U,
                    sizeof(headerSize));

                std::memcpy(
                    &materialSlotCount,
                    header.data() + 40U,
                    sizeof(materialSlotCount));

                return
                    version == 1U &&
                    endianMarker == 0x01020304U &&
                    headerSize == 160U &&
                    materialSlotCount > 0U &&
                    materialSlotCount <= 65536U;
            }
            catch (...)
            {
                materialSlotCount = 0U;
                return false;
            }
        }

        [[nodiscard]]
        bool ParseMaterialSlot(
            const std::wstring& filename,
            const std::wstring& prefix,
            const std::uint32_t materialSlotCount,
            std::uint32_t& slot) noexcept
        {
            slot = 0U;

            if (filename.rfind(prefix, 0U) != 0U)
            {
                return false;
            }

            const std::size_t slotBegin =
                prefix.size();

            const std::size_t slotEnd =
                filename.find(
                    L'_',
                    slotBegin);

            if (
                slotEnd == std::wstring::npos ||
                slotEnd == slotBegin)
            {
                return false;
            }

            for (
                std::size_t index = slotBegin;
                index < slotEnd;
                ++index)
            {
                const wchar_t character =
                    filename[index];

                if (
                    character < L'0' ||
                    character > L'9')
                {
                    return false;
                }

                const std::uint32_t digit =
                    static_cast<std::uint32_t>(
                        character - L'0');

                if (
                    slot >
                    ((std::numeric_limits<std::uint32_t>::max)() -
                        digit) /
                        10U)
                {
                    return false;
                }

                slot =
                    slot * 10U +
                    digit;
            }

            return slot < materialSlotCount;
        }

        [[nodiscard]]
        bool MaterialSetExists(
            const std::filesystem::path& meshPath) noexcept
        {
            try
            {
                std::uint32_t materialSlotCount = 0U;

                if (!ReadMeshMaterialSlotCount(
                        meshPath,
                        materialSlotCount))
                {
                    return false;
                }

                const std::filesystem::path directory =
                    meshPath.parent_path() /
                    L"Materials";

                std::error_code error;

                if (
                    !std::filesystem::is_directory(
                        directory,
                        error) ||
                    error)
                {
                    return false;
                }

                const std::wstring prefix =
                    Lowercase(
                        meshPath.stem().wstring() +
                        L"_");

                std::vector<std::uint8_t> foundSlots(
                    materialSlotCount,
                    0U);

                std::uint32_t foundSlotCount = 0U;

                for (
                    std::filesystem::directory_iterator
                        iterator(directory, error),
                        end;

                    !error &&
                    iterator != end;

                    iterator.increment(error))
                {
                    if (
                        !iterator->is_regular_file(error) ||
                        error)
                    {
                        error.clear();
                        continue;
                    }

                    const std::filesystem::path file =
                        iterator->path();

                    if (
                        Lowercase(
                            file.extension().wstring()) !=
                            L".material")
                    {
                        continue;
                    }

                    const std::wstring filename =
                        Lowercase(
                            file.filename().wstring());

                    std::uint32_t slot = 0U;

                    if (!ParseMaterialSlot(
                            filename,
                            prefix,
                            materialSlotCount,
                            slot))
                    {
                        continue;
                    }

                    const std::uintmax_t size =
                        std::filesystem::file_size(
                            file,
                            error);

                    if (
                        !error &&
                        size >= 192U)
                    {
                        if (foundSlots[slot] == 0U)
                        {
                            foundSlots[slot] = 1U;
                            ++foundSlotCount;
                        }

                        if (
                            foundSlotCount ==
                            materialSlotCount)
                        {
                            return true;
                        }
                    }

                    error.clear();
                }
            }
            catch (...)
            {
            }

            return false;
        }

        [[nodiscard]]
        CachedMesh ResolveMesh(
            const MeshReference& reference,
            LegacyLevelLoadStats& stats,
            std::unordered_map<
                std::wstring,
                CachedMesh>& meshCache,
            std::unordered_set<std::wstring>&
                copiedTextureDirectories,
            std::vector<std::string>& errors)
        {
            const std::wstring key =
                Lowercase(
                    reference.logicalMeshPath.
                        generic_wstring());

            const auto existing =
                meshCache.find(key);

            if (existing != meshCache.end())
            {
                return existing->second;
            }

            ++stats.uniqueMeshes;

            CachedMesh result;
            result.path = reference.meshPath;

            /*
             * XML СѓР¶Рµ СѓРєР°Р·С‹РІР°РµС‚ РЅР° РіРѕС‚РѕРІС‹Р№ .mesh.
             */
            if (!reference.sourceReference)
            {
                result.available =
                    MeshFileExists(
                        reference.meshPath);

                if (result.available)
                {
                    ++stats.cachedMeshes;
                }
                else
                {
                    ++stats.failedMeshes;

                    AddError(
                        errors,
                        "Missing or invalid .mesh:",
                        reference.meshPath);
                }

                meshCache.emplace(
                    key,
                    result);

                return result;
            }

            std::error_code filesystemError;

            if (
                !std::filesystem::is_regular_file(
                    reference.sourcePath,
                    filesystemError) ||
                filesystemError ||
                !HasScbSignature(
                    reference.sourcePath))
            {
                if (
                    MeshFileExists(
                        reference.meshPath) &&
                    MaterialSetExists(
                        reference.meshPath))
                {
                    result.available = true;

                    ++stats.cachedMeshes;

                    meshCache.emplace(
                        key,
                        result);

                    return result;
                }

                ++stats.missingMeshes;

                AddError(
                    errors,
                    "Missing or invalid SCB:",
                    reference.sourcePath);

                meshCache.emplace(
                    key,
                    result);

                return result;
            }

            if (!PreparePackage(
                    reference,
                    copiedTextureDirectories,
                    errors))
            {
                ++stats.failedMeshes;

                meshCache.emplace(
                    key,
                    result);

                return result;
            }

            bool needsImport =
                !MeshFileExists(
                    reference.meshPath) ||
                !MaterialSetExists(
                    reference.meshPath);
            filesystemError.clear();

            if (
                MeshFileExists(
                    reference.meshPath) &&
                MaterialSetExists(
                    reference.meshPath))
            {
                const auto sourceTime =
                    std::filesystem::last_write_time(
                        reference.sourcePath,
                        filesystemError);

                if (!filesystemError)
                {
                    const auto meshTime =
                        std::filesystem::last_write_time(
                            reference.meshPath,
                            filesystemError);

                    if (
                        !filesystemError &&
                        meshTime >= sourceTime)
                    {
                        needsImport = false;
                    }
                }
            }

            if (needsImport)
            {
                std::wstring importError;

                const engine::assets::AssetResult importResult =
                    engine::assets::LegacyMeshImporter::Import(
                        reference.sourcePath,
                        reference.meshPath,
                        importError);

                if (
                    engine::assets::Failed(
                        importResult) ||
                    !MeshFileExists(
                        reference.meshPath) ||
                    !MaterialSetExists(
                        reference.meshPath))
                {
                    ++stats.failedMeshes;

                    AddError(
                        errors,
                        "SCB mesh/material conversion failed:",
                        reference.sourcePath);

                    meshCache.emplace(
                        key,
                        result);

                    return result;
                }

                ++stats.convertedMeshes;
            }
            else
            {
                ++stats.cachedMeshes;
            }

            result.available = true;

            meshCache.emplace(
                key,
                result);

            return result;
        }

        struct RoadDiskVertex final
        {
            float position[3U];
            float texcoord[2U];
            float normal[3U];
            std::int32_t controlPoint = 0;
        };

        static_assert(sizeof(RoadDiskVertex) == 36U);

        template<typename Value>
        [[nodiscard]]
        bool ReadBinary(
            std::ifstream& input,
            Value& value) noexcept
        {
            return
                input.read(
                    reinterpret_cast<char*>(&value),
                    sizeof(value)).good();
        }

        template<typename Value>
        [[nodiscard]]
        bool ReadHeaderValue(
            const std::vector<std::byte>& header,
            const std::size_t offset,
            Value& value) noexcept
        {
            if (
                offset > header.size() ||
                sizeof(value) > header.size() - offset)
            {
                return false;
            }

            std::memcpy(
                &value,
                header.data() + offset,
                sizeof(value));

            return true;
        }

        [[nodiscard]]
        std::array<float, 3U> NormalizeVector(
            const std::array<float, 3U>& value,
            const std::array<float, 3U>& fallback) noexcept
        {
            const float lengthSquared =
                value[0] * value[0] +
                value[1] * value[1] +
                value[2] * value[2];

            if (
                !std::isfinite(lengthSquared) ||
                lengthSquared <= 1.0e-12F)
            {
                return fallback;
            }

            const float inverseLength =
                1.0F / std::sqrt(lengthSquared);

            return
            {
                value[0] * inverseLength,
                value[1] * inverseLength,
                value[2] * inverseLength
            };
        }

        void BuildTangents(
            std::vector<engine::assets::StaticMeshVertex>& vertices,
            const std::vector<std::uint32_t>& indices) noexcept
        {
            std::vector<std::array<float, 3U>> accumulated(
                vertices.size());

            for (
                std::size_t index = 0U;
                index + 2U < indices.size();
                index += 3U)
            {
                const std::uint32_t index0 = indices[index];
                const std::uint32_t index1 = indices[index + 1U];
                const std::uint32_t index2 = indices[index + 2U];

                if (
                    index0 >= vertices.size() ||
                    index1 >= vertices.size() ||
                    index2 >= vertices.size())
                {
                    continue;
                }

                const auto& vertex0 = vertices[index0];
                const auto& vertex1 = vertices[index1];
                const auto& vertex2 = vertices[index2];

                const std::array<float, 3U> edge1
                {
                    vertex1.position[0] - vertex0.position[0],
                    vertex1.position[1] - vertex0.position[1],
                    vertex1.position[2] - vertex0.position[2]
                };

                const std::array<float, 3U> edge2
                {
                    vertex2.position[0] - vertex0.position[0],
                    vertex2.position[1] - vertex0.position[1],
                    vertex2.position[2] - vertex0.position[2]
                };

                const float deltaU1 =
                    vertex1.texcoord0[0] - vertex0.texcoord0[0];
                const float deltaV1 =
                    vertex1.texcoord0[1] - vertex0.texcoord0[1];
                const float deltaU2 =
                    vertex2.texcoord0[0] - vertex0.texcoord0[0];
                const float deltaV2 =
                    vertex2.texcoord0[1] - vertex0.texcoord0[1];
                const float denominator =
                    deltaU1 * deltaV2 - deltaU2 * deltaV1;

                if (
                    !std::isfinite(denominator) ||
                    std::abs(denominator) <= 1.0e-10F)
                {
                    continue;
                }

                const float scale = 1.0F / denominator;
                const std::array<float, 3U> tangent
                {
                    (edge1[0] * deltaV2 - edge2[0] * deltaV1) * scale,
                    (edge1[1] * deltaV2 - edge2[1] * deltaV1) * scale,
                    (edge1[2] * deltaV2 - edge2[2] * deltaV1) * scale
                };

                for (const std::uint32_t vertexIndex :
                    {index0, index1, index2})
                {
                    accumulated[vertexIndex][0] += tangent[0];
                    accumulated[vertexIndex][1] += tangent[1];
                    accumulated[vertexIndex][2] += tangent[2];
                }
            }

            for (
                std::size_t index = 0U;
                index < vertices.size();
                ++index)
            {
                auto& vertex = vertices[index];
                const auto normal = NormalizeVector(
                    vertex.normal,
                    {0.0F, 1.0F, 0.0F});

                const float projection =
                    accumulated[index][0] * normal[0] +
                    accumulated[index][1] * normal[1] +
                    accumulated[index][2] * normal[2];

                const auto tangent = NormalizeVector(
                    {
                        accumulated[index][0] - normal[0] * projection,
                        accumulated[index][1] - normal[1] * projection,
                        accumulated[index][2] - normal[2] * projection
                    },
                    {1.0F, 0.0F, 0.0F});

                vertex.normal = normal;
                vertex.tangent =
                {
                    tangent[0],
                    tangent[1],
                    tangent[2],
                    1.0F
                };
            }
        }

        [[nodiscard]]
        bool WriteAssetData(
            const std::filesystem::path& path,
            const engine::assets::AssetData& data,
            std::wstring& error)
        {
            std::error_code filesystemError;

            std::filesystem::create_directories(
                path.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error = L"Cannot create generated asset directory.";
                return false;
            }

            std::filesystem::path temporaryPath = path;
            temporaryPath += L".tmp";

            std::filesystem::remove(
                temporaryPath,
                filesystemError);

            {
                std::ofstream output(
                    temporaryPath,
                    std::ios::binary |
                        std::ios::trunc);

                if (!output)
                {
                    error = L"Cannot create generated asset.";
                    return false;
                }

                output.write(
                    reinterpret_cast<const char*>(
                        data.GetData()),
                    static_cast<std::streamsize>(
                        data.GetSize()));

                output.flush();

                if (!output.good())
                {
                    output.close();
                    std::filesystem::remove(
                        temporaryPath,
                        filesystemError);
                    error = L"Cannot write generated asset.";
                    return false;
                }
            }

            if (!MoveFileExW(
                    temporaryPath.c_str(),
                    path.c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(
                    temporaryPath,
                    filesystemError);
                error = L"Cannot replace generated asset.";
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool WriteMesh(
            const std::filesystem::path& path,
            const engine::assets::MeshAsset& mesh,
            std::wstring& error)
        {
            engine::assets::AssetData encoded;

            if (engine::assets::Failed(
                    engine::assets::LtsMeshWriter::Encode(
                        mesh,
                        encoded)))
            {
                error = L"Cannot encode generated mesh.";
                return false;
            }

            return WriteAssetData(path, encoded, error);
        }

        [[nodiscard]]
        bool OutputIsCurrent(
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& destinationPath) noexcept
        {
            try
            {
                if (
                    !MeshFileExists(destinationPath) ||
                    !MaterialSetExists(destinationPath))
                {
                    return false;
                }

                std::error_code error;
                const auto sourceTime =
                    std::filesystem::last_write_time(
                        sourcePath,
                        error);

                if (error)
                {
                    return false;
                }

                const auto destinationTime =
                    std::filesystem::last_write_time(
                        destinationPath,
                        error);

                return
                    !error &&
                    destinationTime >= sourceTime;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        std::filesystem::path BuildGeneratedMeshRelativePath(
            const std::filesystem::path& levelRoot,
            const wchar_t* const category,
            const std::filesystem::path& sourceName)
        {
            std::filesystem::path filename =
                sourceName.filename();

            filename.replace_extension(L".mesh");

            return
                std::filesystem::path(
                    LevelDataMeshDirectory) /
                levelRoot.filename() /
                category /
                filename;
        }

        [[nodiscard]]
        std::filesystem::path BuildGeneratedMeshPath(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelRoot,
            const wchar_t* const category,
            const std::filesystem::path& sourceName)
        {
            return
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"StaticMeshes" /
                BuildGeneratedMeshRelativePath(
                    levelRoot,
                    category,
                    sourceName);
        }

        [[nodiscard]]
        std::filesystem::path BuildGeneratedLogicalMeshPath(
            const std::filesystem::path& levelRoot,
            const wchar_t* const category,
            const std::filesystem::path& sourceName)
        {
            return
                std::filesystem::path(L"Data") /
                L"StaticMeshes" /
                BuildGeneratedMeshRelativePath(
                    levelRoot,
                    category,
                    sourceName);
        }

        [[nodiscard]]
        bool ImportRoadMesh(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& settingsSourcePath,
            const std::filesystem::path& destinationPath,
            const std::array<float, 3U>& origin,
            bool& converted,
            std::wstring& error)
        {
            converted = false;

            std::ifstream input(sourcePath, std::ios::binary);

            if (!input)
            {
                error = L"Road data file is missing.";
                return false;
            }

            std::uint16_t version = 0U;
            std::uint32_t headerLength = 0U;

            if (
                !ReadBinary(input, version) ||
                version != 4U ||
                !ReadBinary(input, headerLength) ||
                headerLength < 29U ||
                headerLength > 4096U)
            {
                error = L"Unsupported or corrupt road header.";
                return false;
            }

            std::vector<std::byte> header(headerLength);
            std::memcpy(
                header.data(),
                &headerLength,
                sizeof(headerLength));

            if (!input.read(
                    reinterpret_cast<char*>(
                        header.data() + sizeof(headerLength)),
                    static_cast<std::streamsize>(
                        header.size() - sizeof(headerLength))).good())
            {
                error = L"Road header is truncated.";
                return false;
            }

            std::uint32_t pointCount = 0U;
            std::uint32_t vertexCount = 0U;
            std::uint32_t indexCount = 0U;

            if (
                !ReadHeaderValue(header, 4U, pointCount) ||
                !ReadHeaderValue(header, 16U, vertexCount) ||
                !ReadHeaderValue(header, 20U, indexCount) ||
                pointCount < 2U ||
                pointCount > 1000000U ||
                vertexCount == 0U ||
                vertexCount > 16000000U ||
                indexCount == 0U ||
                indexCount > 48000000U ||
                indexCount % 3U != 0U)
            {
                error = L"Road mesh counts are invalid.";
                return false;
            }

            const std::uint64_t expectedFileSize =
                2U +
                static_cast<std::uint64_t>(headerLength) +
                static_cast<std::uint64_t>(pointCount) * 28U +
                static_cast<std::uint64_t>(vertexCount) *
                    sizeof(RoadDiskVertex) +
                static_cast<std::uint64_t>(indexCount) *
                    sizeof(std::int32_t);
            std::error_code fileSizeError;
            const std::uintmax_t actualFileSize =
                std::filesystem::file_size(
                    sourcePath,
                    fileSizeError);

            if (
                fileSizeError ||
                actualFileSize != expectedFileSize)
            {
                error = L"Road data size does not match its header.";
                return false;
            }

            const char* const materialBegin =
                reinterpret_cast<const char*>(
                    header.data() + 28U);
            const std::size_t materialCapacity =
                header.size() - 28U;
            const void* const terminator =
                std::memchr(
                    materialBegin,
                    '\0',
                    materialCapacity);

            if (terminator == nullptr)
            {
                error = L"Road material name is invalid.";
                return false;
            }

            const std::string materialName(
                materialBegin,
                static_cast<const char*>(terminator));

            const std::filesystem::path sourceMaterialPath =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"ObjectsDepot" /
                L"_roads" /
                L"Materials" /
                std::filesystem::u8path(
                    materialName.empty()
                        ? "_DEFAULT_.mat"
                        : materialName + ".mat");

            std::error_code materialFileError;
            const bool sourceMaterialExists =
                std::filesystem::is_regular_file(
                    sourceMaterialPath,
                    materialFileError) &&
                !materialFileError;

            if (
                OutputIsCurrent(sourcePath, destinationPath) &&
                OutputIsCurrent(
                    settingsSourcePath,
                    destinationPath) &&
                (!sourceMaterialExists ||
                    OutputIsCurrent(
                        sourceMaterialPath,
                        destinationPath)))
            {
                return true;
            }

            constexpr std::uint64_t PointSize = 28U;
            const std::uint64_t pointBytes =
                static_cast<std::uint64_t>(pointCount) *
                PointSize;

            if (
                pointBytes >
                static_cast<std::uint64_t>(
                    (std::numeric_limits<std::streamoff>::max)()))
            {
                error = L"Road control point data is too large.";
                return false;
            }

            input.seekg(
                static_cast<std::streamoff>(pointBytes),
                std::ios::cur);

            if (!input.good())
            {
                error = L"Road control point data is truncated.";
                return false;
            }

            std::vector<engine::assets::StaticMeshVertex>
                vertices(vertexCount);

            for (
                std::uint32_t index = 0U;
                index < vertexCount;
                ++index)
            {
                RoadDiskVertex sourceVertex;

                if (!ReadBinary(input, sourceVertex))
                {
                    error = L"Road vertex data is truncated.";
                    return false;
                }

                auto& destinationVertex = vertices[index];
                destinationVertex.position =
                {
                    sourceVertex.position[0] - origin[0],
                    sourceVertex.position[1] - origin[1],
                    sourceVertex.position[2] - origin[2]
                };
                destinationVertex.normal =
                {
                    sourceVertex.normal[0],
                    sourceVertex.normal[1],
                    sourceVertex.normal[2]
                };
                destinationVertex.texcoord0 =
                {
                    sourceVertex.texcoord[0],
                    sourceVertex.texcoord[1]
                };
            }

            std::vector<std::uint32_t> indices(indexCount);

            for (
                std::uint32_t index = 0U;
                index < indexCount;
                ++index)
            {
                std::int32_t sourceIndex = 0;

                if (
                    !ReadBinary(input, sourceIndex) ||
                    sourceIndex < 0 ||
                    static_cast<std::uint32_t>(sourceIndex) >=
                        vertexCount)
                {
                    error = L"Road index data is invalid.";
                    return false;
                }

                indices[index] =
                    static_cast<std::uint32_t>(sourceIndex);
            }

            BuildTangents(vertices, indices);

            const engine::assets::MeshSubmesh submesh
            {
                0U,
                indexCount,
                0,
                0U
            };

            engine::assets::MeshAsset mesh;

            const std::string debugName =
                sourcePath.stem().u8string();

            if (engine::assets::Failed(
                    engine::assets::MeshAssetBuilder::Build(
                        vertices.data(),
                        vertices.size(),
                        indices.data(),
                        indices.size(),
                        &submesh,
                        1U,
                        1U,
                        debugName,
                        mesh)) ||
                !WriteMesh(destinationPath, mesh, error))
            {
                if (error.empty())
                {
                    error = L"Cannot build road mesh.";
                }
                return false;
            }

            const std::filesystem::path materialSource =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"ObjectsDepot" /
                L"_roads" /
                L"road_material_source.scb";

            std::wstring materialError;

            if (engine::assets::Failed(
                    engine::assets::ScbMaterialConverter::Convert(
                        materialSource,
                        destinationPath,
                        {
                            materialName.empty()
                                ? "__default"
                                : materialName
                        },
                        materialError)))
            {
                error = materialError.empty()
                    ? L"Cannot convert road material."
                    : std::move(materialError);
                return false;
            }

            converted = true;
            return true;
        }

        [[nodiscard]]
        std::filesystem::path ResolveLegacyDataPath(
            const std::filesystem::path& workspaceRoot,
            const std::string& logicalPath)
        {
            std::filesystem::path path =
                std::filesystem::u8path(logicalPath).
                    lexically_normal();

            auto component = path.begin();

            if (
                component != path.end() &&
                Lowercase(component->wstring()) == L"data")
            {
                ++component;
                std::filesystem::path relative;

                for (; component != path.end(); ++component)
                {
                    relative /= *component;
                }

                path = std::move(relative);
            }

            if (!IsSafeRelativePath(path))
            {
                return {};
            }

            return
                workspaceRoot /
                L"bin" /
                L"Data" /
                path;
        }

        [[nodiscard]]
        bool CopyMaterialTexture(
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& destinationMeshPath,
            const std::filesystem::path& gameRoot,
            std::optional<engine::assets::AssetPath>& outputPath)
        {
            outputPath.reset();

            if (sourcePath.empty())
            {
                return true;
            }

            std::error_code error;

            if (
                !std::filesystem::is_regular_file(
                    sourcePath,
                    error) ||
                error)
            {
                return true;
            }

            const std::filesystem::path destinationPath =
                destinationMeshPath.parent_path() /
                L"Textures" /
                sourcePath.filename();

            std::filesystem::create_directories(
                destinationPath.parent_path(),
                error);

            if (error)
            {
                return false;
            }

            std::filesystem::copy_file(
                sourcePath,
                destinationPath,
                std::filesystem::copy_options::update_existing,
                error);

            if (error)
            {
                return false;
            }

            const std::filesystem::path logicalPath =
                std::filesystem::relative(
                    destinationPath,
                    gameRoot,
                    error);

            if (error)
            {
                return false;
            }

            engine::assets::AssetPath assetPath;

            if (engine::assets::Failed(
                    engine::assets::AssetPath::TryCreate(
                        logicalPath.generic_u8string(),
                        assetPath)))
            {
                return false;
            }

            outputPath = std::move(assetPath);
            return true;
        }

        [[nodiscard]]
        std::array<float, 3U> PackedColor(
            const std::uint32_t color) noexcept
        {
            constexpr float Scale = 1.0F / 255.0F;

            return
            {
                static_cast<float>((color >> 16U) & 0xFFU) * Scale,
                static_cast<float>((color >> 8U) & 0xFFU) * Scale,
                static_cast<float>(color & 0xFFU) * Scale
            };
        }

        [[nodiscard]]
        bool WriteWaterMaterial(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& destinationMeshPath,
            const pugi::xml_node& settings,
            std::wstring& error)
        {
            engine::assets::MaterialAssetDesc description;

            const std::uint32_t packedColor =
                settings.attribute("deep_color")
                    ? settings.attribute("deep_color").as_uint()
                    : 0xFF245A73U;
            const auto color = PackedColor(packedColor);

            description.baseColorFactor =
            {
                color[0],
                color[1],
                color[2],
                0.78F
            };
            description.metallicFactor = 0.0F;
            description.roughnessFactor = 0.12F;
            description.alphaMode =
                engine::assets::MaterialAlphaMode::Blend;
            description.doubleSided = true;
            description.normalScale = 1.0F;
            description.specularIntensity =
                (std::clamp)(
                    AttributeFloat(
                        settings.attribute("specIntensity"),
                        1.0F),
                    0.0F,
                    16.0F);
            description.specularPower =
                (std::clamp)(
                    AttributeFloat(
                        settings.attribute("specular"),
                        128.0F),
                    1.0F,
                    8192.0F);
            description.reflectionFactor =
                (std::clamp)(
                    AttributeFloat(
                        settings.attribute("reflectionIntensity"),
                        1.0F),
                    0.0F,
                    16.0F);
            description.debugName = "water";
            description.sampler.filter =
                engine::graphics::TextureFilter::Anisotropic;
            description.sampler.addressU =
                engine::graphics::TextureAddressMode::Wrap;
            description.sampler.addressV =
                engine::graphics::TextureAddressMode::Wrap;
            description.sampler.addressW =
                engine::graphics::TextureAddressMode::Wrap;
            description.sampler.maximumAnisotropy = 16U;

            const std::filesystem::path gameRoot =
                workspaceRoot / L"bin" / L"Data";
            const std::filesystem::path colorTexture =
                gameRoot / L"Water" / L"LakeColor.dds";

            std::string waveTexture =
                settings.attribute("wave_tex").value();

            if (!waveTexture.empty())
            {
                waveTexture += "00.dds";
            }

            if (
                !CopyMaterialTexture(
                    colorTexture,
                    destinationMeshPath,
                    gameRoot,
                    description.baseColorTexture) ||
                !CopyMaterialTexture(
                    ResolveLegacyDataPath(
                        workspaceRoot,
                        waveTexture),
                    destinationMeshPath,
                    gameRoot,
                    description.normalTexture))
            {
                error = L"Cannot copy water material textures.";
                return false;
            }

            engine::assets::MaterialAsset material;

            if (engine::assets::Failed(
                    material.Initialize(
                        std::move(description))))
            {
                error = L"Water material is invalid.";
                return false;
            }

            engine::assets::AssetData encoded;

            if (engine::assets::Failed(
                    engine::assets::MaterialAssetWriter::Encode(
                        material,
                        encoded)))
            {
                error = L"Cannot encode water material.";
                return false;
            }

            const std::filesystem::path materialPath =
                destinationMeshPath.parent_path() /
                L"Materials" /
                (destinationMeshPath.stem().wstring() +
                    L"_0000_water.material");

            return WriteAssetData(materialPath, encoded, error);
        }

        [[nodiscard]]
        bool ImportWaterPlaneMesh(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& settingsSourcePath,
            const std::filesystem::path& destinationPath,
            const pugi::xml_node& settings,
            bool& converted,
            std::wstring& error)
        {
            converted = false;

            if (
                OutputIsCurrent(sourcePath, destinationPath) &&
                OutputIsCurrent(
                    settingsSourcePath,
                    destinationPath))
            {
                return true;
            }

            const float waterHeight =
                AttributeFloat(
                    settings.attribute("waterplaneheight"),
                    0.0F);
            const float cellSize =
                AttributeFloat(
                    settings.attribute("cellgridsize"),
                    50.0F);
            const float planeWidth =
                AttributeFloat(
                    settings.attribute("total_x_size"),
                    0.0F);
            const float planeDepth =
                AttributeFloat(
                    settings.attribute("total_z_size"),
                    0.0F);
            const float centerX =
                AttributeFloat(
                    settings.attribute("center_x"),
                    0.0F);
            const float centerZ =
                AttributeFloat(
                    settings.attribute("center_z"),
                    0.0F);
            const float textureScale =
                AttributeFloat(
                    settings.attribute("waterColorTile"),
                    0.05F);
            const int coastSmoothLevels =
                settings.attribute("coastsmoothlevels")
                    ? settings.attribute(
                        "coastsmoothlevels").as_int()
                    : 0;

            if (
                !std::isfinite(waterHeight) ||
                !std::isfinite(cellSize) ||
                !std::isfinite(planeWidth) ||
                !std::isfinite(planeDepth) ||
                !std::isfinite(centerX) ||
                !std::isfinite(centerZ) ||
                !std::isfinite(textureScale) ||
                cellSize <= 0.0F ||
                planeWidth <= 0.0F ||
                planeDepth <= 0.0F ||
                coastSmoothLevels < 0 ||
                coastSmoothLevels > 6)
            {
                error = L"Water plane settings are invalid.";
                return false;
            }

            std::ifstream input(sourcePath, std::ios::binary);

            if (!input)
            {
                error = L"Water plane data file is missing.";
                return false;
            }

            std::uint16_t version = 0U;
            std::uint32_t width = 0U;
            std::uint32_t height = 0U;

            if (
                !ReadBinary(input, version) ||
                (version != 3U && version != 4U) ||
                !ReadBinary(input, width) ||
                !ReadBinary(input, height) ||
                width == 0U ||
                height == 0U ||
                width > 65536U ||
                height > 65536U ||
                static_cast<std::uint64_t>(width) * height >
                    16000000U)
            {
                error = L"Water plane grid header is invalid.";
                return false;
            }

            std::error_code waterFileSizeError;
            const std::uintmax_t waterFileSize =
                std::filesystem::file_size(
                    sourcePath,
                    waterFileSizeError);
            const std::uint64_t expectedWaterFileSize =
                10U +
                static_cast<std::uint64_t>(width) * height;

            if (
                waterFileSizeError ||
                waterFileSize != expectedWaterFileSize)
            {
                error = L"Water plane data size does not match its header.";
                return false;
            }

            std::vector<std::uint8_t> grid(
                static_cast<std::size_t>(width) * height);

            if (!input.read(
                    reinterpret_cast<char*>(grid.data()),
                    static_cast<std::streamsize>(grid.size())).good())
            {
                error = L"Water plane grid is truncated.";
                return false;
            }

            std::vector<std::uint8_t> visited(grid.size(), 0U);

            for (
                std::uint32_t startZ = 0U;
                startZ < height;
                ++startZ)
            {
                for (
                    std::uint32_t startX = 0U;
                    startX < width;
                    ++startX)
                {
                    const std::size_t startIndex =
                        static_cast<std::size_t>(startZ) * width +
                        startX;

                    if (
                        grid[startIndex] == 0U ||
                        visited[startIndex] != 0U)
                    {
                        continue;
                    }

                    std::vector<std::size_t> region;
                    std::vector<std::size_t> pending;
                    pending.push_back(startIndex);
                    visited[startIndex] = 1U;

                    while (!pending.empty())
                    {
                        const std::size_t index = pending.back();
                        pending.pop_back();
                        region.push_back(index);

                        const std::uint32_t x =
                            static_cast<std::uint32_t>(index % width);
                        const std::uint32_t z =
                            static_cast<std::uint32_t>(index / width);

                        const auto visit =
                            [&](const std::uint32_t neighborX,
                                const std::uint32_t neighborZ)
                            {
                                const std::size_t neighborIndex =
                                    static_cast<std::size_t>(neighborZ) *
                                        width +
                                    neighborX;

                                if (
                                    grid[neighborIndex] == 0U ||
                                    visited[neighborIndex] != 0U)
                                {
                                    return;
                                }

                                visited[neighborIndex] = 1U;
                                pending.push_back(neighborIndex);
                            };

                        if (x > 0U)
                        {
                            visit(x - 1U, z);
                        }
                        if (x + 1U < width)
                        {
                            visit(x + 1U, z);
                        }
                        if (z > 0U)
                        {
                            visit(x, z - 1U);
                        }
                        if (z + 1U < height)
                        {
                            visit(x, z + 1U);
                        }
                    }

                    if (region.size() >= MinimumWaterRegionCells)
                    {
                        continue;
                    }

                    for (const std::size_t index : region)
                    {
                        grid[index] = 0U;
                    }
                }
            }

            const std::size_t activeCellCount =
                static_cast<std::size_t>(
                    std::count_if(
                        grid.begin(),
                        grid.end(),
                        [](const std::uint8_t value)
                        {
                            return value != 0U;
                        }));

            if (
                activeCellCount == 0U ||
                activeCellCount >
                    (std::numeric_limits<std::uint32_t>::max)() /
                        6U)
            {
                error = L"Water plane has no valid cells.";
                return false;
            }

            std::vector<engine::assets::StaticMeshVertex> vertices;
            std::vector<std::uint32_t> indices;
            vertices.reserve(activeCellCount * 4U);
            indices.reserve(activeCellCount * 6U);

            const float offsetX =
                centerX - planeWidth * 0.5F;
            const float offsetZ =
                centerZ - planeDepth * 0.5F;

            const auto isActiveCell =
                [&](const std::int32_t x,
                    const std::int32_t z) noexcept
                {
                    return
                        x >= 0 &&
                        z >= 0 &&
                        x < static_cast<std::int32_t>(width) &&
                        z < static_cast<std::int32_t>(height) &&
                        grid[
                            static_cast<std::size_t>(z) * width +
                            static_cast<std::size_t>(x)] != 0U;
                };

            const float coastSmoothAmount =
                (std::min)(
                    0.65F,
                    static_cast<float>(coastSmoothLevels) *
                        0.25F);

            const auto gridPoint =
                [&](const std::uint32_t gridX,
                    const std::uint32_t gridZ) noexcept
                {
                    std::array<float, 2U> point
                    {
                        offsetX +
                            static_cast<float>(gridX) * cellSize,
                        offsetZ +
                            static_cast<float>(gridZ) * cellSize
                    };

                    if (coastSmoothAmount <= 0.0F)
                    {
                        return point;
                    }

                    float centerSumX = 0.0F;
                    float centerSumZ = 0.0F;
                    std::uint32_t activeCount = 0U;

                    for (const std::int32_t cellZ :
                        {
                            static_cast<std::int32_t>(gridZ) - 1,
                            static_cast<std::int32_t>(gridZ)
                        })
                    {
                        for (const std::int32_t cellX :
                            {
                                static_cast<std::int32_t>(gridX) - 1,
                                static_cast<std::int32_t>(gridX)
                            })
                        {
                            if (!isActiveCell(cellX, cellZ))
                            {
                                continue;
                            }

                            centerSumX +=
                                offsetX +
                                (static_cast<float>(cellX) + 0.5F) *
                                    cellSize;
                            centerSumZ +=
                                offsetZ +
                                (static_cast<float>(cellZ) + 0.5F) *
                                    cellSize;
                            ++activeCount;
                        }
                    }

                    if (
                        activeCount == 0U ||
                        activeCount == 4U)
                    {
                        return point;
                    }

                    const float inverseCount =
                        1.0F /
                        static_cast<float>(activeCount);
                    const float targetX =
                        centerSumX * inverseCount;
                    const float targetZ =
                        centerSumZ * inverseCount;

                    point[0] +=
                        (targetX - point[0]) *
                        coastSmoothAmount;
                    point[1] +=
                        (targetZ - point[1]) *
                        coastSmoothAmount;

                    return point;
                };

            const auto coastFactor =
                [&](const std::uint32_t gridX,
                    const std::uint32_t gridZ) noexcept
                {
                    std::uint32_t activeCount = 0U;

                    for (const std::int32_t cellZ :
                        {
                            static_cast<std::int32_t>(gridZ) - 1,
                            static_cast<std::int32_t>(gridZ)
                        })
                    {
                        for (const std::int32_t cellX :
                            {
                                static_cast<std::int32_t>(gridX) - 1,
                                static_cast<std::int32_t>(gridX)
                            })
                        {
                            if (isActiveCell(cellX, cellZ))
                            {
                                ++activeCount;
                            }
                        }
                    }

                    return
                        activeCount < 4U
                            ? 1.0F
                            : 0.0F;
                };

            for (
                std::uint32_t z = 0U;
                z < height;
                ++z)
            {
                for (
                    std::uint32_t x = 0U;
                    x < width;
                    ++x)
                {
                    if (grid[static_cast<std::size_t>(z) * width + x] == 0U)
                    {
                        continue;
                    }

                    const std::uint32_t firstVertex =
                        static_cast<std::uint32_t>(vertices.size());

                    for (const std::array<std::uint32_t, 2U>& coordinate :
                        {
                            std::array<std::uint32_t, 2U>{x, z},
                            std::array<std::uint32_t, 2U>{x, z + 1U},
                            std::array<std::uint32_t, 2U>{x + 1U, z + 1U},
                            std::array<std::uint32_t, 2U>{x + 1U, z}
                        })
                    {
                        const std::array<float, 2U> position =
                            gridPoint(
                                coordinate[0],
                                coordinate[1]);
                        engine::assets::StaticMeshVertex vertex;
                        vertex.position =
                        {
                            position[0] - centerX,
                            0.0F,
                            position[1] - centerZ
                        };
                        vertex.normal = {0.0F, 1.0F, 0.0F};
                        vertex.tangent =
                        {
                            1.0F,
                            0.0F,
                            0.0F,
                            coastFactor(
                                coordinate[0],
                                coordinate[1])
                        };
                        vertex.texcoord0 =
                        {
                            position[0] * textureScale,
                            position[1] * textureScale
                        };
                        vertices.push_back(vertex);
                    }

                    indices.insert(
                        indices.end(),
                        {
                            firstVertex,
                            firstVertex + 1U,
                            firstVertex + 2U,
                            firstVertex,
                            firstVertex + 2U,
                            firstVertex + 3U
                        });
                }
            }

            const engine::assets::MeshSubmesh submesh
            {
                0U,
                static_cast<std::uint32_t>(indices.size()),
                0,
                0U
            };

            engine::assets::MeshAsset mesh;

            if (engine::assets::Failed(
                    engine::assets::MeshAssetBuilder::Build(
                        vertices.data(),
                        vertices.size(),
                        indices.data(),
                        indices.size(),
                        &submesh,
                        1U,
                        1U,
                        sourcePath.stem().u8string(),
                        mesh)) ||
                !WriteMesh(destinationPath, mesh, error) ||
                !WriteWaterMaterial(
                    workspaceRoot,
                    destinationPath,
                    settings,
                    error))
            {
                if (error.empty())
                {
                    error = L"Cannot build water plane mesh.";
                }
                return false;
            }

            converted = true;
            return true;
        }

        [[nodiscard]]
        bool SaveLevelData(
            const std::filesystem::path& levelDataPath,
            const pugi::xml_document& document,
            std::string& error)
        {
            std::filesystem::path temporaryPath =
                levelDataPath;

            temporaryPath +=
                L".mesh_migration.tmp";

            std::filesystem::path backupPath =
                levelDataPath;

            backupPath +=
                L".before_mesh_migration.bak";

            std::error_code filesystemError;

            std::filesystem::remove(
                temporaryPath,
                filesystemError);

            const std::string temporaryUtf8 =
                temporaryPath.u8string();

            if (!document.save_file(
                    temporaryUtf8.c_str(),
                    "\t",
                    pugi::format_default,
                    pugi::encoding_utf8))
            {
                error =
                    "Cannot write temporary LevelData.xml.";

                return false;
            }

            /*
             * Backup СЃРѕР·РґР°С‘С‚СЃСЏ С‚РѕР»СЊРєРѕ РѕРґРёРЅ СЂР°Р·.
             */
            if (!CopyFileW(
                    levelDataPath.c_str(),
                    backupPath.c_str(),
                    TRUE))
            {
                const DWORD copyError =
                    GetLastError();

                if (copyError != ERROR_FILE_EXISTS)
                {
                    std::filesystem::remove(
                        temporaryPath,
                        filesystemError);

                    error =
                        "Cannot create LevelData.xml backup.";

                    return false;
                }
            }

            /*
             * РћСЂРёРіРёРЅР°Р»СЊРЅС‹Р№ XML Р·Р°РјРµРЅСЏРµС‚СЃСЏ С‚РѕР»СЊРєРѕ РїРѕСЃР»Рµ РїРѕР»РЅРѕР№
             * РєРѕРЅРІРµСЂС‚Р°С†РёРё РІСЃРµС… СѓРЅРёРєР°Р»СЊРЅС‹С… SCB.
             */
            if (!MoveFileExW(
                    temporaryPath.c_str(),
                    levelDataPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(
                    temporaryPath,
                    filesystemError);

                error =
                    "Cannot replace LevelData.xml.";

                return false;
            }

            return true;
        }

        [[nodiscard]]
        std::string BuildWarningMessage(
            const LegacyLevelLoadStats& stats,
            const std::vector<std::string>& errors)
        {
            if (
                stats.missingMeshes == 0U &&
                stats.failedMeshes == 0U)
            {
                return {};
            }

            std::string message =
                "Level geometry import completed with warnings. ";

            message += "Missing SCB: ";
            message +=
                std::to_string(
                    stats.missingMeshes);

            message += ", failed: ";
            message +=
                std::to_string(
                    stats.failedMeshes);

            for (const std::string& item : errors)
            {
                message += "\n";
                message += item;
            }

            return message;
        }
    }

    LegacyLevelLoadResult LoadLegacyLevelData(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelDataPath,
        const std::wstring& mapName) noexcept
    {
        LegacyLevelLoadResult result;

        /*
         * mapName СЂР°РЅСЊС€Рµ РёСЃРїРѕР»СЊР·РѕРІР°Р»СЃСЏ РґР»СЏ
         * game/Cache/LegacyLevels.
         * Р­С‚РѕРіРѕ cache-РєР°С‚Р°Р»РѕРіР° Р±РѕР»СЊС€Рµ РЅРµС‚.
         */
        static_cast<void>(mapName);

        try
        {
            if (
                workspaceRoot.empty() ||
                levelDataPath.empty())
            {
                result.error =
                    "Workspace or LevelData.xml path is empty.";

                return result;
            }

            pugi::xml_document document;

            const std::string levelDataUtf8 =
                levelDataPath.u8string();

            const pugi::xml_parse_result parseResult =
                document.load_file(
                    levelDataUtf8.c_str());

            if (!parseResult)
            {
                result.error =
                    "Cannot parse LevelData.xml: ";

                result.error +=
                    parseResult.description();

                return result;
            }

            const pugi::xml_node level =
                document.child("level");

            if (!level)
            {
                result.error =
                    "LevelData.xml has no <level> root element.";

                return result;
            }

            std::vector<PendingObject>
                pendingObjects;

            pendingObjects.reserve(20000U);

            std::unordered_map<
                std::wstring,
                CachedMesh> meshCache;

            meshCache.reserve(1024U);

            std::unordered_set<std::wstring>
                copiedTextureDirectories;

            copiedTextureDirectories.reserve(
                256U);

            std::vector<std::string>
                reportedErrors;

            reportedErrors.reserve(
                MaximumReportedErrors);

            std::size_t objectIndex = 0U;

            for (
                pugi::xml_node object =
                    level.child("object");

                object;

                object =
                    object.next_sibling("object"))
            {
                ++objectIndex;
                ++result.stats.totalObjects;

                const std::wstring className =
                    ToWide(
                        object.attribute(
                            "className").
                            value());

                const std::wstring fileName =
                    ToWide(
                        object.attribute(
                            "fileName").
                            value());

                if (className == L"obj_Road")
                {
                    const std::filesystem::path sourceName(fileName);

                    if (
                        sourceName.empty() ||
                        sourceName.filename() != sourceName)
                    {
                        ++result.stats.failedMeshes;
                        AddError(
                            reportedErrors,
                            "Invalid obj_Road name:",
                            sourceName);
                        continue;
                    }

                    const std::filesystem::path sourcePath =
                        levelDataPath.parent_path() /
                        L"roads" /
                        (sourceName.wstring() + L".dat");
                    const std::filesystem::path destinationPath =
                        BuildGeneratedMeshPath(
                            workspaceRoot,
                            levelDataPath.parent_path(),
                            L"Roads",
                            sourceName);
                    bool converted = false;
                    std::wstring importError;
                    engine::scene::SceneTransform roadTransform;

                    ReadTransform(object, roadTransform);
                    roadTransform.rotationDegrees =
                        {0.0F, 0.0F, 0.0F};
                    roadTransform.scale =
                        {1.0F, 1.0F, 1.0F};

                    ++result.stats.uniqueMeshes;

                    if (!ImportRoadMesh(
                            workspaceRoot,
                            sourcePath,
                            levelDataPath,
                            destinationPath,
                            roadTransform.position,
                            converted,
                            importError))
                    {
                        ++result.stats.failedMeshes;
                        AddError(
                            reportedErrors,
                            "Road import failed:",
                            sourcePath);
                        continue;
                    }

                    if (converted)
                    {
                        ++result.stats.convertedMeshes;
                    }
                    else
                    {
                        ++result.stats.cachedMeshes;
                    }

                    PendingObject pending;
                    pending.node = object;
                    pending.originalName = fileName;
                    pending.editorFolder = L"LevelData/obj_Road";
                    pending.meshPath = destinationPath;
                    pending.logicalMeshPath =
                        BuildGeneratedLogicalMeshPath(
                            levelDataPath.parent_path(),
                            L"Roads",
                            sourceName);
                    pending.transform = roadTransform;
                    pending.available = true;
                    pending.castShadows = false;
                    pending.disableDistanceCulling = true;
                    const pugi::xml_attribute drawPriority =
                        object.child("road").attribute(
                            "draw_priority");
                    pending.renderOrder =
                        (drawPriority
                            ? drawPriority.as_int()
                            : 5) + 8;
                    pending.objectIndex = objectIndex;
                    pendingObjects.push_back(std::move(pending));
                    ++result.stats.roadObjects;
                    continue;
                }

                if (className == L"obj_WaterPlane")
                {
                    const std::filesystem::path sourceName(fileName);
                    const pugi::xml_node settings =
                        object.child("new_lake");

                    if (
                        sourceName.empty() ||
                        sourceName.filename() != sourceName ||
                        !settings)
                    {
                        ++result.stats.failedMeshes;
                        AddError(
                            reportedErrors,
                            "Invalid obj_WaterPlane entry:",
                            sourceName);
                        continue;
                    }

                    const std::filesystem::path sourcePath =
                        levelDataPath.parent_path() /
                        L"water_planes" /
                        (sourceName.wstring() + L".dat");
                    const std::filesystem::path destinationPath =
                        BuildGeneratedMeshPath(
                            workspaceRoot,
                            levelDataPath.parent_path(),
                            L"WaterPlanes",
                            sourceName);
                    bool converted = false;
                    std::wstring importError;

                    ++result.stats.uniqueMeshes;

                    if (!ImportWaterPlaneMesh(
                            workspaceRoot,
                            sourcePath,
                            levelDataPath,
                            destinationPath,
                            settings,
                            converted,
                            importError))
                    {
                        ++result.stats.failedMeshes;
                        AddError(
                            reportedErrors,
                            "Water plane import failed:",
                            sourcePath);
                        continue;
                    }

                    if (converted)
                    {
                        ++result.stats.convertedMeshes;
                    }
                    else
                    {
                        ++result.stats.cachedMeshes;
                    }

                    PendingObject pending;
                    pending.node = object;
                    pending.originalName = fileName;
                    pending.editorFolder = L"LevelData/obj_WaterPlane";
                    pending.meshPath = destinationPath;
                    pending.logicalMeshPath =
                        BuildGeneratedLogicalMeshPath(
                            levelDataPath.parent_path(),
                            L"WaterPlanes",
                            sourceName);
                    pending.transform.position =
                    {
                        AttributeFloat(
                            settings.attribute("center_x"),
                            0.0F),
                        AttributeFloat(
                            settings.attribute("waterplaneheight"),
                            0.0F),
                        AttributeFloat(
                            settings.attribute("center_z"),
                            0.0F)
                    };
                    pending.available = true;
                    pending.castShadows = false;
                    pending.disableDistanceCulling = true;
                    pending.renderOrder = 100;
                    pending.objectIndex = objectIndex;
                    pendingObjects.push_back(std::move(pending));
                    ++result.stats.waterPlaneObjects;
                    continue;
                }

                if (className != L"obj_Building")
                {
                    continue;
                }

                MeshReference reference;

                if (
                    fileName.empty() ||
                    !BuildMeshReference(
                        workspaceRoot,
                        std::filesystem::path(
                            fileName),
                        reference))
                {
                    ++result.stats.failedMeshes;

                    AddError(
                        reportedErrors,
                        "Unsupported obj_Building path:",
                        std::filesystem::path(
                            fileName));

                    continue;
                }

                const CachedMesh mesh =
                    ResolveMesh(
                        reference,
                        result.stats,
                        meshCache,
                        copiedTextureDirectories,
                        reportedErrors);

                PendingObject pending;

                pending.node = object;
                pending.originalName = fileName;
                pending.editorFolder =
                    L"LevelData/obj_Building";
                pending.meshPath = mesh.path;
                pending.available = mesh.available;
                pending.logicalMeshPath = reference.logicalMeshPath;
                pending.objectIndex = objectIndex;
                pending.rewriteXml = reference.rewriteXml;

                ReadTransform(object, pending.transform);
                pendingObjects.push_back(std::move(pending));
                ++result.stats.buildingObjects;
            }

            bool xmlChanged = false;

            for (
                const PendingObject& pending :
                pendingObjects)
            {
                if (!pending.rewriteXml)
                {
                    continue;
                }

                pugi::xml_attribute fileName =
                    pending.node.attribute(
                        "fileName");

                const std::string meshPath =
                    pending.logicalMeshPath.
                        generic_u8string();

                if (
                    !fileName ||
                    !fileName.set_value(
                        meshPath.c_str()))
                {
                    result.error =
                        "Cannot update obj_Building path. "
                        "LevelData.xml was not changed.";

                    return result;
                }

                xmlChanged = true;
            }

            if (
                xmlChanged &&
                !SaveLevelData(
                    levelDataPath,
                    document,
                    result.error))
            {
                return result;
            }

            result.entities.reserve(
                pendingObjects.size());

            for (
                const PendingObject& pending :
                pendingObjects)
            {
                engine::scene::SceneEntity entity;

                entity.kind =
                    engine::scene::
                        SceneEntityKind::Empty;

                if (!pending.available)
                {
                    continue;
                }

                entity.transform =
                    pending.transform;

                entity.name =
                    std::filesystem::path(
                        pending.originalName).
                        stem().
                        wstring();

                entity.name +=
                    L" #" +
                    std::to_wstring(
                        pending.objectIndex);

                entity.editorFolder =
                    pending.editorFolder;

                entity.staticMesh.emplace();

                const std::filesystem::path& assetPath =
                    pending.logicalMeshPath.empty()
                        ? pending.meshPath
                        : pending.logicalMeshPath;

                entity.staticMesh->assetPath =
                    assetPath.
                        lexically_normal().
                        generic_wstring();

                entity.staticMesh->visible = true;

                entity.staticMesh->castShadows =
                    pending.castShadows;

                entity.staticMesh->disableDistanceCulling =
                    pending.disableDistanceCulling;

                entity.staticMesh->renderOrder =
                    pending.renderOrder;

                result.entities.push_back(
                    std::move(entity));
            }

            result.stats.importedObjects =
                result.entities.size();

            result.stats.staticMeshObjects =
                result.entities.size();

            result.warning =BuildWarningMessage(result.stats, reportedErrors);
            result.succeeded = true;

            return result;
        }
        catch (const std::exception& exception)
        {
            result.error =
                "LevelData.xml import failed: ";

            result.error +=
                exception.what();

            return result;
        }
        catch (...)
        {
            result.error =
                "Unexpected LevelData.xml import failure.";

            return result;
        }
    }
}