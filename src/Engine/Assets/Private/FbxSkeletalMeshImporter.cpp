#include "Assets/FbxSkeletalMeshImporter.h"

#include "Assets/SkeletalMeshAsset.h"

#include "FbxImporterCommon.h"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        struct Influence final
        {
            std::uint8_t boneIndex = 0U;
            float weight = 0.0F;
        };

        [[nodiscard]]
        AssetResult WriteSkeleton(
            const FbxSkeletonData& skeleton,
            const std::filesystem::path& destination,
            const std::string& skeletonAssetPath,
            const bool overwrite,
            std::wstring& error)
        {
            if (!skeleton.IsValid())
            {
                error =
                    L"Cannot write an invalid skeleton.";

                return AssetResult::InvalidArgument;
            }

            fbx_detail::BinaryWriter writer;

            constexpr std::array<char, 8U> magic
            {{
                'S', 'K', 'E', 'L',
                'E', 'T', 'O', 'N'
            }};

            const std::uint32_t version = 1U;

            const std::uint32_t skeletonId =
                fbx_detail::HashSkeletonPath(
                    skeletonAssetPath);

            const std::uint32_t boneCount =
                static_cast<std::uint32_t>(
                    skeleton.bones.size());

            if (
                !writer.WriteBytes(
                    magic.data(),
                    magic.size()) ||
                !writer.Write(version) ||
                !writer.Write(
                    fbx_detail::
                        AssetEndianMarker) ||
                !writer.Write(skeletonId) ||
                !writer.Write(boneCount))
            {
                error =
                    L"Failed to allocate skeleton data.";

                return AssetResult::OutOfMemory;
            }

            for (
                const FbxSkeletonBone& sourceBone :
                skeleton.bones)
            {
                const float boneLength = 0.0F;

                if (
                    !writer.WriteString(
                        sourceBone.name) ||
                    !writer.Write(
                        sourceBone.parentIndex) ||
                    !writer.Write(boneLength))
                {
                    error =
                        L"Failed to encode skeleton bone.";

                    return AssetResult::OutOfMemory;
                }

                for (
                    const float value :
                    sourceBone.
                        modelBindMatrix.values)
                {
                    if (!writer.Write(value))
                    {
                        error =
                            L"Failed to encode skeleton "
                            L"bind matrix.";

                        return AssetResult::OutOfMemory;
                    }
                }
            }

            return
                fbx_detail::SaveBinary(
                    destination,
                    writer.GetBytes(),
                    overwrite,
                    error);
        }

        void CalculateBounds(
            const std::vector<
                SkeletalMeshVertex>& vertices,
            std::array<float, 3U>& minimum,
            std::array<float, 3U>& maximum)
        {
            minimum =
                vertices.front().position;

            maximum =
                vertices.front().position;

            for (
                const SkeletalMeshVertex& vertex :
                vertices)
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
        }

        [[nodiscard]]
        AssetResult WriteSkeletalMesh(
            const ufbx_node& node,
            const ufbx_mesh& mesh,
            const FbxSkeletonData& skeleton,
            const std::unordered_map<
                const ufbx_node*,
                std::uint32_t>& boneIndices,
            const std::string& skeletonAssetPath,
            const std::filesystem::path& destination,
            const std::uint32_t influenceLimit,
            const bool overwrite,
            FbxImportReport& report,
            std::wstring& error)
        {
            if (
                mesh.skin_deformers.count == 0U)
            {
                error =
                    L"Skeletal FBX mesh has no "
                    L"skin deformer.";

                return AssetResult::CorruptData;
            }

            const ufbx_skin_deformer* const skin =
                mesh.skin_deformers.data[0U];

            if (skin == nullptr)
            {
                error =
                    L"Skeletal FBX mesh contains "
                    L"a null skin deformer.";

                return AssetResult::CorruptData;
            }

            if (mesh.skin_deformers.count > 1U)
            {
                report.warnings.push_back(
                    L"FBX mesh has multiple skin "
                    L"deformers; only the first one "
                    L"was imported.");
            }

            std::vector<std::int32_t>
                clusterToBone(
                    skin->clusters.count,
                    -1);

            for (
                std::size_t clusterIndex = 0U;
                clusterIndex <
                    skin->clusters.count;
                ++clusterIndex)
            {
                const ufbx_skin_cluster*
                    const cluster =
                        skin->clusters.data[
                            clusterIndex];

                if (
                    cluster == nullptr ||
                    cluster->bone_node == nullptr)
                {
                    continue;
                }

                const auto found =
                    boneIndices.find(
                        cluster->bone_node);

                if (found != boneIndices.end())
                {
                    clusterToBone[clusterIndex] =
                        static_cast<std::int32_t>(
                            found->second);
                }
            }

            std::vector<
                std::vector<std::uint32_t>>
                cornersByMaterial;

            AssetResult result =
                fbx_detail::BuildMaterialCorners(
                    node,
                    mesh,
                    cornersByMaterial,
                    error);

            if (Failed(result))
            {
                return result;
            }

            std::vector<SkeletalMeshVertex>
                vertices;

            std::vector<std::uint32_t>
                indices;

            std::vector<SkeletalMeshSection>
                sections;

            const std::uint32_t clampedLimit =
                std::clamp(
                    influenceLimit,
                    1U,
                    4U);

            bool influenceWarningWritten = false;

            for (
                std::size_t materialSlot = 0U;
                materialSlot <
                    cornersByMaterial.size();
                ++materialSlot)
            {
                const auto& corners =
                    cornersByMaterial[
                        materialSlot];

                if (corners.empty())
                {
                    continue;
                }

                SkeletalMeshSection section{};

                section.firstIndex =
                    static_cast<std::uint32_t>(
                        indices.size());

                section.materialSlot =
                    static_cast<std::uint32_t>(
                        materialSlot);

                /*
                 * Материалы будут назначаться отдельно.
                 * Loader допускает пустую строку.
                 */
                section.materialAssetPath.clear();

                for (
                    const std::uint32_t cornerIndex :
                    corners)
                {
                    if (
                        cornerIndex >=
                            mesh.num_indices ||
                        vertices.size() >=
                            fbx_detail::
                                MaximumImportedVertices)
                    {
                        error =
                            L"FBX skeletal mesh exceeds "
                            L"the vertex limit or contains "
                            L"an invalid corner.";

                        return AssetResult::FileTooLarge;
                    }

                    const fbx_detail::
                        ImportedVertexFrame frame =
                            fbx_detail::
                                ReadVertexFrame(
                                    node,
                                    mesh,
                                    cornerIndex);

                    SkeletalMeshVertex vertex{};

                    vertex.position =
                        frame.position;

                    vertex.normal =
                        frame.normal;

                    vertex.tangent =
                    {
                        frame.tangent[0U],
                        frame.tangent[1U],
                        frame.tangent[2U]
                    };

                    vertex.tangentSign =
                        frame.tangent[3U];

                    vertex.texcoord0 =
                        frame.texcoord;

                    std::vector<Influence>
                        influences;

                    const std::uint32_t controlPoint =
                        cornerIndex <
                            mesh.vertex_indices.count
                            ? mesh.vertex_indices.data[
                                cornerIndex]
                            : UFBX_NO_INDEX;

                    if (
                        controlPoint != UFBX_NO_INDEX &&
                        controlPoint <
                            skin->vertices.count)
                    {
                        const ufbx_skin_vertex
                            skinVertex =
                                skin->vertices.data[
                                    controlPoint];

                        influences.reserve(
                            skinVertex.num_weights);

                        for (
                            std::uint32_t offset = 0U;
                            offset <
                                skinVertex.num_weights;
                            ++offset)
                        {
                            const std::size_t
                                weightIndex =
                                    static_cast<
                                        std::size_t>(
                                            skinVertex.
                                                weight_begin) +
                                    offset;

                            if (
                                weightIndex >=
                                    skin->weights.count)
                            {
                                break;
                            }

                            const ufbx_skin_weight
                                weight =
                                    skin->weights.data[
                                        weightIndex];

                            if (
                                weight.cluster_index >=
                                    clusterToBone.size())
                            {
                                continue;
                            }

                            const std::int32_t
                                boneIndex =
                                    clusterToBone[
                                        weight.
                                            cluster_index];

                            const float value =
                                static_cast<float>(
                                    weight.weight);

                            if (
                                boneIndex < 0 ||
                                boneIndex > 255 ||
                                !std::isfinite(value) ||
                                value <= 0.0F)
                            {
                                continue;
                            }

                            influences.push_back(
                                Influence
                                {
                                    static_cast<
                                        std::uint8_t>(
                                            boneIndex),
                                    value
                                });
                        }
                    }

                    std::sort(
                        influences.begin(),
                        influences.end(),
                        [](const Influence& left,
                           const Influence& right)
                        {
                            return
                                left.weight >
                                right.weight;
                        });

                    if (
                        influences.size() >
                            clampedLimit &&
                        !influenceWarningWritten)
                    {
                        report.warnings.push_back(
                            L"Some vertices have more "
                            L"than four bone influences; "
                            L"the weakest influences were "
                            L"removed.");

                        influenceWarningWritten = true;
                    }

                    const std::size_t writeCount =
                        (std::min)(
                            influences.size(),
                            static_cast<std::size_t>(
                                clampedLimit));

                    float totalWeight = 0.0F;

                    for (
                        std::size_t index = 0U;
                        index < writeCount;
                        ++index)
                    {
                        vertex.boneIndices[index] =
                            influences[index].
                                boneIndex;

                        vertex.boneWeights[index] =
                            influences[index].
                                weight;

                        totalWeight +=
                            influences[index].
                                weight;
                    }

                    if (totalWeight <= 0.000001F)
                    {
                        vertex.boneIndices[0U] = 0U;
                        vertex.boneWeights[0U] = 1.0F;
                    }
                    else
                    {
                        const float inverseWeight =
                            1.0F / totalWeight;

                        for (
                            std::size_t index = 0U;
                            index < writeCount;
                            ++index)
                        {
                            vertex.boneWeights[index] *=
                                inverseWeight;
                        }
                    }

                    const std::uint32_t newIndex =
                        static_cast<std::uint32_t>(
                            vertices.size());

                    vertices.push_back(vertex);
                    indices.push_back(newIndex);
                }

                section.indexCount =
                    static_cast<std::uint32_t>(
                        indices.size()) -
                    section.firstIndex;

                if (section.indexCount != 0U)
                {
                    sections.push_back(
                        std::move(section));
                }
            }

            if (
                vertices.empty() ||
                indices.empty() ||
                sections.empty())
            {
                error =
                    L"FBX contains no renderable "
                    L"skeletal mesh triangles.";

                return AssetResult::
                    UnsupportedFeature;
            }

            std::array<float, 3U> minimum{};
            std::array<float, 3U> maximum{};

            CalculateBounds(
                vertices,
                minimum,
                maximum);

            const std::array<float, 3U> pivot
            {
                0.0F,
                0.0F,
                0.0F
            };

            fbx_detail::BinaryWriter writer;

            constexpr std::array<char, 8U> magic
            {{
                'S', 'K', 'M', 'E',
                'S', 'H', '\0', '\0'
            }};

            const std::uint32_t version = 1U;

            const std::uint32_t vertexCount =
                static_cast<std::uint32_t>(
                    vertices.size());

            const std::uint32_t indexCount =
                static_cast<std::uint32_t>(
                    indices.size());

            const std::uint32_t sectionCount =
                static_cast<std::uint32_t>(
                    sections.size());

            if (
                !writer.WriteBytes(
                    magic.data(),
                    magic.size()) ||
                !writer.Write(version) ||
                !writer.Write(
                    fbx_detail::
                        AssetEndianMarker) ||
                !writer.WriteString(
                    skeletonAssetPath) ||
                !writer.Write(vertexCount) ||
                !writer.Write(indexCount) ||
                !writer.Write(sectionCount))
            {
                error =
                    L"Failed to allocate .skm header.";

                return AssetResult::OutOfMemory;
            }

            for (const float value : pivot)
            {
                if (!writer.Write(value))
                {
                    return AssetResult::OutOfMemory;
                }
            }

            for (const float value : minimum)
            {
                if (!writer.Write(value))
                {
                    return AssetResult::OutOfMemory;
                }
            }

            for (const float value : maximum)
            {
                if (!writer.Write(value))
                {
                    return AssetResult::OutOfMemory;
                }
            }

            if (
                !writer.WriteBytes(
                    vertices.data(),
                    vertices.size() *
                        sizeof(
                            SkeletalMeshVertex)) ||
                !writer.WriteBytes(
                    indices.data(),
                    indices.size() *
                        sizeof(std::uint32_t)))
            {
                error =
                    L"Failed to encode .skm geometry.";

                return AssetResult::OutOfMemory;
            }

            for (
                const SkeletalMeshSection& section :
                sections)
            {
                if (
                    !writer.Write(
                        section.firstIndex) ||
                    !writer.Write(
                        section.indexCount) ||
                    !writer.Write(
                        section.materialSlot) ||
                    !writer.WriteString(
                        section.
                            materialAssetPath))
                {
                    error =
                        L"Failed to encode .skm section.";

                    return AssetResult::OutOfMemory;
                }
            }

            return
                fbx_detail::SaveBinary(
                    destination,
                    writer.GetBytes(),
                    overwrite,
                    error);
        }

        [[nodiscard]]
        std::filesystem::path MakeMeshPath(
            const std::filesystem::path& directory,
            const ufbx_mesh& mesh,
            std::unordered_map<
                std::string,
                std::size_t>& usedNames)
        {
            std::string baseName =
                fbx_detail::SanitizeName(
                    fbx_detail::ToString(
                        mesh.name,
                        "SkeletalMesh"),
                    "SkeletalMesh");

            std::size_t& counter =
                usedNames[baseName];

            if (counter != 0U)
            {
                baseName +=
                    "_" +
                    std::to_string(counter);
            }

            ++counter;

            return
                directory /
                std::filesystem::u8path(
                    baseName + ".skm");
        }
    }

    bool FbxSkeletalMeshImporter::
        IsSupportedSource(
            const std::filesystem::path&
                sourcePath) noexcept
    {
        try
        {
            return
                fbx_detail::IsSupportedSource(
                    sourcePath);
        }
        catch (...)
        {
            return false;
        }
    }

    AssetResult FbxSkeletalMeshImporter::Import(
        const std::filesystem::path& sourcePath,
        const FbxSkeletalMeshImportOptions& options,
        FbxImportReport& report,
        std::filesystem::path&
            outputSkeletonFile,
        std::string&
            outputSkeletonAssetPath,
        std::wstring& error) noexcept
    {
        report.Clear();
        outputSkeletonFile.clear();
        outputSkeletonAssetPath.clear();
        error.clear();

        try
        {
            if (
                options.destinationDirectory.empty())
            {
                error =
                    L"Skeletal asset destination "
                    L"directory is empty.";

                return AssetResult::InvalidPath;
            }

            if (
                !options.writeSkeleton &&
                options.skeletonFile.empty())
            {
                error =
                    L"An existing .skeleton file is "
                    L"required when skeleton export "
                    L"is disabled.";

                return AssetResult::InvalidArgument;
            }

            fbx_detail::ScenePtr scene;

            AssetResult result =
                fbx_detail::LoadScene(
                    sourcePath,
                    scene,
                    error);

            if (Failed(result))
            {
                return result;
            }

            FbxSkeletonData skeleton;

            std::vector<const ufbx_node*>
                boneNodes;

            std::unordered_map<
                const ufbx_node*,
                std::uint32_t>
                boneIndices;

            result =
                fbx_detail::BuildSkeleton(
                    *scene,
                    skeleton,
                    boneNodes,
                    boneIndices,
                    error);

            if (Failed(result))
            {
                return result;
            }

            outputSkeletonFile =
                options.skeletonFile;

            if (outputSkeletonFile.empty())
            {
                std::string skeletonName =
                    fbx_detail::SanitizeName(
                        sourcePath.stem().
                            u8string(),
                        "Skeleton");

                outputSkeletonFile =
                    options.destinationDirectory /
                    std::filesystem::u8path(
                        skeletonName +
                        ".skeleton");
            }

            result =
                fbx_detail::MakeAssetPath(
                    outputSkeletonFile,
                    outputSkeletonAssetPath,
                    error);

            if (Failed(result))
            {
                return result;
            }

            if (options.writeSkeleton)
            {
                result =
                    WriteSkeleton(
                        skeleton,
                        outputSkeletonFile,
                        outputSkeletonAssetPath,
                        options.
                            overwriteExisting,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                report.writtenFiles.push_back(
                    outputSkeletonFile);
            }
            else
            {
                std::error_code filesystemError;

                if (
                    !std::filesystem::
                        is_regular_file(
                            outputSkeletonFile,
                            filesystemError) ||
                    filesystemError)
                {
                    error =
                        L"Selected .skeleton file "
                        L"does not exist.";

                    return AssetResult::NotFound;
                }
            }

            if (!options.writeMeshes)
            {
                return AssetResult::Success;
            }

            std::unordered_map<
                std::string,
                std::size_t>
                usedNames;

            for (
                std::size_t meshIndex = 0U;
                meshIndex <
                    scene->meshes.count;
                ++meshIndex)
            {
                const ufbx_mesh* const mesh =
                    scene->meshes.data[
                        meshIndex];

                if (
                    mesh == nullptr ||
                    mesh->faces.count == 0U ||
                    mesh->skin_deformers.count == 0U)
                {
                    continue;
                }

                for (
                    std::size_t instanceIndex = 0U;
                    instanceIndex <
                        mesh->instances.count;
                    ++instanceIndex)
                {
                    const ufbx_node* const node =
                        mesh->instances.data[
                            instanceIndex];

                    if (node == nullptr)
                    {
                        report.warnings.push_back(
                            L"Skeletal FBX mesh contains "
                            L"a null instance.");

                        continue;
                    }

                    const std::filesystem::path
                        destination =
                            MakeMeshPath(
                                options.
                                    destinationDirectory,
                                *mesh,
                                usedNames);

                    result =
                        WriteSkeletalMesh(
                            *node,
                            *mesh,
                            skeleton,
                            boneIndices,
                            outputSkeletonAssetPath,
                            destination,
                            options.
                                maximumBoneInfluences,
                            options.
                                overwriteExisting,
                            report,
                            error);

                    if (Failed(result))
                    {
                        report.Clear();
                        return result;
                    }

                    report.writtenFiles.push_back(
                        destination);
                }
            }

            if (
                options.writeMeshes &&
                report.writtenFiles.size() ==
                    (
                        options.writeSkeleton
                            ? 1U
                            : 0U
                    ))
            {
                error =
                    L"FBX contains no skeletal meshes.";

                return AssetResult::
                    UnsupportedFeature;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            report.Clear();

            error =
                L"Not enough memory to import "
                L"skeletal assets.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            report.Clear();

            error =
                L"Unexpected skeletal import failure.";

            return AssetResult::InternalError;
        }
    }
}