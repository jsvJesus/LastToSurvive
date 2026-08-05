#pragma once

#include <Assets/SkeletonAsset.h>

#include <DirectXMath.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace lts::editor
{
    inline constexpr std::size_t
        InvalidCharacterBoneIndex =
            (std::numeric_limits<std::size_t>::max)();

    struct CharacterPoseTransform final
    {
        DirectX::XMFLOAT3 translation
        {
            0.0F,
            0.0F,
            0.0F
        };

        DirectX::XMFLOAT4 rotation
        {
            0.0F,
            0.0F,
            0.0F,
            1.0F
        };

        DirectX::XMFLOAT3 scale
        {
            1.0F,
            1.0F,
            1.0F
        };
    };

    struct CharacterAnimationKeyframe final
    {
        float timeSeconds = 0.0F;
        CharacterPoseTransform transform;
    };

    struct CharacterAnimationTrack final
    {
        std::string boneName;

        std::vector<CharacterAnimationKeyframe>
            keyframes;
    };

    struct CharacterAnimationClip final
    {
        std::string name;

        float durationSeconds = 0.0F;
        bool looping = true;

        std::vector<CharacterAnimationTrack>
            tracks;

        [[nodiscard]]
        bool IsValid() const noexcept;
    };

    class CharacterPose final
    {
    public:
        CharacterPose() noexcept;

        [[nodiscard]]
        bool Initialize(
            const engine::assets::SkeletonAsset& skeleton) noexcept;

        void Clear() noexcept;

        void ResetToBindPose() noexcept;

        [[nodiscard]]
        bool Rebuild() noexcept;

        [[nodiscard]]
        bool IsValid() const noexcept;

        [[nodiscard]]
        std::size_t GetBoneCount() const noexcept;

        [[nodiscard]]
        std::size_t FindBone(
            const std::string& name) const noexcept;

        [[nodiscard]]
        std::int32_t GetParentIndex(
            std::size_t boneIndex) const noexcept;

        [[nodiscard]]
        const std::string& GetBoneName(
            std::size_t boneIndex) const noexcept;

        [[nodiscard]]
        bool SetLocalTransform(
            std::size_t boneIndex,
            const CharacterPoseTransform& transform) noexcept;

        [[nodiscard]]
        const CharacterPoseTransform*
            GetLocalTransform(
                std::size_t boneIndex) const noexcept;

        [[nodiscard]]
        const DirectX::XMFLOAT4X4*
            GetAbsoluteMatrix(
                std::size_t boneIndex) const noexcept;

        [[nodiscard]]
        DirectX::XMFLOAT3 GetBonePosition(
            std::size_t boneIndex) const noexcept;

        [[nodiscard]]
        const DirectX::XMFLOAT4X4*
            GetPaletteData() const noexcept;

        [[nodiscard]]
        std::size_t GetPaletteByteSize() const noexcept;

        /*
         * rootBone  = upperarm_l
         * jointBone = lowerarm_l
         * endBone   = hand_l
         *
         * targetAbsolute задаёт позицию и ориентацию hand_l.
         * polePosition определяет направление локтя.
         */
        [[nodiscard]]
        bool ApplyTwoBoneIk(
            std::size_t rootBone,
            std::size_t jointBone,
            std::size_t endBone,
            const DirectX::XMFLOAT4X4& targetAbsolute,
            const DirectX::XMFLOAT3& polePosition) noexcept;

    private:
        [[nodiscard]]
        bool SetAbsoluteMatrix(
            std::size_t boneIndex,
            DirectX::FXMMATRIX absoluteMatrix) noexcept;

        [[nodiscard]]
        bool RotateBoneToward(
            std::size_t boneIndex,
            DirectX::FXMVECTOR currentDirection,
            DirectX::FXMVECTOR desiredDirection) noexcept;

        std::array<
            std::string,
            engine::assets::MaximumSkeletonBones>
            boneNames_;

        std::array<
            std::int32_t,
            engine::assets::MaximumSkeletonBones>
            parentIndices_{};

        std::array<
            CharacterPoseTransform,
            engine::assets::MaximumSkeletonBones>
            bindLocalTransforms_{};

        std::array<
            CharacterPoseTransform,
            engine::assets::MaximumSkeletonBones>
            localTransforms_{};

        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            inverseBindMatrices_{};

        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            absoluteMatrices_{};

        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            paletteMatrices_{};

        std::size_t boneCount_ = 0U;
        bool valid_ = false;
    };

    class CharacterAnimationPlayer final
    {
    public:
        CharacterAnimationPlayer() noexcept = default;

        [[nodiscard]]
        bool SetClip(
            std::shared_ptr<
                const CharacterAnimationClip> clip) noexcept;

        void ClearClip() noexcept;

        void Play() noexcept;
        void Pause() noexcept;
        void Stop() noexcept;

        void SetLooping(bool looping) noexcept;
        void SetPlaybackSpeed(float speed) noexcept;
        void SetTime(float timeSeconds) noexcept;

        void Update(float deltaSeconds) noexcept;

        [[nodiscard]]
        bool Evaluate(
            CharacterPose& pose) const noexcept;

        [[nodiscard]]
        bool HasClip() const noexcept;

        [[nodiscard]]
        bool IsPlaying() const noexcept;

        [[nodiscard]]
        bool IsLooping() const noexcept;

        [[nodiscard]]
        float GetPlaybackSpeed() const noexcept;

        [[nodiscard]]
        float GetTime() const noexcept;

        [[nodiscard]]
        float GetDuration() const noexcept;

        [[nodiscard]]
        const CharacterAnimationClip*
            GetClip() const noexcept;

    private:
        [[nodiscard]]
        static bool SampleTrack(
            const CharacterAnimationTrack& track,
            float timeSeconds,
            CharacterPoseTransform& output) noexcept;

        std::shared_ptr<
            const CharacterAnimationClip>
            clip_;

        float timeSeconds_ = 0.0F;
        float playbackSpeed_ = 1.0F;

        bool playing_ = false;
        bool looping_ = true;
    };
}