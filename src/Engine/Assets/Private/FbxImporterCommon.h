#pragma once

#include "Assets/AssetResult.h"
#include "Assets/FbxAssetData.h"
#include "Assets/SkeletonAsset.h"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets::fbx_detail
{
    constexpr std::uint32_t AssetEndianMarker =
        0x01020304U;

    constexpr std::size_t MaximumImportedVertices =
        20'000'000U;

    constexpr std::size_t MaximumImportedIndices =
        60'000'000U;

    inline void AppendAscii(
        std::wstring& destination,
        const char* text)
    {
        if (text == nullptr)
        {
            return;
        }

        while (*text != '\0')
        {
            destination.push_back(
                static_cast<unsigned char>(*text));

            ++text;
        }
    }

    [[nodiscard]]
    inline std::string ToString(
        const ufbx_string source,
        const char* fallback)
    {
        if (
            source.data == nullptr ||
            source.length == 0U)
        {
            return fallback != nullptr
                ? std::string(fallback)
                : std::string{};
        }

        return std::string(
            source.data,
            source.length);
    }

    [[nodiscard]]
    inline std::string SanitizeName(
        std::string name,
        const std::string& fallback)
    {
        for (char& character : name)
        {
            const unsigned char value =
                static_cast<unsigned char>(
                    character);

            if (
                value < 32U ||
                character == '<' ||
                character == '>' ||
                character == ':' ||
                character == '"' ||
                character == '/' ||
                character == '\\' ||
                character == '|' ||
                character == '?' ||
                character == '*')
            {
                character = '_';
            }
        }

        while (
            !name.empty() &&
            (
                name.back() == ' ' ||
                name.back() == '.'
            ))
        {
            name.pop_back();
        }

        return name.empty()
            ? fallback
            : name;
    }

    [[nodiscard]]
    inline bool IsSupportedSource(
        const std::filesystem::path& sourcePath)
    {
        std::wstring extension =
            sourcePath.extension().wstring();

        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](const wchar_t value)
            {
                return static_cast<wchar_t>(
                    std::towlower(value));
            });

        return extension == L".fbx";
    }

    struct SceneDeleter final
    {
        void operator()(
            ufbx_scene* scene) const noexcept
        {
            if (scene != nullptr)
            {
                ufbx_free_scene(scene);
            }
        }
    };

    using ScenePtr =
        std::unique_ptr<
            ufbx_scene,
            SceneDeleter>;

    [[nodiscard]]
    inline AssetResult LoadScene(
        const std::filesystem::path& sourcePath,
        ScenePtr& output,
        std::wstring& error)
    {
        output.reset();
        error.clear();

        if (
            sourcePath.empty() ||
            !IsSupportedSource(sourcePath))
        {
            error =
                L"Source is not a valid FBX file.";

            return AssetResult::InvalidPath;
        }

        std::error_code filesystemError;

        if (
            !std::filesystem::is_regular_file(
                sourcePath,
                filesystemError) ||
            filesystemError)
        {
            error =
                L"FBX source file does not exist.";

            return AssetResult::NotFound;
        }

        ufbx_load_opts options{};

        options.target_axes =
            ufbx_axes_right_handed_y_up;

        options.target_unit_meters = 1.0;

        options.space_conversion =
            UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;

        options.generate_missing_normals = true;
        options.load_external_files = true;
        options.retain_vertex_attrib_w = true;

        ufbx_error loadError{};

        const std::string sourceUtf8 =
            sourcePath.u8string();

        output.reset(
            ufbx_load_file(
                sourceUtf8.c_str(),
                &options,
                &loadError));

        if (!output)
        {
            error =
                L"ufbx failed to load FBX: ";

            AppendAscii(
                error,
                loadError.description.data);

            return AssetResult::CorruptData;
        }

        return AssetResult::Success;
    }

    [[nodiscard]]
    inline FbxMatrix4 ConvertMatrix(
        const ufbx_matrix& source) noexcept
    {
        FbxMatrix4 result;

        result.values =
        {
            static_cast<float>(source.m00),
            static_cast<float>(source.m01),
            static_cast<float>(source.m02),
            0.0F,

            static_cast<float>(source.m10),
            static_cast<float>(source.m11),
            static_cast<float>(source.m12),
            0.0F,

            static_cast<float>(source.m20),
            static_cast<float>(source.m21),
            static_cast<float>(source.m22),
            0.0F,

            static_cast<float>(source.m03),
            static_cast<float>(source.m13),
            static_cast<float>(source.m23),
            1.0F
        };

        return result;
    }

    [[nodiscard]]
    inline std::array<float, 3U> Convert(
        const ufbx_vec3 value) noexcept
    {
        return
        {
            static_cast<float>(value.x),
            static_cast<float>(value.y),
            static_cast<float>(value.z)
        };
    }

    [[nodiscard]]
    inline std::array<float, 4U> Convert(
        const ufbx_quat value) noexcept
    {
        return
        {
            static_cast<float>(value.x),
            static_cast<float>(value.y),
            static_cast<float>(value.z),
            static_cast<float>(value.w)
        };
    }

    inline void Normalize(
        std::array<float, 3U>& value,
        const std::array<float, 3U>& fallback)
        noexcept
    {
        const float lengthSquared =
            value[0U] * value[0U] +
            value[1U] * value[1U] +
            value[2U] * value[2U];

        if (
            !std::isfinite(lengthSquared) ||
            lengthSquared <= 0.0000001F)
        {
            value = fallback;
            return;
        }

        const float inverseLength =
            1.0F / std::sqrt(lengthSquared);

        value[0U] *= inverseLength;
        value[1U] *= inverseLength;
        value[2U] *= inverseLength;
    }

    [[nodiscard]]
    inline std::array<float, 3U> Cross(
        const std::array<float, 3U>& left,
        const std::array<float, 3U>& right)
        noexcept
    {
        return
        {
            left[1U] * right[2U] -
                left[2U] * right[1U],

            left[2U] * right[0U] -
                left[0U] * right[2U],

            left[0U] * right[1U] -
                left[1U] * right[0U]
        };
    }

    [[nodiscard]]
    inline float Dot(
        const std::array<float, 3U>& left,
        const std::array<float, 3U>& right)
        noexcept
    {
        return
            left[0U] * right[0U] +
            left[1U] * right[1U] +
            left[2U] * right[2U];
    }

    [[nodiscard]]
    inline std::array<float, 3U>
        BuildFallbackTangent(
            const std::array<float, 3U>& normal)
            noexcept
    {
        const std::array<float, 3U> axis =
            std::fabs(normal[1U]) < 0.999F
                ? std::array<float, 3U>
                    {0.0F, 1.0F, 0.0F}
                : std::array<float, 3U>
                    {1.0F, 0.0F, 0.0F};

        std::array<float, 3U> tangent =
            Cross(axis, normal);

        Normalize(
            tangent,
            {1.0F, 0.0F, 0.0F});

        return tangent;
    }

    struct ImportedVertexFrame final
    {
        std::array<float, 3U> position{};
        std::array<float, 3U> normal{};

        std::array<float, 4U> tangent
        {
            1.0F,
            0.0F,
            0.0F,
            1.0F
        };

        std::array<float, 2U> texcoord{};
    };

    [[nodiscard]]
    inline ImportedVertexFrame ReadVertexFrame(
        const ufbx_node& node,
        const ufbx_mesh& mesh,
        const std::uint32_t cornerIndex)
        noexcept
    {
        ImportedVertexFrame output;

        const ufbx_vec3 localPosition =
            ufbx_get_vertex_vec3(
                &mesh.vertex_position,
                cornerIndex);

        const ufbx_vec3 worldPosition =
            ufbx_transform_position(
                &node.geometry_to_world,
                localPosition);

        output.position =
            Convert(worldPosition);

        const ufbx_matrix normalMatrix =
            ufbx_matrix_for_normals(
                &node.geometry_to_world);

        const ufbx_vec3 localNormal =
            mesh.vertex_normal.exists
                ? ufbx_get_vertex_vec3(
                    &mesh.vertex_normal,
                    cornerIndex)
                : ufbx_vec3
                    {0.0, 1.0, 0.0};

        const ufbx_vec3 worldNormal =
            ufbx_transform_direction(
                &normalMatrix,
                localNormal);

        output.normal =
            Convert(worldNormal);

        Normalize(
            output.normal,
            {0.0F, 1.0F, 0.0F});

        std::array<float, 3U> tangent =
            BuildFallbackTangent(
                output.normal);

        float tangentSign = 1.0F;

        if (mesh.vertex_tangent.exists)
        {
            const ufbx_vec3 localTangent =
                ufbx_get_vertex_vec3(
                    &mesh.vertex_tangent,
                    cornerIndex);

            const ufbx_vec3 worldTangent =
                ufbx_transform_direction(
                    &node.geometry_to_world,
                    localTangent);

            tangent =
                Convert(worldTangent);

            Normalize(
                tangent,
                BuildFallbackTangent(
                    output.normal));
        }

        if (mesh.vertex_bitangent.exists)
        {
            const ufbx_vec3 localBitangent =
                ufbx_get_vertex_vec3(
                    &mesh.vertex_bitangent,
                    cornerIndex);

            const ufbx_vec3 worldBitangent =
                ufbx_transform_direction(
                    &node.geometry_to_world,
                    localBitangent);

            std::array<float, 3U> bitangent =
                Convert(worldBitangent);

            Normalize(
                bitangent,
                Cross(output.normal, tangent));

            const float handedness =
                Dot(
                    Cross(output.normal, tangent),
                    bitangent);

            /*
             * UV V переворачивается ниже,
             * поэтому меняется handedness.
             */
            tangentSign =
                handedness >= 0.0F
                    ? -1.0F
                    : 1.0F;
        }

        output.tangent =
        {
            tangent[0U],
            tangent[1U],
            tangent[2U],
            tangentSign
        };

        if (mesh.vertex_uv.exists)
        {
            const ufbx_vec2 uv =
                ufbx_get_vertex_vec2(
                    &mesh.vertex_uv,
                    cornerIndex);

            output.texcoord =
            {
                static_cast<float>(uv.x),
                static_cast<float>(1.0 - uv.y)
            };
        }

        return output;
    }

    [[nodiscard]]
    inline std::uint32_t ResolveMaterialSlot(
        const ufbx_mesh& mesh,
        const std::size_t faceIndex) noexcept
    {
        if (
            faceIndex <
                mesh.face_material.count)
        {
            return
                mesh.face_material.data[
                    faceIndex];
        }

        return 0U;
    }

    [[nodiscard]]
    inline AssetResult BuildMaterialCorners(
        const ufbx_node& node,
        const ufbx_mesh& mesh,
        std::vector<
            std::vector<std::uint32_t>>&
            output,
        std::wstring& error)
    {
        const std::size_t materialCount =
            (std::max)(
                std::size_t{1U},
                (std::max)(
                    node.materials.count,
                    mesh.materials.count));

        output.clear();
        output.resize(materialCount);

        std::vector<std::uint32_t>
            triangulated;

        triangulated.resize(
            (std::max)(
                mesh.max_face_triangles *
                    std::size_t{3U},
                std::size_t{3U}));

        std::size_t totalIndexCount = 0U;

        for (
            std::size_t faceIndex = 0U;
            faceIndex < mesh.faces.count;
            ++faceIndex)
        {
            const ufbx_face face =
                mesh.faces.data[faceIndex];

            if (face.num_indices < 3U)
            {
                continue;
            }

            const std::uint32_t triangleCount =
                ufbx_triangulate_face(
                    triangulated.data(),
                    triangulated.size(),
                    &mesh,
                    face);

            if (triangleCount == 0U)
            {
                continue;
            }

            const std::size_t cornerCount =
                static_cast<std::size_t>(
                    triangleCount) *
                3U;

            if (
                totalIndexCount >
                    MaximumImportedIndices -
                        cornerCount)
            {
                error =
                    L"FBX mesh exceeds the index limit.";

                return AssetResult::FileTooLarge;
            }

            totalIndexCount += cornerCount;

            std::uint32_t materialSlot =
                ResolveMaterialSlot(
                    mesh,
                    faceIndex);

            if (
                materialSlot >=
                    output.size())
            {
                materialSlot = 0U;
            }

            output[materialSlot].insert(
                output[materialSlot].end(),
                triangulated.begin(),
                triangulated.begin() +
                    static_cast<std::ptrdiff_t>(
                        cornerCount));
        }

        return AssetResult::Success;
    }

    [[nodiscard]]
    inline std::vector<const ufbx_node*>
        CollectSkeletonNodes(
            const ufbx_scene& scene)
    {
        std::unordered_map<
            const ufbx_node*,
            bool>
            included;

        const auto includeParents =
            [&included](const ufbx_node* node)
            {
                while (node != nullptr)
                {
                    included.emplace(node, true);
                    node = node->parent;
                }
            };

        for (
            std::size_t meshIndex = 0U;
            meshIndex < scene.meshes.count;
            ++meshIndex)
        {
            const ufbx_mesh* const mesh =
                scene.meshes.data[meshIndex];

            if (mesh == nullptr)
            {
                continue;
            }

            for (
                std::size_t skinIndex = 0U;
                skinIndex <
                    mesh->skin_deformers.count;
                ++skinIndex)
            {
                const ufbx_skin_deformer*
                    const skin =
                        mesh->skin_deformers.data[
                            skinIndex];

                if (skin == nullptr)
                {
                    continue;
                }

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
                        cluster != nullptr &&
                        cluster->bone_node != nullptr)
                    {
                        includeParents(
                            cluster->bone_node);
                    }
                }
            }
        }

        for (
            std::size_t nodeIndex = 0U;
            nodeIndex < scene.nodes.count;
            ++nodeIndex)
        {
            const ufbx_node* const node =
                scene.nodes.data[nodeIndex];

            if (
                node != nullptr &&
                node->bone != nullptr)
            {
                includeParents(node);
            }
        }

        std::vector<const ufbx_node*> nodes;

        nodes.reserve(included.size());

        for (const auto& pair : included)
        {
            nodes.push_back(pair.first);
        }

        const auto getDepth =
            [](const ufbx_node* node)
            {
                std::size_t depth = 0U;

                while (
                    node != nullptr &&
                    node->parent != nullptr)
                {
                    ++depth;
                    node = node->parent;
                }

                return depth;
            };

        std::sort(
            nodes.begin(),
            nodes.end(),
            [&getDepth](
                const ufbx_node* left,
                const ufbx_node* right)
            {
                const std::size_t leftDepth =
                    getDepth(left);

                const std::size_t rightDepth =
                    getDepth(right);

                if (leftDepth != rightDepth)
                {
                    return leftDepth < rightDepth;
                }

                return
                    left->typed_id <
                    right->typed_id;
            });

        return nodes;
    }

    [[nodiscard]]
    inline AssetResult BuildSkeleton(
        const ufbx_scene& scene,
        FbxSkeletonData& output,
        std::vector<const ufbx_node*>&
            boneNodes,
        std::unordered_map<
            const ufbx_node*,
            std::uint32_t>& boneIndices,
        std::wstring& error)
    {
        output = {};
        boneIndices.clear();

        boneNodes =
            CollectSkeletonNodes(scene);

        if (boneNodes.empty())
        {
            error =
                L"FBX contains no skeleton.";

            return AssetResult::
                UnsupportedFeature;
        }

        if (
            boneNodes.size() >
                MaximumSkeletonBones)
        {
            error =
                L"FBX skeleton exceeds the runtime "
                L"limit of 128 bones.";

            return AssetResult::FileTooLarge;
        }

        output.name = "Skeleton";

        output.bones.reserve(
            boneNodes.size());

        boneIndices.reserve(
            boneNodes.size());

        for (
            std::size_t index = 0U;
            index < boneNodes.size();
            ++index)
        {
            boneIndices.emplace(
                boneNodes[index],
                static_cast<std::uint32_t>(
                    index));
        }

        for (
            const ufbx_node* const node :
            boneNodes)
        {
            if (node == nullptr)
            {
                error =
                    L"FBX skeleton contains a null node.";

                return AssetResult::CorruptData;
            }

            FbxSkeletonBone bone;

            bone.name =
                SanitizeName(
                    ToString(
                        node->name,
                        "Bone"),
                    "Bone");

            const auto parent =
                boneIndices.find(
                    node->parent);

            bone.parentIndex =
                parent != boneIndices.end()
                    ? static_cast<std::int32_t>(
                        parent->second)
                    : -1;

            bone.localBindMatrix =
                ConvertMatrix(
                    node->node_to_parent);

            bone.modelBindMatrix =
                ConvertMatrix(
                    node->node_to_world);

            output.bones.push_back(
                std::move(bone));
        }

        if (!output.IsValid())
        {
            error =
                L"Imported FBX skeleton hierarchy "
                L"is invalid.";

            return AssetResult::CorruptData;
        }

        return AssetResult::Success;
    }

    class BinaryWriter final
    {
    public:
        template<typename Value>
        [[nodiscard]]
        bool Write(
            const Value& value)
        {
            static_assert(
                std::is_trivially_copyable_v<Value>);

            return WriteBytes(
                &value,
                sizeof(Value));
        }

        [[nodiscard]]
        bool WriteBytes(
            const void* source,
            const std::size_t size)
        {
            if (size == 0U)
            {
                return true;
            }

            if (source == nullptr)
            {
                return false;
            }

            try
            {
                const std::size_t oldSize =
                    bytes_.size();

                bytes_.resize(
                    oldSize + size);

                std::memcpy(
                    bytes_.data() + oldSize,
                    source,
                    size);

                return true;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool WriteString(
            const std::string& value)
        {
            if (
                value.size() >
                (std::numeric_limits<
                    std::uint32_t>::max)())
            {
                return false;
            }

            const std::uint32_t length =
                static_cast<std::uint32_t>(
                    value.size());

            return
                Write(length) &&
                WriteBytes(
                    value.data(),
                    value.size());
        }

        [[nodiscard]]
        const std::vector<std::byte>&
            GetBytes() const noexcept
        {
            return bytes_;
        }

    private:
        std::vector<std::byte> bytes_;
    };

    [[nodiscard]]
    inline AssetResult SaveBinary(
        const std::filesystem::path& destination,
        const std::vector<std::byte>& bytes,
        const bool overwrite,
        std::wstring& error)
    {
        if (
            destination.empty() ||
            bytes.empty())
        {
            error =
                L"Invalid output file or empty data.";

            return AssetResult::InvalidArgument;
        }

        std::error_code filesystemError;

        std::filesystem::create_directories(
            destination.parent_path(),
            filesystemError);

        if (filesystemError)
        {
            error =
                L"Failed to create output directory.";

            return AssetResult::IoError;
        }

        if (
            !overwrite &&
            std::filesystem::exists(
                destination,
                filesystemError) &&
            !filesystemError)
        {
            error =
                L"Output file already exists: " +
                destination.wstring();

            return AssetResult::AlreadyExists;
        }

        std::filesystem::path temporary =
            destination;

        temporary += L".tmp";

        std::filesystem::remove(
            temporary,
            filesystemError);

        filesystemError.clear();

        std::ofstream stream(
            temporary,
            std::ios::binary |
            std::ios::trunc);

        if (!stream)
        {
            error =
                L"Failed to create temporary output.";

            return AssetResult::IoError;
        }

        stream.write(
            reinterpret_cast<const char*>(
                bytes.data()),
            static_cast<std::streamsize>(
                bytes.size()));

        stream.close();

        if (!stream)
        {
            std::filesystem::remove(
                temporary,
                filesystemError);

            error =
                L"Failed to write complete output file.";

            return AssetResult::IoError;
        }

        if (overwrite)
        {
            std::filesystem::remove(
                destination,
                filesystemError);

            filesystemError.clear();
        }

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
                L"Failed to replace output file.";

            return AssetResult::IoError;
        }

        return AssetResult::Success;
    }

    [[nodiscard]]
    inline AssetResult MakeAssetPath(
        const std::filesystem::path& file,
        std::string& output,
        std::wstring& error)
    {
        output.clear();

        std::filesystem::path absoluteFile;

        try
        {
            absoluteFile =
                std::filesystem::absolute(file).
                    lexically_normal();
        }
        catch (...)
        {
            error =
                L"Failed to make asset path absolute.";

            return AssetResult::InvalidPath;
        }

        std::filesystem::path dataRoot =
            absoluteFile.parent_path();

        while (!dataRoot.empty())
        {
            if (dataRoot.filename() == L"Data")
            {
                break;
            }

            const std::filesystem::path parent =
                dataRoot.parent_path();

            if (parent == dataRoot)
            {
                dataRoot.clear();
                break;
            }

            dataRoot = parent;
        }

        if (dataRoot.empty())
        {
            error =
                L"Imported assets must be stored "
                L"inside game/Data.";

            return AssetResult::InvalidPath;
        }

        std::error_code relativeError;

        const std::filesystem::path relative =
            std::filesystem::relative(
                absoluteFile,
                dataRoot.parent_path(),
                relativeError);

        if (relativeError || relative.empty())
        {
            error =
                L"Failed to create runtime asset path.";

            return AssetResult::InvalidPath;
        }

        for (const auto& component : relative)
        {
            if (component == L"..")
            {
                error =
                    L"Asset path escapes the Data root.";

                return AssetResult::InvalidPath;
            }
        }

        output =
            relative.generic_u8string();

        return AssetResult::Success;
    }

    [[nodiscard]]
    inline std::uint32_t HashSkeletonPath(
        const std::string& path) noexcept
    {
        std::uint32_t hash =
            2166136261U;

        for (const unsigned char byte : path)
        {
            hash ^= byte;
            hash *= 16777619U;
        }

        return hash != 0U
            ? hash
            : 1U;
    }
}