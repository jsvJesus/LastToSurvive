#pragma once

#include "Scene/CharacterAnimationTypes.h"

namespace engine::scene
{
    /*
     * Полный набор данных, поступающих в
     * animation state machine за один кадр.
     *
     * Здесь нет Win32 input, файловой системы,
     * renderer или загрузки ресурсов.
     */
    struct CharacterAnimationStateInput final
    {
        double deltaSeconds = 0.0;

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

        CharacterUpperBodyState
            upperBodyState =
                CharacterUpperBodyState::Relaxed;

        /*
         * None:
         * не запускать новое действие.
         *
         * Primary / Secondary / Reload:
         * запустить соответствующий one-shot.
         */
        CharacterActionState actionRequest =
            CharacterActionState::None;

        float movementSpeed = 0.0F;

        /*
         * Camera/gameplay yaw is the direction requested
         * for the character. The state machine keeps the
         * actor/legs yaw separate and owns Turn In Place.
         */
        float desiredActorYawDegrees = 0.0F;

        /*
         * Camera pitch is consumed by the procedural
         * Spine1/Neck evaluator as rotation only.
         */
        float lookPitchOffsetDegrees = 0.0F;

        /*
         * Rotation speed while the character is moving.
         * Idle Turn In Place uses tuning.turnInPlaceSpeedDegrees.
         */
        float movementRotationSpeedDegrees = 360.0F;

        bool grounded = true;
        bool aiming = false;

        /*
         * Повторно запускает action-клип,
         * даже если он уже является текущим.
         *
         * Используется при каждом новом клике ЛКМ.
         */
        bool restartAction = false;

        /*
         * Длительность one-shot action.
         *
         * Значение 0 означает, что длительность
         * пока неизвестна. Позже её будет передавать
         * AnimationAsset runtime.
         */
        double actionClipDurationSeconds = 0.0;

        /*
         * Для JumpStart и JumpLand.
         */
        double lowerClipDurationSeconds = 0.0;
        double turnClipDurationSeconds = 0.0;
    };

    /*
     * Выбирает пути из CharacterAnimationSet
     * и обновляет CharacterAnimationRuntime.
     *
     * State machine ничего не знает о содержимом
     * файлов .anim. Сэмплирование выполняет evaluator.
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
            CharacterAnimationComponent&
                component) const noexcept;

        void Update(
            const CharacterAnimationStateInput&
                input,

            CharacterAnimationComponent&
                component) const noexcept;

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
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveTurnInPlaceClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input,

                std::int32_t turnDirection) noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveUpperBodyClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept;

        [[nodiscard]]
        static const std::wstring*
            ResolveActionClip(
                const CharacterAnimationSet&
                    animationSet,

                const CharacterAnimationStateInput&
                    input) noexcept;

        [[nodiscard]]
        static CharacterAnimationLoopMode
            ResolveLowerBodyLoopMode(
                CharacterLocomotionState
                    locomotionState) noexcept;
    };
}
