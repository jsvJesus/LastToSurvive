#include "Scene/CharacterAnimationStateMachine.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace engine::scene
{
    namespace
    {
        constexpr float MaximumProceduralPitchDegrees =
            89.0F;

        [[nodiscard]]
        float NormalizeAngleDegrees(
            const float angleDegrees) noexcept
        {
            return std::isfinite(angleDegrees)
                ? std::remainder(angleDegrees, 360.0F)
                : 0.0F;
        }

        [[nodiscard]]
        float AngleDifferenceDegrees(
            const float targetDegrees,
            const float currentDegrees) noexcept
        {
            return NormalizeAngleDegrees(
                targetDegrees - currentDegrees);
        }

        [[nodiscard]]
        float MoveAngleDegrees(
            const float currentDegrees,
            const float targetDegrees,
            const float maximumDeltaDegrees) noexcept
        {
            const float differenceDegrees =
                AngleDifferenceDegrees(
                    targetDegrees,
                    currentDegrees);

            return NormalizeAngleDegrees(
                currentDegrees +
                std::clamp(
                    differenceDegrees,
                    -(std::max)(maximumDeltaDegrees, 0.0F),
                    (std::max)(maximumDeltaDegrees, 0.0F)));
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
        const std::wstring* ResolveDirectionalClip(
            const CharacterDirectionalAnimationSet&
                directionalSet,

            const CharacterMovementDirection
                movementDirection) noexcept
        {
            const std::wstring&
                directionalClip =
                    directionalSet.Resolve(
                        movementDirection);

            return FirstNonEmpty(
            {
                &directionalClip,
                &directionalSet.forward
            });
        }

        [[nodiscard]]
        const std::wstring* ResolveSprintClip(
            const CharacterSprintAnimationSet&
                sprintSet,

            const CharacterMovementDirection
                movementDirection) noexcept
        {
            const std::wstring&
                directionalClip =
                    sprintSet.Resolve(
                        movementDirection);

            return FirstNonEmpty(
            {
                &directionalClip,
                &sprintSet.forward,
                &sprintSet.forwardLeft,
                &sprintSet.forwardRight
            });
        }
        
        [[nodiscard]]
        bool IsCharacterMoving(
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
        const CharacterViewAnimationSet&
            ResolveViewSet(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterViewMode
                    viewMode) noexcept
        {
            return
                viewMode ==
                    CharacterViewMode::FirstPerson
                    ? animationSet.firstPerson
                    : animationSet.thirdPerson;
        }
    }

    void CharacterAnimationStateMachine::Reset(
        CharacterAnimationComponent&
            component) const noexcept
    {
        component.runtime.Reset();
    }

    void CharacterAnimationStateMachine::Update(
        const CharacterAnimationStateInput&
            input,

        CharacterAnimationComponent&
            component) const noexcept
    {
        CharacterAnimationRuntime& runtime =
            component.runtime;

        const double animationDeltaSeconds =
            std::isfinite(input.deltaSeconds) &&
            input.deltaSeconds > 0.0
                ? input.deltaSeconds
                : 0.0;

        /*
         * Поворот и physics остаются ограниченными.
         * Animation clock получает полное время кадра.
         */
        const float rotationDeltaSeconds =
            static_cast<float>(
                (std::min)(
                    animationDeltaSeconds,
                    0.05));

        const CharacterAnimationSet& animationSet =
            component.animationSet;

        const bool firstUpdate =
            runtime.firstUpdate;

        const float desiredActorYawDegrees =
            NormalizeAngleDegrees(
                input.desiredActorYawDegrees);

        float actorYawDegrees =
            NormalizeAngleDegrees(
                runtime.actorYawDegrees);

        const bool moving =
            IsCharacterMoving(input);

        const float turnEnterDegrees =
            (std::max)(
                std::fabs(
                    animationSet.tuning.
                        turnInPlaceEnterDegrees),
                1.0F);

        const float turnExitDegrees =
            std::clamp(
                std::fabs(
                    animationSet.tuning.
                        turnInPlaceExitDegrees),
                0.0F,
                turnEnterDegrees);

        const float turnSpeedDegrees =
            (std::max)(
                std::fabs(
                    animationSet.tuning.
                        turnInPlaceSpeedDegrees),
                1.0F);

        const float frameTurnDelta =
            turnSpeedDegrees *
                rotationDeltaSeconds;

        const bool canTurnInPlace =
            input.viewMode ==
                CharacterViewMode::ThirdPerson &&
            input.grounded &&
            !moving &&
            input.locomotionState ==
                CharacterLocomotionState::Idle;

        const bool lowerLayerContainsTurnClip =
            (
                runtime.locomotionState ==
                    CharacterLocomotionState::
                        TurnInPlaceLeft ||
                runtime.locomotionState ==
                    CharacterLocomotionState::
                        TurnInPlaceRight
            ) &&
            runtime.lowerBody.loopMode ==
                CharacterAnimationLoopMode::Once &&
            !runtime.lowerBody.currentClip.empty();

        const bool turnClipFinished =
            lowerLayerContainsTurnClip &&
            runtime.lowerBody.completed;

        if (moving)
        {
            actorYawDegrees =
                MoveAngleDegrees(
                    actorYawDegrees,
                    desiredActorYawDegrees,
                    (std::max)(
                        input.
                            movementRotationSpeedDegrees,
                        0.0F) *
                    rotationDeltaSeconds);

            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;

            runtime.turnTargetYawDegrees =
                actorYawDegrees;
        }
        else if (
            input.viewMode ==
                CharacterViewMode::FirstPerson)
        {
            actorYawDegrees =
                MoveAngleDegrees(
                    actorYawDegrees,
                    desiredActorYawDegrees,
                    frameTurnDelta);

            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;

            runtime.turnTargetYawDegrees =
                actorYawDegrees;
        }
        else if (!canTurnInPlace)
        {
            runtime.turnInPlaceActive = false;
            runtime.turnDirection = 0;

            runtime.turnTargetYawDegrees =
                actorYawDegrees;
        }
        else
        {
            const float cameraBodyDifference =
                AngleDifferenceDegrees(
                    desiredActorYawDegrees,
                    actorYawDegrees);

            bool startedTurnThisUpdate =
                false;

            if (
                !runtime.turnInPlaceActive &&
                std::fabs(
                    cameraBodyDifference) >=
                    turnEnterDegrees)
            {
                runtime.turnInPlaceActive =
                    true;

                runtime.turnTargetYawDegrees =
                    desiredActorYawDegrees;

                runtime.turnDirection =
                    cameraBodyDifference >= 0.0F
                        ? 1
                        : -1;

                startedTurnThisUpdate =
                    true;
            }

            if (runtime.turnInPlaceActive)
            {
                float remainingDifference =
                    AngleDifferenceDegrees(
                        runtime.
                            turnTargetYawDegrees,
                        actorYawDegrees);

                if (turnClipFinished)
                {
                    actorYawDegrees =
                        NormalizeAngleDegrees(
                            runtime.
                                turnTargetYawDegrees);

                    runtime.turnInPlaceActive =
                        false;

                    runtime.turnDirection = 0;
                }
                else if (!startedTurnThisUpdate)
                {
                    float maximumTurnDelta =
                        frameTurnDelta;

                    /*
                     * Когда длительность turn-клипа известна,
                     * угол ног синхронизируется с оставшимся
                     * временем клипа.
                     */
                    if (
                        lowerLayerContainsTurnClip &&
                        std::isfinite(
                            input.
                                lowerClipDurationSeconds) &&
                        input.
                            lowerClipDurationSeconds >
                                0.0)
                    {
                        const double remainingClipSeconds =
                            (std::max)(
                                input.
                                    lowerClipDurationSeconds -
                                runtime.lowerBody.
                                    currentTimeSeconds,
                                0.0);

                        if (
                            remainingClipSeconds >
                                0.000001 &&
                            animationDeltaSeconds > 0.0)
                        {
                            const double stepSeconds =
                                (std::min)(
                                    animationDeltaSeconds,
                                    remainingClipSeconds);

                            maximumTurnDelta =
                                std::fabs(
                                    remainingDifference) *
                                static_cast<float>(
                                    stepSeconds /
                                    remainingClipSeconds);
                        }
                    }

                    actorYawDegrees =
                        MoveAngleDegrees(
                            actorYawDegrees,
                            runtime.
                                turnTargetYawDegrees,
                            maximumTurnDelta);

                    remainingDifference =
                        AngleDifferenceDegrees(
                            runtime.
                                turnTargetYawDegrees,
                            actorYawDegrees);

                    if (
                        std::fabs(
                            remainingDifference) <=
                            turnExitDegrees)
                    {
                        actorYawDegrees =
                            NormalizeAngleDegrees(
                                runtime.
                                    turnTargetYawDegrees);

                        runtime.turnInPlaceActive =
                            false;

                        runtime.turnDirection = 0;
                    }
                }
            }
            else
            {
                runtime.turnTargetYawDegrees =
                    actorYawDegrees;

                runtime.turnDirection = 0;
            }
        }

        runtime.actorYawDegrees =
            NormalizeAngleDegrees(
                actorYawDegrees);

        runtime.lowerBodyYawDegrees =
            runtime.actorYawDegrees;

        const float maximumLookYaw =
            (std::max)(
                std::fabs(
                    animationSet.tuning.
                        maximumUpperBodyYawDegrees),
                0.0F);

        runtime.upperBodyYawOffsetDegrees =
            std::clamp(
                AngleDifferenceDegrees(
                    desiredActorYawDegrees,
                    runtime.actorYawDegrees),
                -maximumLookYaw,
                maximumLookYaw);

        runtime.upperBodyPitchOffsetDegrees =
            std::clamp(
                std::isfinite(
                    input.lookPitchOffsetDegrees)
                    ? input.lookPitchOffsetDegrees
                    : 0.0F,
                -MaximumProceduralPitchDegrees,
                MaximumProceduralPitchDegrees);

        CharacterLocomotionState resolvedLocomotionState =
            input.locomotionState;

        if (
            runtime.turnInPlaceActive &&
            runtime.turnDirection != 0)
        {
            resolvedLocomotionState =
                runtime.turnDirection < 0
                    ? CharacterLocomotionState::
                        TurnInPlaceLeft
                    : CharacterLocomotionState::
                        TurnInPlaceRight;
        }

        CharacterAnimationStateInput resolvedInput =
            input;

        resolvedInput.locomotionState =
            resolvedLocomotionState;

        runtime.viewMode =
            input.viewMode;

        runtime.stance =
            input.stance;

        runtime.movementDirection =
            input.movementDirection;

        runtime.locomotionState =
            resolvedLocomotionState;

        runtime.upperBodyState =
            input.upperBodyState;

        runtime.movementSpeed =
            (std::max)(
                input.movementSpeed,
                0.0F);

        runtime.grounded =
            input.grounded;

        runtime.aiming =
            input.aiming;

        if (!component.enabled)
        {
            /*
             * Presentation state (особенно FPS/TPS) остаётся
             * актуальным даже при выключенной анимации.
             */
            runtime.lowerBody.Reset();
            runtime.upperBody.Reset();
            runtime.action.Reset();
            runtime.actionState =
                CharacterActionState::None;
            runtime.firstUpdate = true;
            return;
        }

        /*
         * Сначала продвигаем существующие клипы.
         */
        AdvanceLayer(
            runtime.lowerBody,
            animationDeltaSeconds,
            input.lowerClipDurationSeconds);

        AdvanceLayer(
            runtime.upperBody,
            animationDeltaSeconds,
            0.0);

        AdvanceLayer(
            runtime.action,
            animationDeltaSeconds,
            input.actionClipDurationSeconds);

        /*
         * Нижняя часть тела.
         */
        const std::wstring*
            desiredLowerBodyClip =
                ResolveLowerBodyClip(
                    animationSet,
                    resolvedInput);

        SetLayerClip(
            runtime.lowerBody,
            desiredLowerBodyClip,

            firstUpdate
                ? 0.0F
                : animationSet.tuning.
                    locomotionBlendSeconds,

            ResolveLowerBodyLoopMode(
                resolvedLocomotionState),

            false);

        /*
         * Верхняя часть тела.
         */
        const std::wstring*
            desiredUpperBodyClip =
                ResolveUpperBodyClip(
                    animationSet,
                    resolvedInput);

        SetLayerClip(
            runtime.upperBody,
            desiredUpperBodyClip,

            firstUpdate
                ? 0.0F
                : animationSet.tuning.
                    upperBodyBlendSeconds,

            CharacterAnimationLoopMode::Loop,
            false);

        /*
         * Запуск нового one-shot действия.
         *
         * actionRequest должен подаваться только
         * в кадр нажатия кнопки.
         */
        if (
            input.actionRequest !=
                CharacterActionState::None)
        {
            const std::wstring*
                desiredActionClip =
                    ResolveActionClip(
                        animationSet,
                        resolvedInput);

            if (
                desiredActionClip != nullptr &&
                !desiredActionClip->empty())
            {
                runtime.actionState =
                    input.actionRequest;

                SetLayerClip(
                    runtime.action,
                    desiredActionClip,

                    firstUpdate
                        ? 0.0F
                        : animationSet.tuning.
                            actionBlendInSeconds,

                    CharacterAnimationLoopMode::Once,

                    input.restartAction);
            }
        }

        /*
         * One-shot завершился.
         *
         * Переводим action-слой в плавный fade-out.
         */
        if (
            runtime.action.completed &&
            runtime.actionState !=
                CharacterActionState::None)
        {
            runtime.actionState =
                CharacterActionState::None;

            SetLayerClip(
                runtime.action,
                nullptr,

                animationSet.tuning.
                    actionBlendOutSeconds,

                CharacterAnimationLoopMode::Once,

                false);
        }

        runtime.firstUpdate = false;
    }

    void CharacterAnimationStateMachine::StopAction(
        CharacterAnimationComponent& component,
        const float blendOutSeconds) const noexcept
    {
        component.runtime.actionState =
            CharacterActionState::None;

        SetLayerClip(
            component.runtime.action,
            nullptr,
            blendOutSeconds,
            CharacterAnimationLoopMode::Once,
            false);
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

        if (
            !layer.previousClip.empty() &&
            layer.transitionDurationSeconds > 0.0F)
        {
            layer.transitionElapsedSeconds +=
                static_cast<float>(
                    safeDeltaSeconds);

            if (
                layer.transitionElapsedSeconds >=
                    layer.transitionDurationSeconds)
            {
                layer.previousClip.clear();

                layer.previousTimeSeconds =
                    0.0;

                layer.previousLoopMode =
                    CharacterAnimationLoopMode::Loop;

                layer.transitionElapsedSeconds =
                    layer.transitionDurationSeconds;
            }
        }
        else
        {
            layer.previousClip.clear();

            layer.previousTimeSeconds =
                0.0;

            layer.previousLoopMode =
                CharacterAnimationLoopMode::Loop;

            layer.transitionElapsedSeconds =
                layer.transitionDurationSeconds;
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

            layer.currentTimeSeconds = 0.0;
            layer.previousTimeSeconds = 0.0;

            layer.transitionDurationSeconds =
                0.0F;

            layer.transitionElapsedSeconds =
                0.0F;

            layer.completed = false;
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
            !restart &&
            layer.currentClip == desiredClip &&
            layer.loopMode == loopMode)
        {
            return;
        }

        std::wstring transitionSourceClip;
        double transitionSourceTime = 0.0;

        CharacterAnimationLoopMode
            transitionSourceLoopMode =
                CharacterAnimationLoopMode::Loop;

        if (!layer.currentClip.empty())
        {
            transitionSourceClip =
                layer.currentClip;

            transitionSourceTime =
                layer.currentTimeSeconds;

            transitionSourceLoopMode =
                layer.loopMode;
        }
        else if (!layer.previousClip.empty())
        {
            transitionSourceClip =
                layer.previousClip;

            transitionSourceTime =
                layer.previousTimeSeconds;

            transitionSourceLoopMode =
                layer.previousLoopMode;
        }

        /*
         * WarZ сохранял fCurFrame при смене
         * lower/upper locomotion-анимации.
         *
         * У нас все старые player clips работают
         * с одинаковой частотой кадров, поэтому
         * сохраняем elapsed time.
         */
        const bool preserveLoopPosition =
            !restart &&
            !transitionSourceClip.empty() &&
            !desiredClip.empty() &&
            transitionSourceLoopMode ==
                CharacterAnimationLoopMode::Loop &&
            loopMode ==
                CharacterAnimationLoopMode::Loop;

        layer.previousClip =
            std::move(
                transitionSourceClip);

        layer.previousTimeSeconds =
            transitionSourceTime;

        layer.previousLoopMode =
            transitionSourceLoopMode;

        layer.currentClip =
            desiredClip;

        layer.currentTimeSeconds =
            preserveLoopPosition
                ? transitionSourceTime
                : 0.0;

        layer.transitionDurationSeconds =
            (std::max)(
                transitionSeconds,
                0.0F);

        layer.transitionElapsedSeconds =
            0.0F;

        layer.loopMode =
            loopMode;

        layer.completed =
            false;

        if (
            layer.transitionDurationSeconds <=
                0.0F)
        {
            layer.previousClip.clear();

            layer.previousTimeSeconds =
                0.0;

            layer.previousLoopMode =
                CharacterAnimationLoopMode::Loop;

            layer.transitionDurationSeconds =
                0.0F;

            layer.transitionElapsedSeconds =
                0.0F;
        }

        layer.active =
            !layer.currentClip.empty() ||
            !layer.previousClip.empty();

        layer.weight =
            std::clamp(
                layer.weight,
                0.0F,
                1.0F);

        if (
            layer.active &&
            layer.weight <= 0.0F)
        {
            layer.weight = 1.0F;
        }
    }

    const std::wstring*
        CharacterAnimationStateMachine::
            ResolveLowerBodyClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept
    {
        const CharacterLowerBodyAnimationSet&
            lowerBody =
                animationSet.lowerBody;

        const bool crouched =
            IsCrouched(input);

        switch (input.locomotionState)
        {
            case CharacterLocomotionState::Walk:
            {
                if (crouched)
                {
                    const std::wstring*
                        crouchedMove =
                            ResolveDirectionalClip(
                                lowerBody.crouchedMove,
                                input.
                                    movementDirection);

                    return FirstNonEmpty(
                    {
                        crouchedMove,
                        &lowerBody.crouchedIdle,
                        &lowerBody.standingIdle
                    });
                }

                const std::wstring* walk =
                    ResolveDirectionalClip(
                        lowerBody.walk,
                        input.movementDirection);

                return FirstNonEmpty(
                {
                    walk,
                    &lowerBody.standingIdle
                });
            }

            case CharacterLocomotionState::Run:
            {
                if (crouched)
                {
                    const std::wstring*
                        crouchedMove =
                            ResolveDirectionalClip(
                                lowerBody.crouchedMove,
                                input.
                                    movementDirection);

                    return FirstNonEmpty(
                    {
                        crouchedMove,
                        &lowerBody.crouchedIdle,
                        &lowerBody.standingIdle
                    });
                }

                const std::wstring* run =
                    ResolveDirectionalClip(
                        lowerBody.run,
                        input.movementDirection);

                const std::wstring* walk =
                    ResolveDirectionalClip(
                        lowerBody.walk,
                        input.movementDirection);

                return FirstNonEmpty(
                {
                    run,
                    walk,
                    &lowerBody.standingIdle
                });
            }

        case CharacterLocomotionState::Sprint:
                {
                    if (crouched)
                    {
                        const std::wstring*
                            crouchedMove =
                                ResolveDirectionalClip(
                                    lowerBody.crouchedMove,
                                    input.movementDirection);

                        return FirstNonEmpty(
                        {
                            crouchedMove,
                            &lowerBody.crouchedIdle,
                            &lowerBody.standingIdle
                        });
                    }

                    const std::wstring* sprint =
                        ResolveSprintClip(
                            lowerBody.sprint,
                            input.movementDirection);

                    const std::wstring* run =
                        ResolveDirectionalClip(
                            lowerBody.run,
                            input.movementDirection);

                    const std::wstring* walk =
                        ResolveDirectionalClip(
                            lowerBody.walk,
                            input.movementDirection);

                    return FirstNonEmpty(
                    {
                        sprint,
                        run,
                        walk,
                        &lowerBody.standingIdle
                    });
                }

            case CharacterLocomotionState::
                JumpStart:
                return FirstNonEmpty(
                {
                    &lowerBody.jumpStart,
                    &lowerBody.jumpLoop,
                    &lowerBody.standingIdle
                });

            case CharacterLocomotionState::
                JumpLoop:
                return FirstNonEmpty(
                {
                    &lowerBody.jumpLoop,
                    &lowerBody.jumpStart,
                    &lowerBody.standingIdle
                });

            case CharacterLocomotionState::
                JumpLand:
                return FirstNonEmpty(
                {
                    &lowerBody.jumpLand,
                    &lowerBody.standingIdle
                });

            case CharacterLocomotionState::
                TurnInPlaceLeft:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &lowerBody.
                            crouchedTurnInPlaceLeft,

                        &lowerBody.turnInPlaceLeft,
                        &lowerBody.crouchedIdle,
                        &lowerBody.standingIdle
                    });
                }

                return FirstNonEmpty(
                {
                    &lowerBody.turnInPlaceLeft,
                    &lowerBody.standingIdle
                });
            }

            case CharacterLocomotionState::
                TurnInPlaceRight:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &lowerBody.
                            crouchedTurnInPlaceRight,

                        &lowerBody.turnInPlaceRight,
                        &lowerBody.crouchedIdle,
                        &lowerBody.standingIdle
                    });
                }

                return FirstNonEmpty(
                {
                    &lowerBody.turnInPlaceRight,
                    &lowerBody.standingIdle
                });
            }

            case CharacterLocomotionState::Idle:

            default:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &lowerBody.crouchedIdle,
                        &lowerBody.standingIdle
                    });
                }

                return FirstNonEmpty(
                {
                    &lowerBody.standingIdle
                });
            }
        }
    }

    const std::wstring*
        CharacterAnimationStateMachine::
            ResolveUpperBodyClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept
    {
        const CharacterViewAnimationSet&
            viewSet =
                ResolveViewSet(
                    animationSet,
                    input.viewMode);

        const CharacterUpperBodyAnimationSet&
            upperBody =
                viewSet.upperBody;

        const bool crouched =
            IsCrouched(input);

        const bool moving =
            IsCharacterMoving(input);

        /*
         * Aim upper-body нужен только BodyFPS.
         *
         * В TPS RMB продолжает управлять:
         * - Walk Aim locomotion;
         * - gameplay aiming;
         *
         * Но TPS upper-body остаётся Relaxed.
         */
        const bool aiming =
            input.viewMode ==
                CharacterViewMode::FirstPerson &&
            IsAiming(input);

        if (crouched)
        {
            if (aiming)
            {
                if (moving)
                {
                    return FirstNonEmpty(
                    {
                        &upperBody.crouchedAimMove,
                        &upperBody.crouchedAimIdle,

                        &upperBody.
                            crouchedRelaxedMove,

                        &upperBody.
                            crouchedRelaxedIdle
                    });
                }

                return FirstNonEmpty(
                {
                    &upperBody.crouchedAimIdle,

                    &upperBody.
                        crouchedRelaxedIdle
                });
            }

            if (moving)
            {
                return FirstNonEmpty(
                {
                    &upperBody.crouchedRelaxedMove,
                    &upperBody.crouchedRelaxedIdle
                });
            }

            return FirstNonEmpty(
            {
                &upperBody.crouchedRelaxedIdle
            });
        }

        if (aiming)
        {
            if (moving)
            {
                return FirstNonEmpty(
                {
                    &upperBody.standingAimMove,
                    &upperBody.standingAimIdle,

                    &upperBody.
                        standingRelaxedMove,

                    &upperBody.
                        standingRelaxedIdle
                });
            }

            return FirstNonEmpty(
            {
                &upperBody.standingAimIdle,
                &upperBody.standingRelaxedIdle
            });
        }

        if (moving)
        {
            return FirstNonEmpty(
            {
                &upperBody.standingRelaxedMove,
                &upperBody.standingRelaxedIdle
            });
        }

        return FirstNonEmpty(
        {
            &upperBody.standingRelaxedIdle
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
        const CharacterViewAnimationSet&
            viewSet =
                ResolveViewSet(
                    animationSet,
                    input.viewMode);

        const CharacterActionAnimationSet&
            actions =
                viewSet.actions;

        const bool crouched =
            IsCrouched(input);

        const bool moving =
            IsCharacterMoving(input);

        switch (input.actionRequest)
        {
            case CharacterActionState::Primary:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &actions.primaryCrouched,
                        &actions.primaryStanding
                    });
                }

                if (moving)
                {
                    return FirstNonEmpty(
                    {
                        &actions.primaryMoving,
                        &actions.primaryStanding
                    });
                }

                return FirstNonEmpty(
                {
                    &actions.primaryStanding
                });
            }

            case CharacterActionState::Secondary:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &actions.secondaryCrouched,
                        &actions.secondaryStanding
                    });
                }

                if (moving)
                {
                    return FirstNonEmpty(
                    {
                        &actions.secondaryMoving,
                        &actions.secondaryStanding
                    });
                }

                return FirstNonEmpty(
                {
                    &actions.secondaryStanding
                });
            }

            case CharacterActionState::Reload:
            {
                if (crouched)
                {
                    return FirstNonEmpty(
                    {
                        &actions.reloadCrouched,
                        &actions.reloadStanding
                    });
                }

                if (moving)
                {
                    return FirstNonEmpty(
                    {
                        &actions.reloadMoving,
                        &actions.reloadStanding
                    });
                }

                return FirstNonEmpty(
                {
                    &actions.reloadStanding
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
                    locomotionState) noexcept
    {
        switch (locomotionState)
        {
            case CharacterLocomotionState::
                JumpStart:

            case CharacterLocomotionState::
                JumpLand:

            case CharacterLocomotionState::
                TurnInPlaceLeft:

            case CharacterLocomotionState::
                TurnInPlaceRight:
                return
                    CharacterAnimationLoopMode::Once;

            case CharacterLocomotionState::Idle:
            case CharacterLocomotionState::Walk:
            case CharacterLocomotionState::Run:
            case CharacterLocomotionState::Sprint:
            case CharacterLocomotionState::JumpLoop:

            default:
                return
                    CharacterAnimationLoopMode::Loop;
        }
    }
}
