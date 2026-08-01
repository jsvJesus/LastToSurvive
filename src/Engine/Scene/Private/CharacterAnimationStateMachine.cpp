#include "Scene/CharacterAnimationStateMachine.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace engine::scene
{
    namespace
    {
        constexpr double MaximumAnimationDeltaSeconds =
            0.10;

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

        if (!component.enabled)
        {
            runtime.Reset();
            return;
        }

        double deltaSeconds =
            std::isfinite(input.deltaSeconds)
                ? input.deltaSeconds
                : 0.0;

        deltaSeconds =
            std::clamp(
                deltaSeconds,
                0.0,
                MaximumAnimationDeltaSeconds);

        /*
         * Сначала продвигаем существующие клипы.
         */
        AdvanceLayer(
            runtime.lowerBody,
            deltaSeconds,
            input.lowerClipDurationSeconds);

        AdvanceLayer(
            runtime.upperBody,
            deltaSeconds,
            0.0);

        AdvanceLayer(
            runtime.action,
            deltaSeconds,
            input.actionClipDurationSeconds);

        runtime.viewMode =
            input.viewMode;

        runtime.stance =
            input.stance;

        runtime.movementDirection =
            input.movementDirection;

        runtime.locomotionState =
            input.locomotionState;

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

        const CharacterAnimationSet&
            animationSet =
                component.animationSet;

        const bool firstUpdate =
            runtime.firstUpdate;

        /*
         * Нижняя часть тела.
         */
        const std::wstring*
            desiredLowerBodyClip =
                ResolveLowerBodyClip(
                    animationSet,
                    input);

        SetLayerClip(
            runtime.lowerBody,
            desiredLowerBodyClip,

            firstUpdate
                ? 0.0F
                : animationSet.tuning.
                    locomotionBlendSeconds,

            ResolveLowerBodyLoopMode(
                input.locomotionState),

            false);

        /*
         * Верхняя часть тела.
         */
        const std::wstring*
            desiredUpperBodyClip =
                ResolveUpperBodyClip(
                    animationSet,
                    input);

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
                        input);

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
            std::clamp(
                std::isfinite(deltaSeconds)
                    ? deltaSeconds
                    : 0.0,
                0.0,
                MaximumAnimationDeltaSeconds);

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
                layer.previousTimeSeconds = 0.0;

                layer.transitionElapsedSeconds =
                    layer.transitionDurationSeconds;
            }
        }
        else
        {
            layer.previousClip.clear();
            layer.previousTimeSeconds = 0.0;

            layer.transitionElapsedSeconds =
                layer.transitionDurationSeconds;
        }

        if (
            layer.loopMode ==
                CharacterAnimationLoopMode::Once &&
            !layer.currentClip.empty() &&
            std::isfinite(clipDurationSeconds) &&
            clipDurationSeconds > 0.0)
        {
            if (
                layer.currentTimeSeconds >=
                    clipDurationSeconds)
            {
                layer.currentTimeSeconds =
                    clipDurationSeconds;

                layer.completed = true;
            }
        }

        if (
            layer.currentClip.empty() &&
            layer.previousClip.empty())
        {
            layer.active = false;

            layer.currentTimeSeconds = 0.0;
            layer.previousTimeSeconds = 0.0;

            layer.transitionDurationSeconds = 0.0F;
            layer.transitionElapsedSeconds = 0.0F;

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

        /*
         * Клип не изменился — продолжаем текущее
         * воспроизведение без сброса времени.
         */
        if (
            !restart &&
            layer.currentClip == desiredClip)
        {
            layer.loopMode = loopMode;
            return;
        }

        std::wstring transitionSourceClip;
        double transitionSourceTime = 0.0;

        /*
         * Обычно источником transition является
         * currentClip.
         *
         * Если слой уже выполняет fade-out,
         * currentClip пустой, но previousClip ещё
         * содержит видимый клип.
         */
        if (!layer.currentClip.empty())
        {
            transitionSourceClip =
                layer.currentClip;

            transitionSourceTime =
                layer.currentTimeSeconds;
        }
        else if (!layer.previousClip.empty())
        {
            transitionSourceClip =
                layer.previousClip;

            transitionSourceTime =
                layer.previousTimeSeconds;
        }

        layer.previousClip =
            std::move(
                transitionSourceClip);

        layer.previousTimeSeconds =
            transitionSourceTime;

        layer.currentClip =
            desiredClip;

        layer.currentTimeSeconds = 0.0;

        layer.transitionDurationSeconds =
            (std::max)(
                transitionSeconds,
                0.0F);

        layer.transitionElapsedSeconds = 0.0F;

        layer.loopMode = loopMode;
        layer.completed = false;

        if (
            layer.transitionDurationSeconds <=
                0.0F)
        {
            layer.previousClip.clear();
            layer.previousTimeSeconds = 0.0;

            layer.transitionDurationSeconds = 0.0F;
            layer.transitionElapsedSeconds = 0.0F;
        }

        layer.active =
            !layer.currentClip.empty() ||
            !layer.previousClip.empty();

        layer.weight =
            std::clamp(
                layer.weight,
                0.0F,
                1.0F);

        /*
         * Новый активный слой по умолчанию имеет
         * полный вес.
         */
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

        const bool aiming =
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