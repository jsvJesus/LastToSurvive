#pragma once

#include "Scene/CharacterAnimationTypes.h"

namespace engine::scene
{
    /*
     * Input for one CUberAnim-style update.
     *
     * Animation paths remain in CharacterAnimationSet,
     * loaded from the external JSON profile.
     */
    struct CharacterAnimationStateInput final
    {
        double deltaSeconds = 0.0;

        CharacterViewMode viewMode =
            CharacterViewMode::ThirdPerson;

        CharacterStance stance =
            CharacterStance::Standing;

        CharacterMovementDirection movementDirection =
            CharacterMovementDirection::None;

        CharacterLocomotionState locomotionState =
            CharacterLocomotionState::Idle;

        CharacterUpperBodyState upperBodyState =
            CharacterUpperBodyState::Relaxed;

        CharacterActionState actionRequest =
            CharacterActionState::None;

        float movementSpeed = 0.0F;

        /*
         * Camera/gameplay yaw requested by the player.
         * actorYawDegrees remains the actual legs yaw.
         */
        float desiredActorYawDegrees = 0.0F;

        /*
         * Camera pitch consumed by procedural
         * Bip01_Spine1 / Bip01_Neck adjustment.
         */
        float lookPitchOffsetDegrees = 0.0F;

        /*
         * Moving rotation speed.
         * WarZ default is 720 degrees per second.
         */
        float movementRotationSpeedDegrees = 720.0F;

        bool grounded = true;
        bool aiming = false;

        bool restartAction = false;

        double actionClipDurationSeconds = 0.0;
        double lowerClipDurationSeconds = 0.0;
        double turnClipDurationSeconds = 0.0;
    };

    /*
     * WarZ-style animation stack controller.
     *
     * Runtime order:
     *
     * 1. lowerBody
     * 2. turnInPlace
     * 3. upperBody
     * 4. action
     *
     * The evaluator already consumes the layers
     * in exactly this order.
     */
    class CharacterAnimationStateMachine final
    {
    public:
        CharacterAnimationStateMachine() noexcept =
            default;

        ~CharacterAnimationStateMachine() noexcept =
            default;

        CharacterAnimationStateMachine(
            const CharacterAnimationStateMachine&) =
                delete;

        CharacterAnimationStateMachine& operator=(
            const CharacterAnimationStateMachine&) =
                delete;

        void Reset(
            CharacterAnimationComponent& component)
            const noexcept;

        void Update(
            const CharacterAnimationStateInput& input,
            CharacterAnimationComponent& component)
            const noexcept;

        void StopAction(
            CharacterAnimationComponent& component,
            float blendOutSeconds) const noexcept;

    private:
        static void AdvanceLayer(
            CharacterAnimationLayerRuntime& layer,
            double deltaSeconds,
            double clipDurationSeconds) noexcept;

        static void SetLayerClip(
            CharacterAnimationLayerRuntime& layer,
            const std::wstring* animationPath,
            float transitionSeconds,
            CharacterAnimationLoopMode loopMode,
            bool restart) noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveLowerBodyClip(
                const CharacterAnimationSet& animationSet,
                const CharacterAnimationStateInput& input)
                noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveTurnInPlaceClip(
                const CharacterAnimationSet& animationSet,
                const CharacterAnimationStateInput& input,
                std::int32_t turnDirection) noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveUpperBodyClip(
                const CharacterAnimationSet& animationSet,
                const CharacterAnimationStateInput& input)
                noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveActionClip(
                const CharacterAnimationSet& animationSet,
                const CharacterAnimationStateInput& input)
                noexcept;

        [[nodiscard]]
        static CharacterAnimationLoopMode
            ResolveLowerBodyLoopMode(
                CharacterLocomotionState state) noexcept;
    };
}