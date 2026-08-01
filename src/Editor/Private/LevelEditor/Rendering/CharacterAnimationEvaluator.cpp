#include "Editor/LevelEditor/Rendering/CharacterAnimationEvaluator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string_view>

namespace lts::editor
{
    namespace
    {
        constexpr float MinimumMatrixDeterminant =
            0.0000001F;

        constexpr float MinimumQuaternionLength =
            0.0000001F;

        [[nodiscard]]
        DirectX::XMMATRIX LoadMatrix(
            const std::array<float, 16U>&
                source) noexcept
        {
            DirectX::XMFLOAT4X4 stored;

            std::memcpy(
                &stored,
                source.data(),
                sizeof(stored));

            return DirectX::XMLoadFloat4x4(
                &stored);
        }

        [[nodiscard]]
        bool InvertMatrix(
            const DirectX::XMMATRIX& source,
            DirectX::XMMATRIX& inverse) noexcept
        {
            DirectX::XMVECTOR determinant;

            inverse =
                DirectX::XMMatrixInverse(
                    &determinant,
                    source);

            const float value =
                DirectX::XMVectorGetX(
                    determinant);

            return
                std::isfinite(value) &&
                std::fabs(value) >
                    MinimumMatrixDeterminant;
        }

        [[nodiscard]]
        DirectX::XMVECTOR NormalizeQuaternion(
            const DirectX::XMVECTOR&
                quaternion) noexcept
        {
            const float lengthSquared =
                DirectX::XMVectorGetX(
                    DirectX::XMVector4LengthSq(
                        quaternion));

            if (
                !std::isfinite(lengthSquared) ||
                lengthSquared <
                    MinimumQuaternionLength)
            {
                return DirectX::XMQuaternionIdentity();
            }

            return
                DirectX::XMQuaternionNormalize(
                    quaternion);
        }

        [[nodiscard]]
        DirectX::XMMATRIX BuildKeyMatrix(
            const engine::assets::AnimationKey&
                first,

            const engine::assets::AnimationKey&
                second,

            const float interpolation) noexcept
        {
            const DirectX::XMVECTOR firstRotation =
                NormalizeQuaternion(
                    DirectX::XMVectorSet(
                        first.rotation[0],
                        first.rotation[1],
                        first.rotation[2],
                        first.rotation[3]));

            const DirectX::XMVECTOR secondRotation =
                NormalizeQuaternion(
                    DirectX::XMVectorSet(
                        second.rotation[0],
                        second.rotation[1],
                        second.rotation[2],
                        second.rotation[3]));

            const DirectX::XMVECTOR rotation =
                DirectX::XMQuaternionSlerp(
                    firstRotation,
                    secondRotation,
                    std::clamp(
                        interpolation,
                        0.0F,
                        1.0F));

            const float translationX =
                first.translation[0] +
                (
                    second.translation[0] -
                    first.translation[0]
                ) *
                interpolation;

            const float translationY =
                first.translation[1] +
                (
                    second.translation[1] -
                    first.translation[1]
                ) *
                interpolation;

            const float translationZ =
                first.translation[2] +
                (
                    second.translation[2] -
                    first.translation[2]
                ) *
                interpolation;

            DirectX::XMFLOAT4X4 stored;

            DirectX::XMStoreFloat4x4(
                &stored,
                DirectX::XMMatrixRotationQuaternion(
                    rotation));

            stored._41 = translationX;
            stored._42 = translationY;
            stored._43 = translationZ;

            return DirectX::XMLoadFloat4x4(
                &stored);
        }

