#include "Assets/FbxAssetImporter.h"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <functional>
#include <limits>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        constexpr std::size_t MaximumImportedVertices =
            20'000'000U;

        constexpr std::size_t MaximumImportedIndices =
            60'000'000U;

        constexpr std::size_t MaximumImportedBones =
            1024U;

        void AppendAscii(
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
                    static_cast<unsigned char>(
                        *text));

                ++text;
            }
        }

        [[nodiscard]]
        std::string ToString(
            const ufbx_string value,
            const char* const fallback)
        {
            if (
                value.data == nullptr ||
                value.length == 0U)
            {
                return
                    fallback != nullptr
                        ? std::string(fallback)
                        : std::string{};
            }

            return std::string(
                value.data,
                value.length);
        }

        [[nodiscard]]
        FbxMatrix4 ConvertMatrix(
            const ufbx_matrix& source) noexcept
        {
            /*
             * ufbx stores affine matrices as columns.
             * The engine uses row vectors, so transpose
             * the 3x4 source into a row-major 4x4 matrix.
             */
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
        std::array<float, 3U> Convert(
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
        std::array<float, 4U> Convert(
            const ufbx_vec4 value) noexcept
        {
            return
            {
                static_cast<float>(value.x),
                static_cast<float>(value.y),
                static_cast<float>(value.z),
                static_cast<float>(value.w)
            };
        }

        [[nodiscard]]
        std::array<float, 4U> Convert(
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

        void Normalize3(
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
                1.0F /
                std::sqrt(lengthSquared);

            value[0U] *= inverseLength;
            value[1U] *= inverseLength;
            value[2U] *= inverseLength;
        }

        [[nodiscard]]
        std::array<float, 3U> Cross(
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
        float Dot(
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
        std::array<float, 3U>
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

            Normalize3(
                tangent,
                {1.0F, 0.0F, 0.0F});

            return tangent;
        }

        void ExpandBounds(
            FbxBounds& bounds,
            const std::array<float, 3U>& position,
            bool& initialized) noexcept
        {
            if (!initialized)
            {
                bounds.minimum = position;
                bounds.maximum = position;
                initialized = true;
                return;
            }

            for (
                std::size_t component = 0U;
                component < 3U;
                ++component)
            {
                bounds.minimum[component] =
                    (std::min)(
                        bounds.minimum[component],
                        position[component]);

                bounds.maximum[component] =
                    (std::max)(
                        bounds.maximum[component],
                        position[component]);
            }
        }

        void FinalizeBounds(
            FbxBounds& bounds,
            const std::vector<FbxStaticVertex>&
                vertices) noexcept
        {
            for (
                std::size_t component = 0U;
                component < 3U;
                ++component)
            {
                bounds.center[component] =
                    (
                        bounds.minimum[component] +
                        bounds.maximum[component]
                    ) *
                    0.5F;
            }

            float radiusSquared = 0.0F;

            for (const FbxStaticVertex& vertex : vertices)
            {
                const float x =
                    vertex.position[0U] -
                    bounds.center[0U];

                const float y =
                    vertex.position[1U] -
                    bounds.center[1U];

                const float z =
                    vertex.position[2U] -
                    bounds.center[2U];

                radiusSquared =
                    (std::max)(
                        radiusSquared,
                        x * x + y * y + z * z);
            }

            bounds.radius =
                std::sqrt(radiusSquared);
        }

        void FinalizeBounds(
            FbxBounds& bounds,
            const std::vector<FbxSkeletalVertex>&
                vertices) noexcept
        {
            for (
                std::size_t component = 0U;
                component < 3U;
                ++component)
            {
                bounds.center[component] =
                    (
                        bounds.minimum[component] +
                        bounds.maximum[component]
                    ) *
                    0.5F;
            }

            float radiusSquared = 0.0F;

            for (const FbxSkeletalVertex& vertex : vertices)
            {
                const float x =
                    vertex.position[0U] -
                    bounds.center[0U];

                const float y =
                    vertex.position[1U] -
                    bounds.center[1U];

                const float z =
                    vertex.position[2U] -
                    bounds.center[2U];

                radiusSquared =
                    (std::max)(
                        radiusSquared,
                        x * x + y * y + z * z);
            }

            bounds.radius =
                std::sqrt(radiusSquared);
        }

        [[nodiscard]]
        std::uint32_t ResolveFaceMaterial(
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
        std::vector<FbxMaterialSlot>
            BuildMaterials(
                const ufbx_node& node,
                const ufbx_mesh& mesh)
        {
            const std::size_t materialCount =
                (std::max)(
                    node.materials.count,
                    mesh.materials.count);

            std::vector<FbxMaterialSlot> materials;

            materials.reserve(
                (std::max)(
                    materialCount,
                    std::size_t{1U}));

            for (
                std::size_t materialIndex = 0U;
                materialIndex < materialCount;
                ++materialIndex)
            {
                const ufbx_material* material =
                    materialIndex <
                        node.materials.count
                        ? node.materials.data[
                            materialIndex]
                        : (
                            materialIndex <
                                mesh.materials.count
                                ? mesh.materials.data[
                                    materialIndex]
                                : nullptr
                        );

                FbxMaterialSlot slot;

                slot.name =
                    material != nullptr
                        ? ToString(
                            material->name,
                            "Material")
                        : (
                            "Material_" +
                            std::to_string(
                                materialIndex)
                        );

                materials.push_back(
                    std::move(slot));
            }

            if (materials.empty())
            {
                materials.push_back(
                    FbxMaterialSlot{"Default"});
            }

            return materials;
        }

        [[nodiscard]]
        std::array<float, 4U> ReadColor(
            const ufbx_mesh& mesh,
            const std::uint32_t cornerIndex) noexcept
        {
            return
                mesh.vertex_color.exists
                    ? Convert(
                        ufbx_get_vertex_vec4(
                            &mesh.vertex_color,
                            cornerIndex))
                    : std::array<float, 4U>
                        {1.0F, 1.0F, 1.0F, 1.0F};
        }

        void ReadBasis(
            const ufbx_mesh& mesh,
            const std::uint32_t cornerIndex,
            std::array<float, 3U>& normal,
            std::array<float, 4U>& tangent) noexcept
        {
            normal =
                mesh.vertex_normal.exists
                    ? Convert(
                        ufbx_get_vertex_vec3(
                            &mesh.vertex_normal,
                            cornerIndex))
                    : std::array<float, 3U>
                        {0.0F, 1.0F, 0.0F};

            Normalize3(
                normal,
                {0.0F, 1.0F, 0.0F});

            std::array<float, 3U> tangent3 =
                mesh.vertex_tangent.exists
                    ? Convert(
                        ufbx_get_vertex_vec3(
                            &mesh.vertex_tangent,
                            cornerIndex))
                    : BuildFallbackTangent(normal);

            Normalize3(
                tangent3,
                BuildFallbackTangent(normal));

            float tangentSign = 1.0F;

            if (mesh.vertex_bitangent.exists)
            {
                std::array<float, 3U> bitangent =
                    Convert(
                        ufbx_get_vertex_vec3(
                            &mesh.vertex_bitangent,
                            cornerIndex));

                Normalize3(
                    bitangent,
                    Cross(normal, tangent3));

                tangentSign =
                    Dot(
                        Cross(normal, tangent3),
                        bitangent) < 0.0F
                        ? -1.0F
                        : 1.0F;
            }

            tangent =
            {
                tangent3[0U],
                tangent3[1U],
                tangent3[2U],
                tangentSign
            };
        }

        [[nodiscard]]
        std::array<float, 2U> ReadUv(
            const ufbx_mesh& mesh,
            const std::uint32_t cornerIndex) noexcept
        {
            if (!mesh.vertex_uv.exists)
            {
                return {};
            }

            const ufbx_vec2 uv =
                ufbx_get_vertex_vec2(
                    &mesh.vertex_uv,
                    cornerIndex);

            return
            {
                static_cast<float>(uv.x),
                static_cast<float>(uv.y)
            };
        }

        [[nodiscard]]
        std::vector<const ufbx_node*>
            CollectSkeletonNodes(
                const ufbx_scene& scene)
        {
            std::unordered_map<
                const ufbx_node*,
                bool>
                included;

            const auto includeWithParents =
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
                    const ufbx_skin_deformer* const skin =
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
                        const ufbx_skin_cluster* const
                            cluster =
                                skin->clusters.data[
                                    clusterIndex];

                        if (
                            cluster != nullptr &&
                            cluster->bone_node != nullptr)
                        {
                            includeWithParents(
                                cluster->bone_node);
                        }
                    }
                }
            }

            /*
             * Animation-only UE5 FBX may not contain
             * a skinned mesh. In that case use all
             * explicit FBX bone nodes.
             */
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
                    includeWithParents(node);
                }
            }

            std::vector<const ufbx_node*> nodes;

            nodes.reserve(included.size());

            for (const auto& pair : included)
            {
                nodes.push_back(pair.first);
            }

            const auto depth =
                [](const ufbx_node* node)
                {
                    std::size_t result = 0U;

                    while (
                        node != nullptr &&
                        node->parent != nullptr)
                    {
                        ++result;
                        node = node->parent;
                    }

                    return result;
                };

            std::sort(
                nodes.begin(),
                nodes.end(),
                [&depth](
                    const ufbx_node* left,
                    const ufbx_node* right)
                {
                    const std::size_t leftDepth =
                        depth(left);

                    const std::size_t rightDepth =
                        depth(right);

                    if (leftDepth != rightDepth)
                    {
                        return
                            leftDepth <
                            rightDepth;
                    }

                    return
                        left->typed_id <
                        right->typed_id;
                });

            return nodes;
        }

        [[nodiscard]]
        AssetResult BuildSkeleton(
            const ufbx_scene& scene,
            FbxSkeletonData& output,
            std::vector<const ufbx_node*>& boneNodes,
            std::unordered_map<
                const ufbx_node*,
                std::uint32_t>&
                boneIndices,
            std::wstring& error)
        {
            boneNodes =
                CollectSkeletonNodes(scene);

            if (boneNodes.empty())
            {
                return AssetResult::Success;
            }

            if (
                boneNodes.size() >
                MaximumImportedBones)
            {
                error =
                    L"FBX skeleton exceeds the importer "
                    L"bone limit.";

                return AssetResult::FileTooLarge;
            }

            output = {};
            output.name = "Skeleton";
            output.bones.reserve(
                boneNodes.size());

            boneIndices.clear();
            boneIndices.reserve(
                boneNodes.size());

            for (
                std::size_t boneIndex = 0U;
                boneIndex < boneNodes.size();
                ++boneIndex)
            {
                boneIndices.emplace(
                    boneNodes[boneIndex],
                    static_cast<std::uint32_t>(
                        boneIndex));
            }

            for (
                const ufbx_node* const node :
                boneNodes)
            {
                if (node == nullptr)
                {
                    error =
                        L"FBX skeleton contains a null "
                        L"node.";

                    return AssetResult::CorruptData;
                }

                FbxSkeletonBone bone;

                bone.name =
                    ToString(
                        node->name,
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
                    L"The FBX skeleton hierarchy is "
                    L"invalid.";

                return AssetResult::CorruptData;
            }

            return AssetResult::Success;
        }

        template<typename Vertex>
        [[nodiscard]]
        AssetResult AppendGeometry(
            const ufbx_node& node,
            const ufbx_mesh& mesh,
            std::vector<Vertex>& vertices,
            std::vector<std::uint32_t>& indices,
            std::vector<FbxMeshSection>& sections,
            FbxBounds& bounds,
            const std::function<void(
                Vertex&,
                std::uint32_t)>& fillSkin,
            std::wstring& error)
        {
            std::vector<
                std::vector<std::uint32_t>>
                materialCorners;

            const std::size_t materialCount =
                (std::max)(
                    node.materials.count,
                    (std::max)(
                        mesh.materials.count,
                        std::size_t{1U}));

            materialCorners.resize(
                materialCount);

            std::vector<std::uint32_t>
                triangulated;

            triangulated.resize(
                (std::max)(
                    mesh.max_face_triangles *
                        std::size_t{3U},
                    std::size_t{3U}));

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

                std::uint32_t materialSlot =
                    ResolveFaceMaterial(
                        mesh,
                        faceIndex);

                if (
                    materialSlot >=
                        materialCorners.size())
                {
                    materialSlot = 0U;
                }

                std::vector<std::uint32_t>& corners =
                    materialCorners[
                        materialSlot];

                const std::size_t cornerCount =
                    static_cast<std::size_t>(
                        triangleCount) *
                    3U;

                corners.insert(
                    corners.end(),
                    triangulated.begin(),
                    triangulated.begin() +
                        static_cast<
                            std::ptrdiff_t>(
                                cornerCount));
            }

            bool boundsInitialized = false;

            for (
                std::size_t materialSlot = 0U;
                materialSlot <
                    materialCorners.size();
                ++materialSlot)
            {
                const auto& corners =
                    materialCorners[
                        materialSlot];

                if (corners.empty())
                {
                    continue;
                }

                FbxMeshSection section;

                section.firstIndex =
                    static_cast<std::uint32_t>(
                        indices.size());

                section.materialSlot =
                    static_cast<std::uint32_t>(
                        materialSlot);

                for (
                    const std::uint32_t cornerIndex :
                    corners)
                {
                    if (
                        cornerIndex >=
                            mesh.num_indices ||
                        vertices.size() >=
                            MaximumImportedVertices ||
                        indices.size() >=
                            MaximumImportedIndices)
                    {
                        error =
                            L"FBX mesh exceeds importer "
                            L"limits or contains an "
                            L"invalid corner index.";

                        return AssetResult::FileTooLarge;
                    }

                    Vertex vertex;

                    vertex.position =
                        Convert(
                            ufbx_get_vertex_vec3(
                                &mesh.vertex_position,
                                cornerIndex));

                    ReadBasis(
                        mesh,
                        cornerIndex,
                        vertex.normal,
                        vertex.tangent);

                    vertex.texcoord0 =
                        ReadUv(
                            mesh,
                            cornerIndex);

                    vertex.color =
                        ReadColor(
                            mesh,
                            cornerIndex);

                    fillSkin(
                        vertex,
                        cornerIndex);

                    ExpandBounds(
                        bounds,
                        vertex.position,
                        boundsInitialized);

                    const std::uint32_t newIndex =
                        static_cast<std::uint32_t>(
                            vertices.size());

                    vertices.push_back(
                        std::move(vertex));

                    indices.push_back(
                        newIndex);
                }

                section.indexCount =
                    static_cast<std::uint32_t>(
                        indices.size()) -
                    section.firstIndex;

                if (section.indexCount != 0U)
                {
                    sections.push_back(section);
                }
            }

            if (
                vertices.empty() ||
                indices.empty() ||
                sections.empty() ||
                !boundsInitialized)
            {
                error =
                    L"FBX mesh does not contain "
                    L"renderable triangles.";

                return AssetResult::CorruptData;
            }

            return AssetResult::Success;
        }

        [[nodiscard]]
        AssetResult ImportStaticMesh(
            const ufbx_node& node,
            const ufbx_mesh& mesh,
            FbxStaticMeshData& output,
            std::wstring& error)
        {
            output = {};

            output.name =
                ToString(
                    mesh.name,
                    "StaticMesh");

            output.sourceNodeName =
                ToString(
                    node.name,
                    "Node");

            output.localToWorld =
                ConvertMatrix(
                    node.geometry_to_world);

            output.materials =
                BuildMaterials(
                    node,
                    mesh);

            const auto noSkin =
                [](
                    FbxStaticVertex&,
                    std::uint32_t)
                {
                };

            const AssetResult result =
                AppendGeometry(
                    node,
                    mesh,
                    output.vertices,
                    output.indices,
                    output.sections,
                    output.bounds,
                    noSkin,
                    error);

            if (Failed(result))
            {
                return result;
            }

            FinalizeBounds(
                output.bounds,
                output.vertices);

            if (!output.IsValid())
            {
                error =
                    L"Imported FBX static mesh failed "
                    L"validation.";

                return AssetResult::CorruptData;
            }

            return AssetResult::Success;
        }

        [[nodiscard]]
        AssetResult ImportSkeletalMesh(
            const ufbx_node& node,
            const ufbx_mesh& mesh,
            const FbxSkeletonData& skeleton,
            const std::unordered_map<
                const ufbx_node*,
                std::uint32_t>& boneIndices,
            const std::uint32_t maximumInfluences,
            FbxSkeletalMeshData& output,
            std::wstring& error,
            std::vector<std::wstring>& warnings)
        {
            if (!skeleton.IsValid())
            {
                error =
                    L"Cannot import a skinned FBX mesh "
                    L"without a valid skeleton.";

                return AssetResult::CorruptData;
            }

            if (mesh.skin_deformers.count == 0U)
            {
                error =
                    L"FBX skeletal mesh has no skin "
                    L"deformer.";

                return AssetResult::CorruptData;
            }

            const ufbx_skin_deformer* const skin =
                mesh.skin_deformers.data[0U];

            if (skin == nullptr)
            {
                error =
                    L"FBX skeletal mesh contains a null "
                    L"skin deformer.";

                return AssetResult::CorruptData;
            }

            output = {};

            output.name =
                ToString(
                    mesh.name,
                    "SkeletalMesh");

            output.sourceNodeName =
                ToString(
                    node.name,
                    "Node");

            output.localToWorld =
                ConvertMatrix(
                    node.geometry_to_world);

            output.materials =
                BuildMaterials(
                    node,
                    mesh);

            output.inverseBindMatrices.resize(
                skeleton.bones.size());

            /*
             * Default inverse bind for bones that do
             * not influence this mesh.
             */
            for (
                const auto& pair : boneIndices)
            {
                const ufbx_node* const boneNode =
                    pair.first;

                const std::uint32_t boneIndex =
                    pair.second;

                const ufbx_matrix inverse =
                    ufbx_matrix_invert(
                        &boneNode->node_to_world);

                output.inverseBindMatrices[
                    boneIndex] =
                        ConvertMatrix(inverse);
            }

            std::vector<std::int32_t>
                clusterToBone;

            clusterToBone.assign(
                skin->clusters.count,
                -1);

            for (
                std::size_t clusterIndex = 0U;
                clusterIndex <
                    skin->clusters.count;
                ++clusterIndex)
            {
                const ufbx_skin_cluster* const cluster =
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

                if (found == boneIndices.end())
                {
                    continue;
                }

                clusterToBone[clusterIndex] =
                    static_cast<std::int32_t>(
                        found->second);

                output.inverseBindMatrices[
                    found->second] =
                        ConvertMatrix(
                            cluster->geometry_to_bone);
            }

            const std::uint32_t influenceLimit =
                std::clamp(
                    maximumInfluences,
                    1U,
                    static_cast<std::uint32_t>(
                        MaximumFbxBoneInfluences));

            const auto fillSkin =
                [&mesh,
                 skin,
                 &clusterToBone,
                 influenceLimit,
                 &warnings](
                    FbxSkeletalVertex& vertex,
                    const std::uint32_t cornerIndex)
                {
                    const std::uint32_t controlPoint =
                        cornerIndex <
                            mesh.vertex_indices.count
                            ? mesh.vertex_indices.data[
                                cornerIndex]
                            : UFBX_NO_INDEX;

                    if (
                        controlPoint ==
                            UFBX_NO_INDEX ||
                        controlPoint >=
                            skin->vertices.count)
                    {
                        vertex.boneIndices[0U] = 0U;
                        vertex.boneWeights[0U] = 1.0F;
                        return;
                    }

                    const ufbx_skin_vertex skinVertex =
                        skin->vertices.data[
                            controlPoint];

                    std::uint32_t written = 0U;
                    float totalWeight = 0.0F;

                    for (
                        std::uint32_t weightOffset = 0U;
                        weightOffset <
                            skinVertex.num_weights &&
                        written < influenceLimit;
                        ++weightOffset)
                    {
                        const std::size_t weightIndex =
                            static_cast<std::size_t>(
                                skinVertex.weight_begin) +
                            weightOffset;

                        if (
                            weightIndex >=
                                skin->weights.count)
                        {
                            break;
                        }

                        const ufbx_skin_weight weight =
                            skin->weights.data[
                                weightIndex];

                        if (
                            weight.cluster_index >=
                                clusterToBone.size())
                        {
                            continue;
                        }

                        const std::int32_t boneIndex =
                            clusterToBone[
                                weight.cluster_index];

                        const float influence =
                            static_cast<float>(
                                weight.weight);

                        if (
                            boneIndex < 0 ||
                            !std::isfinite(influence) ||
                            influence <= 0.0F)
                        {
                            continue;
                        }

                        vertex.boneIndices[written] =
                            static_cast<std::uint16_t>(
                                boneIndex);

                        vertex.boneWeights[written] =
                            influence;

                        totalWeight += influence;
                        ++written;
                    }

                    if (totalWeight <= 0.000001F)
                    {
                        vertex.boneIndices[0U] = 0U;
                        vertex.boneWeights[0U] = 1.0F;
                        return;
                    }

                    const float inverseWeight =
                        1.0F / totalWeight;

                    for (
                        std::uint32_t influenceIndex = 0U;
                        influenceIndex < written;
                        ++influenceIndex)
                    {
                        vertex.boneWeights[
                            influenceIndex] *=
                                inverseWeight;
                    }

                    if (
                        skinVertex.num_weights >
                            influenceLimit &&
                        warnings.size() < 64U)
                    {
                        warnings.push_back(
                            L"FBX vertex has more bone "
                            L"influences than the import "
                            L"limit; weakest influences "
                            L"were removed.");
                    }
                };

            const AssetResult result =
                AppendGeometry(
                    node,
                    mesh,
                    output.vertices,
                    output.indices,
                    output.sections,
                    output.bounds,
                    fillSkin,
                    error);

            if (Failed(result))
            {
                return result;
            }

            FinalizeBounds(
                output.bounds,
                output.vertices);

            if (!output.IsValid(skeleton))
            {
                error =
                    L"Imported FBX skeletal mesh failed "
                    L"validation.";

                return AssetResult::CorruptData;
            }

            return AssetResult::Success;
        }

        void EnsureQuaternionContinuity(
            const std::array<float, 4U>& previous,
            std::array<float, 4U>& current) noexcept
        {
            const float dot =
                previous[0U] * current[0U] +
                previous[1U] * current[1U] +
                previous[2U] * current[2U] +
                previous[3U] * current[3U];

            if (dot < 0.0F)
            {
                for (float& value : current)
                {
                    value = -value;
                }
            }
        }

        [[nodiscard]]
        AssetResult ImportAnimations(
            const ufbx_scene& scene,
            const FbxSkeletonData& skeleton,
            const std::vector<const ufbx_node*>&
                boneNodes,
            const float requestedSampleRate,
            std::vector<FbxAnimationClipData>& output,
            std::wstring& error)
        {
            if (
                scene.anim_stacks.count == 0U ||
                !skeleton.IsValid() ||
                boneNodes.size() !=
                    skeleton.bones.size())
            {
                return AssetResult::Success;
            }

            const float sampleRate =
                std::isfinite(requestedSampleRate) &&
                requestedSampleRate > 0.0F
                    ? std::clamp(
                        requestedSampleRate,
                        1.0F,
                        240.0F)
                    : 30.0F;

            for (
                std::size_t stackIndex = 0U;
                stackIndex <
                    scene.anim_stacks.count;
                ++stackIndex)
            {
                const ufbx_anim_stack* const stack =
                    scene.anim_stacks.data[
                        stackIndex];

                if (
                    stack == nullptr ||
                    stack->anim == nullptr)
                {
                    continue;
                }

                double startTime =
                    stack->time_begin;

                double endTime =
                    stack->time_end;

                if (
                    !std::isfinite(startTime) ||
                    !std::isfinite(endTime) ||
                    endTime < startTime)
                {
                    startTime =
                        stack->anim->time_begin;

                    endTime =
                        stack->anim->time_end;
                }

                if (
                    !std::isfinite(startTime) ||
                    !std::isfinite(endTime) ||
                    endTime < startTime)
                {
                    continue;
                }

                const double duration =
                    endTime - startTime;

                const std::size_t frameCount =
                    (std::max)(
                        static_cast<std::size_t>(
                            std::ceil(
                                duration *
                                static_cast<double>(
                                    sampleRate))) +
                            1U,
                        std::size_t{1U});

                FbxAnimationClipData clip;

                clip.name =
                    ToString(
                        stack->name,
                        "Animation");

                clip.durationSeconds =
                    static_cast<float>(
                        duration);

                clip.sampleRate =
                    sampleRate;

                clip.tracks.resize(
                    skeleton.bones.size());

                for (
                    std::size_t boneIndex = 0U;
                    boneIndex <
                        skeleton.bones.size();
                    ++boneIndex)
                {
                    FbxAnimationTrack& track =
                        clip.tracks[boneIndex];

                    track.boneIndex =
                        static_cast<std::uint32_t>(
                            boneIndex);

                    track.boneName =
                        skeleton.bones[
                            boneIndex].name;

                    track.keys.reserve(
                        frameCount);
                }

                for (
                    std::size_t frameIndex = 0U;
                    frameIndex < frameCount;
                    ++frameIndex)
                {
                    const double localTime =
                        frameCount > 1U
                            ? (
                                duration *
                                static_cast<double>(
                                    frameIndex) /
                                static_cast<double>(
                                    frameCount - 1U)
                            )
                            : 0.0;

                    const double sourceTime =
                        startTime + localTime;

                    ufbx_error evaluationError{};

                    ufbx_evaluate_opts evaluateOptions{};

                    ufbx_scene* const evaluated =
                        ufbx_evaluate_scene(
                            &scene,
                            stack->anim,
                            sourceTime,
                            &evaluateOptions,
                            &evaluationError);

                    if (evaluated == nullptr)
                    {
                        error =
                            L"ufbx failed to evaluate "
                            L"an animation frame: ";

                        AppendAscii(
                            error,
                            evaluationError.
                                description.data);

                        return AssetResult::CorruptData;
                    }

                    for (
                        std::size_t boneIndex = 0U;
                        boneIndex <
                            boneNodes.size();
                        ++boneIndex)
                    {
                        const ufbx_node* const sourceBone =
                            boneNodes[boneIndex];

                        if (
                            sourceBone == nullptr ||
                            sourceBone->typed_id >=
                                evaluated->nodes.count)
                        {
                            ufbx_free_scene(evaluated);

                            error =
                                L"Evaluated FBX scene does "
                                L"not contain the expected "
                                L"bone node.";

                            return AssetResult::CorruptData;
                        }

                        const ufbx_node* const evaluatedBone =
                            evaluated->nodes.data[
                                sourceBone->typed_id];

                        if (evaluatedBone == nullptr)
                        {
                            ufbx_free_scene(evaluated);

                            error =
                                L"Evaluated FBX bone node "
                                L"is null.";

                            return AssetResult::CorruptData;
                        }

                        const ufbx_transform transform =
                            evaluatedBone->
                                local_transform;

                        FbxAnimationKey key;

                        key.timeSeconds =
                            static_cast<float>(
                                localTime);

                        key.translation =
                            Convert(
                                transform.translation);

                        key.rotation =
                            Convert(
                                transform.rotation);

                        key.scale =
                            Convert(
                                transform.scale);

                        FbxAnimationTrack& track =
                            clip.tracks[boneIndex];

                        if (!track.keys.empty())
                        {
                            EnsureQuaternionContinuity(
                                track.keys.back().
                                    rotation,
                                key.rotation);
                        }

                        track.keys.push_back(
                            std::move(key));
                    }

                    ufbx_free_scene(evaluated);
                }

                if (!clip.IsValid(skeleton))
                {
                    error =
                        L"Imported FBX animation failed "
                        L"validation.";

                    return AssetResult::CorruptData;
                }

                output.push_back(
                    std::move(clip));
            }

            return AssetResult::Success;
        }
    }

    bool FbxAssetImporter::IsSupportedSource(
        const std::filesystem::path&
            sourcePath) noexcept
    {
        try
        {
            std::wstring extension =
                sourcePath.extension().wstring();

            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](const wchar_t value)
                {
                    return
                        static_cast<wchar_t>(
                            std::towlower(value));
                });

            return extension == L".fbx";
        }
        catch (...)
        {
            return false;
        }
    }

    AssetResult FbxAssetImporter::Import(
        const std::filesystem::path& sourcePath,
        const FbxImportOptions& options,
        FbxImportedScene& output,
        std::wstring& error,
        std::vector<std::wstring>* const warnings)
        noexcept
    {
        output.Clear();
        error.clear();

        std::vector<std::wstring> localWarnings;
        std::vector<std::wstring>& warningOutput =
            warnings != nullptr
                ? *warnings
                : localWarnings;

        warningOutput.clear();

        try
        {
            if (
                !IsSupportedSource(sourcePath) ||
                sourcePath.empty())
            {
                error =
                    L"Source path is not a valid FBX "
                    L"file path.";

                return AssetResult::InvalidPath;
            }

            std::error_code filesystemError;

            if (!std::filesystem::is_regular_file(
                    sourcePath,
                    filesystemError) ||
                filesystemError)
            {
                error =
                    L"FBX source file does not exist.";

                return AssetResult::NotFound;
            }

            ufbx_load_opts loadOptions{};

            loadOptions.target_axes =
                ufbx_axes_right_handed_y_up;

            loadOptions.target_unit_meters =
                1.0;

            loadOptions.space_conversion =
                UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;

            loadOptions.generate_missing_normals =
                true;

            loadOptions.load_external_files =
                true;

            loadOptions.retain_vertex_attrib_w =
                true;

            ufbx_error loadError{};

            const std::string utf8Path =
                sourcePath.u8string();

            ufbx_scene* const scene =
                ufbx_load_file(
                    utf8Path.c_str(),
                    &loadOptions,
                    &loadError);

            if (scene == nullptr)
            {
                error =
                    L"ufbx failed to load FBX: ";

                AppendAscii(
                    error,
                    loadError.description.data);

                return AssetResult::CorruptData;
            }

            std::vector<const ufbx_node*>
                boneNodes;

            std::unordered_map<
                const ufbx_node*,
                std::uint32_t>
                boneIndices;

            AssetResult result =
                BuildSkeleton(
                    *scene,
                    output.skeleton,
                    boneNodes,
                    boneIndices,
                    error);

            if (Failed(result))
            {
                ufbx_free_scene(scene);
                output.Clear();
                return result;
            }

            for (
                std::size_t meshIndex = 0U;
                meshIndex < scene->meshes.count;
                ++meshIndex)
            {
                const ufbx_mesh* const mesh =
                    scene->meshes.data[meshIndex];

                if (
                    mesh == nullptr ||
                    mesh->faces.count == 0U)
                {
                    continue;
                }

                const bool skinned =
                    mesh->skin_deformers.count > 0U;

                const std::size_t instanceCount =
                    mesh->instances.count > 0U
                        ? mesh->instances.count
                        : 1U;

                for (
                    std::size_t instanceIndex = 0U;
                    instanceIndex < instanceCount;
                    ++instanceIndex)
                {
                    const ufbx_node* const node =
                        mesh->instances.count > 0U
                            ? mesh->instances.data[
                                instanceIndex]
                            : nullptr;

                    if (node == nullptr)
                    {
                        warningOutput.push_back(
                            L"FBX mesh has no node "
                            L"instance and was skipped.");

                        continue;
                    }

                    if (
                        skinned &&
                        options.importSkeletalMeshes)
                    {
                        FbxSkeletalMeshData imported;

                        result =
                            ImportSkeletalMesh(
                                *node,
                                *mesh,
                                output.skeleton,
                                boneIndices,
                                options.
                                    maximumBoneInfluences,
                                imported,
                                error,
                                warningOutput);

                        if (Failed(result))
                        {
                            ufbx_free_scene(scene);
                            output.Clear();
                            return result;
                        }

                        output.skeletalMeshes.push_back(
                            std::move(imported));
                    }
                    else if (
                        !skinned &&
                        options.importStaticMeshes)
                    {
                        FbxStaticMeshData imported;

                        result =
                            ImportStaticMesh(
                                *node,
                                *mesh,
                                imported,
                                error);

                        if (Failed(result))
                        {
                            ufbx_free_scene(scene);
                            output.Clear();
                            return result;
                        }

                        output.staticMeshes.push_back(
                            std::move(imported));
                    }
                }
            }

            if (options.importAnimations)
            {
                result =
                    ImportAnimations(
                        *scene,
                        output.skeleton,
                        boneNodes,
                        options.animationSampleRate,
                        output.animationClips,
                        error);

                if (Failed(result))
                {
                    ufbx_free_scene(scene);
                    output.Clear();
                    return result;
                }
            }

            ufbx_free_scene(scene);

            if (output.IsEmpty())
            {
                error =
                    L"FBX contains no supported static "
                    L"mesh, skeletal mesh, skeleton, "
                    L"or animation data.";

                return AssetResult::UnsupportedFormat;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            output.Clear();

            error =
                L"Not enough memory to import FBX.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            output.Clear();

            error =
                L"Unexpected error while importing FBX.";

            return AssetResult::InternalError;
        }
    }
}
