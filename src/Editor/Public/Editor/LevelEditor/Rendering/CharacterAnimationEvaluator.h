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
    /*
     * Уже загруженные ресурсы одного runtime-слоя.
     *
     * Evaluator не занимается файловой системой
     * и не загружает .anim самостоятельно.
     */
    struct CharacterAnimationLayerSample final
    {
        const engine::assets::AnimationAsset*
            currentAnimation = nullptr;

        const engine::assets::AnimationAsset*
            previousAnimation = nullptr;

        double currentTimeSeconds = 0.0;
        double previousTimeSeconds = 0.0;

        /*
         * 0 = полностью previousAnimation.
         * 1 = полностью currentAnimation.
         */
        float transitionAlpha = 1.0F;

        /*
         * Итоговый вес слоя.
         */
        float weight = 1.0F;

        engine::scene::CharacterAnimationLoopMode
            loopMode =
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
        CharacterAnimationLayerSample upperBody;
        CharacterAnimationLayerSample action;

        /*
         * Начало маски верхней части тела.
         */
        std::string upperBodyRootBone =
            "Bip01_Spine";

        /*
         * Начало маски action-слоя.
         */
        std::string actionRootBone =
            "Bip01_Spine";

        /*
         * Кость, которая процедурно следует
         * за направлением камеры.
         */
        std::string lookRootBone =
            "Bip01_Neck";

        /*
         * Горизонтальный поворот Look относительно
         * текущего направления ног.
         */
        float lookYawOffsetDegrees = 0.0F;

        /*
         * Пока CharacterController сам перемещает
         * персонажа, горизонтальный root motion
         * из клипа блокируется.
         */
        bool blockHorizontalRootMotion = true;
    };

    struct CharacterAnimationPose final
    {
        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            boneMatrices{};

        std::uint32_t boneCount = 0U;

        /*
         * true, если хотя бы один animation track
         * участвовал в построении позы.
         */
        bool animated = false;

        void Reset() noexcept;
    };

    /*
     * Собирает итоговую позу:
     *
     * Lower Body
     *     ↓
     * Upper Body по bone mask
     *     ↓
     * Action по bone mask
     *     ↓
     * Absolute pose
     *     ↓
     * Skinning matrices
     */
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