        [[nodiscard]]
        DirectX::XMMATRIX BlendMatrices(
            const DirectX::XMMATRIX& first,
            const DirectX::XMMATRIX& second,
            const float alphaValue) noexcept
        {
            const float alpha =
                std::clamp(
                    alphaValue,
                    0.0F,
                    1.0F);

            if (alpha <= 0.0F)
            {
                return first;
            }

            if (alpha >= 1.0F)
            {
                return second;
            }

            DirectX::XMVECTOR firstScale;
            DirectX::XMVECTOR firstRotation;
            DirectX::XMVECTOR firstTranslation;

            DirectX::XMVECTOR secondScale;
            DirectX::XMVECTOR secondRotation;
            DirectX::XMVECTOR secondTranslation;

            if (
                !DirectX::XMMatrixDecompose(
                    &firstScale,
                    &firstRotation,
                    &firstTranslation,
                    first) ||
                !DirectX::XMMatrixDecompose(
                    &secondScale,
                    &secondRotation,
                    &secondTranslation,
                    second))
            {
                return
                    alpha < 0.5F
                        ? first
                        : second;
            }

            firstRotation =
                NormalizeQuaternion(
                    firstRotation);

            secondRotation =
                NormalizeQuaternion(
                    secondRotation);

            const DirectX::XMVECTOR scale =
                DirectX::XMVectorLerp(
                    firstScale,
                    secondScale,
                    alpha);

            const DirectX::XMVECTOR rotation =
                DirectX::XMQuaternionSlerp(
                    firstRotation,
                    secondRotation,
                    alpha);

            const DirectX::XMVECTOR translation =
                DirectX::XMVectorLerp(
                    firstTranslation,
                    secondTranslation,
                    alpha);

            return
                DirectX::XMMatrixScalingFromVector(
                    scale) *
                DirectX::XMMatrixRotationQuaternion(
                    rotation) *
                DirectX::XMMatrixTranslationFromVector(
                    translation);
        }

