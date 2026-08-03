#include "Assets/FbxAssetData.h"

#include <ufbx.h>

#include <cwctype>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <limits>

namespace engine::assets
{
    namespace
    {
        template<std::size_t Size>
        [[nodiscard]]
        bool IsFinite(
            const std::array<float, Size>& values)
            noexcept
        {
            for (const float value : values)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool IsFiniteMatrix(
            const FbxMatrix4& matrix) noexcept
        {
            return IsFinite(matrix.values);
        }

        template<typename Vertex>
        [[nodiscard]]
        bool ValidateMeshCommon(
            const std::vector<Vertex>& vertices,
            const std::vector<std::uint32_t>& indices,
            const std::vector<FbxMeshSection>& sections,
            const std::vector<FbxMaterialSlot>& materials,
            const FbxBounds& bounds) noexcept
        {
            if (
                vertices.empty() ||
                indices.empty() ||
                sections.empty() ||
                materials.empty() ||
                !bounds.IsValid())
            {
                return false;
            }

            for (const std::uint32_t index : indices)
            {
                if (
                    static_cast<std::size_t>(index) >=
                    vertices.size())
                {
                    return false;
                }
            }

            for (const FbxMeshSection& section : sections)
            {
                if (
                    section.indexCount == 0U ||
                    section.materialSlot >=
                        materials.size() ||
                    section.firstIndex >
                        indices.size() ||
                    section.indexCount >
                        indices.size() -
                            section.firstIndex)
                {
                    return false;
                }
            }

            return true;
        }
    }

    bool FbxBounds::IsValid() const noexcept
    {
        return
            IsFinite(minimum) &&
            IsFinite(maximum) &&
            IsFinite(center) &&
            std::isfinite(radius) &&
            radius >= 0.0F &&
            minimum[0U] <= maximum[0U] &&
            minimum[1U] <= maximum[1U] &&
            minimum[2U] <= maximum[2U];
    }

    bool FbxSkeletonData::IsValid() const noexcept
    {
        if (bones.empty())
        {
            return false;
        }

        for (
            std::size_t boneIndex = 0U;
            boneIndex < bones.size();
            ++boneIndex)
        {
            const FbxSkeletonBone& bone =
                bones[boneIndex];

            if (
                bone.name.empty() ||
                !IsFiniteMatrix(
                    bone.localBindMatrix) ||
                !IsFiniteMatrix(
                    bone.modelBindMatrix))
            {
                return false;
            }

            if (
                bone.parentIndex >= 0 &&
                static_cast<std::size_t>(
                    bone.parentIndex) >=
                    boneIndex)
            {
                return false;
            }
        }

        return true;
    }

    bool FbxStaticMeshData::IsValid() const noexcept
    {
        if (
            name.empty() ||
            !IsFiniteMatrix(localToWorld) ||
            !ValidateMeshCommon(
                vertices,
                indices,
                sections,
                materials,
                bounds))
        {
            return false;
        }

        for (const FbxStaticVertex& vertex : vertices)
        {
            if (
                !IsFinite(vertex.position) ||
                !IsFinite(vertex.normal) ||
                !IsFinite(vertex.tangent) ||
                !IsFinite(vertex.texcoord0) ||
                !IsFinite(vertex.color))
            {
                return false;
            }
        }

        return true;
    }

    bool FbxSkeletalMeshData::IsValid(
        const FbxSkeletonData& skeleton) const noexcept
    {
        if (
            name.empty() ||
            !skeleton.IsValid() ||
            inverseBindMatrices.size() !=
                skeleton.bones.size() ||
            !IsFiniteMatrix(localToWorld) ||
            !ValidateMeshCommon(
                vertices,
                indices,
                sections,
                materials,
                bounds))
        {
            return false;
        }

        for (
            const FbxMatrix4& matrix :
            inverseBindMatrices)
        {
            if (!IsFiniteMatrix(matrix))
            {
                return false;
            }
        }

        for (const FbxSkeletalVertex& vertex : vertices)
        {
            if (
                !IsFinite(vertex.position) ||
                !IsFinite(vertex.normal) ||
                !IsFinite(vertex.tangent) ||
                !IsFinite(vertex.texcoord0) ||
                !IsFinite(vertex.color) ||
                !IsFinite(vertex.boneWeights))
            {
                return false;
            }

            float totalWeight = 0.0F;

            for (
                std::size_t influenceIndex = 0U;
                influenceIndex <
                    MaximumFbxBoneInfluences;
                ++influenceIndex)
            {
                const float weight =
                    vertex.boneWeights[
                        influenceIndex];

                if (
                    weight < 0.0F ||
                    (
                        weight > 0.0F &&
                        vertex.boneIndices[
                            influenceIndex] >=
                            skeleton.bones.size()
                    ))
                {
                    return false;
                }

                totalWeight += weight;
            }

            if (
                !std::isfinite(totalWeight) ||
                totalWeight <= 0.0F)
            {
                return false;
            }
        }

        return true;
    }

