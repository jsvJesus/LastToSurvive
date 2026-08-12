#include "Assets/LegacyMeshImporter.h"

#include "Assets/AssetData.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/MeshAsset.h"
#include "Assets/MeshAssetBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        constexpr std::uint32_t LegacyScbVersion =
            0xFADC0038U;

        constexpr std::uint32_t MaximumVertexCount =
            10'000'000U;

        constexpr std::uint32_t MaximumIndexCount =
            30'000'000U;

        constexpr std::uint32_t MaximumSubmeshCount =
            65'536U;

        constexpr std::int32_t MaximumStringLength =
            4096;

        [[nodiscard]] bool HasScbSignature(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::ifstream input(path, std::ios::binary);
                std::uint32_t version = 0U;

                return
                    input.read(
                        reinterpret_cast<char*>(&version),
                        sizeof(version)).good() &&
                    version == LegacyScbVersion;
            }
            catch (...)
            {
                return false;
            }
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

        void AppendAscii(
            std::wstring& destination,
            const char* const text)
        {
            if (text == nullptr)
            {
                return;
            }

            for (
                const unsigned char* character =
                    reinterpret_cast<
                        const unsigned char*>(text);
                *character != 0U;
                ++character)
            {
                destination.push_back(
                    static_cast<wchar_t>(
                        *character));
            }
        }

        void SetAssetError(
            std::wstring& error,
            const std::wstring_view operation,
            const AssetResult result)
        {
            error.assign(
                operation.begin(),
                operation.end());

            error += L": ";

            AppendAscii(
                error,
                ToString(result));
        }

        [[nodiscard]]
        bool IsFinite3(
            const std::array<float, 3U>& value) noexcept
        {
            return
                std::isfinite(value[0]) &&
                std::isfinite(value[1]) &&
                std::isfinite(value[2]);
        }

        void NormalizeDirection(
            std::array<float, 3U>& value,
            const std::array<float, 3U>& fallback) noexcept
        {
            if (!IsFinite3(value))
            {
                value = fallback;
                return;
            }

            const float lengthSquared =
                value[0] * value[0] +
                value[1] * value[1] +
                value[2] * value[2];

            if (
                !std::isfinite(lengthSquared) ||
                lengthSquared <= 0.000001F)
            {
                value = fallback;
                return;
            }

            const float inverseLength =
                1.0F /
                std::sqrt(lengthSquared);

            value[0] *= inverseLength;
            value[1] *= inverseLength;
            value[2] *= inverseLength;
        }

        void NormalizeVertex(
            StaticMeshVertex& vertex) noexcept
        {
            NormalizeDirection(
                vertex.normal,
                {
                    0.0F,
                    1.0F,
                    0.0F
                });

            std::array<float, 3U> tangent
            {
                vertex.tangent[0],
                vertex.tangent[1],
                vertex.tangent[2]
            };

            NormalizeDirection(
                tangent,
                {
                    1.0F,
                    0.0F,
                    0.0F
                });

            vertex.tangent[0] = tangent[0];
            vertex.tangent[1] = tangent[1];
            vertex.tangent[2] = tangent[2];

            vertex.tangent[3] =
                vertex.tangent[3] >= 0.0F
                    ? 1.0F
                    : -1.0F;
        }

        [[nodiscard]]
        std::uint32_t GetMaterialSlot(
            std::unordered_map<
                std::string,
                std::uint32_t>& slots,
            std::string material)
        {
            if (material.empty())
            {
                material = "__default";
            }

            const auto existing =
                slots.find(material);

            if (existing != slots.end())
            {
                return existing->second;
            }

            const std::uint32_t slot =
                static_cast<std::uint32_t>(
                    slots.size());

            slots.emplace(
                std::move(material),
                slot);

            return slot;
        }

        [[nodiscard]]
        std::string BuildDebugName(
            const std::filesystem::path& sourcePath,
            const std::string& objectName)
        {
            if (!objectName.empty())
            {
                return objectName;
            }

            return sourcePath.
                filename().
                generic_u8string();
        }

        [[nodiscard]]
        AssetResult BuildMesh(
            const std::vector<StaticMeshVertex>& vertices,
            const std::vector<std::uint32_t>& indices,
            const std::vector<MeshSubmesh>& submeshes,
            const std::uint32_t materialSlotCount,
            const std::string& debugName,
            MeshAsset& output,
            std::wstring& error)
        {
            if (
                vertices.empty() ||
                indices.empty() ||
                submeshes.empty() ||
                materialSlotCount == 0U)
            {
                error =
                    L"Legacy mesh contains no renderable geometry.";

                return AssetResult::CorruptData;
            }

            const AssetResult result =
                MeshAssetBuilder::Build(
                    vertices.data(),
                    vertices.size(),
                    indices.data(),
                    indices.size(),
                    submeshes.data(),
                    submeshes.size(),
                    materialSlotCount,
                    debugName,
                    output);

            if (Failed(result))
            {
                SetAssetError(
                    error,
                    L"MeshAssetBuilder failed",
                    result);
            }

            return result;
        }

        class BinaryReader final
        {
        public:
            explicit BinaryReader(
                const std::filesystem::path& path)
                : input_(
                    path,
                    std::ios::binary)
            {
            }

            [[nodiscard]]
            bool IsOpen() const noexcept
            {
                return input_.is_open();
            }

            template<typename ValueType>
            [[nodiscard]]
            bool Read(ValueType& value)
            {
                static_assert(
                    std::is_trivially_copyable_v<
                        ValueType>);

                return static_cast<bool>(
                    input_.read(
                        reinterpret_cast<char*>(
                            &value),
                        sizeof(ValueType)));
            }

            [[nodiscard]]
            bool ReadBytes(
                void* const destination,
                const std::size_t byteCount)
            {
                if (byteCount == 0U)
                {
                    return true;
                }

                if (destination == nullptr)
                {
                    return false;
                }

                return static_cast<bool>(
                    input_.read(
                        static_cast<char*>(
                            destination),
                        static_cast<std::streamsize>(
                            byteCount)));
            }

        private:
            std::ifstream input_;
        };

        [[nodiscard]]
        bool ReadScbString(
            BinaryReader& reader,
            std::string& value,
            std::wstring& error)
        {
            std::int32_t length = 0;

            if (
                !reader.Read(length) ||
                length < 0 ||
                length > MaximumStringLength)
            {
                error =
                    L"SCB contains an invalid string length.";

                return false;
            }

            value.resize(
                static_cast<std::size_t>(
                    length));

            if (
                length > 0 &&
                !reader.ReadBytes(
                    value.data(),
                    value.size()))
            {
                error =
                    L"SCB string data ended unexpectedly.";

                return false;
            }

            return true;
        }

        [[nodiscard]]
        AssetResult LoadScb(
            const std::filesystem::path& sourcePath,
            MeshAsset& output,
            std::vector<std::string>& materialNames,
            std::wstring& error)
        {
            BinaryReader reader(sourcePath);

            if (!reader.IsOpen())
            {
                error =
                    L"Failed to open the SCB source file.";

                return AssetResult::IoError;
            }

            std::uint32_t version = 0U;
            std::uint32_t flags = 0U;

            if (
                !reader.Read(version) ||
                !reader.Read(flags))
            {
                error =
                    L"SCB header is truncated.";

                return AssetResult::CorruptData;
            }

            if (version != LegacyScbVersion)
            {
                error =
                    L"Unsupported SCB version.";

                return AssetResult::UnsupportedFormat;
            }

            if ((flags & 1U) != 0U)
            {
                error =
                    L"Skinned SCB meshes are not supported by the static mesh importer.";

                return AssetResult::UnsupportedFeature;
            }

            if ((flags & ~2U) != 0U)
            {
                error =
                    L"SCB contains unsupported flags.";

                return AssetResult::UnsupportedFeature;
            }

            std::string objectName;

            if (!ReadScbString(
                    reader,
                    objectName,
                    error))
            {
                return AssetResult::CorruptData;
            }

            std::array<float, 3U> pivot{};

            if (!reader.ReadBytes(
                    pivot.data(),
                    sizeof(float) *
                        pivot.size()) ||
                !IsFinite3(pivot))
            {
                error =
                    L"SCB pivot is invalid.";

                return AssetResult::CorruptData;
            }

            std::int32_t vertexCountSigned = 0;

            if (
                !reader.Read(vertexCountSigned) ||
                vertexCountSigned <= 0 ||
                static_cast<std::uint32_t>(
                    vertexCountSigned) >
                    MaximumVertexCount)
            {
                error =
                    L"SCB vertex count is invalid.";

                return AssetResult::CorruptData;
            }

            const std::uint32_t vertexCount =
                static_cast<std::uint32_t>(
                    vertexCountSigned);

            std::vector<std::array<float, 3U>>
                positions(vertexCount);

            std::vector<std::array<float, 2U>>
                textureCoordinates(vertexCount);

            std::vector<std::array<float, 3U>>
                normals(vertexCount);

            std::vector<std::array<float, 3U>>
                tangents(vertexCount);

            std::vector<std::uint8_t>
                tangentSigns(vertexCount);

            if (
                !reader.ReadBytes(
                    positions.data(),
                    positions.size() *
                        sizeof(positions.front())) ||
                !reader.ReadBytes(
                    textureCoordinates.data(),
                    textureCoordinates.size() *
                        sizeof(textureCoordinates.front())) ||
                !reader.ReadBytes(
                    normals.data(),
                    normals.size() *
                        sizeof(normals.front())) ||
                !reader.ReadBytes(
                    tangents.data(),
                    tangents.size() *
                        sizeof(tangents.front())) ||
                !reader.ReadBytes(
                    tangentSigns.data(),
                    tangentSigns.size()))
            {
                error =
                    L"SCB vertex arrays ended unexpectedly.";

                return AssetResult::CorruptData;
            }

            std::int32_t indexCountSigned = 0;

            if (
                !reader.Read(indexCountSigned) ||
                indexCountSigned <= 0 ||
                indexCountSigned % 3 != 0 ||
                static_cast<std::uint32_t>(
                    indexCountSigned) >
                    MaximumIndexCount)
            {
                error =
                    L"SCB index count is invalid.";

                return AssetResult::CorruptData;
            }

            const std::uint32_t indexCount =
                static_cast<std::uint32_t>(
                    indexCountSigned);

            std::vector<std::uint32_t>
                indices(indexCount);

            if (!reader.ReadBytes(
                    indices.data(),
                    indices.size() *
                        sizeof(indices.front())))
            {
                error =
                    L"SCB index data ended unexpectedly.";

                return AssetResult::CorruptData;
            }

            for (const std::uint32_t index : indices)
            {
                if (index >= vertexCount)
                {
                    error =
                        L"SCB contains an invalid vertex index.";

                    return AssetResult::CorruptData;
                }
            }

            std::int32_t materialChunkCountSigned = 0;

            if (
                !reader.Read(
                    materialChunkCountSigned) ||
                materialChunkCountSigned < 0 ||
                materialChunkCountSigned >
                    static_cast<std::int32_t>(
                        MaximumSubmeshCount))
            {
                error =
                    L"SCB material chunk count is invalid.";

                return AssetResult::CorruptData;
            }

            std::vector<MeshSubmesh> submeshes;

            std::unordered_map<
                std::string,
                std::uint32_t> materialSlots;

            std::uint32_t expectedStart = 0U;

            for (
                std::int32_t chunkIndex = 0;
                chunkIndex <
                    materialChunkCountSigned;
                ++chunkIndex)
            {
                std::int32_t startIndex = 0;
                std::int32_t endIndex = 0;

                if (
                    !reader.Read(startIndex) ||
                    !reader.Read(endIndex))
                {
                    error =
                        L"SCB material chunk is truncated.";

                    return AssetResult::CorruptData;
                }

                std::string materialName;

                if (!ReadScbString(
                        reader,
                        materialName,
                        error))
                {
                    return AssetResult::CorruptData;
                }

                if (
                    startIndex < 0 ||
                    endIndex <= startIndex ||
                    static_cast<std::uint32_t>(
                        startIndex) != expectedStart ||
                    static_cast<std::uint32_t>(
                        endIndex) > indexCount)
                {
                    error =
                        L"SCB material chunk range is invalid.";

                    return AssetResult::CorruptData;
                }

                MeshSubmesh submesh;

                submesh.firstIndex =
                    static_cast<std::uint32_t>(
                        startIndex);

                submesh.indexCount =
                    static_cast<std::uint32_t>(
                        endIndex - startIndex);

                submesh.baseVertex = 0;

                submesh.materialSlot =
                    GetMaterialSlot(
                        materialSlots,
                        materialName);

                submeshes.push_back(
                    submesh);

                expectedStart =
                    static_cast<std::uint32_t>(
                        endIndex);
            }

            if (submeshes.empty())
            {
                MeshSubmesh submesh;

                submesh.firstIndex = 0U;
                submesh.indexCount = indexCount;
                submesh.baseVertex = 0;
                submesh.materialSlot = 0U;

                submeshes.push_back(
                    submesh);

                materialSlots.emplace(
                    "__default",
                    0U);
            }
            else if (expectedStart != indexCount)
            {
                error =
                    L"SCB material chunks do not cover the complete index buffer.";

                return AssetResult::CorruptData;
            }

            std::vector<StaticMeshVertex>
                vertices(vertexCount);

            for (
                std::uint32_t index = 0U;
                index < vertexCount;
                ++index)
            {
                StaticMeshVertex vertex{};

                vertex.position =
                {
                    positions[index][0] - pivot[0],
                    positions[index][1] - pivot[1],
                    positions[index][2] - pivot[2]
                };

                vertex.normal =
                    normals[index];

                vertex.tangent =
                {
                    tangents[index][0],
                    tangents[index][1],
                    tangents[index][2],
                    tangentSigns[index] > 127U
                        ? 1.0F
                        : -1.0F
                };

                vertex.texcoord0 =
                    textureCoordinates[index];

                NormalizeVertex(vertex);

                vertices[index] = vertex;
            }

            materialNames.assign(materialSlots.size(), {});
            for (const auto& [name, slot] : materialSlots)
            {
                if (slot < materialNames.size())
                {
                    materialNames[slot] = name;
                }
            }

            return BuildMesh(
                vertices,
                indices,
                submeshes,
                static_cast<std::uint32_t>(
                    materialSlots.size()),
                BuildDebugName(
                    sourcePath,
                    objectName),
                output,
                error);
        }

        [[nodiscard]]
        AssetResult SaveMesh(
            const std::filesystem::path& destinationPath,
            const MeshAsset& mesh,
            std::wstring& error)
        {
            AssetData encoded;

            const AssetResult encodeResult =
                LtsMeshWriter::Encode(
                    mesh,
                    encoded);

            if (Failed(encodeResult))
            {
                SetAssetError(
                    error,
                    L"LtsMeshWriter failed",
                    encodeResult);

                return encodeResult;
            }

            std::error_code filesystemError;

            if (!destinationPath.parent_path().empty())
            {
                std::filesystem::create_directories(
                    destinationPath.parent_path(),
                    filesystemError);

                if (filesystemError)
                {
                    error =
                        L"Failed to create the destination mesh directory.";

                    return AssetResult::IoError;
                }
            }

            std::filesystem::path temporaryPath =
                destinationPath;

            temporaryPath += L".tmp";

            std::filesystem::remove(
                temporaryPath,
                filesystemError);

            filesystemError.clear();

            {
                std::ofstream output(
                    temporaryPath,
                    std::ios::binary |
                    std::ios::trunc);

                if (!output)
                {
                    error = L"Failed to create the temporary mesh file.";

                    return AssetResult::IoError;
                }

                output.write(
                    reinterpret_cast<const char*>(
                        encoded.GetData()),
                    static_cast<std::streamsize>(
                        encoded.GetSize()));

                output.flush();

                if (!output.good())
                {
                    error = L"Failed to write the complete mesh file.";

                    output.close();

                    std::filesystem::remove(
                        temporaryPath,
                        filesystemError);

                    return AssetResult::IoError;
                }
            }

            std::filesystem::remove(
                destinationPath,
                filesystemError);

            filesystemError.clear();

            std::filesystem::rename(
                temporaryPath,
                destinationPath,
                filesystemError);

            if (filesystemError)
            {
                error = L"Failed to replace the destination mesh file.";

                std::filesystem::remove(
                    temporaryPath,
                    filesystemError);

                return AssetResult::IoError;
            }

            return AssetResult::Success;
        }
    }

    bool LegacyMeshImporter::IsSupportedSource(
        const std::filesystem::path& path) noexcept
    {
        try
        {
            return
                Lowercase(path.extension().wstring()) ==
                L".scb";
        }
        catch (...)
        {
            return false;
        }
    }

    AssetResult LegacyMeshImporter::Import(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& destinationPath,
        std::wstring& error) noexcept
    {
        error.clear();

        try
        {
            if (
                sourcePath.empty() ||
                destinationPath.empty())
            {
                error =
                    L"Source or destination mesh path is empty.";

                return AssetResult::InvalidPath;
            }

            if (
                Lowercase(sourcePath.extension().wstring()) !=
                L".scb")
            {
                error =
                    L"Only SCB source files are supported.";

                return AssetResult::UnsupportedFormat;
            }

            if (
                Lowercase(destinationPath.extension().wstring()) !=
                L".mesh")
            {
                error =
                    L"Converted static mesh must use the .mesh extension.";

                return AssetResult::InvalidPath;
            }

            if (!HasScbSignature(sourcePath))
            {
                error =
                    L"SCB signature 0xFADC0038 is missing.";

                return AssetResult::CorruptData;
            }

            MeshAsset mesh;
            std::vector<std::string> materialNames;

            const AssetResult loadResult =
                LoadScb(
                    sourcePath,
                    mesh,
                    materialNames,
                    error);

            if (Failed(loadResult))
            {
                return loadResult;
            }

            /*
             * materialNames пока используются SCB-парсером для правильного
             * построения materialSlot. Старый .mesh.materials sidecar
             * больше не записывается.
             */
            return SaveMesh(
                destinationPath,
                mesh,
                error);
        }
        catch (const std::bad_alloc&)
        {
            error =
                L"Not enough memory to import the SCB mesh.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            error =
                L"Unexpected SCB mesh import failure.";

            return AssetResult::InternalError;
        }
    }
}
