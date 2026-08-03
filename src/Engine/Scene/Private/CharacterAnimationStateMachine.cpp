#include "Scene/CharacterAnimationStateMachine.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace engine::scene
{
    namespace
    {
        /*
         * Original AI_Player.CPP values.
         */
        constexpr float WarZIdleTurnSpeedDegrees =
            360.0F;

        constexpr float WarZMoveTurnSpeedDegrees =
            720.0F;

        constexpr float WarZBodyYawResponseIdle =
            4.0F;

        /*
         * Moving code called:
         *
         * UpdateBodyAdjustX(&bodyAdjust_x, 0, dt * 4)
         *
         * and UpdateBodyAdjustX multiplies dt by 4
         * internally. Effective response: 16.
         */
        constexpr float WarZBodyYawResponseMoving =
            16.0F;

        /*
         * AI_Player::UpdateRotation:
         * frameTime * 3.2 radians per second.
         */
        constexpr float WarZBodyPitchSpeedDegrees =
            183.346497F;

        constexpr float MaximumBodyPitchDegrees =
            89.0F;

        constexpr double LegacyFrameRate =
            30.0;

        constexpr double LegacyPausedFrameOneSeconds =
            1.0 / LegacyFrameRate;

        [[nodiscard]]
        float NormalizeAngleDegrees(
            const float value) noexcept
        {
            return
                std::isfinite(value)
                    ? std::remainder(
                        value,
                        360.0F)
                    : 0.0F;
        }

        [[nodiscard]]
        float AngleDifferenceDegrees(
            const float target,
            const float current) noexcept
        {
            return NormalizeAngleDegrees(
                target - current);
        }

        [[nodiscard]]
        float MoveAngleDegrees(
            const float current,
            const float target,
            const float maximumDelta) noexcept
        {
            const float safeDelta =
                std::isfinite(maximumDelta)
                    ? (std::max)(
                        maximumDelta,
                        0.0F)
                    : 0.0F;

            const float difference =
                AngleDifferenceDegrees(
                    target,
                    current);

            return NormalizeAngleDegrees(
                current +
                std::clamp(
                    difference,
                    -safeDelta,
                    safeDelta));
        }

        /*
         * Exact UpdateBodyAdjustX behaviour:
         *
         * current += (target - current) * dt * 4
         */
        [[nodiscard]]
        float UpdateBodyAdjust(
            const float current,
            const float target,
            const float response,
            const float deltaSeconds) noexcept
        {
            if (
                !std::isfinite(current) ||
                !std::isfinite(target) ||
                !std::isfinite(response) ||
                !std::isfinite(deltaSeconds) ||
                response <= 0.0F ||
                deltaSeconds <= 0.0F)
            {
                return
                    std::isfinite(current)
                        ? current
                        : 0.0F;
            }

            const float alpha =
                std::clamp(
                    deltaSeconds *
                        response,
                    0.0F,
                    1.0F);

            return
                current +
                (
                    target -
                    current
                ) *
                alpha;
        }

        [[nodiscard]]
        float MoveTowardsValue(
            const float current,
            const float target,
            const float maximumDelta) noexcept
        {
            const float safeDelta =
                std::isfinite(maximumDelta)
                    ? (std::max)(
                        maximumDelta,
                        0.0F)
                    : 0.0F;

            if (current < target)
            {
                return
                    (std::min)(
                        current +
                            safeDelta,
                        target);
            }

            if (current > target)
            {
                return
                    (std::max)(
                        current -
                            safeDelta,
                        target);
            }

            return target;
        }

        [[nodiscard]]
        bool IsMoving(
            const CharacterAnimationStateInput&
                input) noexcept
        {
            return
                input.movementDirection !=
                    CharacterMovementDirection::None ||
                input.movementSpeed > 0.05F;
        }

        [[nodiscard]]
        bool IsCrouched(
            const CharacterAnimationStateInput&
                input) noexcept
        {
            return
                input.stance ==
                    CharacterStance::Crouched;
        }

        [[nodiscard]]
        bool IsAiming(
            const CharacterAnimationStateInput&
                input) noexcept
        {
            return
                input.aiming ||
                input.upperBodyState ==
                    CharacterUpperBodyState::Aiming;
        }

        [[nodiscard]]
        bool IsJumpState(
            const CharacterLocomotionState
                state) noexcept
        {
            return
                state ==
                    CharacterLocomotionState::
                        JumpStart ||
                state ==
                    CharacterLocomotionState::
                        JumpLoop ||
                state ==
                    CharacterLocomotionState::
                        JumpLand;
        }

        [[nodiscard]]
        bool IsForwardDirectionGroup(
            const CharacterMovementDirection
                direction) noexcept
        {
            return
                direction ==
                    CharacterMovementDirection::
                        Forward ||
                direction ==
                    CharacterMovementDirection::
                        ForwardLeft ||
                direction ==
                    CharacterMovementDirection::
                        ForwardRight;
        }

        [[nodiscard]]
        bool IsBackwardDirectionGroup(
            const CharacterMovementDirection
                direction) noexcept
        {
            return
                direction ==
                    CharacterMovementDirection::
                        Backward ||
                direction ==
                    CharacterMovementDirection::
                        BackwardLeft ||
                direction ==
                    CharacterMovementDirection::
                        BackwardRight;
        }

        [[nodiscard]]
        bool IsSynchronizedDirectionTransition(
            const CharacterMovementDirection
                previousDirection,

            const CharacterMovementDirection
                currentDirection) noexcept
        {
            if (
                previousDirection ==
                    currentDirection ||
                previousDirection ==
                    CharacterMovementDirection::None ||
                currentDirection ==
                    CharacterMovementDirection::None)
            {
                return false;
            }

            return
                (
                    IsForwardDirectionGroup(
                        previousDirection) &&
                    IsForwardDirectionGroup(
                        currentDirection)
                ) ||
                (
                    IsBackwardDirectionGroup(
                        previousDirection) &&
                    IsBackwardDirectionGroup(
                        currentDirection)
                );
        }

        [[nodiscard]]
        double PlaybackSpeed(
            const CharacterLocomotionState
                state) noexcept
        {
            switch (state)
            {
                case CharacterLocomotionState::Run:
                    return 1.05;

                case CharacterLocomotionState::Sprint:
                    return 1.10;

                default:
                    return 1.0;
            }
        }

        [[nodiscard]]
        const std::wstring* FirstNonEmpty(
            const std::initializer_list<
                const std::wstring*>&
                    candidates) noexcept
        {
            for (
                const std::wstring* const candidate :
                candidates)
            {
                if (
                    candidate != nullptr &&
                    !candidate->empty())
                {
                    return candidate;
                }
            }

            return nullptr;
        }

        [[nodiscard]]
        const std::wstring* ResolveDirectional(
            const CharacterDirectionalAnimationSet&
                set,

            CharacterMovementDirection
                direction) noexcept
        {
            if (
                direction ==
                    CharacterMovementDirection::None)
            {
                direction =
                    CharacterMovementDirection::
                        Forward;
            }

            const std::wstring& resolved =
                set.Resolve(direction);

            return FirstNonEmpty(
            {
                &resolved,
                &set.forward,
                &set.backward,
                &set.forwardLeft,
                &set.forwardRight
            });
        }

        [[nodiscard]]
        const std::wstring* ResolveSprint(
            const CharacterSprintAnimationSet& set,
            CharacterMovementDirection
                direction) noexcept
        {
            if (
                direction ==
                    CharacterMovementDirection::None)
            {
                direction =
                    CharacterMovementDirection::
                        Forward;
            }

            const std::wstring& resolved =
                set.Resolve(direction);

            return FirstNonEmpty(
            {
                &resolved,
                &set.forward,
                &set.forwardLeft,
                &set.forwardRight
            });
        }

        [[nodiscard]]
        const CharacterViewAnimationSet& ResolveView(
            const CharacterAnimationSet& set,
            const CharacterViewMode mode) noexcept
        {
            return
                mode ==
                    CharacterViewMode::FirstPerson
                    ? set.firstPerson
                    : set.thirdPerson;
        }
    }

    void CharacterAnimationStateMachine::Reset(
        CharacterAnimationComponent&
            component) const noexcept
    {
        component.runtime.Reset();
    }

    void CharacterAnimationStateMachine::Update(
        const CharacterAnimationStateInput& input,
        CharacterAnimationComponent&
            component) const noexcept
    {
        CharacterAnimationRuntime& runtime =
            component.runtime;

        if (!component.enabled)
        {
            runtime.Reset();
            return;
        }

        const double animationDeltaSeconds =
            std::isfinite(input.deltaSeconds) &&
            input.deltaSeconds > 0.0
                ? input.deltaSeconds
                : 0.0;

        /*
         * Physics/rotation remains bounded.
         * Animation clocks receive the full delta.
         */
        const float rotationDeltaSeconds =
            static_cast<float>(
                (std::min)(
                    animationDeltaSeconds,
                    0.05));

        const bool firstUpdate =
            runtime.firstUpdate;

        const CharacterViewMode previousViewMode =
            runtime.viewMode;

        const CharacterStance previousStance =
            runtime.stance;

        const CharacterMovementDirection
            previousDirection =
                runtime.movementDirection;

        const CharacterLocomotionState
            previousLocomotion =
                runtime.locomotionState;

        const std::wstring previousLowerClip =
            runtime.lowerBody.currentClip;

        const std::wstring previousUpperClip =
            runtime.upperBody.currentClip;

        const double previousLowerTime =
            runtime.lowerBody.currentTimeSeconds;

        const double previousUpperTime =
            runtime.upperBody.currentTimeSeconds;

        /*
         * WarZ animation speeds:
         *
         * Run    1.05
         * Sprint 1.10
         * Other  1.00
         */
        const double locomotionDeltaSeconds =
            animationDeltaSeconds *
            PlaybackSpeed(previousLocomotion);

        AdvanceLayer(
            runtime.lowerBody,
            locomotionDeltaSeconds,
            input.lowerClipDurationSeconds);

        AdvanceLayer(
            runtime.turnInPlace,
            animationDeltaSeconds,
            input.turnClipDurationSeconds);

        AdvanceLayer(
            runtime.upperBody,
            locomotionDeltaSeconds,
            0.0);

        AdvanceLayer(
            runtime.action,
            animationDeltaSeconds,
            input.actionClipDurationSeconds);

        /*
         * Completed top/action animation fades out.
         */
        if (
            runtime.action.completed &&
            runtime.actionState !=
                CharacterActionState::None)
        {
            SetLayerClip(
                runtime.action,
                nullptr,
                component.animationSet.tuning.
                    actionBlendOutSeconds,
                CharacterAnimationLoopMode::Once,
                false);

            runtime.actionState =
                CharacterActionState::None;
        }

        const bool moving =
            IsMoving(input);

        const float desiredYaw =
            NormalizeAngleDegrees(
                input.desiredActorYawDegrees);

        float actorYaw =
            NormalizeAngleDegrees(
                runtime.actorYawDegrees);

        float bodyAdjustYaw =
            std::isfinite(
                runtime.upperBodyYawOffsetDegrees)
                ? runtime.
                    upperBodyYawOffsetDegrees
                : 0.0F;

        const float cameraToBodyDifference =
            AngleDifferenceDegrees(
                desiredYaw,
                actorYaw);

        const float turnEnterDegrees =
            (std::max)(
                std::fabs(
                    component.animationSet.tuning.
                        turnInPlaceEnterDegrees),
                1.0F);

        const float idleTurnSpeed =
            (std::max)(
                std::fabs(
                    component.animationSet.tuning.
                        turnInPlaceSpeedDegrees),
                WarZIdleTurnSpeedDegrees);

        /*
         * Natural completion removes the temporary
         * Turn In Place track, exactly like
         * UpdateTurnInPlaceAnim observing that the
         * old r3dAnimation track disappeared.
         */
        if (runtime.turnInPlace.completed)
        {
            SetLayerClip(
                runtime.turnInPlace,
                nullptr,
                0.10F,
                CharacterAnimationLoopMode::Once,
                false);

            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;
        }

        if (
            input.viewMode ==
                CharacterViewMode::FirstPerson)
        {
            /*
             * CUberAnim::StartTurnInPlaceAnim:
             * no Turn In Place for local FPS.
             */
            runtime.turnInPlace.Reset();
            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;

            actorYaw =
                MoveAngleDegrees(
                    actorYaw,
                    desiredYaw,
                    idleTurnSpeed *
                        rotationDeltaSeconds);

            bodyAdjustYaw = 0.0F;

            runtime.turnTargetYawDegrees =
                actorYaw;
        }
        else if (moving)
        {
            /*
             * AI_Player::UpdateRotation:
             * moving immediately fades out the
             * temporary turn track.
             */
            if (
                runtime.turnInPlace.active ||
                runtime.turnInPlaceActive)
            {
                SetLayerClip(
                    runtime.turnInPlace,
                    nullptr,
                    0.10F,
                    CharacterAnimationLoopMode::Once,
                    false);
            }

            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;

            bodyAdjustYaw =
                UpdateBodyAdjust(
                    bodyAdjustYaw,
                    0.0F,
                    WarZBodyYawResponseMoving,
                    rotationDeltaSeconds);

            actorYaw =
                MoveAngleDegrees(
                    actorYaw,
                    desiredYaw,
                    (std::max)(
                        input.
                            movementRotationSpeedDegrees,
                        WarZMoveTurnSpeedDegrees) *
                    rotationDeltaSeconds);

            runtime.turnTargetYawDegrees =
                actorYaw;
        }
        else if (
            input.grounded &&
            input.locomotionState ==
                CharacterLocomotionState::Idle)
        {
            /*
             * UpdateUpperBodyAngLegs:
             *
             * 1. torso follows the camera;
             * 2. after 45 degrees a temporary
             *    lower turn track starts;
             * 3. while that track exists, actor/legs
             *    rotate with _ai_fTurnSpeedIdle.
             */
            bodyAdjustYaw =
                UpdateBodyAdjust(
                    bodyAdjustYaw,
                    cameraToBodyDifference,
                    WarZBodyYawResponseIdle,
                    rotationDeltaSeconds);

            if (
                !runtime.turnInPlaceActive &&
                std::fabs(bodyAdjustYaw) >
                    turnEnterDegrees)
            {
                const std::int32_t direction =
                    bodyAdjustYaw >= 0.0F
                        ? 1
                        : -1;

                const std::wstring* const turnClip =
                    ResolveTurnInPlaceClip(
                        component.animationSet,
                        input,
                        direction);

                if (
                    turnClip != nullptr &&
                    !turnClip->empty())
                {
                    runtime.turnInPlaceActive = true;
                    runtime.turnDirection = direction;
                    runtime.turnTargetYawDegrees =
                        desiredYaw;

                    bodyAdjustYaw =
                        std::clamp(
                            bodyAdjustYaw,
                            -turnEnterDegrees,
                            turnEnterDegrees);

                    SetLayerClip(
                        runtime.turnInPlace,
                        turnClip,
                        0.10F,
                        CharacterAnimationLoopMode::Once,
                        true);
                }
            }

            if (runtime.turnInPlaceActive)
            {
                /*
                 * Original code recalculated fY from
                 * the live camera target every frame.
                 */
                runtime.turnTargetYawDegrees =
                    desiredYaw;

                actorYaw =
                    MoveAngleDegrees(
                        actorYaw,
                        desiredYaw,
                        idleTurnSpeed *
                            rotationDeltaSeconds);

                bodyAdjustYaw =
                    AngleDifferenceDegrees(
                        desiredYaw,
                        actorYaw);
            }
            else
            {
                runtime.turnTargetYawDegrees =
                    actorYaw;
            }
        }
        else
        {
            /*
             * Airborne/non-idle non-moving fallback.
             */
            if (
                runtime.turnInPlace.active ||
                runtime.turnInPlaceActive)
            {
                SetLayerClip(
                    runtime.turnInPlace,
                    nullptr,
                    0.10F,
                    CharacterAnimationLoopMode::Once,
                    false);
            }

            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;

            bodyAdjustYaw =
                UpdateBodyAdjust(
                    bodyAdjustYaw,
                    cameraToBodyDifference,
                    WarZBodyYawResponseIdle,
                    rotationDeltaSeconds);

            runtime.turnTargetYawDegrees =
                actorYaw;
        }

        runtime.actorYawDegrees =
            NormalizeAngleDegrees(actorYaw);

        runtime.lowerBodyYawDegrees =
            runtime.actorYawDegrees;

        runtime.upperBodyYawOffsetDegrees =
            std::clamp(
                bodyAdjustYaw,
                -std::fabs(
                    component.animationSet.tuning.
                        maximumUpperBodyYawDegrees),
                std::fabs(
                    component.animationSet.tuning.
                        maximumUpperBodyYawDegrees));

        const float requestedPitch =
            std::isfinite(
                input.lookPitchOffsetDegrees)
                ? input.lookPitchOffsetDegrees
                : 0.0F;

        const float targetPitch =
            input.viewMode ==
                CharacterViewMode::ThirdPerson
                ? std::clamp(
                    requestedPitch,
                    -MaximumBodyPitchDegrees,
                    MaximumBodyPitchDegrees)
                : 0.0F;

        runtime.upperBodyPitchOffsetDegrees =
            MoveTowardsValue(
                runtime.upperBodyPitchOffsetDegrees,
                targetPitch,
                WarZBodyPitchSpeedDegrees *
                    rotationDeltaSeconds);

        /*
         * Base locomotion remains independent from
         * the temporary Turn In Place track.
         */
        const CharacterLocomotionState
            resolvedLocomotion =
                input.locomotionState;

        const bool synchronizedDirectionChange =
            !firstUpdate &&
            previousViewMode == input.viewMode &&
            previousStance == input.stance &&
            previousLocomotion ==
                resolvedLocomotion &&
            IsSynchronizedDirectionTransition(
                previousDirection,
                input.movementDirection);

        /*
         * Lower body.
         *
         * In FPS the original SwitchToState did not
         * start a lower-body animation.
         */
        if (
            input.viewMode ==
                CharacterViewMode::FirstPerson)
        {
            runtime.lowerBody.Reset();
        }
        else
        {
            const std::wstring* const lowerClip =
                ResolveLowerBodyClip(
                    component.animationSet,
                    input);

            SetLayerClip(
                runtime.lowerBody,
                lowerClip,

                firstUpdate
                    ? 0.0F
                    : component.animationSet.tuning.
                        locomotionBlendSeconds,

                ResolveLowerBodyLoopMode(
                    resolvedLocomotion),

                false);

            if (
                synchronizedDirectionChange &&
                runtime.lowerBody.currentClip !=
                    previousLowerClip &&
                !runtime.lowerBody.
                    currentClip.empty())
            {
                /*
                 * Equivalent to copying fCurFrame
                 * between synchronized directional
                 * animations.
                 */
                runtime.lowerBody.currentTimeSeconds =
                    previousLowerTime;
            }
        }

        /*
         * Upper body.
         *
         * TPS jump removes upper animation, matching
         * the old jumpState/FallingDown branches.
         */
        const std::wstring* upperClip =
            nullptr;

        if (!(
                input.viewMode ==
                    CharacterViewMode::ThirdPerson &&
                IsJumpState(
                    resolvedLocomotion)))
        {
            upperClip =
                ResolveUpperBodyClip(
                    component.animationSet,
                    input);
        }

        SetLayerClip(
            runtime.upperBody,
            upperClip,

            firstUpdate
                ? 0.0F
                : component.animationSet.tuning.
                    upperBodyBlendSeconds,

            CharacterAnimationLoopMode::Loop,
            false);

        if (
            synchronizedDirectionChange &&
            runtime.upperBody.currentClip !=
                previousUpperClip &&
            !runtime.upperBody.currentClip.empty())
        {
            runtime.upperBody.currentTimeSeconds =
                previousUpperTime;
        }

        /*
         * Exact SwitchToState frame policies.
         */
        if (
            input.viewMode ==
                CharacterViewMode::ThirdPerson)
        {
            const bool stationary =
                !moving &&
                resolvedLocomotion ==
                    CharacterLocomotionState::Idle;

            if (
                stationary &&
                IsCrouched(input))
            {
                /*
                 * CrouchBlend / CrouchAim:
                 * frame 0 + paused.
                 */
                runtime.upperBody.currentTimeSeconds =
                    0.0;

                runtime.upperBody.previousTimeSeconds =
                    0.0;
            }
            else if (
                IsAiming(input) &&
                (
                    stationary ||
                    resolvedLocomotion ==
                        CharacterLocomotionState::Walk
                ))
            {
                /*
                 * StandUpper / WalkAim:
                 * frame 1 + paused.
                 */
                runtime.upperBody.currentTimeSeconds =
                    LegacyPausedFrameOneSeconds;

                runtime.upperBody.previousTimeSeconds =
                    LegacyPausedFrameOneSeconds;
            }
            else if (
                stationary &&
                !IsCrouched(input) &&
                runtime.lowerBody.active)
            {
                /*
                 * PLAYER_IDLE:
                 * lower and upper share one
                 * fIdleAnimFrame even when the clip
                 * names are different.
                 */
                runtime.upperBody.currentTimeSeconds =
                    runtime.lowerBody.
                        currentTimeSeconds;

                runtime.upperBody.previousTimeSeconds =
                    runtime.lowerBody.
                        previousTimeSeconds;
            }
        }

        /*
         * Top/action track.
         */
        if (
            input.actionRequest !=
                CharacterActionState::None)
        {
            const std::wstring* const actionClip =
                ResolveActionClip(
                    component.animationSet,
                    input);

            if (
                actionClip != nullptr &&
                !actionClip->empty())
            {
                SetLayerClip(
                    runtime.action,
                    actionClip,
                    component.animationSet.tuning.
                        actionBlendInSeconds,
                    CharacterAnimationLoopMode::Once,
                    input.restartAction);

                runtime.actionState =
                    input.actionRequest;
            }
        }

        runtime.viewMode =
            input.viewMode;

        runtime.stance =
            input.stance;

        runtime.movementDirection =
            input.movementDirection;

        runtime.locomotionState =
            resolvedLocomotion;

        runtime.upperBodyState =
            input.upperBodyState;

        runtime.movementSpeed =
            std::isfinite(input.movementSpeed)
                ? (std::max)(
                    input.movementSpeed,
                    0.0F)
                : 0.0F;

        runtime.grounded =
            input.grounded;

        runtime.aiming =
            IsAiming(input);

        runtime.firstUpdate =
            false;
    }

    void CharacterAnimationStateMachine::StopAction(
        CharacterAnimationComponent& component,
        const float blendOutSeconds) const noexcept
    {
        SetLayerClip(
            component.runtime.action,
            nullptr,
            blendOutSeconds,
            CharacterAnimationLoopMode::Once,
            false);

        component.runtime.actionState =
            CharacterActionState::None;
    }

    void CharacterAnimationStateMachine::AdvanceLayer(
        CharacterAnimationLayerRuntime& layer,
        const double deltaSeconds,
        const double clipDurationSeconds) noexcept
    {
        if (!layer.active)
        {
            return;
        }

        const double safeDeltaSeconds =
            std::isfinite(deltaSeconds) &&
            deltaSeconds > 0.0
                ? deltaSeconds
                : 0.0;

        if (!layer.currentClip.empty())
        {
            layer.currentTimeSeconds +=
                safeDeltaSeconds;
        }

        if (!layer.previousClip.empty())
        {
            layer.previousTimeSeconds +=
                safeDeltaSeconds;
        }

        const bool transitionActive =
            layer.transitionDurationSeconds > 0.0F &&
            layer.transitionElapsedSeconds <
                layer.transitionDurationSeconds;

        if (transitionActive)
        {
            layer.transitionElapsedSeconds =
                (std::min)(
                    layer.transitionElapsedSeconds +
                        static_cast<float>(
                            safeDeltaSeconds),

                    layer.transitionDurationSeconds);

            if (
                layer.transitionElapsedSeconds >=
                    layer.transitionDurationSeconds)
            {
                layer.previousClip.clear();
                layer.previousTimeSeconds = 0.0;

                layer.previousLoopMode =
                    CharacterAnimationLoopMode::Loop;
            }
        }
        else
        {
            layer.previousClip.clear();
            layer.previousTimeSeconds = 0.0;

            layer.previousLoopMode =
                CharacterAnimationLoopMode::Loop;
        }

        if (
            layer.loopMode ==
                CharacterAnimationLoopMode::Once &&
            !layer.currentClip.empty() &&
            std::isfinite(
                clipDurationSeconds) &&
            clipDurationSeconds > 0.0 &&
            layer.currentTimeSeconds >=
                clipDurationSeconds)
        {
            layer.currentTimeSeconds =
                clipDurationSeconds;

            layer.completed = true;
        }

        if (
            layer.currentClip.empty() &&
            layer.previousClip.empty())
        {
            layer.active = false;
            layer.completed = false;

            layer.currentTimeSeconds = 0.0;
            layer.previousTimeSeconds = 0.0;

            layer.transitionDurationSeconds = 0.0F;
            layer.transitionElapsedSeconds = 0.0F;

            layer.weight = 1.0F;
        }
    }

    void CharacterAnimationStateMachine::SetLayerClip(
        CharacterAnimationLayerRuntime& layer,
        const std::wstring* const animationPath,
        const float transitionSeconds,
        const CharacterAnimationLoopMode loopMode,
        const bool restart) noexcept
    {
        static const std::wstring emptyClip;

        const std::wstring& desiredClip =
            animationPath != nullptr
                ? *animationPath
                : emptyClip;

        if (
            layer.currentClip == desiredClip &&
            layer.loopMode == loopMode)
        {
            if (restart)
            {
                /*
                 * Repeated shoot/melee restarts the
                 * same top track without rebuilding
                 * the whole animation stack.
                 */
                layer.currentTimeSeconds = 0.0;
                layer.completed = false;
                layer.active =
                    !layer.currentClip.empty();

                layer.previousClip.clear();
                layer.previousTimeSeconds = 0.0;

                layer.transitionDurationSeconds =
                    0.0F;

                layer.transitionElapsedSeconds =
                    0.0F;
            }

            return;
        }

        std::wstring previousClip;
        double previousTimeSeconds = 0.0;

        CharacterAnimationLoopMode
            previousLoopMode =
                CharacterAnimationLoopMode::Loop;

        if (!layer.currentClip.empty())
        {
            previousClip =
                std::move(layer.currentClip);

            previousTimeSeconds =
                layer.currentTimeSeconds;

            previousLoopMode =
                layer.loopMode;
        }
        else if (!layer.previousClip.empty())
        {
            previousClip =
                std::move(layer.previousClip);

            previousTimeSeconds =
                layer.previousTimeSeconds;

            previousLoopMode =
                layer.previousLoopMode;
        }

        layer.previousClip =
            std::move(previousClip);

        layer.previousTimeSeconds =
            previousTimeSeconds;

        layer.previousLoopMode =
            previousLoopMode;

        layer.currentClip =
            desiredClip;

        layer.currentTimeSeconds = 0.0;
        layer.loopMode = loopMode;
        layer.completed = false;

        layer.transitionDurationSeconds =
            (std::max)(
                transitionSeconds,
                0.0F);

        layer.transitionElapsedSeconds =
            0.0F;

        if (
            layer.transitionDurationSeconds <=
                0.0F)
        {
            layer.previousClip.clear();
            layer.previousTimeSeconds = 0.0;

            layer.previousLoopMode =
                CharacterAnimationLoopMode::Loop;
        }

        layer.active =
            !layer.currentClip.empty() ||
            !layer.previousClip.empty();

        layer.weight = 1.0F;
    }

    const std::wstring*
        CharacterAnimationStateMachine::
            ResolveLowerBodyClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept
    {
        const CharacterLowerBodyAnimationSet& lower =
            animationSet.lowerBody;

        if (IsCrouched(input))
        {
            if (!IsMoving(input))
            {
                return FirstNonEmpty(
                {
                    &lower.crouchedIdle,
                    &lower.standingIdle
                });
            }

            return ResolveDirectional(
                lower.crouchedMove,
                input.movementDirection);
        }

        switch (input.locomotionState)
        {
            case CharacterLocomotionState::Walk:
                return ResolveDirectional(
                    lower.walk,
                    input.movementDirection);

            case CharacterLocomotionState::Run:
                return ResolveDirectional(
                    lower.run,
                    input.movementDirection);

            case CharacterLocomotionState::Sprint:
                return ResolveSprint(
                    lower.sprint,
                    input.movementDirection);

            case CharacterLocomotionState::JumpStart:
                return FirstNonEmpty(
                {
                    &lower.jumpStart,
                    &lower.jumpLoop,
                    &lower.standingIdle
                });

            case CharacterLocomotionState::JumpLoop:
                return FirstNonEmpty(
                {
                    &lower.jumpLoop,
                    &lower.jumpStart,
                    &lower.standingIdle
                });

            case CharacterLocomotionState::JumpLand:
                return FirstNonEmpty(
                {
                    &lower.jumpLand,
                    &lower.standingIdle
                });

            case CharacterLocomotionState::
                TurnInPlaceLeft:

            case CharacterLocomotionState::
                TurnInPlaceRight:

            case CharacterLocomotionState::Idle:

            default:
                return FirstNonEmpty(
                {
                    &lower.standingIdle
                });
        }
    }

    const std::wstring*
        CharacterAnimationStateMachine::
            ResolveTurnInPlaceClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input,

                const std::int32_t
                    turnDirection) noexcept
    {
        if (turnDirection == 0)
        {
            return nullptr;
        }

        const CharacterLowerBodyAnimationSet& lower =
            animationSet.lowerBody;

        if (IsCrouched(input))
        {
            /*
             * Original data uses one Crouch_Str
             * clip for both directions.
             */
            return
                turnDirection < 0
                    ? FirstNonEmpty(
                    {
                        &lower.
                            crouchedTurnInPlaceLeft,

                        &lower.
                            crouchedTurnInPlaceRight
                    })
                    : FirstNonEmpty(
                    {
                        &lower.
                            crouchedTurnInPlaceRight,

                        &lower.
                            crouchedTurnInPlaceLeft
                    });
        }

        /*
         * Original data uses one walk_stand_BL
         * clip for both directions.
         */
        return
            turnDirection < 0
                ? FirstNonEmpty(
                {
                    &lower.turnInPlaceLeft,
                    &lower.turnInPlaceRight
                })
                : FirstNonEmpty(
                {
                    &lower.turnInPlaceRight,
                    &lower.turnInPlaceLeft
                });
    }

    const std::wstring*
        CharacterAnimationStateMachine::
            ResolveUpperBodyClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept
    {
        const CharacterUpperBodyAnimationSet& upper =
            ResolveView(
                animationSet,
                input.viewMode).
                    upperBody;

        const bool crouched =
            IsCrouched(input);

        const bool moving =
            IsMoving(input);

        const bool aiming =
            IsAiming(input);

        if (crouched)
        {
            if (aiming)
            {
                return
                    moving
                        ? FirstNonEmpty(
                        {
                            &upper.crouchedAimMove,
                            &upper.crouchedAimIdle,
                            &upper.crouchedRelaxedMove,
                            &upper.crouchedRelaxedIdle
                        })
                        : FirstNonEmpty(
                        {
                            &upper.crouchedAimIdle,
                            &upper.crouchedAimMove,
                            &upper.crouchedRelaxedIdle,
                            &upper.crouchedRelaxedMove
                        });
            }

            return
                moving
                    ? FirstNonEmpty(
                    {
                        &upper.crouchedRelaxedMove,
                        &upper.crouchedRelaxedIdle
                    })
                    : FirstNonEmpty(
                    {
                        &upper.crouchedRelaxedIdle,
                        &upper.crouchedRelaxedMove
                    });
        }

        if (aiming)
        {
            return
                moving
                    ? FirstNonEmpty(
                    {
                        &upper.standingAimMove,
                        &upper.standingAimIdle,
                        &upper.standingRelaxedMove,
                        &upper.standingRelaxedIdle
                    })
                    : FirstNonEmpty(
                    {
                        &upper.standingAimIdle,
                        &upper.standingAimMove,
                        &upper.standingRelaxedIdle,
                        &upper.standingRelaxedMove
                    });
        }

        return
            moving
                ? FirstNonEmpty(
                {
                    &upper.standingRelaxedMove,
                    &upper.standingRelaxedIdle
                })
                : FirstNonEmpty(
                {
                    &upper.standingRelaxedIdle,
                    &upper.standingRelaxedMove
                });
    }

    const std::wstring*
        CharacterAnimationStateMachine::
            ResolveActionClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept
    {
        const CharacterActionAnimationSet& actions =
            ResolveView(
                animationSet,
                input.viewMode).
                    actions;

        const bool crouched =
            IsCrouched(input);

        const bool moving =
            IsMoving(input);

        switch (input.actionRequest)
        {
            case CharacterActionState::Primary:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &actions.primaryCrouched,
                        &actions.primaryStanding,
                        &actions.primaryMoving
                    });
                }

                return
                    moving
                        ? FirstNonEmpty(
                        {
                            &actions.primaryMoving,
                            &actions.primaryStanding
                        })
                        : FirstNonEmpty(
                        {
                            &actions.primaryStanding,
                            &actions.primaryMoving
                        });
            }

            case CharacterActionState::Secondary:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &actions.secondaryCrouched,
                        &actions.secondaryStanding,
                        &actions.secondaryMoving
                    });
                }

                return
                    moving
                        ? FirstNonEmpty(
                        {
                            &actions.secondaryMoving,
                            &actions.secondaryStanding
                        })
                        : FirstNonEmpty(
                        {
                            &actions.secondaryStanding,
                            &actions.secondaryMoving
                        });
            }

            case CharacterActionState::Reload:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &actions.reloadCrouched,
                        &actions.reloadStanding,
                        &actions.reloadMoving
                    });
                }

                return
                    moving
                        ? FirstNonEmpty(
                        {
                            &actions.reloadMoving,
                            &actions.reloadStanding
                        })
                        : FirstNonEmpty(
                        {
                            &actions.reloadStanding,
                            &actions.reloadMoving
                        });
            }

            case CharacterActionState::None:

            default:
                return nullptr;
        }
    }

    CharacterAnimationLoopMode
        CharacterAnimationStateMachine::
            ResolveLowerBodyLoopMode(
                const CharacterLocomotionState
                    state) noexcept
    {
        switch (state)
        {
            case CharacterLocomotionState::JumpStart:
            case CharacterLocomotionState::JumpLand:
                return
                    CharacterAnimationLoopMode::Once;

            default:
                return
                    CharacterAnimationLoopMode::Loop;
        }
    }
}