#include "Assets/SkeletonAssetLoader.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
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

        constexpr std::uint32_t MaximumStringLength =
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

    AssetResult SkeletonAssetLoader::Load(
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

        if (metadata.type != AssetType::Skeleton)
        {
            return AssetResult::TypeMismatch;
        }

        constexpr std::size_t MinimumFileSize =
            24U;

        if (
            source.GetData() == nullptr ||
            source.GetSize() < MinimumFileSize)
        {
            return AssetResult::CorruptData;
        }

        BinaryReader reader(
            source.GetData(),
            source.GetSize());

        constexpr char ExpectedMagic[8]
        {
            'S', 'K', 'E', 'L',
            'E', 'T', 'O', 'N'
        };

        char magic[8]{};

        if (
            !reader.ReadBytes(
                magic,
                sizeof(magic)) ||
            std::memcmp(
                magic,
                ExpectedMagic,
                sizeof(magic)) != 0)
        {
            return AssetResult::UnsupportedFormat;
        }

        std::uint32_t version = 0U;
        std::uint32_t endianMarker = 0U;
        std::uint32_t boneCount = 0U;

        SkeletonAsset skeleton;

        if (
            !reader.Read(version) ||
            !reader.Read(endianMarker) ||
            !reader.Read(skeleton.skeletonId_) ||
            !reader.Read(boneCount))
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
            boneCount == 0U ||
            boneCount > MaximumSkeletonBones)
        {
            return AssetResult::UnsupportedFeature;
        }

        /*
         * Минимальный размер одной записи:
         *
         * string length     4
         * parent index      4
         * length            4
         * matrix           64
         */
        constexpr std::size_t MinimumBoneSize =
            76U;

        if (
            reader.GetRemainingSize() <
            static_cast<std::size_t>(
                boneCount) *
                MinimumBoneSize)
        {
            return AssetResult::CorruptData;
        }

        try
        {
            skeleton.bones_.resize(
                boneCount);

            skeleton.debugName_ =
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

        for (
            SkeletonBone& bone :
            skeleton.bones_)
        {
            if (
                !reader.ReadString(
                    bone.name) ||
                !reader.Read(
                    bone.parentIndex) ||
                !reader.Read(
                    bone.length))
            {
                return AssetResult::CorruptData;
            }

            for (
                float& value :
                bone.absoluteBindMatrix)
            {
                if (!reader.Read(value))
                {
                    return AssetResult::CorruptData;
                }
            }
        }

        if (
            !reader.IsAtEnd() ||
            !skeleton.IsValid())
        {
            return AssetResult::CorruptData;
        }

        try
        {
            outAsset =
                std::make_unique<
                    SkeletonLoadedAsset>(
                        std::move(skeleton));
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