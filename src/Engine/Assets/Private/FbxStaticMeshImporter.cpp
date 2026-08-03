#include "Assets/FbxStaticMeshImporter.h"

#include "Assets/AssetData.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/MeshAsset.h"
#include "Assets/MeshAssetBuilder.h"

#include "FbxImporterCommon.h"

#include <ufbx.h>

#include <algorithm>
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
        [[nodiscard]]
        AssetResult SaveStaticMesh(
            const ufbx_node& node,
            const ufbx_mesh& sourceMesh,
            const std::filesystem::path&
                destination,
            const bool overwrite,
            std::wstring& error)
        {
            std::vector<
                std::vector<std::uint32_t>>
                cornersByMaterial;

            AssetResult result =
                fbx_detail::BuildMaterialCorners(
                    node,
                    sourceMesh,
                    cornersByMaterial,
                    error);

            if (Failed(result))
            {
                return result;
            }

            std::vector<StaticMeshVertex>
                vertices;

            std::vector<std::uint32_t>
                indices;

            std::vector<MeshSubmesh>
                submeshes;

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

                MeshSubmesh submesh{};

                submesh.firstIndex =
                    static_cast<std::uint32_t>(
                        indices.size());

                submesh.baseVertex = 0;

                submesh.materialSlot =
                    static_cast<std::uint32_t>(
                        materialSlot);

                for (
                    const std::uint32_t cornerIndex :
                    corners)
                {
                    if (
                        cornerIndex >=
                            sourceMesh.num_indices ||
                        vertices.size() >=
                            fbx_detail::
                                MaximumImportedVertices)
                    {
                        error =
                            L"FBX static mesh exceeds "
                            L"the vertex limit or contains "
                            L"an invalid corner.";

                        return AssetResult::FileTooLarge;
                    }

                    const fbx_detail::
                        ImportedVertexFrame frame =
                            fbx_detail::
                                ReadVertexFrame(
                                    node,
                                    sourceMesh,
                                    cornerIndex);

                    StaticMeshVertex vertex{};

                    vertex.position =
                        frame.position;

                    vertex.normal =
                        frame.normal;

                    vertex.tangent =
                        frame.tangent;

                    vertex.texcoord0 =
                        frame.texcoord;

                    const std::uint32_t newIndex =
                        static_cast<std::uint32_t>(
                            vertices.size());

                    vertices.push_back(vertex);
                    indices.push_back(newIndex);
                }

                submesh.indexCount =
                    static_cast<std::uint32_t>(
                        indices.size()) -
                    submesh.firstIndex;

                if (submesh.indexCount != 0U)
                {
                    submeshes.push_back(submesh);
                }
            }

            if (
                vertices.empty() ||
                indices.empty() ||
                submeshes.empty())
            {
                error =
                    L"FBX contains no renderable "
                    L"static mesh triangles.";

                return AssetResult::
                    UnsupportedFeature;
            }

            MeshAsset mesh;

            const std::string debugName =
                fbx_detail::SanitizeName(
                    fbx_detail::ToString(
                        sourceMesh.name,
                        "StaticMesh"),
                    "StaticMesh");

            result =
                MeshAssetBuilder::Build(
                    vertices.data(),
                    vertices.size(),
                    indices.data(),
                    indices.size(),
                    submeshes.data(),
                    submeshes.size(),
                    static_cast<std::uint32_t>(
                        cornersByMaterial.size()),
                    debugName,
                    mesh);

            if (Failed(result))
            {
                error =
                    L"MeshAssetBuilder failed: ";

                fbx_detail::AppendAscii(
                    error,
                    ToString(result));

                return result;
            }

            AssetData encoded;

            result =
                LtsMeshWriter::Encode(
                    mesh,
                    encoded);

            if (Failed(result))
            {
                error =
                    L"LtsMeshWriter failed: ";

                fbx_detail::AppendAscii(
                    error,
                    ToString(result));

                return result;
            }

            const std::byte* const data =
                encoded.GetData();

            if (
                data == nullptr ||
                encoded.GetSize() == 0U)
            {
                error =
                    L"LtsMeshWriter returned empty data.";

                return AssetResult::InternalError;
            }

            std::vector<std::byte> bytes(
                data,
                data + encoded.GetSize());

            return
                fbx_detail::SaveBinary(
                    destination,
                    bytes,
                    overwrite,
                    error);
        }

        [[nodiscard]]
        std::filesystem::path MakeOutputPath(
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
                        "StaticMesh"),
                    "StaticMesh");

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
                    baseName + ".sm");
        }
    }

    bool FbxStaticMeshImporter::
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

    AssetResult FbxStaticMeshImporter::Import(
        const std::filesystem::path& sourcePath,
        const FbxStaticMeshImportOptions& options,
        FbxImportReport& report,
        std::wstring& error) noexcept
    {
        report.Clear();
        error.clear();

        try
        {
            if (
                options.destinationDirectory.empty())
            {
                error =
                    L"Static mesh destination "
                    L"directory is empty.";

                return AssetResult::InvalidPath;
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
                    mesh->skin_deformers.count != 0U)
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
                            L"Static FBX mesh contains "
                            L"a null instance.");

                        continue;
                    }

                    const std::filesystem::path
                        destination =
                            MakeOutputPath(
                                options.
                                    destinationDirectory,
                                *mesh,
                                usedNames);

                    result =
                        SaveStaticMesh(
                            *node,
                            *mesh,
                            destination,
                            options.
                                overwriteExisting,
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

            if (report.writtenFiles.empty())
            {
                error =
                    L"FBX contains no static meshes.";

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
                L"static meshes.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            report.Clear();

            error =
                L"Unexpected static mesh import failure.";

            return AssetResult::InternalError;
        }
    }
}