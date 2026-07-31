#pragma once

#include <cstdint>
#include <string>

namespace engine::scene
{
    /*
     * Режим отображения управляемого персонажа.
     */
    enum class CharacterViewMode : std::uint8_t
    {
        ThirdPerson = 0,
        FirstPerson
    };

    /*
     * Физическая стойка персонажа.
     *
     * В дальнейшем сюда можно добавить Prone,
     * но сейчас начинаем только со Stand/Crouch.
     */
    enum class CharacterStance : std::uint8_t
    {
        Standing = 0,
        Crouched
    };

    /*
     * Направление движения относительно
     * игрового направления персонажа.
     */
    enum class CharacterMovementDirection :
        std::uint8_t
    {
        None = 0,

        Forward,
        ForwardLeft,
        ForwardRight,

        Left,
        Right,

        Backward,
        BackwardLeft,
        BackwardRight
    };

    /*
     * Состояние нижнего locomotion-слоя.
     */
    enum class CharacterLocomotionState :
        std::uint8_t
    {
        Idle = 0,

        Walk,
        Run,

        JumpStart,
        JumpLoop,
        JumpLand,

        TurnInPlaceLeft,
        TurnInPlaceRight
    };

    /*
     * Состояние верхней части тела.
     *
     * Relaxed:
     * обычное положение рук.
     *
     * Aiming:
     * удерживается ПКМ.
     */
    enum class CharacterUpperBodyState :
        std::uint8_t
    {
        Relaxed = 0,
        Aiming
    };

    /*
     * One-shot действие action-слоя.
     */
    enum class CharacterActionState :
        std::uint8_t
    {
        None = 0,

        Primary,
        Secondary,
        Reload
    };

    enum class CharacterAnimationLoopMode :
        std::uint8_t
    {
        Loop = 0,
        Once
    };

    /*
     * Набор направленных клипов.
     *
     * Здесь нет имён WarZ-анимаций.
     * Все пути назначаются данными.
     */
    struct CharacterDirectionalAnimationSet final
    {
        std::wstring forward;
        std::wstring forwardLeft;
        std::wstring forwardRight;

        std::wstring left;
        std::wstring right;

        std::wstring backward;
        std::wstring backwardLeft;
        std::wstring backwardRight;

        [[nodiscard]]
        const std::wstring& Resolve(
            const CharacterMovementDirection
                direction) const noexcept
        {
            switch (direction)
            {
                case CharacterMovementDirection::
                    Forward:
                    return forward;

                case CharacterMovementDirection::
                    ForwardLeft:
                    return forwardLeft;

                case CharacterMovementDirection::
                    ForwardRight:
                    return forwardRight;

                case CharacterMovementDirection::
                    Left:
                    return left;

                case CharacterMovementDirection::
                    Right:
                    return right;

                case CharacterMovementDirection::
                    Backward:
                    return backward;

                case CharacterMovementDirection::
                    BackwardLeft:
                    return backwardLeft;

                case CharacterMovementDirection::
                    BackwardRight:
                    return backwardRight;

                case CharacterMovementDirection::None:
                default:
                {
                    static const std::wstring empty;

                    return empty;
                }
            }
        }
    };

    /*
     * Все клипы нижней части тела.
     *
     * Этот слой будет управлять:
     *
     * Bip01
     * Pelvis
     * ногами
     *
     * Кости верхней части будут заменяться
     * результатом upper-body слоя.
     */
    struct CharacterLowerBodyAnimationSet final
    {
        std::wstring standingIdle;

        CharacterDirectionalAnimationSet walk;
        CharacterDirectionalAnimationSet run;

        std::wstring crouchedIdle;

        CharacterDirectionalAnimationSet
            crouchedMove;

        std::wstring turnInPlaceLeft;
        std::wstring turnInPlaceRight;

        std::wstring crouchedTurnInPlaceLeft;
        std::wstring crouchedTurnInPlaceRight;

        std::wstring jumpStart;
        std::wstring jumpLoop;
        std::wstring jumpLand;
    };

    /*
     * Верхний слой одного режима камеры.
     *
     * TPS и FPS используют одинаковую механику,
     * но разные наборы ресурсов.
     */
    struct CharacterUpperBodyAnimationSet final
    {
        std::wstring standingRelaxedIdle;
        std::wstring standingRelaxedMove;

        std::wstring standingAimIdle;
        std::wstring standingAimMove;

        std::wstring crouchedRelaxedIdle;
        std::wstring crouchedRelaxedMove;

        std::wstring crouchedAimIdle;
        std::wstring crouchedAimMove;
    };

    /*
     * One-shot действия одного режима камеры.
     *
     * Primary:
     * ЛКМ — удар или выстрел.
     *
     * Secondary:
     * дополнительное действие оружия.
     *
     * Aim не находится здесь, потому что Aim —
     * удерживаемое состояние upper-body слоя.
     */
    struct CharacterActionAnimationSet final
    {
        std::wstring primaryStanding;
        std::wstring primaryMoving;
        std::wstring primaryCrouched;

        std::wstring secondaryStanding;
        std::wstring secondaryMoving;
        std::wstring secondaryCrouched;

        std::wstring reloadStanding;
        std::wstring reloadMoving;
        std::wstring reloadCrouched;
    };

    struct CharacterViewAnimationSet final
    {
        CharacterUpperBodyAnimationSet upperBody;
        CharacterActionAnimationSet actions;
    };

    /*
     * Настройки нашей новой animation state machine.
     *
     * Они не копируют старые WarZ-константы.
     */
    struct CharacterAnimationTuning final
    {
        float locomotionBlendSeconds = 0.15F;
        float upperBodyBlendSeconds = 0.10F;

