#pragma once

#include <Assets/AnimationAsset.h>
#include <Assets/SkeletonAsset.h>

#include <Scene/CharacterAnimationTypes.h>

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <string>

namespace lts::editor
{
    struct CharacterAnimationLayerSample final
    {
        const engine::assets::AnimationAsset*
            currentAnimation = nullptr;

        const engine::assets::AnimationAsset*
            previousAnimation = nullptr;

        double currentTimeSeconds = 0.0;
        double previousTimeSeconds = 0.0;

        float transitionAlpha = 1.0F;
        float weight = 1.0F;

        engine::scene::CharacterAnimationLoopMode
            loopMode =
                engine::scene::
                    CharacterAnimationLoopMode::Loop;

        engine::scene::CharacterAnimationLoopMode
            previousLoopMode =
                engine::scene::
                    CharacterAnimationLoopMode::Loop;

        bool active = false;
    };

    struct CharacterAnimationEvaluationInput final
    {
        const engine::assets::SkeletonAsset*
            skeleton = nullptr;

        std::array<float, 3U> pivot{};

        CharacterAnimationLayerSample lowerBody;

        /*
         * Второй lower-body track как в CUberAnim.
         */
        CharacterAnimationLayerSample turnInPlace;

        CharacterAnimationLayerSample upperBody;
        CharacterAnimationLayerSample action;

        std::string upperBodyRootBone =
            "Bip01_Spine1";

        std::string actionRootBone =
            "Bip01_Spine1";

        std::string lookRootBone =
            "Bip01_Neck";

        float lookYawOffsetDegrees = 0.0F;
        float lookPitchOffsetDegrees = 0.0F;

        /*
         * CharacterController владеет горизонтальным movement и yaw.
         * При true у root-motion track блокируются X/Z и Y-twist;
         * animated Y, pitch и roll сохраняются.
         */
        bool blockControllerOwnedRootTransform = true;
    };

    struct CharacterAnimationPose final
    {
        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            boneMatrices{};

        /*
         * Абсолютная model-space поза после pivot.
         * Используется камерой и будущими sockets.
         */
        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            modelBoneMatrices{};

        std::uint32_t boneCount = 0U;
        bool animated = false;

        void Reset() noexcept;
    };

    class CharacterAnimationEvaluator final
    {
    public:
        CharacterAnimationEvaluator() noexcept =
            default;

        ~CharacterAnimationEvaluator() noexcept =
            default;

        CharacterAnimationEvaluator(
            const CharacterAnimationEvaluator&) =
                delete;

        CharacterAnimationEvaluator& operator=(
            const CharacterAnimationEvaluator&) =
                delete;

        [[nodiscard]]
        bool Evaluate(
            const CharacterAnimationEvaluationInput&
                input,

            CharacterAnimationPose&
                output) const noexcept;
    };
}
