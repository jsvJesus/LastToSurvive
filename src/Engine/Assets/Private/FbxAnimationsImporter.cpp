#include "Assets/FbxAnimationsImporter.h"

#include "FbxImporterCommon.h"

#include <ufbx.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    namespace
    {
        void EnsureQuaternionContinuity(
            const std::array<float, 4U>& previous,
            std::array<float, 4U>& current)
            noexcept
        {
            const float dot =
                previous[0U] * current[0U] +
                previous[1U] * current[1U] +
                previous[2U] * current[2U] +
                previous[3U] * current[3U];

            if (dot < 0.0F)
            {
                for (float& value : current)
                {
                    value = -value;
                }
            }
        }

        [[nodiscard]]
        std::filesystem::path MakeClipPath(
            const std::filesystem::path& directory,
            const ufbx_anim_stack& stack,
            std::unordered_map<
                std::string,
                std::size_t>& usedNames)
        {
            std::string baseName =
                fbx_detail::SanitizeName(
                    fbx_detail::ToString(
                        stack.name,
                        "Animation"),
                    "Animation");

            std::size_t& counter =
                usedNames[baseName];

            if (counter != 0U)
            {
                baseName +=
                    "_" +
                    std::to_string(counter);
            }

            ++counter;

            return
                directory /
                std::filesystem::u8path(
                    baseName + ".anim");
        }

        [[nodiscard]]
        AssetResult WriteAnimation(
            const ufbx_scene& scene,
            const ufbx_anim_stack& stack,
            const FbxSkeletonData& skeleton,
            const std::vector<
                const ufbx_node*>& boneNodes,
            const std::string& skeletonAssetPath,
            const float requestedSampleRate,
            const std::filesystem::path& destination,
            const bool overwrite,
            FbxImportReport& report,
            std::wstring& error)
        {
            if (
                stack.anim == nullptr ||
                !skeleton.IsValid() ||
                boneNodes.size() !=
                    skeleton.bones.size())
            {
                error =
                    L"Invalid FBX animation stack.";

                return AssetResult::InvalidArgument;
            }

            const float sampleRate =
                std::isfinite(
                    requestedSampleRate) &&
                requestedSampleRate > 0.0F
                    ? std::clamp(
                        requestedSampleRate,
                        1.0F,
                        240.0F)
                    : 30.0F;

            double startTime =
                stack.time_begin;

            double endTime =
                stack.time_end;

            if (
                !std::isfinite(startTime) ||
                !std::isfinite(endTime) ||
                endTime < startTime)
            {
                startTime =
                    stack.anim->time_begin;

                endTime =
                    stack.anim->time_end;
            }

            if (
                !std::isfinite(startTime) ||
                !std::isfinite(endTime) ||
                endTime < startTime)
            {
                error =
                    L"FBX animation contains an "
                    L"invalid time range.";

                return AssetResult::CorruptData;
            }

            const double duration =
                endTime - startTime;

            const std::size_t frameCountSize =
                (std::max)(
                    static_cast<std::size_t>(
                        std::ceil(
                            duration *
                            static_cast<double>(
                                sampleRate))) +
                        1U,
                    std::size_t{1U});

            if (
                frameCountSize >
                (std::numeric_limits<
                    std::uint32_t>::max)())
            {
                error =
                    L"FBX animation has too many frames.";

                return AssetResult::FileTooLarge;
            }

            const std::uint32_t frameCount =
                static_cast<std::uint32_t>(
                    frameCountSize);

            std::vector<
                std::vector<FbxAnimationKey>>
                keysByBone(
                    skeleton.bones.size());

            for (auto& keys : keysByBone)
            {
                keys.reserve(frameCountSize);
            }

            bool scaleWarningWritten = false;

            for (
                std::size_t frameIndex = 0U;
                frameIndex < frameCountSize;
                ++frameIndex)
            {
                const double localTime =
                    frameCountSize > 1U
                        ? (
                            duration *
                            static_cast<double>(
                                frameIndex) /
                            static_cast<double>(
                                frameCountSize - 1U)
                        )
                        : 0.0;

                const double sourceTime =
                    startTime + localTime;

                ufbx_error evaluateError{};
                ufbx_evaluate_opts options{};

                ufbx_scene* const evaluated =
                    ufbx_evaluate_scene(
                        &scene,
                        stack.anim,
                        sourceTime,
                        &options,
                        &evaluateError);

                if (evaluated == nullptr)
                {
                    error =
                        L"ufbx failed to evaluate "
                        L"animation frame: ";

                    fbx_detail::AppendAscii(
                        error,
                        evaluateError.
                            description.data);

                    return AssetResult::CorruptData;
                }

                for (
                    std::size_t boneIndex = 0U;
                    boneIndex <
                        boneNodes.size();
                    ++boneIndex)
                {
                    const ufbx_node* const sourceBone =
                        boneNodes[boneIndex];

                    if (
                        sourceBone == nullptr ||
                        sourceBone->typed_id >=
                            evaluated->nodes.count)
                    {
                        ufbx_free_scene(evaluated);

                        error =
                            L"Evaluated FBX scene is "
                            L"missing a skeleton bone.";

                        return AssetResult::CorruptData;
                    }

                    const ufbx_node* const bone =
                        evaluated->nodes.data[
                            sourceBone->typed_id];

                    if (bone == nullptr)
                    {
                        ufbx_free_scene(evaluated);

                        error =
                            L"Evaluated FBX bone is null.";

                        return AssetResult::CorruptData;
                    }

                    const ufbx_transform transform =
                        bone->local_transform;

                    FbxAnimationKey key;

                    key.timeSeconds =
                        static_cast<float>(
                            localTime);

                    key.translation =
                        fbx_detail::Convert(
                            transform.translation);

                    key.rotation =
                        fbx_detail::Convert(
                            transform.rotation);

                    key.scale =
                        fbx_detail::Convert(
                            transform.scale);

                    auto& keys =
                        keysByBone[boneIndex];

                    if (!keys.empty())
                    {
                        EnsureQuaternionContinuity(
                            keys.back().rotation,
                            key.rotation);
                    }

                    if (
                        !scaleWarningWritten &&
                        (
                            std::fabs(
                                key.scale[0U] -
                                1.0F) > 0.0001F ||
                            std::fabs(
                                key.scale[1U] -
                                1.0F) > 0.0001F ||
                            std::fabs(
                                key.scale[2U] -
                                1.0F) > 0.0001F
                        ))
                    {
                        /*
                         * Текущий .anim runtime-формат
                         * не хранит scale.
                         */
                        report.warnings.push_back(
                            L"Animation contains bone "
                            L"scale keys. Current .anim "
                            L"format stores rotation and "
                            L"translation only.");

                        scaleWarningWritten = true;
                    }

                    keys.push_back(
                        std::move(key));
                }

                ufbx_free_scene(evaluated);
            }

            fbx_detail::BinaryWriter writer;

            constexpr std::array<char, 8U> magic
            {{
                'A', 'N', 'I', 'M',
                'C', 'L', 'I', 'P'
            }};

            const std::uint32_t version = 1U;

            const std::uint32_t skeletonId =
                fbx_detail::HashSkeletonPath(
                    skeletonAssetPath);

            const float durationSeconds =
                static_cast<float>(duration);

            const std::uint32_t trackCount =
                static_cast<std::uint32_t>(
                    skeleton.bones.size());

            if (
                !writer.WriteBytes(
                    magic.data(),
                    magic.size()) ||
                !writer.Write(version) ||
                !writer.Write(
                    fbx_detail::
                        AssetEndianMarker) ||
                !writer.WriteString(
                    skeletonAssetPath) ||
                !writer.Write(skeletonId) ||
                !writer.Write(frameCount) ||
                !writer.Write(sampleRate) ||
                !writer.Write(durationSeconds) ||
                !writer.Write(trackCount))
            {
                error =
                    L"Failed to encode .anim header.";

                return AssetResult::OutOfMemory;
            }

            for (
                std::size_t boneIndex = 0U;
                boneIndex <
                    skeleton.bones.size();
                ++boneIndex)
            {
                const FbxSkeletonBone& bone =
                    skeleton.bones[boneIndex];

                const std::int32_t
                    skeletonBoneIndex =
                        static_cast<std::int32_t>(
                            boneIndex);

                const std::uint32_t flags =
                    bone.parentIndex < 0
                        ? (1U << 1U)
                        : 0U;

                const std::uint32_t keyCount =
                    static_cast<std::uint32_t>(
                        keysByBone[
                            boneIndex].size());

                if (
                    !writer.WriteString(
                        bone.name) ||
                    !writer.Write(
                        skeletonBoneIndex) ||
                    !writer.Write(flags) ||
                    !writer.Write(keyCount))
                {
                    error =
                        L"Failed to encode .anim track.";

                    return AssetResult::OutOfMemory;
                }

                for (
                    const FbxAnimationKey& key :
                    keysByBone[boneIndex])
                {
                    for (
                        const float value :
                        key.rotation)
                    {
                        if (!writer.Write(value))
                        {
                            return AssetResult::
                                OutOfMemory;
                        }
                    }

                    for (
                        const float value :
                        key.translation)
                    {
                        if (!writer.Write(value))
                        {
                            return AssetResult::
                                OutOfMemory;
                        }
                    }
                }
            }

            return
                fbx_detail::SaveBinary(
                    destination,
                    writer.GetBytes(),
                    overwrite,
                    error);
        }
    }

    bool FbxAnimationsImporter::
        IsSupportedSource(
            const std::filesystem::path&
                sourcePath) noexcept
    {
        try
        {
            return
                fbx_detail::IsSupportedSource(
                    sourcePath);
        }
        catch (...)
        {
            return false;
        }
    }

    AssetResult FbxAnimationsImporter::Import(
        const std::filesystem::path& sourcePath,
        const FbxAnimationImportOptions& options,
        FbxImportReport& report,
        std::wstring& error) noexcept
    {
        report.Clear();
        error.clear();

        try
        {
            if (
                options.destinationDirectory.empty() ||
                options.skeletonFile.empty())
            {
                error =
                    L"Animation destination and "
                    L".skeleton file are required.";

                return AssetResult::InvalidArgument;
            }

            std::error_code filesystemError;

            if (
                !std::filesystem::is_regular_file(
                    options.skeletonFile,
                    filesystemError) ||
                filesystemError)
            {
                error =
                    L"Selected .skeleton file "
                    L"does not exist.";

                return AssetResult::NotFound;
            }

            std::string skeletonAssetPath;

            AssetResult result =
                fbx_detail::MakeAssetPath(
                    options.skeletonFile,
                    skeletonAssetPath,
                    error);

            if (Failed(result))
            {
                return result;
            }

            fbx_detail::ScenePtr scene;

            result =
                fbx_detail::LoadScene(
                    sourcePath,
                    scene,
                    error);

            if (Failed(result))
            {
                return result;
            }

            FbxSkeletonData skeleton;

            std::vector<const ufbx_node*>
                boneNodes;

            std::unordered_map<
                const ufbx_node*,
                std::uint32_t>
                boneIndices;

            result =
                fbx_detail::BuildSkeleton(
                    *scene,
                    skeleton,
                    boneNodes,
                    boneIndices,
                    error);

            if (Failed(result))
            {
                return result;
            }

            std::unordered_map<
                std::string,
                std::size_t>
                usedNames;

            for (
                std::size_t stackIndex = 0U;
                stackIndex <
                    scene->anim_stacks.count;
                ++stackIndex)
            {
                const ufbx_anim_stack* const stack =
                    scene->anim_stacks.data[
                        stackIndex];

                if (
                    stack == nullptr ||
                    stack->anim == nullptr)
                {
                    continue;
                }

                const std::filesystem::path
                    destination =
                        MakeClipPath(
                            options.
                                destinationDirectory,
                            *stack,
                            usedNames);

                result =
                    WriteAnimation(
                        *scene,
                        *stack,
                        skeleton,
                        boneNodes,
                        skeletonAssetPath,
                        options.sampleRate,
                        destination,
                        options.
                            overwriteExisting,
                        report,
                        error);

                if (Failed(result))
                {
                    report.Clear();
                    return result;
                }

                report.writtenFiles.push_back(
                    destination);
            }

            if (report.writtenFiles.empty())
            {
                error =
                    L"FBX contains no animation clips.";

                return AssetResult::
                    UnsupportedFeature;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            report.Clear();

            error =
                L"Not enough memory to import "
                L"animations.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            report.Clear();

            error =
                L"Unexpected animation import failure.";

            return AssetResult::InternalError;
        }
    }
}