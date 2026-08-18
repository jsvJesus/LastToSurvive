#include "Assets/AnimationAssetLoader.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace engine::assets
{
    namespace
    {
        constexpr std::uint32_t AssetEndianMarker =
            0x01020304U;

        constexpr std::uint32_t
            MaximumFrameCount =
                1000000U;

        constexpr std::uint64_t
            MaximumTotalKeyCount =
                50000000ULL;

        constexpr std::uint32_t
            MaximumStringLength =
                1024U * 1024U;

        class BinaryReader final
        {
        public:
            BinaryReader(
                const std::byte* const bytes,
                const std::size_t size) noexcept :
                bytes_(bytes),
                size_(size)
            {
            }

            template<typename Value>
            [[nodiscard]]
            bool Read(Value& output) noexcept
            {
                static_assert(
                    std::is_trivially_copyable_v<Value>);

                return ReadBytes(
                    &output,
                    sizeof(Value));
            }

            [[nodiscard]]
            bool ReadBytes(
                void* const destination,
                const std::size_t size) noexcept
            {
                if (size == 0U)
                {
                    return true;
                }

                if (
                    destination == nullptr ||
                    bytes_ == nullptr ||
                    offset_ > size_ ||
                    size > size_ - offset_)
                {
                    return false;
                }

                std::memcpy(
                    destination,
                    bytes_ + offset_,
                    size);

                offset_ += size;
                return true;
            }

            [[nodiscard]]
            bool ReadString(
                std::string& output) noexcept
            {
                std::uint32_t length = 0U;

                if (
                    !Read(length) ||
                    length > MaximumStringLength ||
                    offset_ > size_ ||
                    length > size_ - offset_)
                {
                    return false;
                }

                try
                {
                    output.assign(
                        reinterpret_cast<const char*>(
                            bytes_ + offset_),
                        static_cast<std::size_t>(
                            length));
                }
                catch (...)
                {
                    return false;
                }

                offset_ +=
                    static_cast<std::size_t>(
                        length);

                return true;
            }

            [[nodiscard]]
            std::size_t GetRemainingSize() const noexcept
            {
                return offset_ <= size_
                    ? size_ - offset_
                    : 0U;
            }

            [[nodiscard]]
            bool IsAtEnd() const noexcept
            {
                return offset_ == size_;
            }

        private:
            const std::byte* bytes_ = nullptr;

            std::size_t size_ = 0U;
            std::size_t offset_ = 0U;
        };
    }

    AssetResult AnimationAssetLoader::Load(
        const AssetMetadata& metadata,
        const AssetData& source,
        std::unique_ptr<LoadedAsset>&
            outAsset) noexcept
    {
        outAsset.reset();

        if (!metadata.IsValid())
        {
            return AssetResult::InvalidMetadata;
        }

        if (
            metadata.type !=
            AssetType::Animation)
        {
            return AssetResult::TypeMismatch;
        }

        constexpr std::size_t MinimumFileSize =
            40U;

        if (
            source.GetData() == nullptr ||
            source.GetSize() < MinimumFileSize)
        {
            return AssetResult::CorruptData;
        }

        BinaryReader reader(
            source.GetData(),
            source.GetSize());

        constexpr std::array<char, 8U>
            ExpectedMagic
            {{
                'A', 'N', 'I', 'M',
                'C', 'L', 'I', 'P'
            }};

        std::array<char, 8U> magic{};

        if (
            !reader.ReadBytes(
                magic.data(),
                magic.size()) ||
            magic != ExpectedMagic)
        {
            return AssetResult::UnsupportedFormat;
        }

        std::uint32_t version = 0U;
        std::uint32_t endianMarker = 0U;
        std::uint32_t trackCount = 0U;

        AnimationAsset animation;

        if (
            !reader.Read(version) ||
            !reader.Read(endianMarker) ||
            !reader.ReadString(
                animation.skeletonAssetPath_) ||
            !reader.Read(
                animation.skeletonId_) ||
            !reader.Read(
                animation.frameCount_) ||
            !reader.Read(
                animation.frameRate_) ||
            !reader.Read(
                animation.durationSeconds_) ||
            !reader.Read(trackCount))
        {
            return AssetResult::CorruptData;
        }

        if (version != 1U)
        {
            return AssetResult::UnsupportedFormat;
        }

        if (endianMarker != AssetEndianMarker)
        {
            return AssetResult::CorruptData;
        }

        if (
            animation.skeletonAssetPath_.empty() ||
            animation.frameCount_ == 0U ||
            animation.frameCount_ >
                MaximumFrameCount ||
            !std::isfinite(
                animation.frameRate_) ||
            animation.frameRate_ <= 0.0F ||
            !std::isfinite(
                animation.durationSeconds_) ||
            animation.durationSeconds_ < 0.0F ||
            trackCount == 0U ||
            trackCount >
                MaximumSkeletonBones)
        {
            return AssetResult::CorruptData;
        }

        try
        {
            animation.tracks_.resize(
                trackCount);

            animation.debugName_ =
                metadata.path.String();
        }
        catch (const std::bad_alloc&)
        {
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            return AssetResult::InternalError;
        }

        std::uint64_t totalKeyCount = 0U;

        for (
            std::size_t trackIndex = 0U;
            trackIndex <
                animation.tracks_.size();
            ++trackIndex)
        {
            AnimationTrack& track =
                animation.tracks_[trackIndex];

            std::uint32_t keyCount = 0U;

            if (
                !reader.ReadString(
                    track.boneName) ||
                !reader.Read(
                    track.skeletonBoneIndex) ||
                !reader.Read(track.flags) ||
                !reader.Read(keyCount))
            {
                return AssetResult::CorruptData;
            }

            if (
                track.boneName.empty() ||
                track.skeletonBoneIndex < 0 ||
                static_cast<std::size_t>(
                    track.skeletonBoneIndex) >=
                    MaximumSkeletonBones ||
                keyCount !=
                    animation.frameCount_)
            {
                return AssetResult::CorruptData;
            }

            const std::size_t boneIndex =
                static_cast<std::size_t>(
                    track.skeletonBoneIndex);

            if (
                animation.boneToTrack_[
                    boneIndex] >= 0)
            {
                return AssetResult::CorruptData;
            }

            if (
                totalKeyCount >
                MaximumTotalKeyCount -
                    keyCount)
            {
                return AssetResult::FileTooLarge;
            }

            totalKeyCount += keyCount;

            const std::size_t requiredBytes =
                static_cast<std::size_t>(
                    keyCount) *
                sizeof(AnimationKey);

            if (
                requiredBytes >
                reader.GetRemainingSize())
            {
                return AssetResult::CorruptData;
            }

            try
            {
                track.keys.resize(keyCount);
            }
            catch (const std::bad_alloc&)
            {
                return AssetResult::OutOfMemory;
            }
            catch (...)
            {
                return AssetResult::InternalError;
            }

            for (
                AnimationKey& key :
                track.keys)
            {
                for (
                    float& value :
                    key.rotation)
                {
                    if (!reader.Read(value))
                    {
                        return AssetResult::CorruptData;
                    }
                }

                for (
                    float& value :
                    key.translation)
                {
                    if (!reader.Read(value))
                    {
                        return AssetResult::CorruptData;
                    }
                }
            }

            animation.boneToTrack_[boneIndex] =
                static_cast<std::int32_t>(
                    trackIndex);
        }

        if (
            !reader.IsAtEnd() ||
            !animation.IsValid())
        {
            return AssetResult::CorruptData;
        }

        try
        {
            outAsset =
                std::make_unique<
                    AnimationLoadedAsset>(
                        std::move(animation));
        }
        catch (const std::bad_alloc&)
        {
            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            return AssetResult::InternalError;
        }

        return AssetResult::Success;
    }
}