#include "Assets/FbxStaticMeshImporter.h"

#include "Assets/AssetData.h"
#include "Assets/AssetPath.h"
#include "Assets/MaterialAssetWriter.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/MaterialAsset.h"
#include "Assets/MeshAsset.h"
#include "Assets/MeshAssetBuilder.h"
#include "Assets/StaticMeshPrefab.h"

#include "FbxImporterCommon.h"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <new>
#include <optional>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        struct OutputPackagePaths final
        {
            std::filesystem::path meshDirectory;
            std::filesystem::path meshesRoot;
            std::filesystem::path dataRoot;
            std::filesystem::path packagePath;
            std::filesystem::path materialDirectory;
            std::filesystem::path textureDirectory;
        };

        [[nodiscard]]
        std::wstring Lowercase(
            std::wstring value)
        {
            for (wchar_t& character : value)
            {
                character =
                    static_cast<wchar_t>(
                        std::towlower(character));
            }

            return value;
        }

        [[nodiscard]]
        AssetResult ResolveOutputPackage(
            const std::filesystem::path&
                destinationDirectory,
            OutputPackagePaths& output,
            std::wstring& error)
        {
            output = {};

            if (destinationDirectory.empty())
            {
                error =
                    L"Static mesh destination "
                    L"directory is empty.";

                return AssetResult::InvalidPath;
            }

            try
            {
                output.meshDirectory =
                    std::filesystem::absolute(
                        destinationDirectory).
                        lexically_normal();

                std::filesystem::path cursor =
                    output.meshDirectory;

                while (!cursor.empty())
                {
                    if (
                        Lowercase(
                            cursor.filename().
                                wstring()) ==
                        L"meshes")
                    {
                        output.meshesRoot =
                            cursor;

                        break;
                    }

                    const std::filesystem::path parent =
                        cursor.parent_path();

                    if (parent == cursor)
                    {
                        break;
                    }

                    cursor = parent;
                }

                if (output.meshesRoot.empty())
                {
                    error =
                        L"Static meshes must be imported "
                        L"inside game/Data/Meshes.";

                    return AssetResult::InvalidPath;
                }

                output.packagePath =
                    output.meshDirectory.
                        lexically_relative(
                            output.meshesRoot);

                if (
                    output.packagePath == L".")
                {
                    output.packagePath.clear();
                }

                for (
                    const auto& component :
                    output.packagePath)
                {
                    if (component == L"..")
                    {
                        error =
                            L"Destination path escapes "
                            L"Data/Meshes.";

                        return AssetResult::InvalidPath;
                    }
                }

                output.dataRoot =
                    output.meshesRoot.parent_path();

                output.materialDirectory =
                    output.dataRoot /
                    L"Materials" /
                    output.packagePath;

                output.textureDirectory =
                    output.dataRoot /
                    L"Textures" /
                    output.packagePath;

                return AssetResult::Success;
            }
            catch (...)
            {
                output = {};

                error =
                    L"Failed to resolve mirrored "
                    L"asset directories.";

                return AssetResult::InvalidPath;
            }
        }

        void AppendWrittenFile(
            FbxImportReport& report,
            const std::filesystem::path& file)
        {
            const auto found =
                std::find(
                    report.writtenFiles.begin(),
                    report.writtenFiles.end(),
                    file);

            if (found == report.writtenFiles.end())
            {
                report.writtenFiles.push_back(file);
            }
        }

        [[nodiscard]]
        bool FilesEqual(
            const std::filesystem::path& left,
            const std::filesystem::path& right)
        {
            std::error_code filesystemError;

            const std::uintmax_t leftSize =
                std::filesystem::file_size(
                    left,
                    filesystemError);

            if (filesystemError)
            {
                return false;
            }

            filesystemError.clear();

            const std::uintmax_t rightSize =
                std::filesystem::file_size(
                    right,
                    filesystemError);

            if (
                filesystemError ||
                leftSize != rightSize)
            {
                return false;
            }

            std::ifstream leftStream(
                left,
                std::ios::binary);

            std::ifstream rightStream(
                right,
                std::ios::binary);

            if (!leftStream || !rightStream)
            {
                return false;
            }

            std::array<char, 64U * 1024U>
                leftBytes{};

            std::array<char, 64U * 1024U>
                rightBytes{};

            while (leftStream && rightStream)
            {
                leftStream.read(
                    leftBytes.data(),
                    static_cast<std::streamsize>(
                        leftBytes.size()));

                rightStream.read(
                    rightBytes.data(),
                    static_cast<std::streamsize>(
                        rightBytes.size()));

                const std::streamsize leftCount =
                    leftStream.gcount();

                const std::streamsize rightCount =
                    rightStream.gcount();

                if (leftCount != rightCount)
                {
                    return false;
                }

                if (leftCount <= 0)
                {
                    break;
                }

                const std::size_t byteCount =
                    static_cast<std::size_t>(
                        leftCount);

                if (
                    !std::equal(
                        leftBytes.begin(),
                        leftBytes.begin() +
                            static_cast<
                                std::ptrdiff_t>(
                                    byteCount),
                        rightBytes.begin()))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        std::filesystem::path FindExternalTexture(
            const ufbx_texture& texture,
            const std::filesystem::path&
                sourceDirectory)
        {
            const ufbx_string names[]
            {
                texture.filename,
                texture.absolute_filename,
                texture.relative_filename
            };

            for (const ufbx_string name : names)
            {
                if (
                    name.data == nullptr ||
                    name.length == 0U)
                {
                    continue;
                }

                const std::filesystem::path candidate =
                    std::filesystem::u8path(
                        std::string(
                            name.data,
                            name.length));

                const std::filesystem::path resolved =
                    candidate.is_absolute()
                        ? candidate
                        : sourceDirectory /
                            candidate;

                std::error_code filesystemError;

                if (
                    std::filesystem::is_regular_file(
                        resolved,
                        filesystemError) &&
                    !filesystemError)
                {
                    return
                        resolved.lexically_normal();
                }
            }

            return {};
        }

        [[nodiscard]]
        std::optional<AssetPath> ImportTexture(
            const ufbx_material_map& map,
            const std::filesystem::path&
                sourceDirectory,
            const OutputPackagePaths& package,
            const bool overwrite,
            FbxImportReport& report)
        {
            if (
                map.texture == nullptr ||
                map.texture->type !=
                    UFBX_TEXTURE_FILE)
            {
                return std::nullopt;
            }

            const ufbx_texture& texture =
                *map.texture;

            const std::filesystem::path source =
                FindExternalTexture(
                    texture,
                    sourceDirectory);

            if (source.empty())
            {
                std::wstring warning =
                    L"Missing external FBX texture: ";

                if (
                    texture.relative_filename.data !=
                    nullptr)
                {
                    fbx_detail::AppendAscii(
                        warning,
                        texture.relative_filename.data);
                }

                report.warnings.push_back(
                    std::move(warning));

                return std::nullopt;
            }

            std::filesystem::path filename =
                source.filename();

            std::filesystem::path destination =
                package.textureDirectory /
                filename;

            std::error_code filesystemError;

            std::filesystem::create_directories(
                destination.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                report.warnings.push_back(
                    L"Failed to create texture "
                    L"destination directory.");

                return std::nullopt;
            }

            bool destinationExists =
                std::filesystem::is_regular_file(
                    destination,
                    filesystemError) &&
                !filesystemError;

            if (
                destinationExists &&
                !FilesEqual(
                    source,
                    destination))
            {
                filename =
                    filename.stem().wstring() +
                    L"_" +
                    std::to_wstring(
                        texture.element_id) +
                    filename.extension().wstring();

                destination =
                    package.textureDirectory /
                    filename;

                filesystemError.clear();

                destinationExists =
                    std::filesystem::is_regular_file(
                        destination,
                        filesystemError) &&
                    !filesystemError;
            }

            bool samePhysicalFile = false;

            filesystemError.clear();

            if (destinationExists)
            {
                samePhysicalFile =
                    std::filesystem::equivalent(
                        source,
                        destination,
                        filesystemError) &&
                    !filesystemError;
            }

            if (
                !samePhysicalFile &&
                (
                    !destinationExists ||
                    !FilesEqual(
                        source,
                        destination)
                ))
            {
                filesystemError.clear();

                const std::filesystem::
                    copy_options copyOptions =
                        overwrite
                            ? std::filesystem::
                                copy_options::
                                    overwrite_existing
                            : std::filesystem::
                                copy_options::none;

                std::filesystem::copy_file(
                    source,
                    destination,
                    copyOptions,
                    filesystemError);

                if (filesystemError)
                {
                    report.warnings.push_back(
                        L"Failed to copy FBX texture: " +
                        source.wstring());

                    return std::nullopt;
                }
            }

            AppendWrittenFile(
                report,
                destination);

            const std::filesystem::path runtimePath =
                std::filesystem::path(L"Data") /
                L"Textures" /
                package.packagePath /
                filename;

            AssetPath assetPath;

            const AssetResult pathResult =
                AssetPath::TryCreate(
                    runtimePath.generic_u8string(),
                    assetPath);

            if (Failed(pathResult))
            {
                report.warnings.push_back(
                    L"Copied texture has an invalid "
                    L"runtime asset path: " +
                    runtimePath.wstring());

                return std::nullopt;
            }

            return assetPath;
        }

        [[nodiscard]]
        float Clamp01(
            const ufbx_real value,
            const float fallback) noexcept
        {
            if (!std::isfinite(value))
            {
                return fallback;
            }

            return std::clamp(
                static_cast<float>(value),
                0.0F,
                1.0F);
        }

        [[nodiscard]]
        const ufbx_material_map& PreferTexture(
            const ufbx_material_map& preferred,
            const ufbx_material_map& fallback)
            noexcept
        {
            return preferred.texture != nullptr
                ? preferred
                : fallback;
        }

        [[nodiscard]]
        const ufbx_material*
            ResolveSourceMaterial(
                const ufbx_node& node,
                const ufbx_mesh& mesh,
                const std::size_t slot) noexcept
        {
            if (
                slot < node.materials.count &&
                node.materials.data[slot] != nullptr)
            {
                return node.materials.data[slot];
            }

            if (
                slot < mesh.materials.count &&
                mesh.materials.data[slot] != nullptr)
            {
                return mesh.materials.data[slot];
            }

            return nullptr;
        }

        [[nodiscard]]
        std::string MakeSlotPrefix(
            const std::size_t slot)
        {
            if (slot < 10U)
            {
                return
                    "000" +
                    std::to_string(slot);
            }

            if (slot < 100U)
            {
                return
                    "00" +
                    std::to_string(slot);
            }

            if (slot < 1000U)
            {
                return
                    "0" +
                    std::to_string(slot);
            }

            return std::to_string(slot);
        }

        [[nodiscard]]
        AssetResult WriteMaterial(
            const ufbx_material* sourceMaterial,
            const std::size_t slot,
            const std::string& meshBaseName,
            const std::filesystem::path&
                sourceDirectory,
            const OutputPackagePaths& package,
            const bool overwrite,
            FbxImportReport& report,
            std::wstring& error)
        {
            MaterialAssetDesc description;

            const std::string fallbackName =
                "Material_" +
                std::to_string(slot);

            const std::string materialName =
                sourceMaterial != nullptr
                    ? fbx_detail::SanitizeName(
                        fbx_detail::ToString(
                            sourceMaterial->name,
                            fallbackName.c_str()),
                        fallbackName)
                    : fallbackName;

            description.debugName =
                materialName;

            std::string lowercaseMaterialName =
                materialName;

            std::transform(
                lowercaseMaterialName.begin(),
                lowercaseMaterialName.end(),
                lowercaseMaterialName.begin(),
                [](const unsigned char value)
                {
                    return static_cast<char>(
                        std::tolower(value));
                });

            const bool glassFallback =
                lowercaseMaterialName.find(
                    "glass") !=
                std::string::npos;

            if (sourceMaterial != nullptr)
            {
                const auto& pbr =
                    sourceMaterial->pbr;

                if (pbr.base_color.has_value)
                {
                    description.baseColorFactor =
                    {
                        Clamp01(
                            pbr.base_color.
                                value_vec4.x,
                            1.0F),

                        Clamp01(
                            pbr.base_color.
                                value_vec4.y,
                            1.0F),

                        Clamp01(
                            pbr.base_color.
                                value_vec4.z,
                            1.0F),

                        Clamp01(
                            pbr.base_color.
                                value_vec4.w,
                            1.0F)
                    };
                }

                if (pbr.base_factor.has_value)
                {
                    const float factor =
                        Clamp01(
                            pbr.base_factor.
                                value_real,
                            1.0F);

                    for (
                        std::size_t component = 0U;
                        component < 3U;
                        ++component)
                    {
                        description.
                            baseColorFactor[
                                component] *=
                            factor;
                    }
                }

                if (pbr.opacity.has_value)
                {
                    description.baseColorFactor[3U] *=
                        Clamp01(
                            pbr.opacity.value_real,
                            1.0F);

                    if (
                        description.
                            baseColorFactor[3U] <
                        0.999F)
                    {
                        description.alphaMode =
                            MaterialAlphaMode::Blend;
                    }
                }

                description.roughnessFactor =
                    pbr.roughness.has_value
                        ? Clamp01(
                            pbr.roughness.value_real,
                            1.0F)
                        : 1.0F;

                description.metallicFactor =
                    pbr.metalness.has_value
                        ? Clamp01(
                            pbr.metalness.value_real,
                            0.0F)
                        : 0.0F;

                if (pbr.emission_color.has_value)
                {
                    description.emissiveFactor =
                    {
                        (std::max)(
                            0.0F,
                            static_cast<float>(
                                pbr.emission_color.
                                    value_vec3.x)),

                        (std::max)(
                            0.0F,
                            static_cast<float>(
                                pbr.emission_color.
                                    value_vec3.y)),

                        (std::max)(
                            0.0F,
                            static_cast<float>(
                                pbr.emission_color.
                                    value_vec3.z))
                    };
                }

                description.emissiveStrength =
                    pbr.emission_factor.has_value
                        ? std::clamp(
                            static_cast<float>(
                                pbr.emission_factor.
                                    value_real),
                            0.0F,
                            64.0F)
                        : 0.0F;

                description.doubleSided =
                    sourceMaterial->
                        features.
                        double_sided.enabled;

                const auto& fbx =
                    sourceMaterial->fbx;

                description.baseColorTexture =
                    ImportTexture(
                        PreferTexture(
                            pbr.base_color,
                            fbx.diffuse_color),
                        sourceDirectory,
                        package,
                        overwrite,
                        report);

                const ufbx_material_map&
                    legacyNormal =
                        fbx.normal_map.texture !=
                            nullptr
                            ? fbx.normal_map
                            : fbx.bump;

                description.normalTexture =
                    ImportTexture(
                        PreferTexture(
                            pbr.normal_map,
                            legacyNormal),
                        sourceDirectory,
                        package,
                        overwrite,
                        report);

                description.roughnessTexture =
                    ImportTexture(
                        pbr.roughness,
                        sourceDirectory,
                        package,
                        overwrite,
                        report);

                description.emissiveTexture =
                    ImportTexture(
                        PreferTexture(
                            pbr.emission_color,
                            fbx.emission_color),
                        sourceDirectory,
                        package,
                        overwrite,
                        report);
            }

            if (
                glassFallback &&
                description.alphaMode ==
                    MaterialAlphaMode::Opaque)
            {
                description.alphaMode =
                    MaterialAlphaMode::Blend;

                description.baseColorFactor[3U] =
                    0.32F;

                description.doubleSided = true;

                description.roughnessFactor =
                    (std::min)(
                        description.roughnessFactor,
                        0.18F);
            }

            MaterialAsset material;

            AssetResult result =
                material.Initialize(
                    std::move(description));

            if (Failed(result))
            {
                error =
                    L"Failed to initialize imported "
                    L"material: ";

                fbx_detail::AppendAscii(
                    error,
                    ToString(result));

                return result;
            }

            AssetData encoded;

            result =
                MaterialAssetWriter::Encode(
                    material,
                    encoded);

            if (Failed(result))
            {
                error =
                    L"Failed to encode imported "
                    L"material: ";

                fbx_detail::AppendAscii(
                    error,
                    ToString(result));

                return result;
            }

            const std::string filename =
                meshBaseName +
                "_" +
                MakeSlotPrefix(slot) +
                "_" +
                materialName +
                ".material";

            const std::filesystem::path destination =
                package.materialDirectory /
                std::filesystem::u8path(
                    filename);

            const std::byte* const data =
                encoded.GetData();

            if (
                data == nullptr ||
                encoded.GetSize() == 0U)
            {
                error = L"MaterialAssetWriter returned " L"empty data.";
                return AssetResult::InternalError;
            }

            std::vector<std::byte> bytes(
                data,
                data + encoded.GetSize());

            result =
                fbx_detail::SaveBinary(
                    destination,
                    bytes,
                    overwrite,
                    error);

            if (Failed(result))
            {
                return result;
            }

            AppendWrittenFile(
                report,
                destination);

            return AssetResult::Success;
        }

        [[nodiscard]]
        AssetResult SaveStaticMesh(
            const ufbx_node& node,
            const ufbx_mesh& sourceMesh,
            const std::filesystem::path&
                destination,
            const OutputPackagePaths& package,
            const std::filesystem::path&
                sourceDirectory,
            const bool overwrite,
            FbxImportReport& report,
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
                destination.stem().u8string();

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

            /*
             * Материалы называются:
             *
             * MeshName_0000_Material.material
             * MeshName_0001_Material.material
             *
             * Поэтому renderer может отличить
             * материалы разных .mesh в одной папке.
             */
            for (
                std::size_t slot = 0U;
                slot < cornersByMaterial.size();
                ++slot)
            {
                const ufbx_material*
                    const sourceMaterial =
                        ResolveSourceMaterial(
                            node,
                            sourceMesh,
                            slot);

                result =
                    WriteMaterial(
                        sourceMaterial,
                        slot,
                        debugName,
                        sourceDirectory,
                        package,
                        overwrite,
                        report,
                        error);

                if (Failed(result))
                {
                    return result;
                }
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

            result =
                fbx_detail::SaveBinary(
                    destination,
                    bytes,
                    overwrite,
                    error);

            if (Failed(result))
            {
                return result;
            }

            AppendWrittenFile(
                report,
                destination);

            return AssetResult::Success;
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
                    baseName + ".mesh");
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
            OutputPackagePaths package;

            AssetResult result =
                ResolveOutputPackage(
                    options.destinationDirectory,
                    package,
                    error);

            if (Failed(result))
            {
                return result;
            }

            fbx_detail::ScenePtr scene;

            result =
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

            StaticMeshPrefab prefab;

            prefab.name =
                fbx_detail::SanitizeName(
                    sourcePath.stem().u8string(),
                    "ImportedPrefab");

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
                                package.meshDirectory,
                                *mesh,
                                usedNames);

                    result =
                        SaveStaticMesh(
                            *node,
                            *mesh,
                            destination,
                            package,
                            sourcePath.parent_path(),
                            options.overwriteExisting,
                            report,
                            error);

                    if (Failed(result))
                    {
                        return result;
                    }

                    std::string runtimeMeshPath;

                    result =
                        fbx_detail::MakeAssetPath(
                            destination,
                            runtimeMeshPath,
                            error);

                    if (Failed(result))
                    {
                        return result;
                    }

                    AssetPath meshAssetPath;

                    result =
                        AssetPath::TryCreate(
                            runtimeMeshPath,
                            meshAssetPath);

                    if (Failed(result))
                    {
                        error =
                            L"Imported .mesh has an invalid "
                            L"runtime asset path.";

                        return result;
                    }

                    StaticMeshPrefabPart prefabPart;

                    prefabPart.name =
                        fbx_detail::SanitizeName(
                            fbx_detail::ToString(
                                node->name,
                                mesh->name.data != nullptr
                                    ? mesh->name.data
                                    : "StaticMesh"),
                            "StaticMesh");

                    prefabPart.meshPath =
                        std::move(meshAssetPath);

                    prefab.parts.push_back(
                        std::move(prefabPart));
                }
            }

            const bool shouldCreatePrefab =
            options.createPrefab &&
            (
                prefab.parts.size() > 1U ||
                (
                    options.createSingleMeshPrefab &&
                    prefab.parts.size() == 1U
                )
            );

            if (shouldCreatePrefab)
            {
                const std::filesystem::path prefabPath =
                    package.meshDirectory /
                    std::filesystem::u8path(
                        prefab.name + ".prefab");

                result =
                    StaticMeshPrefabCodec::Save(
                        prefabPath,
                        prefab,
                        options.overwriteExisting,
                        error);

                if (Failed(result))
                {
                    return result;
                }

                AppendWrittenFile(
                    report,
                    prefabPath);
            }

            const auto meshFile =
                std::find_if(
                    report.writtenFiles.begin(),
                    report.writtenFiles.end(),
                    [](const std::filesystem::path&
                           path)
                    {
                        return
                            Lowercase(
                                path.extension().
                                    wstring()) ==
                            L".mesh";
                    });

            if (
                meshFile ==
                report.writtenFiles.end())
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
            error =
                L"Not enough memory to import "
                L"static meshes.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            error =
                L"Unexpected static mesh "
                L"import failure.";

            return AssetResult::InternalError;
        }
    }
}