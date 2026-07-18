#include "Assets/FbxStaticMeshImporter.h"

#include "Assets/AssetData.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/LtsMaterialWriter.h"
#include "Assets/MaterialAsset.h"
#include "Assets/MeshAsset.h"
#include "Assets/MeshAssetBuilder.h"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        constexpr std::size_t MaximumVertices = 10'000'000U;
        constexpr std::size_t MaximumIndices = 30'000'000U;

        void AppendAscii(std::wstring& destination, const char* text)
        {
            if (text == nullptr) return;
            while (*text != '\0')
            {
                destination.push_back(static_cast<unsigned char>(*text));
                ++text;
            }
        }

        void Normalize(std::array<float, 3U>& value,
                       const std::array<float, 3U>& fallback) noexcept
        {
            const float lengthSquared = value[0] * value[0] +
                value[1] * value[1] + value[2] * value[2];
            if (!std::isfinite(lengthSquared) || lengthSquared <= 0.000001F)
            {
                value = fallback;
                return;
            }
            const float inverseLength = 1.0F / std::sqrt(lengthSquared);
            for (float& component : value) component *= inverseLength;
        }

        std::uint32_t ResolveMaterialSlot(
            const ufbx_node& node, const ufbx_mesh& mesh,
            const std::size_t faceIndex,
            std::unordered_map<std::string, std::uint32_t>& slots,
            std::vector<const ufbx_material*>& slotMaterials)
        {
            const std::uint32_t localSlot = faceIndex < mesh.face_material.count
                ? mesh.face_material.data[faceIndex] : 0U;
            std::string name = "__default";
            const ufbx_material* material = nullptr;
            if (localSlot < node.materials.count && node.materials.data[localSlot] != nullptr)
            {
                material = node.materials.data[localSlot];
                const ufbx_string sourceName = material->name;
                name.assign(sourceName.data, sourceName.length);
            }
            const auto found = slots.find(name);
            if (found != slots.end()) return found->second;
            const std::uint32_t slot = static_cast<std::uint32_t>(slots.size());
            slots.emplace(std::move(name), slot);
            slotMaterials.push_back(material);
            return slot;
        }

        std::string SanitizeName(const ufbx_string source, const std::string& fallback)
        {
            std::string result(source.data != nullptr ? source.data : "", source.length);
            for (char& character : result)
            {
                const unsigned char value = static_cast<unsigned char>(character);
                if (value < 32U || character == '<' || character == '>' ||
                    character == ':' || character == '"' || character == '/' ||
                    character == '\\' || character == '|' || character == '?' ||
                    character == '*') character = '_';
            }
            while (!result.empty() && (result.back() == ' ' || result.back() == '.'))
                result.pop_back();
            return result.empty() ? fallback : result;
        }

        bool FilesEqual(const std::filesystem::path& left,
                        const std::filesystem::path& right)
        {
            std::error_code filesystemError;
            const auto leftSize = std::filesystem::file_size(left, filesystemError);
            if (filesystemError) return false;
            const auto rightSize = std::filesystem::file_size(right, filesystemError);
            if (filesystemError || leftSize != rightSize) return false;
            std::ifstream leftStream(left, std::ios::binary);
            std::ifstream rightStream(right, std::ios::binary);
            std::array<char, 64U * 1024U> leftBytes{};
            std::array<char, 64U * 1024U> rightBytes{};
            while (leftStream && rightStream)
            {
                leftStream.read(leftBytes.data(), static_cast<std::streamsize>(leftBytes.size()));
                rightStream.read(rightBytes.data(), static_cast<std::streamsize>(rightBytes.size()));
                const std::streamsize count = leftStream.gcount();
                if (count != rightStream.gcount() ||
                    !std::equal(leftBytes.begin(), leftBytes.begin() + count, rightBytes.begin()))
                    return false;
            }
            return true;
        }

        std::filesystem::path FindExternalTexture(
            const ufbx_texture& texture, const std::filesystem::path& sourceDirectory)
        {
            const ufbx_string names[] =
            {
                texture.filename,
                texture.absolute_filename,
                texture.relative_filename
            };
            for (const ufbx_string name : names)
            {
                if (name.data == nullptr || name.length == 0U) continue;
                const std::filesystem::path candidate =
                    std::filesystem::u8path(std::string(name.data, name.length));
                const std::filesystem::path resolved = candidate.is_absolute()
                    ? candidate : sourceDirectory / candidate;
                std::error_code filesystemError;
                if (std::filesystem::is_regular_file(resolved, filesystemError) &&
                    !filesystemError) return resolved.lexically_normal();
            }
            return {};
        }

        std::optional<AssetPath> ImportTexture(
            const ufbx_material_map& map,
            const std::filesystem::path& sourceDirectory,
            const std::filesystem::path& textureRoot,
            const std::filesystem::path& packagePath,
            std::vector<std::wstring>& warnings)
        {
            if (map.texture == nullptr ||
                map.texture->type != UFBX_TEXTURE_FILE) return std::nullopt;
            const ufbx_texture& texture = *map.texture;
            const std::filesystem::path source = FindExternalTexture(texture, sourceDirectory);
            if (source.empty())
            {
                std::wstring warning = L"Missing external FBX texture: ";
                if (texture.relative_filename.data != nullptr)
                    AppendAscii(warning, texture.relative_filename.data);
                warnings.push_back(std::move(warning));
                return std::nullopt;
            }
            std::filesystem::path filename = source.filename();
            std::filesystem::path destination = textureRoot / packagePath / filename;
            std::error_code filesystemError;
            std::filesystem::create_directories(destination.parent_path(), filesystemError);
            if (filesystemError)
            {
                warnings.push_back(L"Failed to create texture destination directory.");
                return std::nullopt;
            }
            if (std::filesystem::is_regular_file(destination, filesystemError) &&
                !filesystemError && !FilesEqual(source, destination))
            {
                filename = filename.stem().wstring() + L"_" +
                    std::to_wstring(texture.element_id) + filename.extension().wstring();
                destination = textureRoot / packagePath / filename;
            }
            if (!std::filesystem::is_regular_file(destination, filesystemError) || filesystemError)
            {
                filesystemError.clear();
                std::filesystem::copy_file(source, destination,
                    std::filesystem::copy_options::none, filesystemError);
                if (filesystemError)
                {
                    warnings.push_back(L"Failed to copy FBX texture: " + source.wstring());
                    return std::nullopt;
                }
            }
            const std::filesystem::path runtime =
                std::filesystem::path(L"Data") / L"Textures" / packagePath / filename;
            AssetPath assetPath;
            if (Failed(AssetPath::TryCreate(runtime.generic_u8string(), assetPath)))
            {
                warnings.push_back(L"Copied texture has an invalid asset path: " + runtime.wstring());
                return std::nullopt;
            }
            return assetPath;
        }

        float Clamp01(const ufbx_real value, const float fallback) noexcept
        {
            return std::isfinite(value)
                ? std::clamp(static_cast<float>(value), 0.0F, 1.0F) : fallback;
        }

        const ufbx_material_map& PreferTexture(
            const ufbx_material_map& preferred,
            const ufbx_material_map& fallback) noexcept
        {
            return preferred.texture != nullptr ? preferred : fallback;
        }

        AssetResult WriteMaterial(
            const ufbx_material* sourceMaterial,
            const std::size_t slot,
            const std::filesystem::path& sourceDirectory,
            const std::filesystem::path& materialRoot,
            const std::filesystem::path& textureRoot,
            const std::filesystem::path& packagePath,
            std::vector<std::wstring>& warnings)
        {
            MaterialAssetDesc desc;
            const std::string fallbackName = "Material_" + std::to_string(slot);
            const std::string materialName = sourceMaterial != nullptr
                ? SanitizeName(sourceMaterial->name, fallbackName) : fallbackName;
            desc.debugName = materialName;
            std::string lowercaseMaterialName = materialName;
            std::transform(lowercaseMaterialName.begin(), lowercaseMaterialName.end(),
                lowercaseMaterialName.begin(), [](const unsigned char value)
                { return static_cast<char>(std::tolower(value)); });
            const bool glassFallback =
                lowercaseMaterialName.find("glass") != std::string::npos;
            if (sourceMaterial != nullptr)
            {
                const auto& pbr = sourceMaterial->pbr;
                if (pbr.base_color.has_value)
                {
                    desc.baseColorFactor = {
                        Clamp01(pbr.base_color.value_vec4.x, 1.0F),
                        Clamp01(pbr.base_color.value_vec4.y, 1.0F),
                        Clamp01(pbr.base_color.value_vec4.z, 1.0F),
                        Clamp01(pbr.base_color.value_vec4.w, 1.0F) };
                }
                if (pbr.base_factor.has_value)
                {
                    const float factor = Clamp01(pbr.base_factor.value_real, 1.0F);
                    for (std::size_t component = 0U; component < 3U; ++component)
                        desc.baseColorFactor[component] *= factor;
                }
                if (pbr.opacity.has_value)
                {
                    desc.baseColorFactor[3] *= Clamp01(pbr.opacity.value_real, 1.0F);
                    if (desc.baseColorFactor[3] < 0.999F)
                        desc.alphaMode = MaterialAlphaMode::Blend;
                }
                desc.roughnessFactor = pbr.roughness.has_value
                    ? Clamp01(pbr.roughness.value_real, 1.0F) : 1.0F;
                desc.metallicFactor = pbr.metalness.has_value
                    ? Clamp01(pbr.metalness.value_real, 0.0F) : 0.0F;
                if (pbr.emission_color.has_value)
                {
                    desc.emissiveFactor = {
                        std::max(0.0F, static_cast<float>(pbr.emission_color.value_vec3.x)),
                        std::max(0.0F, static_cast<float>(pbr.emission_color.value_vec3.y)),
                        std::max(0.0F, static_cast<float>(pbr.emission_color.value_vec3.z)) };
                }
                desc.emissiveStrength = pbr.emission_factor.has_value
                    ? std::clamp(static_cast<float>(pbr.emission_factor.value_real), 0.0F, 64.0F)
                    : 0.0F;
                desc.doubleSided = sourceMaterial->features.double_sided.enabled;
                const auto& fbx = sourceMaterial->fbx;
                desc.baseColorTexture = ImportTexture(
                    PreferTexture(pbr.base_color, fbx.diffuse_color), sourceDirectory,
                    textureRoot, packagePath, warnings);
                const ufbx_material_map& legacyNormal = fbx.normal_map.texture != nullptr
                    ? fbx.normal_map : fbx.bump;
                desc.normalTexture = ImportTexture(
                    PreferTexture(pbr.normal_map, legacyNormal), sourceDirectory,
                    textureRoot, packagePath, warnings);
                desc.roughnessTexture = ImportTexture(pbr.roughness, sourceDirectory,
                    textureRoot, packagePath, warnings);
                desc.emissiveTexture = ImportTexture(
                    PreferTexture(pbr.emission_color, fbx.emission_color), sourceDirectory,
                    textureRoot, packagePath, warnings);
            }
            if (glassFallback && desc.alphaMode == MaterialAlphaMode::Opaque)
            {
                desc.alphaMode = MaterialAlphaMode::Blend;
                desc.baseColorFactor[3] = 0.32F;
                desc.doubleSided = true;
                desc.roughnessFactor = std::min(desc.roughnessFactor, 0.18F);
            }
            MaterialAsset material;
            AssetResult result = material.Initialize(std::move(desc));
            if (Failed(result)) return result;
            AssetData encoded;
            result = LtsMaterialWriter::Encode(material, encoded);
            if (Failed(result)) return result;
            const std::filesystem::path destination =
                materialRoot / packagePath / std::filesystem::u8path(
                    (slot < 10U ? "000" : slot < 100U ? "00" : slot < 1000U ? "0" : "") +
                    std::to_string(slot) + "_" + materialName + ".ltsmaterial");
            std::error_code filesystemError;
            std::filesystem::create_directories(destination.parent_path(), filesystemError);
            if (filesystemError) return AssetResult::IoError;
            std::ofstream output(destination, std::ios::binary | std::ios::trunc);
            if (!output) return AssetResult::IoError;
            output.write(reinterpret_cast<const char*>(encoded.GetData()),
                static_cast<std::streamsize>(encoded.GetSize()));
            output.close();
            return output ? AssetResult::Success : AssetResult::IoError;
        }

        AssetResult Save(const std::filesystem::path& destination,
                         const MeshAsset& mesh, std::wstring& error)
        {
            AssetData encoded;
            const AssetResult encodeResult = LtsMeshWriter::Encode(mesh, encoded);
            if (Failed(encodeResult))
            {
                error = L"LtsMeshWriter failed: ";
                AppendAscii(error, ToString(encodeResult));
                return encodeResult;
            }
            std::error_code filesystemError;
            std::filesystem::create_directories(destination.parent_path(), filesystemError);
            if (filesystemError)
            {
                error = L"Failed to create the destination mesh directory.";
                return AssetResult::IoError;
            }
            std::filesystem::path temporary = destination;
            temporary += L".tmp";
            std::filesystem::remove(temporary, filesystemError);
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            if (!output)
            {
                error = L"Failed to create the temporary LTS mesh.";
                return AssetResult::IoError;
            }
            output.write(reinterpret_cast<const char*>(encoded.GetData()),
                         static_cast<std::streamsize>(encoded.GetSize()));
            output.close();
            if (!output)
            {
                std::filesystem::remove(temporary, filesystemError);
                error = L"Failed to write the complete LTS mesh.";
                return AssetResult::IoError;
            }
            std::filesystem::remove(destination, filesystemError);
            filesystemError.clear();
            std::filesystem::rename(temporary, destination, filesystemError);
            if (filesystemError)
            {
                std::filesystem::remove(temporary, filesystemError);
                error = L"Failed to replace the destination LTS mesh.";
                return AssetResult::IoError;
            }
            return AssetResult::Success;
        }
    }

    bool FbxStaticMeshImporter::IsSupportedSource(
        const std::filesystem::path& path) noexcept
    {
        try
        {
            std::wstring extension = path.extension().wstring();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
            return extension == L".fbx";
        }
        catch (...) { return false; }
    }

    AssetResult FbxStaticMeshImporter::Import(
        const std::filesystem::path& sourcePath,
        const std::filesystem::path& destinationPath,
        std::wstring& error,
        std::vector<std::wstring>* const outputWarnings) noexcept
    {
        error.clear();
        try
        {
            if (!IsSupportedSource(sourcePath) || destinationPath.empty())
            {
                error = L"Source is not a valid FBX path.";
                return AssetResult::InvalidPath;
            }

            ufbx_load_opts options{};
            options.target_axes = ufbx_axes_right_handed_y_up;
            options.target_unit_meters = 1.0;
            options.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
            options.generate_missing_normals = true;
            options.load_external_files = true;
            ufbx_error loadError{};
            const std::string utf8Path = sourcePath.u8string();
            ufbx_scene* scene = ufbx_load_file(utf8Path.c_str(), &options, &loadError);
            if (scene == nullptr)
            {
                error = L"ufbx failed to load FBX: ";
                AppendAscii(error, loadError.description.data);
                return AssetResult::CorruptData;
            }

            std::vector<StaticMeshVertex> vertices;
            std::vector<std::vector<std::uint32_t>> indicesByMaterial;
            std::unordered_map<std::string, std::uint32_t> materialSlots;
            std::vector<const ufbx_material*> slotMaterials;
            std::vector<std::wstring> warnings;

            for (std::size_t nodeIndex = 0; nodeIndex < scene->nodes.count; ++nodeIndex)
            {
                const ufbx_node* node = scene->nodes.data[nodeIndex];
                if (node == nullptr || node->mesh == nullptr) continue;
                const ufbx_mesh& mesh = *node->mesh;
                const ufbx_matrix normalMatrix = ufbx_matrix_for_normals(&node->geometry_to_world);

                for (std::size_t faceIndex = 0; faceIndex < mesh.faces.count; ++faceIndex)
                {
                    const ufbx_face face = mesh.faces.data[faceIndex];
                    if (face.num_indices < 3U) continue;
                    std::vector<std::uint32_t> triangleIndices((face.num_indices - 2U) * 3U);
                    const std::uint32_t triangleCount = ufbx_triangulate_face(
                        triangleIndices.data(), triangleIndices.size(), &mesh, face);
                    const std::uint32_t indexCount = triangleCount * 3U;
                    const std::uint32_t materialSlot = ResolveMaterialSlot(
                        *node, mesh, faceIndex, materialSlots, slotMaterials);
                    if (indicesByMaterial.size() <= materialSlot)
                        indicesByMaterial.resize(static_cast<std::size_t>(materialSlot) + 1U);

                    for (std::uint32_t triangleIndex = 0U;
                         triangleIndex < indexCount; ++triangleIndex)
                    {
                        const std::uint32_t sourceIndex = triangleIndices[triangleIndex];
                        StaticMeshVertex vertex{};
                        const ufbx_vec3 localPosition =
                            ufbx_get_vertex_vec3(&mesh.vertex_position, sourceIndex);
                        const ufbx_vec3 position =
                            ufbx_transform_position(&node->geometry_to_world, localPosition);
                        const ufbx_vec3 localNormal =
                            ufbx_get_vertex_vec3(&mesh.vertex_normal, sourceIndex);
                        const ufbx_vec3 normal =
                            ufbx_transform_direction(&normalMatrix, localNormal);
                        vertex.position = { static_cast<float>(position.x),
                            static_cast<float>(position.y), static_cast<float>(position.z) };
                        vertex.normal = { static_cast<float>(normal.x),
                            static_cast<float>(normal.y), static_cast<float>(normal.z) };
                        Normalize(vertex.normal, { 0.0F, 1.0F, 0.0F });
                        if (mesh.vertex_uv.exists)
                        {
                            const ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh.vertex_uv, sourceIndex);
                            vertex.texcoord0 = { static_cast<float>(uv.x),
                                static_cast<float>(1.0 - uv.y) };
                        }
                        vertex.tangent = { 1.0F, 0.0F, 0.0F, 1.0F };
                        if (mesh.vertex_tangent.exists)
                        {
                            const ufbx_vec3 localTangent =
                                ufbx_get_vertex_vec3(&mesh.vertex_tangent, sourceIndex);
                            const ufbx_vec3 tangent =
                                ufbx_transform_direction(&node->geometry_to_world, localTangent);
                            std::array<float, 3U> normalizedTangent{
                                static_cast<float>(tangent.x), static_cast<float>(tangent.y),
                                static_cast<float>(tangent.z) };
                            Normalize(normalizedTangent, { 1.0F, 0.0F, 0.0F });
                            float tangentSign = 1.0F;
                            if (mesh.vertex_bitangent.exists)
                            {
                                const ufbx_vec3 localBitangent =
                                    ufbx_get_vertex_vec3(&mesh.vertex_bitangent, sourceIndex);
                                const ufbx_vec3 bitangent = ufbx_transform_direction(
                                    &node->geometry_to_world, localBitangent);
                                const float crossX = vertex.normal[1] * normalizedTangent[2] -
                                    vertex.normal[2] * normalizedTangent[1];
                                const float crossY = vertex.normal[2] * normalizedTangent[0] -
                                    vertex.normal[0] * normalizedTangent[2];
                                const float crossZ = vertex.normal[0] * normalizedTangent[1] -
                                    vertex.normal[1] * normalizedTangent[0];
                                const float handedness = crossX * static_cast<float>(bitangent.x) +
                                    crossY * static_cast<float>(bitangent.y) +
                                    crossZ * static_cast<float>(bitangent.z);
                                // texcoord V is flipped above, so its tangent basis changes handedness.
                                tangentSign = handedness >= 0.0F ? -1.0F : 1.0F;
                            }
                            vertex.tangent = { normalizedTangent[0], normalizedTangent[1],
                                normalizedTangent[2], tangentSign };
                        }
                        if (vertices.size() >= MaximumVertices)
                        {
                            ufbx_free_scene(scene);
                            error = L"FBX exceeds the static mesh vertex limit.";
                            return AssetResult::FileTooLarge;
                        }
                        const std::uint32_t outputIndex =
                            static_cast<std::uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                        indicesByMaterial[materialSlot].push_back(outputIndex);
                    }
                }
            }
            std::filesystem::path meshesRoot;
            std::filesystem::path cursor = destinationPath.parent_path();
            while (!cursor.empty())
            {
                if (cursor.filename() == L"Meshes")
                {
                    meshesRoot = cursor;
                    break;
                }
                const std::filesystem::path parent = cursor.parent_path();
                if (parent == cursor) break;
                cursor = parent;
            }
            if (meshesRoot.empty())
            {
                ufbx_free_scene(scene);
                error = L"Destination mesh must be under Data/Meshes.";
                return AssetResult::InvalidPath;
            }
            std::error_code relativeError;
            const std::filesystem::path packagePath = std::filesystem::relative(
                destinationPath.parent_path(), meshesRoot, relativeError);
            if (relativeError)
            {
                ufbx_free_scene(scene);
                error = L"Failed to resolve the FBX package path.";
                return AssetResult::InvalidPath;
            }
            const std::filesystem::path dataRoot = meshesRoot.parent_path();
            for (std::size_t slot = 0U; slot < slotMaterials.size(); ++slot)
            {
                const AssetResult materialResult = WriteMaterial(
                    slotMaterials[slot], slot, sourcePath.parent_path(),
                    dataRoot / L"Materials", dataRoot / L"Textures",
                    packagePath, warnings);
                if (Failed(materialResult))
                {
                    ufbx_free_scene(scene);
                    error = L"Failed to create an LTS material: ";
                    AppendAscii(error, ToString(materialResult));
                    return materialResult;
                }
            }
            ufbx_free_scene(scene);
            if (outputWarnings != nullptr) *outputWarnings = std::move(warnings);

            std::vector<std::uint32_t> indices;
            std::vector<MeshSubmesh> submeshes;
            for (std::size_t slot = 0; slot < indicesByMaterial.size(); ++slot)
            {
                const auto& slotIndices = indicesByMaterial[slot];
                if (slotIndices.empty()) continue;
                if (indices.size() + slotIndices.size() > MaximumIndices)
                {
                    error = L"FBX exceeds the static mesh index limit.";
                    return AssetResult::FileTooLarge;
                }
                MeshSubmesh submesh{};
                submesh.firstIndex = static_cast<std::uint32_t>(indices.size());
                submesh.indexCount = static_cast<std::uint32_t>(slotIndices.size());
                submesh.materialSlot = static_cast<std::uint32_t>(slot);
                indices.insert(indices.end(), slotIndices.begin(), slotIndices.end());
                submeshes.push_back(submesh);
            }
            if (vertices.empty() || indices.empty() || submeshes.empty())
            {
                error = L"FBX contains no renderable static mesh geometry.";
                return AssetResult::UnsupportedFeature;
            }

            MeshAsset mesh;
            const std::string debugName = sourcePath.stem().u8string();
            const AssetResult buildResult = MeshAssetBuilder::Build(
                vertices.data(), vertices.size(), indices.data(), indices.size(),
                submeshes.data(), submeshes.size(),
                static_cast<std::uint32_t>(materialSlots.size()), debugName, mesh);
            if (Failed(buildResult))
            {
                error = L"MeshAssetBuilder failed: ";
                AppendAscii(error, ToString(buildResult));
                return buildResult;
            }
            return Save(destinationPath, mesh, error);
        }
        catch (const std::bad_alloc&)
        {
            error = L"Not enough memory to import FBX.";
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            error = L"Unexpected FBX import failure.";
            return AssetResult::InternalError;
        }
    }
}
