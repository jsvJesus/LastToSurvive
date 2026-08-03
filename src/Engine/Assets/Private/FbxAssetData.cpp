#include "Assets/FbxAssetData.h"

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
}
