#include "Editor/Tools/Character/CharacterPose.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace lts::editor
{
    namespace
    {
        constexpr float PoseEpsilon = 0.00001F;

        [[nodiscard]]
        bool IsFinite(
            const float value) noexcept
        {
            return std::isfinite(value);
        }

        [[nodiscard]]
        bool IsFinite(
            const DirectX::XMFLOAT3& value) noexcept
        {
            return
                IsFinite(value.x) &&
                IsFinite(value.y) &&
                IsFinite(value.z);
        }

        [[nodiscard]]
        bool IsFinite(
            const DirectX::XMFLOAT4& value) noexcept
        {
            return
                IsFinite(value.x) &&
                IsFinite(value.y) &&
                IsFinite(value.z) &&
                IsFinite(value.w);
        }

        [[nodiscard]]
        bool IsFinite(
            const CharacterPoseTransform& transform) noexcept
        {
            return
                IsFinite(transform.translation) &&
                IsFinite(transform.rotation) &&
                IsFinite(transform.scale);
        }

        [[nodiscard]]
        DirectX::XMMATRIX ComposeTransform(
            const CharacterPoseTransform& transform) noexcept
        {
            const DirectX::XMVECTOR scale =
                DirectX::XMLoadFloat3(
                    &transform.scale);

            const DirectX::XMVECTOR rotation =
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &transform.rotation));

            const DirectX::XMVECTOR translation =
                DirectX::XMLoadFloat3(
                    &transform.translation);

            return
                DirectX::XMMatrixScalingFromVector(
                    scale) *
                DirectX::XMMatrixRotationQuaternion(
                    rotation) *
                DirectX::XMMatrixTranslationFromVector(
                    translation);
        }

        [[nodiscard]]
        bool DecomposeTransform(
            DirectX::FXMMATRIX matrix,
            CharacterPoseTransform& output) noexcept
        {
            DirectX::XMVECTOR scale;
            DirectX::XMVECTOR rotation;
            DirectX::XMVECTOR translation;

            if (!DirectX::XMMatrixDecompose(
                    &scale,
                    &rotation,
                    &translation,
                    matrix))
            {
                return false;
            }

            rotation =
                DirectX::XMQuaternionNormalize(
                    rotation);

            DirectX::XMStoreFloat3(
                &output.scale,
                scale);

            DirectX::XMStoreFloat4(
                &output.rotation,
                rotation);

            DirectX::XMStoreFloat3(
                &output.translation,
                translation);

            return IsFinite(output);
        }

        [[nodiscard]]
        DirectX::XMFLOAT4X4 IdentityMatrix() noexcept
        {
            DirectX::XMFLOAT4X4 result;

            DirectX::XMStoreFloat4x4(
                &result,
                DirectX::XMMatrixIdentity());

            return result;
        }

        [[nodiscard]]
        DirectX::XMFLOAT3 LerpFloat3(
            const DirectX::XMFLOAT3& first,
            const DirectX::XMFLOAT3& second,
            const float amount) noexcept
        {
            return
            {
                first.x +
                    (second.x - first.x) *
                    amount,

                first.y +
                    (second.y - first.y) *
                    amount,

                first.z +
                    (second.z - first.z) *
                    amount
            };
        }

        [[nodiscard]]
        DirectX::XMVECTOR GetMatrixTranslation(
            DirectX::FXMMATRIX matrix) noexcept
        {
            return matrix.r[3];
        }

        [[nodiscard]]
        DirectX::XMVECTOR RotationBetweenDirections(
            DirectX::FXMVECTOR sourceDirection,
            DirectX::FXMVECTOR targetDirection) noexcept
        {
            const DirectX::XMVECTOR source =
                DirectX::XMVector3Normalize(
                    sourceDirection);

            const DirectX::XMVECTOR target =
                DirectX::XMVector3Normalize(
                    targetDirection);

            const float dot =
                std::clamp(
                    DirectX::XMVectorGetX(
                        DirectX::XMVector3Dot(
                            source,
                            target)),
                    -1.0F,
                    1.0F);

            if (dot >= 0.99999F)
            {
                return DirectX::XMQuaternionIdentity();
            }

            if (dot <= -0.99999F)
            {
                DirectX::XMVECTOR axis =
                    DirectX::XMVector3Cross(
                        source,
                        DirectX::XMVectorSet(
                            1.0F,
                            0.0F,
                            0.0F,
                            0.0F));

                if (DirectX::XMVectorGetX(
                        DirectX::XMVector3LengthSq(
                            axis)) <
                    PoseEpsilon)
                {
                    axis =
                        DirectX::XMVector3Cross(
                            source,
                            DirectX::XMVectorSet(
                                0.0F,
                                1.0F,
                                0.0F,
                                0.0F));
                }

                axis =
                    DirectX::XMVector3Normalize(
                        axis);

                return
                    DirectX::XMQuaternionRotationAxis(
                        axis,
                        DirectX::XM_PI);
            }

            DirectX::XMVECTOR axis =
                DirectX::XMVector3Normalize(
                    DirectX::XMVector3Cross(
                        source,
                        target));

            const float angle =
                std::acos(dot);

            return
                DirectX::XMQuaternionRotationAxis(
                    axis,
                    angle);
        }

        [[nodiscard]]
        DirectX::XMMATRIX PreserveTranslation(
            DirectX::FXMMATRIX source,
            DirectX::FXMMATRIX orientation) noexcept
        {
            DirectX::XMFLOAT4X4 result;

            DirectX::XMStoreFloat4x4(
                &result,
                orientation);

            const DirectX::XMVECTOR translation =
                GetMatrixTranslation(source);

            result._41 =
                DirectX::XMVectorGetX(
                    translation);

            result._42 =
                DirectX::XMVectorGetY(
                    translation);

            result._43 =
                DirectX::XMVectorGetZ(
                    translation);

            result._44 = 1.0F;

            return
                DirectX::XMLoadFloat4x4(
                    &result);
        }
    }

    bool CharacterAnimationClip::IsValid() const noexcept
    {
        if (!IsFinite(durationSeconds) ||
            durationSeconds <= 0.0F)
        {
            return false;
        }

        for (const CharacterAnimationTrack& track :
             tracks)
        {
            if (track.boneName.empty() ||
                track.keyframes.empty())
            {
                return false;
            }

            float previousTime = -1.0F;

            for (const CharacterAnimationKeyframe& keyframe :
                 track.keyframes)
            {
                if (!IsFinite(keyframe.timeSeconds) ||
                    keyframe.timeSeconds < 0.0F ||
                    keyframe.timeSeconds >
                        durationSeconds ||
                    keyframe.timeSeconds <
                        previousTime ||
                    !IsFinite(keyframe.transform))
                {
                    return false;
                }

                previousTime =
                    keyframe.timeSeconds;
            }
        }

        return true;
    }

    CharacterPose::CharacterPose() noexcept
    {
        Clear();
    }

    bool CharacterPose::Initialize(
        const engine::assets::SkeletonAsset& skeleton) noexcept
    {
        Clear();

        if (!skeleton.IsValid())
        {
            return false;
        }

        const std::size_t boneCount =
            skeleton.GetBoneCount();

        if (boneCount == 0U ||
            boneCount >
                engine::assets::MaximumSkeletonBones)
        {
            return false;
        }

        for (std::size_t boneIndex = 0U;
             boneIndex < boneCount;
             ++boneIndex)
        {
            const engine::assets::SkeletonBone*
                bone =
                    skeleton.GetBone(
                        boneIndex);

            if (bone == nullptr)
            {
                Clear();
                return false;
            }

            boneNames_[boneIndex] =
                bone->name;

            parentIndices_[boneIndex] =
                bone->parentIndex;

            DirectX::XMFLOAT4X4 absoluteBind;

            static_assert(
                sizeof(absoluteBind) ==
                sizeof(bone->absoluteBindMatrix));

            std::memcpy(
                &absoluteBind,
                bone->absoluteBindMatrix.data(),
                sizeof(absoluteBind));

            const DirectX::XMMATRIX
                absoluteBindMatrix =
                    DirectX::XMLoadFloat4x4(
                        &absoluteBind);

            DirectX::XMVECTOR determinant;

            const DirectX::XMMATRIX
                inverseBindMatrix =
                    DirectX::XMMatrixInverse(
                        &determinant,
                        absoluteBindMatrix);

            const float determinantValue =
                DirectX::XMVectorGetX(
                    determinant);

            if (!IsFinite(determinantValue) ||
                std::fabs(determinantValue) <
                    PoseEpsilon)
            {
                Clear();
                return false;
            }

            DirectX::XMStoreFloat4x4(
                &inverseBindMatrices_[boneIndex],
                inverseBindMatrix);

            DirectX::XMMATRIX localBindMatrix =
                absoluteBindMatrix;

            if (bone->parentIndex >= 0)
            {
                const std::size_t parentIndex =
                    static_cast<std::size_t>(
                        bone->parentIndex);

                if (parentIndex >= boneIndex)
                {
                    Clear();
                    return false;
                }

                const DirectX::XMMATRIX
                    parentAbsolute =
                        DirectX::XMLoadFloat4x4(
                            &absoluteMatrices_[
                                parentIndex]);

                const DirectX::XMMATRIX
                    parentInverse =
                        DirectX::XMMatrixInverse(
                            nullptr,
                            parentAbsolute);

                /*
                 * Row-vector convention:
                 *
                 * local * parentAbsolute = absolute
                 */
                localBindMatrix =
                    absoluteBindMatrix *
                    parentInverse;
            }

            CharacterPoseTransform
                bindTransform;

            if (!DecomposeTransform(
                    localBindMatrix,
                    bindTransform))
            {
                Clear();
                return false;
            }

            bindLocalTransforms_[boneIndex] =
                bindTransform;

            localTransforms_[boneIndex] =
                bindTransform;

            DirectX::XMStoreFloat4x4(
                &absoluteMatrices_[boneIndex],
                absoluteBindMatrix);
        }

        boneCount_ = boneCount;
        valid_ = true;

        if (!Rebuild())
        {
            Clear();
            return false;
        }

        return true;
    }

    void CharacterPose::Clear() noexcept
    {
        const DirectX::XMFLOAT4X4 identity =
            IdentityMatrix();

        for (std::size_t index = 0U;
             index <
                 engine::assets::MaximumSkeletonBones;
             ++index)
        {
            boneNames_[index].clear();
            parentIndices_[index] = -1;

            bindLocalTransforms_[index] = {};
            localTransforms_[index] = {};

            inverseBindMatrices_[index] =
                identity;

            absoluteMatrices_[index] =
                identity;

            paletteMatrices_[index] =
                identity;
        }

        boneCount_ = 0U;
        valid_ = false;
    }

    void CharacterPose::ResetToBindPose() noexcept
    {
        if (!valid_)
        {
            return;
        }

        for (std::size_t index = 0U;
             index < boneCount_;
             ++index)
        {
            localTransforms_[index] =
                bindLocalTransforms_[index];
        }
    }

    bool CharacterPose::Rebuild() noexcept
    {
        if (!valid_ ||
            boneCount_ == 0U)
        {
            return false;
        }

        for (std::size_t boneIndex = 0U;
             boneIndex < boneCount_;
             ++boneIndex)
        {
            const DirectX::XMMATRIX localMatrix =
                ComposeTransform(
                    localTransforms_[boneIndex]);

            DirectX::XMMATRIX absoluteMatrix =
                localMatrix;

            const std::int32_t parentIndex =
                parentIndices_[boneIndex];

            if (parentIndex >= 0)
            {
                const std::size_t parent =
                    static_cast<std::size_t>(
                        parentIndex);

                if (parent >= boneIndex)
                {
                    return false;
                }

                absoluteMatrix =
                    localMatrix *
                    DirectX::XMLoadFloat4x4(
                        &absoluteMatrices_[parent]);
            }

            DirectX::XMStoreFloat4x4(
                &absoluteMatrices_[boneIndex],
                absoluteMatrix);

            const DirectX::XMMATRIX
                inverseBind =
                    DirectX::XMLoadFloat4x4(
                        &inverseBindMatrices_[
                            boneIndex]);

            /*
             * Row-vector skinning:
             *
             * bindVertex * inverseBind * currentAbsolute
             */
            const DirectX::XMMATRIX paletteMatrix =
                inverseBind *
                absoluteMatrix;

            DirectX::XMStoreFloat4x4(
                &paletteMatrices_[boneIndex],
                paletteMatrix);
        }

        const DirectX::XMFLOAT4X4 identity =
            IdentityMatrix();

        for (std::size_t boneIndex = boneCount_;
             boneIndex <
                 engine::assets::MaximumSkeletonBones;
             ++boneIndex)
        {
            paletteMatrices_[boneIndex] =
                identity;
        }

        return true;
    }

    bool CharacterPose::IsValid() const noexcept
    {
        return valid_ &&
            boneCount_ > 0U &&
            boneCount_ <=
                engine::assets::MaximumSkeletonBones;
    }

    std::size_t CharacterPose::GetBoneCount() const noexcept
    {
        return boneCount_;
    }

    std::size_t CharacterPose::FindBone(
        const std::string& name) const noexcept
    {
        if (name.empty())
        {
            return InvalidCharacterBoneIndex;
        }

        for (std::size_t index = 0U;
             index < boneCount_;
             ++index)
        {
            if (boneNames_[index] == name)
            {
                return index;
            }
        }

        return InvalidCharacterBoneIndex;
    }

    std::int32_t CharacterPose::GetParentIndex(
        const std::size_t boneIndex) const noexcept
    {
        return boneIndex < boneCount_
            ? parentIndices_[boneIndex]
            : -1;
    }

    const std::string& CharacterPose::GetBoneName(
        const std::size_t boneIndex) const noexcept
    {
        static const std::string emptyName;

        return boneIndex < boneCount_
            ? boneNames_[boneIndex]
            : emptyName;
    }

    bool CharacterPose::SetLocalTransform(
        const std::size_t boneIndex,
        const CharacterPoseTransform& transform) noexcept
    {
        if (!valid_ ||
            boneIndex >= boneCount_ ||
            !IsFinite(transform))
        {
            return false;
        }

        localTransforms_[boneIndex] =
            transform;

        localTransforms_[boneIndex].rotation =
        {
            DirectX::XMVectorGetX(
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &transform.rotation))),

            DirectX::XMVectorGetY(
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &transform.rotation))),

            DirectX::XMVectorGetZ(
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &transform.rotation))),

            DirectX::XMVectorGetW(
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &transform.rotation)))
        };

        return true;
    }

    const CharacterPoseTransform*
    CharacterPose::GetLocalTransform(
        const std::size_t boneIndex) const noexcept
    {
        return boneIndex < boneCount_
            ? &localTransforms_[boneIndex]
            : nullptr;
    }

    const DirectX::XMFLOAT4X4*
    CharacterPose::GetAbsoluteMatrix(
        const std::size_t boneIndex) const noexcept
    {
        return boneIndex < boneCount_
            ? &absoluteMatrices_[boneIndex]
            : nullptr;
    }

    DirectX::XMFLOAT3 CharacterPose::GetBonePosition(
        const std::size_t boneIndex) const noexcept
    {
        if (boneIndex >= boneCount_)
        {
            return {};
        }

        const DirectX::XMFLOAT4X4& matrix =
            absoluteMatrices_[boneIndex];

        return
        {
            matrix._41,
            matrix._42,
            matrix._43
        };
    }

    const DirectX::XMFLOAT4X4*
    CharacterPose::GetPaletteData() const noexcept
    {
        return paletteMatrices_.data();
    }

    std::size_t CharacterPose::GetPaletteByteSize() const noexcept
    {
        return sizeof(paletteMatrices_);
    }

    bool CharacterPose::SetAbsoluteMatrix(
        const std::size_t boneIndex,
        DirectX::FXMMATRIX absoluteMatrix) noexcept
    {
        if (!valid_ ||
            boneIndex >= boneCount_)
        {
            return false;
        }

        DirectX::XMMATRIX localMatrix =
            absoluteMatrix;

        const std::int32_t parentIndex =
            parentIndices_[boneIndex];

        if (parentIndex >= 0)
        {
            const std::size_t parent =
                static_cast<std::size_t>(
                    parentIndex);

            if (parent >= boneIndex)
            {
                return false;
            }

            const DirectX::XMMATRIX
                parentAbsolute =
                    DirectX::XMLoadFloat4x4(
                        &absoluteMatrices_[parent]);

            const DirectX::XMMATRIX
                parentInverse =
                    DirectX::XMMatrixInverse(
                        nullptr,
                        parentAbsolute);

            localMatrix =
                absoluteMatrix *
                parentInverse;
        }

        CharacterPoseTransform transform;

        if (!DecomposeTransform(
                localMatrix,
                transform))
        {
            return false;
        }

        localTransforms_[boneIndex] =
            transform;

        return true;
    }

    bool CharacterPose::RotateBoneToward(
        const std::size_t boneIndex,
        DirectX::FXMVECTOR currentDirection,
        DirectX::FXMVECTOR desiredDirection) noexcept
    {
        if (boneIndex >= boneCount_)
        {
            return false;
        }

        const float currentLengthSquared =
            DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(
                    currentDirection));

        const float desiredLengthSquared =
            DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(
                    desiredDirection));

        if (currentLengthSquared < PoseEpsilon ||
            desiredLengthSquared < PoseEpsilon)
        {
            return false;
        }

        const DirectX::XMVECTOR deltaRotation =
            RotationBetweenDirections(
                currentDirection,
                desiredDirection);

        const DirectX::XMMATRIX
            currentAbsolute =
                DirectX::XMLoadFloat4x4(
                    &absoluteMatrices_[boneIndex]);

        DirectX::XMFLOAT4X4
            orientationOnly;

        DirectX::XMStoreFloat4x4(
            &orientationOnly,
            currentAbsolute);

        orientationOnly._41 = 0.0F;
        orientationOnly._42 = 0.0F;
        orientationOnly._43 = 0.0F;
        orientationOnly._44 = 1.0F;

        const DirectX::XMMATRIX
            rotatedOrientation =
                DirectX::XMLoadFloat4x4(
                    &orientationOnly) *
                DirectX::XMMatrixRotationQuaternion(
                    deltaRotation);

        const DirectX::XMMATRIX
            rotatedAbsolute =
                PreserveTranslation(
                    currentAbsolute,
                    rotatedOrientation);

        if (!SetAbsoluteMatrix(
                boneIndex,
                rotatedAbsolute))
        {
            return false;
        }

        return Rebuild();
    }

    bool CharacterPose::ApplyTwoBoneIk(
        const std::size_t rootBone,
        const std::size_t jointBone,
        const std::size_t endBone,
        const DirectX::XMFLOAT4X4& targetAbsolute,
        const DirectX::XMFLOAT3& polePosition) noexcept
    {
        if (!valid_ ||
            rootBone >= boneCount_ ||
            jointBone >= boneCount_ ||
            endBone >= boneCount_ ||
            parentIndices_[jointBone] !=
                static_cast<std::int32_t>(
                    rootBone) ||
            parentIndices_[endBone] !=
                static_cast<std::int32_t>(
                    jointBone))
        {
            return false;
        }

        if (!Rebuild())
        {
            return false;
        }

        const DirectX::XMFLOAT3 rootPositionFloat =
            GetBonePosition(rootBone);

        const DirectX::XMFLOAT3 jointPositionFloat =
            GetBonePosition(jointBone);

        const DirectX::XMFLOAT3 endPositionFloat =
            GetBonePosition(endBone);

        const DirectX::XMVECTOR rootPosition =
            DirectX::XMLoadFloat3(
                &rootPositionFloat);

        const DirectX::XMVECTOR jointPosition =
            DirectX::XMLoadFloat3(
                &jointPositionFloat);

        const DirectX::XMVECTOR endPosition =
            DirectX::XMLoadFloat3(
                &endPositionFloat);

        const DirectX::XMVECTOR targetPosition =
            DirectX::XMVectorSet(
                targetAbsolute._41,
                targetAbsolute._42,
                targetAbsolute._43,
                1.0F);

        const float upperLength =
            DirectX::XMVectorGetX(
                DirectX::XMVector3Length(
                    DirectX::XMVectorSubtract(
                        jointPosition,
                        rootPosition)));

        const float lowerLength =
            DirectX::XMVectorGetX(
                DirectX::XMVector3Length(
                    DirectX::XMVectorSubtract(
                        endPosition,
                        jointPosition)));

        if (upperLength < PoseEpsilon ||
            lowerLength < PoseEpsilon)
        {
            return false;
        }

        DirectX::XMVECTOR rootToTarget =
            DirectX::XMVectorSubtract(
                targetPosition,
                rootPosition);

        float targetDistance =
            DirectX::XMVectorGetX(
                DirectX::XMVector3Length(
                    rootToTarget));

        if (targetDistance < PoseEpsilon)
        {
            return false;
        }

        rootToTarget =
            DirectX::XMVector3Normalize(
                rootToTarget);

        const float minimumDistance =
            std::fabs(
                upperLength -
                lowerLength) +
            PoseEpsilon;

        const float maximumDistance =
            upperLength +
            lowerLength -
            PoseEpsilon;

        targetDistance =
            std::clamp(
                targetDistance,
                minimumDistance,
                maximumDistance);

        const DirectX::XMVECTOR pole =
            DirectX::XMLoadFloat3(
                &polePosition);

        DirectX::XMVECTOR poleDirection =
            DirectX::XMVectorSubtract(
                pole,
                rootPosition);

        poleDirection =
            DirectX::XMVectorSubtract(
                poleDirection,
                DirectX::XMVectorScale(
                    rootToTarget,
                    DirectX::XMVectorGetX(
                        DirectX::XMVector3Dot(
                            poleDirection,
                            rootToTarget))));

        if (DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(
                    poleDirection)) <
            PoseEpsilon)
        {
            poleDirection =
                DirectX::XMVectorSubtract(
                    jointPosition,
                    rootPosition);

            poleDirection =
                DirectX::XMVectorSubtract(
                    poleDirection,
                    DirectX::XMVectorScale(
                        rootToTarget,
                        DirectX::XMVectorGetX(
                            DirectX::XMVector3Dot(
                                poleDirection,
                                rootToTarget))));
        }

        if (DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(
                    poleDirection)) <
            PoseEpsilon)
        {
            poleDirection =
                DirectX::XMVectorSet(
                    0.0F,
                    0.0F,
                    1.0F,
                    0.0F);
        }

        poleDirection =
            DirectX::XMVector3Normalize(
                poleDirection);

        const float cosine =
            std::clamp(
                (
                    upperLength *
                        upperLength +
                    targetDistance *
                        targetDistance -
                    lowerLength *
                        lowerLength
                ) /
                (
                    2.0F *
                    upperLength *
                    targetDistance
                ),
                -1.0F,
                1.0F);

        const float sine =
            std::sqrt(
                std::max(
                    1.0F -
                        cosine *
                        cosine,
                    0.0F));

        const DirectX::XMVECTOR
            desiredJointPosition =
                DirectX::XMVectorAdd(
                    rootPosition,
                    DirectX::XMVectorAdd(
                        DirectX::XMVectorScale(
                            rootToTarget,
                            upperLength *
                                cosine),

                        DirectX::XMVectorScale(
                            poleDirection,
                            upperLength *
                                sine)));

        const DirectX::XMVECTOR
            desiredEndPosition =
                DirectX::XMVectorAdd(
                    rootPosition,
                    DirectX::XMVectorScale(
                        rootToTarget,
                        targetDistance));

        if (!RotateBoneToward(
                rootBone,
                DirectX::XMVectorSubtract(
                    jointPosition,
                    rootPosition),
                DirectX::XMVectorSubtract(
                    desiredJointPosition,
                    rootPosition)))
        {
            return false;
        }

        const DirectX::XMFLOAT3
            updatedJointFloat =
                GetBonePosition(
                    jointBone);

        const DirectX::XMFLOAT3
            updatedEndFloat =
                GetBonePosition(
                    endBone);

        const DirectX::XMVECTOR
            updatedJointPosition =
                DirectX::XMLoadFloat3(
                    &updatedJointFloat);

        const DirectX::XMVECTOR
            updatedEndPosition =
                DirectX::XMLoadFloat3(
                    &updatedEndFloat);

        if (!RotateBoneToward(
                jointBone,
                DirectX::XMVectorSubtract(
                    updatedEndPosition,
                    updatedJointPosition),
                DirectX::XMVectorSubtract(
                    desiredEndPosition,
                    updatedJointPosition)))
        {
            return false;
        }

        /*
         * После решения локтя берём ориентацию target,
         * но сохраняем реально достигнутую позицию hand_l.
         */
        const DirectX::XMFLOAT3 solvedEndPosition =
            GetBonePosition(endBone);

        CharacterPoseTransform targetTransform;

        if (!DecomposeTransform(
                DirectX::XMLoadFloat4x4(
                    &targetAbsolute),
                targetTransform))
        {
            return false;
        }

        targetTransform.translation =
            solvedEndPosition;

        if (!SetAbsoluteMatrix(
                endBone,
                ComposeTransform(
                    targetTransform)))
        {
            return false;
        }

        return Rebuild();
    }

    bool CharacterAnimationPlayer::SetClip(
        std::shared_ptr<
            const CharacterAnimationClip> clip) noexcept
    {
        if (clip == nullptr ||
            !clip->IsValid())
        {
            return false;
        }

        clip_ = std::move(clip);
        timeSeconds_ = 0.0F;
        looping_ = clip_->looping;
        playing_ = false;

        return true;
    }

    void CharacterAnimationPlayer::ClearClip() noexcept
    {
        clip_.reset();
        timeSeconds_ = 0.0F;
        playing_ = false;
    }

    void CharacterAnimationPlayer::Play() noexcept
    {
        if (clip_ != nullptr)
        {
            playing_ = true;
        }
    }

    void CharacterAnimationPlayer::Pause() noexcept
    {
        playing_ = false;
    }

    void CharacterAnimationPlayer::Stop() noexcept
    {
        playing_ = false;
        timeSeconds_ = 0.0F;
    }

    void CharacterAnimationPlayer::SetLooping(
        const bool looping) noexcept
    {
        looping_ = looping;
    }

    void CharacterAnimationPlayer::SetPlaybackSpeed(
        const float speed) noexcept
    {
        if (IsFinite(speed))
        {
            playbackSpeed_ =
                std::clamp(
                    speed,
                    -8.0F,
                    8.0F);
        }
    }

    void CharacterAnimationPlayer::SetTime(
        const float timeSeconds) noexcept
    {
        if (clip_ == nullptr ||
            !IsFinite(timeSeconds))
        {
            return;
        }

        timeSeconds_ =
            std::clamp(
                timeSeconds,
                0.0F,
                clip_->durationSeconds);
    }

    void CharacterAnimationPlayer::Update(
        const float deltaSeconds) noexcept
    {
        if (!playing_ ||
            clip_ == nullptr ||
            !IsFinite(deltaSeconds))
        {
            return;
        }

        timeSeconds_ +=
            deltaSeconds *
            playbackSpeed_;

        const float duration =
            clip_->durationSeconds;

        if (looping_)
        {
            if (duration > PoseEpsilon)
            {
                timeSeconds_ =
                    std::fmod(
                        timeSeconds_,
                        duration);

                if (timeSeconds_ < 0.0F)
                {
                    timeSeconds_ +=
                        duration;
                }
            }
        }
        else
        {
            if (timeSeconds_ >= duration)
            {
                timeSeconds_ = duration;
                playing_ = false;
            }
            else if (timeSeconds_ <= 0.0F)
            {
                timeSeconds_ = 0.0F;

                if (playbackSpeed_ < 0.0F)
                {
                    playing_ = false;
                }
            }
        }
    }

    bool CharacterAnimationPlayer::Evaluate(
        CharacterPose& pose) const noexcept
    {
        if (!pose.IsValid())
        {
            return false;
        }

        pose.ResetToBindPose();

        if (clip_ == nullptr)
        {
            return pose.Rebuild();
        }

        for (const CharacterAnimationTrack& track :
             clip_->tracks)
        {
            const std::size_t boneIndex =
                pose.FindBone(
                    track.boneName);

            if (boneIndex ==
                InvalidCharacterBoneIndex)
            {
                continue;
            }

            CharacterPoseTransform transform;

            if (!SampleTrack(
                    track,
                    timeSeconds_,
                    transform))
            {
                continue;
            }

            if (!pose.SetLocalTransform(
                    boneIndex,
                    transform))
            {
                return false;
            }
        }

        return pose.Rebuild();
    }

    bool CharacterAnimationPlayer::HasClip() const noexcept
    {
        return clip_ != nullptr;
    }

    bool CharacterAnimationPlayer::IsPlaying() const noexcept
    {
        return playing_;
    }

    bool CharacterAnimationPlayer::IsLooping() const noexcept
    {
        return looping_;
    }

    float CharacterAnimationPlayer::GetPlaybackSpeed() const noexcept
    {
        return playbackSpeed_;
    }

    float CharacterAnimationPlayer::GetTime() const noexcept
    {
        return timeSeconds_;
    }

    float CharacterAnimationPlayer::GetDuration() const noexcept
    {
        return clip_ != nullptr
            ? clip_->durationSeconds
            : 0.0F;
    }

    const CharacterAnimationClip*
    CharacterAnimationPlayer::GetClip() const noexcept
    {
        return clip_.get();
    }

    bool CharacterAnimationPlayer::SampleTrack(
        const CharacterAnimationTrack& track,
        const float timeSeconds,
        CharacterPoseTransform& output) noexcept
    {
        if (track.keyframes.empty())
        {
            return false;
        }

        if (track.keyframes.size() == 1U ||
            timeSeconds <=
                track.keyframes.front().timeSeconds)
        {
            output =
                track.keyframes.front().transform;

            return true;
        }

        if (timeSeconds >=
            track.keyframes.back().timeSeconds)
        {
            output =
                track.keyframes.back().transform;

            return true;
        }

        const auto upper =
            std::upper_bound(
                track.keyframes.begin(),
                track.keyframes.end(),
                timeSeconds,
                [](
                    const float value,
                    const CharacterAnimationKeyframe&
                        keyframe)
                {
                    return value <
                        keyframe.timeSeconds;
                });

        if (upper == track.keyframes.begin() ||
            upper == track.keyframes.end())
        {
            return false;
        }

        const auto lower =
            upper - 1;

        const float frameDuration =
            upper->timeSeconds -
            lower->timeSeconds;

        if (frameDuration <= PoseEpsilon)
        {
            output =
                upper->transform;

            return true;
        }

        const float amount =
            std::clamp(
                (
                    timeSeconds -
                    lower->timeSeconds
                ) /
                frameDuration,
                0.0F,
                1.0F);

        output.translation =
            LerpFloat3(
                lower->transform.translation,
                upper->transform.translation,
                amount);

        output.scale =
            LerpFloat3(
                lower->transform.scale,
                upper->transform.scale,
                amount);

        const DirectX::XMVECTOR
            firstRotation =
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &lower->transform.rotation));

        const DirectX::XMVECTOR
            secondRotation =
                DirectX::XMQuaternionNormalize(
                    DirectX::XMLoadFloat4(
                        &upper->transform.rotation));

        const DirectX::XMVECTOR
            sampledRotation =
                DirectX::XMQuaternionSlerp(
                    firstRotation,
                    secondRotation,
                    amount);

        DirectX::XMStoreFloat4(
            &output.rotation,
            DirectX::XMQuaternionNormalize(
                sampledRotation));

        return true;
    }
}