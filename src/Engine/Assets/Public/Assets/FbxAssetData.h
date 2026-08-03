#pragma once

#include "Assets/AssetResult.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace engine::assets
{
    inline constexpr std::size_t
        MaximumFbxBoneInfluences = 8U;

    struct FbxMatrix4 final
    {
        /*
         * Row-major matrix for the engine row-vector convention.
         */
        std::array<float, 16U> values
        {
            1.0F, 0.0F, 0.0F, 0.0F,
            0.0F, 1.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F, 0.0F,
            0.0F, 0.0F, 0.0F, 1.0F
        };
    };

    struct FbxBounds final
    {
        std::array<float, 3U> minimum{};
        std::array<float, 3U> maximum{};
        std::array<float, 3U> center{};
        float radius = 0.0F;

        [[nodiscard]]
        bool IsValid() const noexcept;
    };

    struct FbxMaterialSlot final
    {
        std::string name;
    };

    struct FbxMeshSection final
    {
        std::uint32_t firstIndex = 0U;
        std::uint32_t indexCount = 0U;
        std::uint32_t materialSlot = 0U;
    };

    struct FbxStaticVertex final
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

        std::array<float, 2U> texcoord0{};
        std::array<float, 4U> color
        {
            1.0F,
            1.0F,
            1.0F,
            1.0F
        };
    };

    struct FbxSkeletalVertex final
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

        std::array<float, 2U> texcoord0{};
        std::array<float, 4U> color
        {
            1.0F,
            1.0F,
            1.0F,
            1.0F
        };

        std::array<
            std::uint16_t,
            MaximumFbxBoneInfluences>
            boneIndices{};

        std::array<
            float,
            MaximumFbxBoneInfluences>
            boneWeights{};
    };

    struct FbxSkeletonBone final
    {
        std::string name;
        std::int32_t parentIndex = -1;

        FbxMatrix4 localBindMatrix;
        FbxMatrix4 modelBindMatrix;
    };

    struct FbxSkeletonData final
    {
        std::string name;
        std::vector<FbxSkeletonBone> bones;

        [[nodiscard]]
        bool IsValid() const noexcept;
    };

    struct FbxStaticMeshData final
    {
        std::string name;
        std::string sourceNodeName;

        FbxMatrix4 localToWorld;
        FbxBounds bounds;

        std::vector<FbxStaticVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<FbxMeshSection> sections;
        std::vector<FbxMaterialSlot> materials;

        [[nodiscard]]
        bool IsValid() const noexcept;
    };

    struct FbxSkeletalMeshData final
    {
        std::string name;
        std::string sourceNodeName;

        /*
         * Geometry remains in FBX mesh-local space.
         * localToWorld is the source node transform.
         */
        FbxMatrix4 localToWorld;
        FbxBounds bounds;

        std::vector<FbxSkeletalVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<FbxMeshSection> sections;
        std::vector<FbxMaterialSlot> materials;

        /*
         * One inverse bind matrix per FbxSkeletonData::bones entry.
         * Matrices map mesh-local geometry into bone-local bind space.
         */
        std::vector<FbxMatrix4> inverseBindMatrices;

        [[nodiscard]]
        bool IsValid(
            const FbxSkeletonData& skeleton) const noexcept;
    };

    struct FbxAnimationKey final
    {
        float timeSeconds = 0.0F;

        std::array<float, 3U> translation{};

        std::array<float, 4U> rotation
        {
            0.0F,
            0.0F,
            0.0F,
            1.0F
        };

        std::array<float, 3U> scale
        {
            1.0F,
            1.0F,
            1.0F
        };
    };

    struct FbxAnimationTrack final
    {
        std::uint32_t boneIndex = 0U;
        std::string boneName;
        std::vector<FbxAnimationKey> keys;
    };

    struct FbxAnimationClipData final
    {
        std::string name;

        float durationSeconds = 0.0F;
        float sampleRate = 30.0F;

        std::vector<FbxAnimationTrack> tracks;

        [[nodiscard]]
        bool IsValid(
            const FbxSkeletonData& skeleton) const noexcept;
    };

    struct FbxImportedScene final
    {
        FbxSkeletonData skeleton;

        std::vector<FbxStaticMeshData>
            staticMeshes;

        std::vector<FbxSkeletalMeshData>
            skeletalMeshes;

        std::vector<FbxAnimationClipData>
            animationClips;

        void Clear() noexcept;

        [[nodiscard]]
        bool HasSkeleton() const noexcept;

        [[nodiscard]]
        bool IsEmpty() const noexcept;
    };

    struct FbxSourceInfo final
    {
        std::size_t staticMeshCount = 0U;
        std::size_t skeletalMeshCount = 0U;
        std::size_t skeletonBoneCount = 0U;
        std::size_t animationClipCount = 0U;

        [[nodiscard]]
        bool HasStaticMeshes() const noexcept
        {
            return staticMeshCount != 0U;
        }

        [[nodiscard]]
        bool HasSkeletalMeshes() const noexcept
        {
            return skeletalMeshCount != 0U;
        }

        [[nodiscard]]
        bool HasSkeleton() const noexcept
        {
            return skeletonBoneCount != 0U;
        }

        [[nodiscard]]
        bool HasAnimations() const noexcept
        {
            return animationClipCount != 0U;
        }

        [[nodiscard]]
        bool IsEmpty() const noexcept
        {
            return
                !HasStaticMeshes() &&
                !HasSkeletalMeshes() &&
                !HasSkeleton() &&
                !HasAnimations();
        }
    };

    struct FbxImportReport final
    {
        std::vector<std::filesystem::path> writtenFiles;
        std::vector<std::wstring> warnings;

        void Clear() noexcept
        {
            writtenFiles.clear();
            warnings.clear();
        }
    };

    [[nodiscard]]
    AssetResult InspectFbxSource(
        const std::filesystem::path& sourcePath,
        FbxSourceInfo& output,
        std::wstring& error) noexcept;
}