    bool FbxAnimationClipData::IsValid(
        const FbxSkeletonData& skeleton) const noexcept
    {
        if (
            name.empty() ||
            !skeleton.IsValid() ||
            !std::isfinite(durationSeconds) ||
            durationSeconds < 0.0F ||
            !std::isfinite(sampleRate) ||
            sampleRate <= 0.0F ||
            tracks.empty())
        {
            return false;
        }

        for (const FbxAnimationTrack& track : tracks)
        {
            if (
                track.boneName.empty() ||
                track.boneIndex >=
                    skeleton.bones.size() ||
                track.keys.empty())
            {
                return false;
            }

            float previousTime = -1.0F;

            for (const FbxAnimationKey& key : track.keys)
            {
                if (
                    !std::isfinite(
                        key.timeSeconds) ||
                    key.timeSeconds <
                        previousTime ||
                    !IsFinite(key.translation) ||
                    !IsFinite(key.rotation) ||
                    !IsFinite(key.scale))
                {
                    return false;
                }

                previousTime =
                    key.timeSeconds;
            }
        }

        return true;
    }

    void FbxImportedScene::Clear() noexcept
    {
        skeleton = {};
        staticMeshes.clear();
        skeletalMeshes.clear();
        animationClips.clear();
    }

    bool FbxImportedScene::HasSkeleton() const noexcept
    {
        return skeleton.IsValid();
    }

    bool FbxImportedScene::IsEmpty() const noexcept
    {
        return
            !HasSkeleton() &&
            staticMeshes.empty() &&
            skeletalMeshes.empty() &&
            animationClips.empty();
    }

    namespace
    {
        void AppendFbxError(
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
        bool IsFbxExtension(
            const std::filesystem::path& path)
        {
            std::wstring extension =
                path.extension().wstring();

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
    }

    AssetResult InspectFbxSource(const std::filesystem::path& sourcePath, FbxSourceInfo& output,
        std::wstring& error) noexcept
    {
        output = {};
        error.clear();

        try
        {
            if (
                sourcePath.empty() ||
                !IsFbxExtension(sourcePath))
            {
                error =
                    L"Source path is not an FBX file.";

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

            options.load_external_files = false;
            options.generate_missing_normals = false;

            ufbx_error loadError{};

            const std::string sourceUtf8 =
                sourcePath.u8string();

            ufbx_scene* const scene =
                ufbx_load_file(
                    sourceUtf8.c_str(),
                    &options,
                    &loadError);

            if (scene == nullptr)
            {
                error =
                    L"ufbx failed to inspect FBX: ";

                AppendFbxError(
                    error,
                    loadError.description.data);

                return AssetResult::CorruptData;
            }

            std::unordered_set<
                const ufbx_node*>
                skeletonNodes;

            const auto includeParents =
                [&skeletonNodes](
                    const ufbx_node* node)
                {
                    while (node != nullptr)
                    {
                        skeletonNodes.insert(node);
                        node = node->parent;
                    }
                };

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

                const std::size_t instanceCount =
                    mesh->instances.count != 0U
                        ? mesh->instances.count
                        : 1U;

                if (mesh->skin_deformers.count != 0U)
                {
                    output.skeletalMeshCount +=
                        instanceCount;
                }
                else
                {
                    output.staticMeshCount +=
                        instanceCount;
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
                nodeIndex < scene->nodes.count;
                ++nodeIndex)
            {
                const ufbx_node* const node =
                    scene->nodes.data[nodeIndex];

                if (
                    node != nullptr &&
                    node->bone != nullptr)
                {
                    includeParents(node);
                }
            }

            output.skeletonBoneCount =
                skeletonNodes.size();

            for (
                std::size_t stackIndex = 0U;
                stackIndex <
                    scene->anim_stacks.count;
                ++stackIndex)
            {
                const ufbx_anim_stack* const stack =
                    scene->anim_stacks.data[
                        stackIndex];

                if (
                    stack != nullptr &&
                    stack->anim != nullptr)
                {
                    ++output.animationClipCount;
                }
            }

            ufbx_free_scene(scene);

            if (output.IsEmpty())
            {
                error =
                    L"FBX contains no supported assets.";

                return AssetResult::UnsupportedFormat;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            output = {};

            error =
                L"Not enough memory to inspect FBX.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            output = {};

            error =
                L"Unexpected FBX inspection failure.";

            return AssetResult::InternalError;
        }
    }
}
