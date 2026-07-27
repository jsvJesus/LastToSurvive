#include "Editor/Tools/Import/LegacyMeshReader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint32_t ScbVersion =
            0xFADC0038U;

        constexpr std::uint32_t ScbHasWeights = 1U;
        constexpr std::uint32_t ScbHasVertexColors = 2U;

        constexpr std::uint32_t MaximumVertexCount =
            10000000U;

        constexpr std::uint32_t MaximumIndexCount =
            30000000U;

        constexpr std::uint32_t MaximumMaterialChunkCount =
            4096U;

        constexpr std::int32_t MaximumStringLength =
            65536;

        static_assert(
            sizeof(std::array<float, 3U>) ==
            sizeof(float) * 3U);

        static_assert(
            sizeof(std::array<float, 2U>) ==
            sizeof(float) * 2U);

        template<typename Value>
        [[nodiscard]]
        bool ReadValue(
            std::ifstream& stream,
            Value& value) noexcept
        {
            stream.read(
                reinterpret_cast<char*>(&value),
                static_cast<std::streamsize>(
                    sizeof(Value)));

            return stream.gcount() ==
                static_cast<std::streamsize>(
                    sizeof(Value));
        }

        [[nodiscard]]
        bool ReadBytes(
            std::ifstream& stream,
            void* destination,
            const std::size_t byteCount) noexcept
        {
            if (byteCount == 0U)
            {
                return true;
            }

            if (destination == nullptr ||
                byteCount >
                    static_cast<std::size_t>(
                        (std::numeric_limits<
                            std::streamsize>::max)()))
            {
                return false;
            }

            stream.read(
                static_cast<char*>(destination),
                static_cast<std::streamsize>(
                    byteCount));

            return stream.gcount() ==
                static_cast<std::streamsize>(
                    byteCount);
        }

        [[nodiscard]]
        bool FileExists(
            const std::filesystem::path& path) noexcept
        {
            if (path.empty())
            {
                return false;
            }

            std::error_code error;

            return
                std::filesystem::is_regular_file(
                    path,
                    error) &&
                !error;
        }

        [[nodiscard]]
        bool GetFileSize(
            const std::filesystem::path& path,
            std::uintmax_t& size) noexcept
        {
            std::error_code error;

            size = std::filesystem::file_size(
                path,
                error);

            return !error;
        }

        [[nodiscard]]
        bool ReadString(
            std::ifstream& stream,
            const std::int32_t length,
            std::string& output) noexcept
        {
            if (length < 0 ||
                length > MaximumStringLength)
            {
                return false;
            }

            output.clear();

            if (length == 0)
            {
                return true;
            }

            output.resize(
                static_cast<std::size_t>(length));

            return ReadBytes(
                stream,
                output.data(),
                output.size());
        }

        [[nodiscard]]
        bool IsFiniteVector(
            const std::array<float, 3U>& value) noexcept
        {
            return
                std::isfinite(value[0]) &&
                std::isfinite(value[1]) &&
                std::isfinite(value[2]);
        }

        [[nodiscard]]
        bool IsFiniteVector(
            const std::array<float, 2U>& value) noexcept
        {
            return
                std::isfinite(value[0]) &&
                std::isfinite(value[1]);
        }

        void ApplyLegacyPivot(LegacyMeshData& mesh) noexcept
        {
            const float pivotX = mesh.pivot[0];
            const float pivotY = mesh.pivot[1];
            const float pivotZ = mesh.pivot[2];

            if (!std::isfinite(pivotX) ||
                !std::isfinite(pivotY) ||
                !std::isfinite(pivotZ))
            {
                if (!mesh.warning.empty())
                {
                    mesh.warning += ' ';
                }

                mesh.warning +=
                    "Mesh pivot contains non-finite values and was ignored.";

                return;
            }
            
            for (LegacyMeshVertex& vertex : mesh.vertices)
            {
                vertex.position[0] -= pivotX;
                vertex.position[1] -= pivotY;
                vertex.position[2] -= pivotZ;
            }
        }

        void ValidateMesh(
            LegacyMeshData& mesh) noexcept
        {
            mesh.nonFiniteVertexCount = 0U;
            mesh.invalidIndexCount = 0U;
            mesh.degenerateTriangleCount = 0U;

            for (const LegacyMeshVertex& vertex :
                 mesh.vertices)
            {
                if (!IsFiniteVector(vertex.position) ||
                    !IsFiniteVector(vertex.normal) ||
                    !IsFiniteVector(vertex.tangent) ||
                    !IsFiniteVector(vertex.uv) ||
                    !std::isfinite(vertex.tangentSign))
                {
                    ++mesh.nonFiniteVertexCount;
                }
            }

            for (const std::uint32_t index :
                 mesh.indices)
            {
                if (index >= mesh.vertices.size())
                {
                    ++mesh.invalidIndexCount;
                }
            }

            mesh.indexCountNotTriangleList =
                mesh.indices.size() % 3U != 0U;

            const std::size_t triangleCount =
                mesh.indices.size() / 3U;

            for (std::size_t triangleIndex = 0U;
                 triangleIndex < triangleCount;
                 ++triangleIndex)
            {
                const std::uint32_t first =
                    mesh.indices[
                        triangleIndex * 3U + 0U];

                const std::uint32_t second =
                    mesh.indices[
                        triangleIndex * 3U + 1U];

                const std::uint32_t third =
                    mesh.indices[
                        triangleIndex * 3U + 2U];

                if (first == second ||
                    second == third ||
                    first == third)
                {
                    ++mesh.degenerateTriangleCount;
                }
            }
        }

        void AnalyzeWeightData(
            LegacyWeightData& weights,
            const LegacySkeletonData* skeleton) noexcept
        {
            weights.zeroWeightVertexCount = 0U;
            weights.nonNormalizedVertexCount = 0U;

            weights.invalidWeightValueCount = 0U;
            weights.invalidBoneReferenceCount = 0U;

            weights.maximumBoneIndex = 0U;

            weights.minimumWeightSum =
                weights.vertices.empty()
                    ? 0.0F
                    : (std::numeric_limits<float>::max)();

            weights.maximumWeightSum = 0.0F;

            const std::size_t boneCount =
                skeleton != nullptr
                    ? skeleton->bones.size()
                    : 0U;

            if (skeleton != nullptr &&
                weights.skeletonId != 0U &&
                skeleton->skeletonId != 0U &&
                weights.skeletonId !=
                    skeleton->skeletonId)
            {
                weights.skeletonIdMismatch = true;
            }

            for (const LegacySkinVertex& vertex :
                 weights.vertices)
            {
                float weightSum = 0.0F;

                for (std::size_t influenceIndex = 0U;
                     influenceIndex <
                         vertex.weights.size();
                     ++influenceIndex)
                {
                    const float weight =
                        vertex.weights[influenceIndex];

                    const std::uint32_t boneIndex =
                        vertex.boneIndices[
                            influenceIndex];

                    weights.maximumBoneIndex =
                        (std::max)(
                            weights.maximumBoneIndex,
                            boneIndex);

                    if (!std::isfinite(weight) ||
                        weight < 0.0F ||
                        weight > 1.0F)
                    {
                        ++weights.invalidWeightValueCount;
                        continue;
                    }

                    weightSum += weight;

                    if (weight > 0.000001F &&
                        skeleton != nullptr &&
                        boneIndex >= boneCount)
                    {
                        ++weights.invalidBoneReferenceCount;
                    }
                }

                weights.minimumWeightSum =
                    (std::min)(
                        weights.minimumWeightSum,
                        weightSum);

                weights.maximumWeightSum =
                    (std::max)(
                        weights.maximumWeightSum,
                        weightSum);

                if (weightSum <= 0.000001F)
                {
                    ++weights.zeroWeightVertexCount;
                }
                else if (
                    std::abs(weightSum - 1.0F) >
                    0.01F)
                {
                    ++weights.nonNormalizedVertexCount;
                }
            }
        }

        [[nodiscard]]
        bool ReadEmbeddedWeights(
            std::ifstream& stream,
            const std::filesystem::path& path,
            const std::uint32_t meshVertexCount,
            const LegacySkeletonData* skeleton,
            LegacyMeshData& mesh) noexcept
        {
            LegacyWeightData weights;
            weights.sourcePath = path;

            std::uint32_t weightVertexCount = 0U;

            if (!ReadValue(stream, weights.skeletonId) ||
                !ReadValue(stream, weightVertexCount))
            {
                mesh.error =
                    "Failed to read embedded SCB weight header.";

                return false;
            }

            if (weightVertexCount == 0U ||
                weightVertexCount > MaximumVertexCount)
            {
                mesh.error =
                    "Invalid embedded SCB weight count.";

                return false;
            }

            mesh.embeddedWeightVertexCountMismatch =
                weightVertexCount != meshVertexCount;

            weights.vertices.resize(
                weightVertexCount);

            for (LegacySkinVertex& vertex :
                 weights.vertices)
            {
                if (!ReadBytes(
                        stream,
                        vertex.boneIndices.data(),
                        vertex.boneIndices.size()) ||
                    !ReadBytes(
                        stream,
                        vertex.weights.data(),
                        sizeof(vertex.weights)))
                {
                    mesh.error =
                        "SCB embedded weight data is truncated.";

                    return false;
                }
            }

            AnalyzeWeightData(
                weights,
                skeleton);

            mesh.embeddedWeights =
                std::move(weights);

            mesh.hasEmbeddedWeights = true;
            return true;
        }

        [[nodiscard]]
        bool ReadRequiredLine(
            std::ifstream& stream,
            std::string& line)
        {
            while (std::getline(stream, line))
            {
                if (!line.empty() &&
                    line.back() == '\r')
                {
                    line.pop_back();
                }

                if (!line.empty())
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        bool ReadLabeledCount(
            const std::string& line,
            std::int32_t& value)
        {
            std::istringstream parser(line);

            std::string label;

            return
                static_cast<bool>(
                    parser >> label >> value);
        }

        [[nodiscard]]
        std::string ReadObjectName(
            const std::string& line)
        {
            const std::size_t separator =
                line.find('=');

            if (separator == std::string::npos)
            {
                return {};
            }

            std::string result =
                line.substr(separator + 1U);

            const std::size_t first =
                result.find_first_not_of(
                    " \t");

            if (first == std::string::npos)
            {
                return {};
            }

            result.erase(0U, first);

            const std::size_t last =
                result.find_last_not_of(
                    " \t");

            if (last != std::string::npos)
            {
                result.erase(last + 1U);
            }

            return result;
        }
    }

    bool LegacyMeshReader::Read(
        const std::filesystem::path& scbPath,
        const std::filesystem::path& scoPath,
        const LegacySkeletonData* skeleton,
        LegacyMeshData& output) noexcept
    {
        output = {};

        std::string scbError;

        if (FileExists(scbPath))
        {
            LegacyMeshData scbData;

            if (ReadScb(
                    scbPath,
                    skeleton,
                    scbData))
            {
                output = std::move(scbData);
                return true;
            }

            scbError = scbData.error;
        }

        if (FileExists(scoPath))
        {
            LegacyMeshData scoData;

            if (ReadSco(scoPath, scoData))
            {
                if (!scbError.empty())
                {
                    scoData.usedScoFallback = true;

                    scoData.warning =
                        "SCB parsing failed, SCO fallback was used: " +
                        scbError;
                }

                output = std::move(scoData);
                return true;
            }

            output = std::move(scoData);

            if (!scbError.empty())
            {
                output.error =
                    "SCB: " +
                    scbError +
                    " SCO: " +
                    output.error;
            }

            return false;
        }

        output.error = !scbError.empty()
            ? scbError
            : "Neither SCB nor SCO geometry was found.";

        return false;
    }

    bool LegacyMeshReader::ReadScb(
        const std::filesystem::path& path,
        const LegacySkeletonData* skeleton,
        LegacyMeshData& output) noexcept
    {
        output = {};
        output.sourcePath = path;
        output.format = LegacyMeshFormat::Scb;

        try
        {
            std::uintmax_t fileSize = 0U;

            if (!GetFileSize(path, fileSize))
            {
                output.error =
                    "Failed to query SCB file size.";

                return false;
            }

            std::ifstream stream(
                path,
                std::ios::binary);

            if (!stream)
            {
                output.error =
                    "Failed to open SCB file.";

                return false;
            }

            std::uint32_t version = 0U;
            std::uint32_t flags = 0U;

            std::int32_t nameLength = 0;

            if (!ReadValue(stream, version) ||
                !ReadValue(stream, flags) ||
                !ReadValue(stream, nameLength))
            {
                output.error =
                    "SCB header is truncated.";

                return false;
            }

            if (version != ScbVersion)
            {
                output.error =
                    "Unsupported SCB version: 0x" +
                    std::to_string(version) +
                    '.';

                return false;
            }

            if (!ReadString(
                    stream,
                    nameLength,
                    output.name))
            {
                output.error =
                    "Invalid SCB mesh name.";

                return false;
            }

            if (!ReadBytes(
                    stream,
                    output.pivot.data(),
                    sizeof(output.pivot)))
            {
                output.error =
                    "Failed to read SCB pivot.";

                return false;
            }

            std::int32_t signedVertexCount = 0;

            if (!ReadValue(
                    stream,
                    signedVertexCount) ||
                signedVertexCount <= 0 ||
                signedVertexCount >
                    static_cast<std::int32_t>(
                        MaximumVertexCount))
            {
                output.error =
                    "Invalid SCB vertex count.";

                return false;
            }

            const std::uint32_t vertexCount =
                static_cast<std::uint32_t>(
                    signedVertexCount);

            std::vector<std::array<float, 3U>>
                positions(vertexCount);

            std::vector<std::array<float, 2U>>
                uvs(vertexCount);

            std::vector<std::array<float, 3U>>
                normals(vertexCount);

            std::vector<std::array<float, 3U>>
                tangents(vertexCount);

            std::vector<std::uint8_t>
                tangentSigns(vertexCount);

            if (!ReadBytes(
                    stream,
                    positions.data(),
                    positions.size() *
                        sizeof(positions[0])) ||
                !ReadBytes(
                    stream,
                    uvs.data(),
                    uvs.size() *
                        sizeof(uvs[0])) ||
                !ReadBytes(
                    stream,
                    normals.data(),
                    normals.size() *
                        sizeof(normals[0])) ||
                !ReadBytes(
                    stream,
                    tangents.data(),
                    tangents.size() *
                        sizeof(tangents[0])) ||
                !ReadBytes(
                    stream,
                    tangentSigns.data(),
                    tangentSigns.size()))
            {
                output.error =
                    "SCB vertex data is truncated.";

                return false;
            }

            output.vertices.resize(vertexCount);

            for (std::size_t vertexIndex = 0U;
                 vertexIndex < output.vertices.size();
                 ++vertexIndex)
            {
                LegacyMeshVertex& vertex =
                    output.vertices[vertexIndex];

                vertex.position =
                    positions[vertexIndex];

                vertex.uv =
                    uvs[vertexIndex];

                vertex.normal =
                    normals[vertexIndex];

                vertex.tangent =
                    tangents[vertexIndex];

                vertex.tangentSign =
                    tangentSigns[vertexIndex] != 0U
                        ? 1.0F
                        : -1.0F;
            }

            std::int32_t signedIndexCount = 0;

            if (!ReadValue(
                    stream,
                    signedIndexCount) ||
                signedIndexCount <= 0 ||
                signedIndexCount >
                    static_cast<std::int32_t>(
                        MaximumIndexCount))
            {
                output.error =
                    "Invalid SCB index count.";

                return false;
            }

            output.indices.resize(
                static_cast<std::size_t>(
                    signedIndexCount));

            if (!ReadBytes(
                    stream,
                    output.indices.data(),
                    output.indices.size() *
                        sizeof(output.indices[0])))
            {
                output.error =
                    "SCB index data is truncated.";

                return false;
            }

            std::int32_t signedChunkCount = 0;

            if (!ReadValue(
                    stream,
                    signedChunkCount) ||
                signedChunkCount < 0 ||
                signedChunkCount >
                    static_cast<std::int32_t>(
                        MaximumMaterialChunkCount))
            {
                output.error =
                    "Invalid SCB material chunk count.";

                return false;
            }

            output.materialChunks.reserve(
                static_cast<std::size_t>(
                    signedChunkCount));

            for (std::int32_t chunkIndex = 0;
                 chunkIndex < signedChunkCount;
                 ++chunkIndex)
            {
                std::int32_t startIndex = 0;
                std::int32_t endIndex = 0;
                std::int32_t materialNameLength = 0;

                std::string materialName;

                if (!ReadValue(stream, startIndex) ||
                    !ReadValue(stream, endIndex) ||
                    !ReadValue(
                        stream,
                        materialNameLength) ||
                    !ReadString(
                        stream,
                        materialNameLength,
                        materialName))
                {
                    output.error =
                        "SCB material chunk is truncated.";

                    return false;
                }

                if (startIndex < 0 ||
                    endIndex < startIndex ||
                    endIndex >
                        signedIndexCount)
                {
                    ++output.invalidMaterialRangeCount;
                }

                const std::int32_t safeStart =
                    std::clamp(
                        startIndex,
                        0,
                        signedIndexCount);

                const std::int32_t safeEnd =
                    std::clamp(
                        endIndex,
                        safeStart,
                        signedIndexCount);

                LegacyMaterialChunk chunk;
                chunk.materialName =
                    std::move(materialName);

                chunk.firstIndex =
                    static_cast<std::uint32_t>(
                        safeStart);

                chunk.indexCount =
                    static_cast<std::uint32_t>(
                        safeEnd - safeStart);

                output.materialChunks.push_back(
                    std::move(chunk));
            }

            if ((flags & ScbHasWeights) != 0U)
            {
                if (!ReadEmbeddedWeights(
                        stream,
                        path,
                        vertexCount,
                        skeleton,
                        output))
                {
                    return false;
                }
            }

            if ((flags & ScbHasVertexColors) != 0U)
            {
                output.hasVertexColors = true;

                std::vector<std::uint8_t> colors(
                    static_cast<std::size_t>(
                        vertexCount) *
                    4U);

                if (!ReadBytes(
                        stream,
                        colors.data(),
                        colors.size()))
                {
                    output.error =
                        "SCB vertex color data is truncated.";

                    return false;
                }
            }

            const std::streampos finalPosition =
                stream.tellg();

            if (finalPosition >=
                    static_cast<std::streampos>(0) &&
                fileSize >=
                    static_cast<std::uintmax_t>(
                        finalPosition))
            {
                output.trailingByteCount =
                    static_cast<std::size_t>(
                        fileSize -
                        static_cast<std::uintmax_t>(
                            finalPosition));
            }

            ApplyLegacyPivot(output);
            ValidateMesh(output);
            return true;
        }
        catch (const std::exception& exception)
        {
            output.error =
                "SCB read failed: " +
                std::string(exception.what());

            output.vertices.clear();
            output.indices.clear();

            return false;
        }
        catch (...)
        {
            output.error =
                "SCB read failed with an unknown error.";

            output.vertices.clear();
            output.indices.clear();

            return false;
        }
    }

    bool LegacyMeshReader::ReadSco(
        const std::filesystem::path& path,
        LegacyMeshData& output) noexcept
    {
        output = {};
        output.sourcePath = path;
        output.format = LegacyMeshFormat::Sco;

        try
        {
            std::ifstream stream(path);

            if (!stream)
            {
                output.error =
                    "Failed to open SCO file.";

                return false;
            }

            std::string line;
            bool objectBeginFound = false;

            while (std::getline(stream, line))
            {
                if (line.rfind(
                        "[ObjectBegin]",
                        0U) == 0U)
                {
                    objectBeginFound = true;
                    break;
                }
            }

            if (!objectBeginFound)
            {
                output.error =
                    "SCO ObjectBegin section was not found.";

                return false;
            }

            if (!ReadRequiredLine(stream, line))
            {
                output.error =
                    "SCO mesh name is missing.";

                return false;
            }

            output.name = ReadObjectName(line);

            if (!ReadRequiredLine(stream, line))
            {
                output.error =
                    "SCO pivot is missing.";

                return false;
            }

            {
                std::istringstream parser(line);
                std::string label;

                if (!(parser >>
                      label >>
                      output.pivot[0] >>
                      output.pivot[1] >>
                      output.pivot[2]))
                {
                    output.error =
                        "Invalid SCO pivot.";

                    return false;
                }
            }

            if (!ReadRequiredLine(stream, line))
            {
                output.error =
                    "SCO vertex count is missing.";

                return false;
            }

            std::int32_t signedVertexCount = 0;

            if (!ReadLabeledCount(
                    line,
                    signedVertexCount) ||
                signedVertexCount <= 0 ||
                signedVertexCount >
                    static_cast<std::int32_t>(
                        MaximumVertexCount))
            {
                output.error =
                    "Invalid SCO vertex count.";

                return false;
            }

            output.vertices.resize(
                static_cast<std::size_t>(
                    signedVertexCount));

            for (LegacyMeshVertex& vertex :
                 output.vertices)
            {
                if (!ReadRequiredLine(stream, line))
                {
                    output.error =
                        "SCO vertex data is truncated.";

                    return false;
                }

                std::istringstream parser(line);

                if (!(parser >>
                      vertex.position[0] >>
                      vertex.position[1] >>
                      vertex.position[2] >>
                      vertex.normal[0] >>
                      vertex.normal[1] >>
                      vertex.normal[2] >>
                      vertex.tangent[0] >>
                      vertex.tangent[1] >>
                      vertex.tangent[2] >>
                      vertex.tangentSign))
                {
                    output.error =
                        "Invalid SCO vertex data.";

                    return false;
                }

                vertex.tangentSign =
                    vertex.tangentSign >= 0.0F
                        ? 1.0F
                        : -1.0F;
            }

            if (!ReadRequiredLine(stream, line))
            {
                output.error =
                    "SCO face count is missing.";

                return false;
            }

            std::int32_t signedFaceCount = 0;

            if (!ReadLabeledCount(
                    line,
                    signedFaceCount) ||
                signedFaceCount <= 0 ||
                signedFaceCount >
                    static_cast<std::int32_t>(
                        MaximumIndexCount / 3U))
            {
                output.error =
                    "Invalid SCO face count.";

                return false;
            }

            output.indices.reserve(
                static_cast<std::size_t>(
                    signedFaceCount) *
                3U);

            std::vector<bool> uvAssigned(
                output.vertices.size(),
                false);

            std::string currentMaterial;

            for (std::int32_t faceIndex = 0;
                 faceIndex < signedFaceCount;
                 ++faceIndex)
            {
                if (!ReadRequiredLine(stream, line))
                {
                    output.error =
                        "SCO face data is truncated.";

                    return false;
                }

                std::istringstream parser(line);

                std::int32_t faceId = 0;

                std::array<std::int32_t, 3U>
                    faceIndices{};

                std::string materialName;

                std::array<float, 3U> u{};
                std::array<float, 3U> v{};

                if (!(parser >>
                      faceId >>
                      faceIndices[0] >>
                      faceIndices[1] >>
                      faceIndices[2] >>
                      materialName >>
                      u[0] >> v[0] >>
                      u[1] >> v[1] >>
                      u[2] >> v[2]))
                {
                    output.error =
                        "Invalid SCO face data.";

                    return false;
                }

                static_cast<void>(faceId);

                const std::uint32_t firstIndex =
                    static_cast<std::uint32_t>(
                        output.indices.size());

                if (output.materialChunks.empty() ||
                    currentMaterial != materialName)
                {
                    if (!output.materialChunks.empty())
                    {
                        LegacyMaterialChunk& previous =
                            output.materialChunks.back();

                        previous.indexCount =
                            firstIndex -
                            previous.firstIndex;
                    }

                    LegacyMaterialChunk chunk;
                    chunk.materialName = materialName;
                    chunk.firstIndex = firstIndex;

                    output.materialChunks.push_back(
                        std::move(chunk));

                    currentMaterial = materialName;
                }

                for (std::size_t cornerIndex = 0U;
                     cornerIndex < 3U;
                     ++cornerIndex)
                {
                    const std::int32_t signedIndex =
                        faceIndices[cornerIndex];

                    if (signedIndex < 0 ||
                        signedIndex >=
                            signedVertexCount)
                    {
                        ++output.invalidIndexCount;

                        output.indices.push_back(
                            (std::numeric_limits<
                                std::uint32_t>::max)());

                        continue;
                    }

                    const std::uint32_t vertexIndex =
                        static_cast<std::uint32_t>(
                            signedIndex);

                    output.indices.push_back(
                        vertexIndex);

                    const std::array<float, 2U> uv
                    {
                        u[cornerIndex],
                        v[cornerIndex]
                    };

                    if (uvAssigned[vertexIndex])
                    {
                        const std::array<float, 2U>&
                            existingUv =
                                output.vertices[
                                    vertexIndex].uv;

                        if (std::abs(
                                existingUv[0] -
                                uv[0]) > 0.0001F ||
                            std::abs(
                                existingUv[1] -
                                uv[1]) > 0.0001F)
                        {
                            ++output.uvConflictCount;
                        }
                    }
                    else
                    {
                        output.vertices[
                            vertexIndex].uv = uv;

                        uvAssigned[vertexIndex] = true;
                    }
                }
            }

            if (!output.materialChunks.empty())
            {
                LegacyMaterialChunk& lastChunk =
                    output.materialChunks.back();

                lastChunk.indexCount =
                    static_cast<std::uint32_t>(
                        output.indices.size()) -
                    lastChunk.firstIndex;
            }

            ApplyLegacyPivot(output);
            ValidateMesh(output);
            return true;
        }
        catch (const std::exception& exception)
        {
            output.error =
                "SCO read failed: " +
                std::string(exception.what());

            output.vertices.clear();
            output.indices.clear();

            return false;
        }
        catch (...)
        {
            output.error =
                "SCO read failed with an unknown error.";

            output.vertices.clear();
            output.indices.clear();

            return false;
        }
    }
}