        float actionBlendInSeconds = 0.05F;
        float actionBlendOutSeconds = 0.10F;

        /*
         * Максимальное отклонение туловища
         * относительно ног.
         */
        float maximumUpperBodyYawDegrees = 65.0F;

        /*
         * После превышения этого угла запускается
         * Turn In Place / Rot Legs.
         */
        float turnInPlaceEnterDegrees = 65.0F;

        /*
         * При достижении этого остаточного угла
         * поворот ног считается завершённым.
         */
        float turnInPlaceExitDegrees = 8.0F;

        float turnInPlaceSpeedDegrees = 240.0F;
    };

    /*
     * Полный набор анимаций одного типа персонажа.
     *
     * Пути здесь изначально пустые.
     * Их позже будет заполнять наш Inspector
     * или отдельный animation-set asset.
     */
    struct CharacterAnimationSet final
    {
        /*
         * Первая кость upper-body mask.
         *
         * Это имя кости, а не путь к анимации.
         */
        std::string upperBodyRootBone =
            "Bip01_Spine";

        /*
         * Action-слой обычно использует ту же
         * границу, но оставляем настройку отдельной.
         */
        std::string actionRootBone =
            "Bip01_Spine";

        CharacterLowerBodyAnimationSet lowerBody;

        CharacterViewAnimationSet thirdPerson;
        CharacterViewAnimationSet firstPerson;

        CharacterAnimationTuning tuning;
    };

    /*
     * Runtime одного слоя.
     *
     * currentClip:
     * активный клип.
     *
     * previousClip:
     * предыдущий клип во время cross-fade.
     */
    struct CharacterAnimationLayerRuntime final
    {
        std::wstring currentClip;
        std::wstring previousClip;

        double currentTimeSeconds = 0.0;
        double previousTimeSeconds = 0.0;

        float transitionDurationSeconds = 0.0F;
        float transitionElapsedSeconds = 0.0F;

        float weight = 1.0F;

        CharacterAnimationLoopMode loopMode =
            CharacterAnimationLoopMode::Loop;

        bool active = false;
        bool completed = false;

        void Reset() noexcept
        {
            currentClip.clear();
            previousClip.clear();

            currentTimeSeconds = 0.0;
            previousTimeSeconds = 0.0;

            transitionDurationSeconds = 0.0F;
            transitionElapsedSeconds = 0.0F;

            weight = 1.0F;

            loopMode =
                CharacterAnimationLoopMode::Loop;

            active = false;
            completed = false;
        }

        [[nodiscard]]
        bool IsTransitioning() const noexcept
        {
            return
                !previousClip.empty() &&
                transitionDurationSeconds > 0.0F &&
                transitionElapsedSeconds <
                    transitionDurationSeconds;
        }
    };

    /*
     * Полностью transient runtime.
     *
     * Эти данные не должны записываться в .level:
     * таймеры, текущие клипы и поворот ног
     * пересоздаются при запуске игры.
     */
    struct CharacterAnimationRuntime final
    {
        CharacterAnimationLayerRuntime lowerBody;
        CharacterAnimationLayerRuntime upperBody;
        CharacterAnimationLayerRuntime action;

        CharacterViewMode viewMode =
            CharacterViewMode::ThirdPerson;

        CharacterStance stance =
            CharacterStance::Standing;

        CharacterMovementDirection
            movementDirection =
                CharacterMovementDirection::None;

        CharacterLocomotionState
            locomotionState =
                CharacterLocomotionState::Idle;

        CharacterUpperBodyState upperBodyState =
            CharacterUpperBodyState::Relaxed;

        CharacterActionState actionState =
            CharacterActionState::None;

        float movementSpeed = 0.0F;

        /*
         * Направление всей сущности в игровом мире.
         */
        float actorYawDegrees = 0.0F;

        /*
         * Текущий визуальный угол таза и ног.
         */
        float lowerBodyYawDegrees = 0.0F;

        /*
         * Отклонение Spine относительно ног.
         */
        float upperBodyYawOffsetDegrees = 0.0F;

        /*
         * Целевой угол для Rot Legs.
         */
        float turnTargetYawDegrees = 0.0F;

        bool grounded = true;
        bool aiming = false;

        bool turnInPlaceActive = false;
        bool firstUpdate = true;

        void Reset() noexcept
        {
            lowerBody.Reset();
            upperBody.Reset();
            action.Reset();

            viewMode =
                CharacterViewMode::ThirdPerson;

            stance =
                CharacterStance::Standing;

            movementDirection =
                CharacterMovementDirection::None;

            locomotionState =
                CharacterLocomotionState::Idle;

            upperBodyState =
                CharacterUpperBodyState::Relaxed;

            actionState =
                CharacterActionState::None;

            movementSpeed = 0.0F;

            actorYawDegrees = 0.0F;
            lowerBodyYawDegrees = 0.0F;
            upperBodyYawOffsetDegrees = 0.0F;
            turnTargetYawDegrees = 0.0F;

            grounded = true;
            aiming = false;

            turnInPlaceActive = false;
            firstUpdate = true;
        }
    };

    /*
     * Отдельный компонент анимации персонажа.
     *
     * SkeletalMeshComponent отвечает за геометрию.
     * CharacterAnimationComponent отвечает за позу.
     */
    struct CharacterAnimationComponent final
    {
        CharacterAnimationSet animationSet;
        CharacterAnimationRuntime runtime;

        bool enabled = true;
    };
}