        [[nodiscard]]
        bool EqualAsciiInsensitive(
            const std::string_view first,
            const std::string_view second) noexcept
        {
            if (first.size() != second.size())
            {
                return false;
            }

            for (
                std::size_t index = 0U;
                index < first.size();
                ++index)
            {
                const unsigned char firstCharacter =
                    static_cast<unsigned char>(
                        first[index]);

                const unsigned char secondCharacter =
                    static_cast<unsigned char>(
                        second[index]);

                if (
                    std::tolower(firstCharacter) !=
                    std::tolower(secondCharacter))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        std::int32_t FindBoneIndex(
            const engine::assets::SkeletonAsset&
                skeleton,

            const std::string_view boneName) noexcept
        {
            if (boneName.empty())
            {
                return -1;
            }

            /*
             * Сначала точное сравнение.
             */
            for (
                std::size_t boneIndex = 0U;
                boneIndex <
                    skeleton.GetBoneCount();
                ++boneIndex)
            {
                const engine::assets::SkeletonBone*
                    bone =
                        skeleton.GetBone(
                            boneIndex);

                if (
                    bone != nullptr &&
                    bone->name == boneName)
                {
                    return
                        static_cast<std::int32_t>(
                            boneIndex);
                }
            }

            /*
             * Затем нечувствительное к регистру.
             */
            for (
                std::size_t boneIndex = 0U;
                boneIndex <
                    skeleton.GetBoneCount();
                ++boneIndex)
            {
                const engine::assets::SkeletonBone*
                    bone =
                        skeleton.GetBone(
                            boneIndex);

                if (
                    bone != nullptr &&
                    EqualAsciiInsensitive(
                        bone->name,
                        boneName))
                {
                    return
                        static_cast<std::int32_t>(
                            boneIndex);
                }
            }

            return -1;
        }

        void BuildBoneMask(
            const engine::assets::SkeletonAsset&
                skeleton,

            const std::string_view rootBoneName,

            std::array<
                bool,
                engine::assets::MaximumSkeletonBones>&
                output) noexcept
        {
            output.fill(false);

            const std::int32_t rootBoneIndex =
                FindBoneIndex(
                    skeleton,
                    rootBoneName);

            if (rootBoneIndex < 0)
            {
                return;
            }

            const std::size_t boneCount =
                skeleton.GetBoneCount();

            for (
                std::size_t boneIndex = 0U;
                boneIndex < boneCount;
                ++boneIndex)
            {
                std::int32_t currentIndex =
                    static_cast<std::int32_t>(
                        boneIndex);

                for (
                    std::size_t guard = 0U;
                    guard < boneCount;
                    ++guard)
                {
                    if (currentIndex < 0)
                    {
                        break;
                    }

                    if (currentIndex == rootBoneIndex)
                    {
                        output[boneIndex] = true;
                        break;
                    }

                    const std::size_t current =
                        static_cast<std::size_t>(
                            currentIndex);

                    if (current >= boneCount)
                    {
                        break;
                    }

                    const engine::assets::SkeletonBone*
                        currentBone =
                            skeleton.GetBone(
                                current);

                    if (currentBone == nullptr)
                    {
                        break;
                    }

                    currentIndex =
                        currentBone->parentIndex;
                }
            }
        }

        [[nodiscard]]
        bool BuildBindLocalMatrix(
            const engine::assets::SkeletonAsset&
                skeleton,

            const std::size_t boneIndex,

            DirectX::XMMATRIX&
                output) noexcept
        {
            const engine::assets::SkeletonBone*
                bone =
                    skeleton.GetBone(
                        boneIndex);

            if (bone == nullptr)
            {
                return false;
            }

            const DirectX::XMMATRIX bindAbsolute =
                LoadMatrix(
                    bone->absoluteBindMatrix);

            output = bindAbsolute;

            if (bone->parentIndex < 0)
            {
                return true;
            }

            const std::size_t parentIndex =
                static_cast<std::size_t>(
                    bone->parentIndex);

            /*
             * Skeleton loader должен хранить родителей
             * раньше дочерних костей.
             */
            if (
                parentIndex >= boneIndex ||
                parentIndex >=
                    skeleton.GetBoneCount())
            {
                return false;
            }

            const engine::assets::SkeletonBone*
                parentBone =
                    skeleton.GetBone(
                        parentIndex);

            if (parentBone == nullptr)
            {
                return false;
            }

            DirectX::XMMATRIX inverseParentBind;

            if (!InvertMatrix(
                    LoadMatrix(
                        parentBone->
                            absoluteBindMatrix),
                    inverseParentBind))
            {
                return false;
            }

            output =
                bindAbsolute *
                inverseParentBind;

            return true;
        }

        [[nodiscard]]
        bool IsAnimationUsable(
            const engine::assets::AnimationAsset*
                animation,

            const engine::assets::SkeletonAsset&
                skeleton) noexcept
        {
            return
                animation != nullptr &&
                animation->IsValid() &&
                animation->IsCompatibleWith(
                    skeleton);
        }

        struct AnimationLoopSamplingInfo final
        {
            std::size_t frameCount = 0U;
            double sampleIntervalSeconds = 0.0;
            double durationSeconds = 0.0;
        };

        struct LayerLoopSamplingInfo final
        {
            AnimationLoopSamplingInfo current;
            AnimationLoopSamplingInfo previous;
        };

        [[nodiscard]]
AnimationLoopSamplingInfo
    BuildAnimationLoopSamplingInfo(
        const engine::assets::AnimationAsset*
            animation) noexcept
        {
            AnimationLoopSamplingInfo result;

            if (
                animation == nullptr ||
                !animation->IsValid())
            {
                return result;
            }

            const std::size_t declaredFrameCount =
                static_cast<std::size_t>(
                    animation->GetFrameCount());

            if (declaredFrameCount == 0U)
            {
                return result;
            }

            /*
             * WarZ loop semantics:
             *
             * Последний authored frame является копией
             * первого и не образует отдельный интервал.
             */
            result.frameCount =
                declaredFrameCount > 1U
                    ? declaredFrameCount - 1U
                    : 1U;

            const double frameRate =
                static_cast<double>(
                    animation->GetFrameRate());

            if (
                std::isfinite(frameRate) &&
                frameRate > 0.0)
            {
                result.sampleIntervalSeconds =
                    1.0 / frameRate;
            }
            else
            {
                const double authoredDurationSeconds =
                    static_cast<double>(
                        animation->GetDurationSeconds());

                if (
                    declaredFrameCount > 1U &&
                    std::isfinite(
                        authoredDurationSeconds) &&
                    authoredDurationSeconds > 0.0)
                {
                    result.sampleIntervalSeconds =
                        authoredDurationSeconds /
                        static_cast<double>(
                            declaredFrameCount - 1U);
                }
            }

            if (
                std::isfinite(
                    result.sampleIntervalSeconds) &&
                result.sampleIntervalSeconds > 0.0)
            {
                result.durationSeconds =
                    static_cast<double>(
                        result.frameCount) *
                    result.sampleIntervalSeconds;
            }

            return result;
        }

        [[nodiscard]]
        LayerLoopSamplingInfo
            BuildLayerLoopSamplingInfo(
                const CharacterAnimationLayerSample&
                    layer) noexcept
        {
            LayerLoopSamplingInfo result;

            result.current =
                BuildAnimationLoopSamplingInfo(
                    layer.currentAnimation);

            result.previous =
                BuildAnimationLoopSamplingInfo(
                    layer.previousAnimation);

            return result;
        }

        [[nodiscard]]
        bool SampleAnimationBone(
            const engine::assets::AnimationAsset*
                animation,

            const engine::assets::SkeletonAsset&
                skeleton,

            const std::size_t boneIndex,

            const double timeSecondsValue,

            const engine::scene::
                CharacterAnimationLoopMode
                    loopMode,

            const AnimationLoopSamplingInfo&
                loopSamplingInfo,

            DirectX::XMMATRIX&
                output) noexcept
        {
            if (!IsAnimationUsable(
                    animation,
                    skeleton))
            {
                return false;
            }

            const engine::assets::AnimationTrack*
                track =
                    animation->GetTrackForBone(
                        boneIndex);

            if (
                track == nullptr ||
                track->keys.empty())
            {
                return false;
            }

            const std::size_t declaredFrameCount =
                static_cast<std::size_t>(
                    animation->GetFrameCount());

            const std::size_t usableKeyCount =
                declaredFrameCount > 0U
                    ? (std::min)(
                        track->keys.size(),
                        declaredFrameCount)
                    : track->keys.size();

            if (usableKeyCount == 0U)
            {
                return false;
            }

            if (usableKeyCount == 1U)
            {
                output =
                    BuildKeyMatrix(
                        track->keys[0U],
                        track->keys[0U],
                        0.0F);

                return true;
            }

            double timeSeconds =
                std::isfinite(timeSecondsValue)
                    ? timeSecondsValue
                    : 0.0;

            timeSeconds =
                (std::max)(
                    timeSeconds,
                    0.0);

            double sampleIntervalSeconds =
                loopSamplingInfo.
                    sampleIntervalSeconds;

            if (
                !std::isfinite(
                    sampleIntervalSeconds) ||
                sampleIntervalSeconds <= 0.0)
            {
                const double frameRate =
                    static_cast<double>(
                        animation->GetFrameRate());

                sampleIntervalSeconds =
                    std::isfinite(frameRate) &&
                    frameRate > 0.0
                        ? 1.0 / frameRate
                        : 1.0;
            }

            const bool looping =
                loopMode ==
                    engine::scene::
                        CharacterAnimationLoopMode::
                            Loop;

            std::size_t activeFrameCount =
                usableKeyCount;

            double framePosition = 0.0;

            if (looping)
            {
                /*
                 * Точная логика старого WarZ:
                 *
                 * activeFrameCount = NumFrames - 1.
                 */
                activeFrameCount =
                    loopSamplingInfo.frameCount > 0U
                        ? (std::min)(
                            usableKeyCount,
                            loopSamplingInfo.frameCount)
                        : usableKeyCount - 1U;

                if (activeFrameCount == 0U)
                {
                    activeFrameCount = 1U;
                }

                const double loopDurationSeconds =
                    static_cast<double>(
                        activeFrameCount) *
                    sampleIntervalSeconds;

                timeSeconds =
                    std::fmod(
                        timeSeconds,
                        loopDurationSeconds);

                if (timeSeconds < 0.0)
                {
                    timeSeconds +=
                        loopDurationSeconds;
                }

                framePosition =
                    timeSeconds /
                    sampleIntervalSeconds;
            }
            else
            {
                const double maximumFrame =
                    static_cast<double>(
                        usableKeyCount - 1U);

                framePosition =
                    std::clamp(
                        timeSeconds /
                            sampleIntervalSeconds,
                        0.0,
                        maximumFrame);
            }

            std::size_t firstFrame =
                static_cast<std::size_t>(
                    std::floor(
                        framePosition));

            if (firstFrame >= activeFrameCount)
            {
                firstFrame =
                    activeFrameCount - 1U;
            }

            std::size_t secondFrame =
                firstFrame;

            if (looping)
            {
                /*
                 * В старом WarZ loop имеет период
                 * NumFrames - 1, но интерполяция
                 * последнего интервала выполняется:
                 *
                 * frame N-2 -> authored frame N-1.
                 *
                 * Кадр N-1 является конечной копией
                 * первого кадра. Нельзя сразу заменять
                 * его индексом 0: старые анимации могут
                 * содержать небольшую разницу между
                 * authored endpoint и первым кадром.
                 */
                secondFrame =
                    (std::min)(
                        firstFrame +
                            std::size_t{1U},
                        usableKeyCount -
                            std::size_t{1U});
            }
            else
            {
                secondFrame =
                    (std::min)(
                        firstFrame +
                            std::size_t{1U},
                        usableKeyCount -
                            std::size_t{1U});
            }

            const float interpolation =
                static_cast<float>(
                    std::clamp(
                        framePosition -
                            static_cast<double>(
                                firstFrame),
                        0.0,
                        1.0));

            output =
                BuildKeyMatrix(
                    track->keys[firstFrame],
                    track->keys[secondFrame],
                    interpolation);

            return true;
        }

        /*
         * Возвращает позу одного слоя для одной кости.
         *
         * coverage:
         * дополнительный вес при переходе между
         * отсутствующим и существующим track.
         */
        [[nodiscard]]
        bool SampleLayerBone(
            const CharacterAnimationLayerSample&
                layer,

            const LayerLoopSamplingInfo&
                loopSamplingInfo,

            const engine::assets::SkeletonAsset&
                skeleton,

            const std::size_t boneIndex,

            DirectX::XMMATRIX&
                output,

            float& coverage) noexcept
        {
            coverage = 0.0F;

            if (!layer.active)
            {
                return false;
            }

            DirectX::XMMATRIX currentMatrix =
                DirectX::XMMatrixIdentity();

            DirectX::XMMATRIX previousMatrix =
                DirectX::XMMatrixIdentity();

            const bool currentSampled =
                SampleAnimationBone(
                    layer.currentAnimation,
                    skeleton,
                    boneIndex,
                    layer.currentTimeSeconds,
                    layer.loopMode,
                    loopSamplingInfo.current,
                    currentMatrix);

            const float transitionAlpha =
                std::clamp(
                    layer.transitionAlpha,
                    0.0F,
                    1.0F);

            const bool transitioning =
                layer.previousAnimation != nullptr &&
                transitionAlpha < 1.0F;

            const bool previousSampled =
                transitioning &&
                SampleAnimationBone(
                    layer.previousAnimation,
                    skeleton,
                    boneIndex,
                    layer.previousTimeSeconds,
                    layer.previousLoopMode,
                    loopSamplingInfo.previous,
                    previousMatrix);

            if (
                currentSampled &&
                previousSampled)
            {
                output =
                    BlendMatrices(
                        previousMatrix,
                        currentMatrix,
                        transitionAlpha);

                coverage = 1.0F;

                return true;
            }

            if (currentSampled)
            {
                output = currentMatrix;

                coverage =
                    transitioning
                        ? transitionAlpha
                        : 1.0F;

                return coverage > 0.0F;
            }

            if (previousSampled)
            {
                output = previousMatrix;

                coverage =
                    1.0F -
                    transitionAlpha;

                return coverage > 0.0F;
            }

            return false;
        }

        void PreserveControllerOwnedRootTransform(
    const DirectX::XMMATRIX& bindLocal,
    DirectX::XMMATRIX& animatedLocal) noexcept
        {
            DirectX::XMFLOAT4X4 bindStored;
            DirectX::XMFLOAT4X4 animatedStored;

            DirectX::XMStoreFloat4x4(
                &bindStored,
                bindLocal);

            DirectX::XMStoreFloat4x4(
                &animatedStored,
                animatedLocal);

            /*
             * Как в r3dSkeleton::Recalc:
             *
             * controller управляет горизонтальным
             * перемещением, поэтому блокируются X/Z.
             *
             * Y и rotation остаются из анимации.
             */
            animatedStored._41 =
                bindStored._41;

            animatedStored._43 =
                bindStored._43;

            animatedLocal =
                DirectX::XMLoadFloat4x4(
                    &animatedStored);
        }
    }

    void CharacterAnimationPose::Reset() noexcept
    {
        boneCount = 0U;
        animated = false;

        for (
            DirectX::XMFLOAT4X4& matrix :
            boneMatrices)
        {
            DirectX::XMStoreFloat4x4(
                &matrix,
                DirectX::XMMatrixIdentity());
        }

        for (
            DirectX::XMFLOAT4X4& matrix :
            modelBoneMatrices)
        {
            DirectX::XMStoreFloat4x4(
                &matrix,
                DirectX::XMMatrixIdentity());
        }
    }

    bool CharacterAnimationEvaluator::Evaluate(
        const CharacterAnimationEvaluationInput&
            input,

        CharacterAnimationPose&
            output) const noexcept
    {
        output.Reset();

        if (
            input.skeleton == nullptr ||
            !input.skeleton->IsValid())
        {
            return false;
        }

        const engine::assets::SkeletonAsset&
            skeleton =
                *input.skeleton;

        const std::size_t boneCount =
            skeleton.GetBoneCount();

        if (
            boneCount == 0U ||
            boneCount >
                engine::assets::
                    MaximumSkeletonBones)
        {
            return false;
        }

        std::array<
            bool,
            engine::assets::MaximumSkeletonBones>
            upperBodyMask{};

        std::array<
            bool,
            engine::assets::MaximumSkeletonBones>
            actionMask{};

        BuildBoneMask(
            skeleton,
            input.upperBodyRootBone,
            upperBodyMask);

        const std::int32_t lookSpineBoneIndex =
            FindBoneIndex(
                skeleton,
                input.upperBodyRootBone);

        const std::int32_t lookRootBoneIndex =
            FindBoneIndex(
                skeleton,
                input.lookRootBone);

        std::size_t proceduralLookBoneCount = 0U;

        if (lookSpineBoneIndex >= 0)
        {
            ++proceduralLookBoneCount;
        }

        if (
            lookRootBoneIndex >= 0 &&
            lookRootBoneIndex !=
                lookSpineBoneIndex)
        {
            ++proceduralLookBoneCount;
        }

        const float proceduralLookBoneWeight =
            proceduralLookBoneCount > 0U
                ? 1.0F /
                    static_cast<float>(
                        proceduralLookBoneCount)
                : 0.0F;

        BuildBoneMask(
            skeleton,
            input.actionRootBone.empty()
                ? input.upperBodyRootBone
                : input.actionRootBone,
            actionMask);

        const LayerLoopSamplingInfo
            lowerBodyLoopSampling =
                BuildLayerLoopSamplingInfo(
                    input.lowerBody);

        const LayerLoopSamplingInfo
            upperBodyLoopSampling =
                BuildLayerLoopSamplingInfo(
                    input.upperBody);

        const LayerLoopSamplingInfo
            actionLoopSampling =
                BuildLayerLoopSamplingInfo(
                    input.action);

        std::array<
            DirectX::XMFLOAT4X4,
            engine::assets::MaximumSkeletonBones>
            currentAbsolute{};

        const DirectX::XMMATRIX pivotTransform =
            DirectX::XMMatrixTranslation(
                -input.pivot[0],
                -input.pivot[1],
                -input.pivot[2]);

        bool usedAnimationTrack = false;

        for (
            std::size_t boneIndex = 0U;
            boneIndex < boneCount;
            ++boneIndex)
        {
            const engine::assets::SkeletonBone*
                bone =
                    skeleton.GetBone(
                        boneIndex);

            if (bone == nullptr)
            {
                return false;
            }

            DirectX::XMMATRIX bindLocal;

            if (!BuildBindLocalMatrix(
                    skeleton,
                    boneIndex,
                    bindLocal))
            {
                return false;
            }

            DirectX::XMMATRIX localMatrix =
                bindLocal;

            /*
             * 1. Lower Body.
             *
             * Lower clip является основной позой.
             * Если track отсутствует, остаётся bind pose.
             */
            DirectX::XMMATRIX lowerMatrix;
            float lowerCoverage = 0.0F;

            if (SampleLayerBone(
                    input.lowerBody,
                    lowerBodyLoopSampling,
                    skeleton,
                    boneIndex,
                    lowerMatrix,
                    lowerCoverage))
            {
                const float lowerWeight =
                    std::clamp(
                        input.lowerBody.weight *
                            lowerCoverage,
                        0.0F,
                        1.0F);

                localMatrix =
                    BlendMatrices(
                        bindLocal,
                        lowerMatrix,
                        lowerWeight);

                usedAnimationTrack = true;
            }

            /*
             * 2. Upper Body.
             *
             * Применяется только к root bone
             * верхней маски и его потомкам.
             */
            if (upperBodyMask[boneIndex])
            {
                DirectX::XMMATRIX upperMatrix;
                float upperCoverage = 0.0F;

                if (SampleLayerBone(
                        input.upperBody,
                        upperBodyLoopSampling,
                        skeleton,
                        boneIndex,
                        upperMatrix,
                        upperCoverage))
                {
                    const float upperWeight =
                        std::clamp(
                            input.upperBody.weight *
                                upperCoverage,
                            0.0F,
                            1.0F);

                    localMatrix =
                        BlendMatrices(
                            localMatrix,
                            upperMatrix,
                            upperWeight);

                    usedAnimationTrack = true;
                }
            }

            /*
             * 3. Action.
             *
             * Action накладывается последним:
             * удар, выстрел, перезарядка.
             */
            if (actionMask[boneIndex])
            {
                DirectX::XMMATRIX actionMatrix;
                float actionCoverage = 0.0F;

                if (SampleLayerBone(
                        input.action,
                        actionLoopSampling,
                        skeleton,
                        boneIndex,
                        actionMatrix,
                        actionCoverage))
                {
                    const float actionWeight =
                        std::clamp(
                            input.action.weight *
                                actionCoverage,
                            0.0F,
                            1.0F);

                    localMatrix =
                        BlendMatrices(
                            localMatrix,
                            actionMatrix,
                            actionWeight);

                    usedAnimationTrack = true;
                }
            }

            if (
                input.blockControllerOwnedRootTransform &&
                bone->parentIndex < 0)
            {
                PreserveControllerOwnedRootTransform(
                    bindLocal,
                    localMatrix);
            }

            DirectX::XMMATRIX absoluteMatrix =
                localMatrix;

            if (bone->parentIndex >= 0)
            {
                const std::size_t parentIndex =
                    static_cast<std::size_t>(
                        bone->parentIndex);

                if (parentIndex >= boneIndex)
                {
                    return false;
                }

                absoluteMatrix *=
                    DirectX::XMLoadFloat4x4(
                        &currentAbsolute[
                            parentIndex]);
            }

            /*
             * Поведение повторяет принцип старого
             * AdjustBoneCallback: yaw/pitch делятся
             * между Spine1 и Neck, а translation
             * каждой absolute matrix сохраняется.
             */
            const bool isLookSpineBone =
                lookSpineBoneIndex >= 0 &&
                boneIndex ==
                    static_cast<std::size_t>(
                        lookSpineBoneIndex);

            const bool isLookNeckBone =
                lookRootBoneIndex >= 0 &&
                lookRootBoneIndex !=
                    lookSpineBoneIndex &&
                boneIndex ==
                    static_cast<std::size_t>(
                        lookRootBoneIndex);

            if (
                proceduralLookBoneWeight > 0.0F &&
                (isLookSpineBone || isLookNeckBone) &&
                std::isfinite(
                    input.lookYawOffsetDegrees) &&
                std::isfinite(
                    input.lookPitchOffsetDegrees))
            {
                const float yawDegrees =
                    std::remainder(
                        input.lookYawOffsetDegrees,
                        360.0F) *
                    proceduralLookBoneWeight;

                const float pitchDegrees =
                    std::remainder(
                        input.lookPitchOffsetDegrees,
                        360.0F) *
                    proceduralLookBoneWeight;

                const DirectX::XMMATRIX pitchRotation =
                    DirectX::XMMatrixRotationX(
                        DirectX::XMConvertToRadians(
                            pitchDegrees));

                const DirectX::XMMATRIX yawRotation =
                    DirectX::XMMatrixRotationY(
                        DirectX::XMConvertToRadians(
                            yawDegrees));

                DirectX::XMFLOAT4X4 storedAbsolute;

                DirectX::XMStoreFloat4x4(
                    &storedAbsolute,
                    absoluteMatrix);

                const float translationX =
                    storedAbsolute._41;

                const float translationY =
                    storedAbsolute._42;

                const float translationZ =
                    storedAbsolute._43;

                absoluteMatrix =
                    absoluteMatrix *
                    pitchRotation *
                    yawRotation;

                DirectX::XMStoreFloat4x4(
                    &storedAbsolute,
                    absoluteMatrix);

                storedAbsolute._41 =
                    translationX;

                storedAbsolute._42 =
                    translationY;

                storedAbsolute._43 =
                    translationZ;

                absoluteMatrix =
                    DirectX::XMLoadFloat4x4(
                        &storedAbsolute);

                usedAnimationTrack = true;
            }

            DirectX::XMStoreFloat4x4(
                &currentAbsolute[boneIndex],
                absoluteMatrix);

            const DirectX::XMMATRIX bindAbsolute =
                LoadMatrix(
                    bone->absoluteBindMatrix);

            const DirectX::XMMATRIX shiftedBind =
                bindAbsolute *
                pivotTransform;

            const DirectX::XMMATRIX shiftedCurrent =
                absoluteMatrix *
                pivotTransform;

            DirectX::XMStoreFloat4x4(
                &output.modelBoneMatrices[
                    boneIndex],
                shiftedCurrent);

            DirectX::XMMATRIX inverseBind;

            if (!InvertMatrix(
                    shiftedBind,
                    inverseBind))
            {
                return false;
            }

            DirectX::XMStoreFloat4x4(
                &output.boneMatrices[
                    boneIndex],

                inverseBind *
                    shiftedCurrent);
        }

        output.boneCount =
            static_cast<std::uint32_t>(
                boneCount);

        output.animated =
            usedAnimationTrack;

        return true;
    }
}
