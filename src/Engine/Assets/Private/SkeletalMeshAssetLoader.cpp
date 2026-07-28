#include "Assets/SkeletalMeshAssetLoader.h"

#include <algorithm>
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
            MaximumVertexCount =
                10000000U;

        constexpr std::uint32_t
            MaximumIndexCount =
                30000000U;

        constexpr std::uint32_t
            MaximumSectionCount =
                65536U;

        constexpr std::uint32_t
            MaximumStringLength =
                16U * 1024U * 1024U;

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
                void* const output,
                const std::size_t byteCount) noexcept
            {
                if (byteCount == 0U)
                {
                    return true;
                }

                if (
                    output == nullptr ||
                    bytes_ == nullptr ||
                    offset_ > size_ ||
                    byteCount > size_ - offset_)
                {
                    return false;
                }

                std::memcpy(
                    output,
                    bytes_ + offset_,
                    byteCount);

                offset_ += byteCount;
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
                catch (const std::bad_alloc&)
                {
                    return false;
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
            std::size_t
                GetRemainingSize() const noexcept
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

        [[nodiscard]]
        bool CheckedMultiply(
            const std::uint64_t left,
            const std::uint64_t right,
            std::uint64_t& result) noexcept
        {
            if (
                right != 0U &&
                left >
                    (std::numeric_limits<
                        std::uint64_t>::max)() /
                        right)
            {
                return false;
            }

            result = left * right;
            return true;
        }

        [[nodiscard]]
        bool CheckedAdd(
            const std::uint64_t left,
            const std::uint64_t right,
            std::uint64_t& result) noexcept
        {
            if (
                left >
                (std::numeric_limits<
                    std::uint64_t>::max)() -
                    right)
            {
                return false;
            }

            result = left + right;
            return true;
        }

        [[nodiscard]]
        bool ReadFloatArray3(
            BinaryReader& reader,
            std::array<float, 3U>& values) noexcept
        {
            for (float& value : values)
            {
                if (!reader.Read(value))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool IsFiniteArray3(
            const std::array<float, 3U>& values) noexcept
        {
            for (const float value : values)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            return true;
        }
    }

    AssetResult SkeletalMeshAssetLoader::Load(
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
            AssetType::SkeletalMesh)
        {
            return AssetResult::TypeMismatch;
        }

        /*
         * Минимум:
         *
         * magic              8
         * version            4
         * endian             4
         * skeleton length    4
         * counts             12
         * pivot/min/max      36
         */
        constexpr std::size_t
            MinimumFileSize =
                68U;

        if (
            source.GetData() == nullptr ||
            source.GetSize() < MinimumFileSize)
        {
            return AssetResult::CorruptData;
        }

        BinaryReader reader(
            source.GetData(),
            source.GetSize());

        constexpr std::array<char, 8U> ExpectedMagic
        {{
            'S', 'K', 'M', 'E',
            'S', 'H', '\0', '\0'
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

        if (
            !reader.Read(version) ||
            !reader.Read(endianMarker))
        {
            return AssetResult::CorruptData;
        }

        if (version != 1U)
        {
            return AssetResult::UnsupportedFormat;
        }

        if (
            endianMarker !=
            AssetEndianMarker)
        {
            return AssetResult::CorruptData;
        }

        SkeletalMeshAsset mesh;

        if (!reader.ReadString(
                mesh.skeletonAssetPath_))
        {
            return AssetResult::CorruptData;
        }

        std::uint32_t vertexCount = 0U;
        std::uint32_t indexCount = 0U;
        std::uint32_t sectionCount = 0U;

        if (
            !reader.Read(vertexCount) ||
            !reader.Read(indexCount) ||
            !reader.Read(sectionCount))
        {
            return AssetResult::CorruptData;
        }

        if (
            mesh.skeletonAssetPath_.empty() ||
            vertexCount == 0U ||
            vertexCount > MaximumVertexCount ||
            indexCount == 0U ||
            indexCount > MaximumIndexCount ||
            sectionCount == 0U ||
            sectionCount > MaximumSectionCount)
        {
            return AssetResult::CorruptData;
        }

        /*
         * Проверяем минимально необходимый размер до
         * выделения памяти.
         *
         * Каждая секция содержит минимум:
         *
         * firstIndex     4
         * indexCount     4
         * materialSlot   4
         * string length  4
         */
        std::uint64_t vertexBytes = 0U;
        std::uint64_t indexBytes = 0U;
        std::uint64_t sectionMinimumBytes = 0U;
        std::uint64_t minimumRemainingBytes = 36U;

        if (
            !CheckedMultiply(
                vertexCount,
                sizeof(SkeletalMeshVertex),
                vertexBytes) ||
            !CheckedMultiply(
                indexCount,
                sizeof(std::uint32_t),
                indexBytes) ||
            !CheckedMultiply(
                sectionCount,
                16U,
                sectionMinimumBytes) ||
            !CheckedAdd(
                minimumRemainingBytes,
                vertexBytes,
                minimumRemainingBytes) ||
            !CheckedAdd(
                minimumRemainingBytes,
                indexBytes,
                minimumRemainingBytes) ||
            !CheckedAdd(
                minimumRemainingBytes,
                sectionMinimumBytes,
                minimumRemainingBytes) ||
            minimumRemainingBytes >
                reader.GetRemainingSize())
        {
            return AssetResult::CorruptData;
        }

        std::array<float, 3U> minimum{};
        std::array<float, 3U> maximum{};

        if (
            !ReadFloatArray3(
                reader,
                mesh.pivot_) ||
            !ReadFloatArray3(
                reader,
                minimum) ||
            !ReadFloatArray3(
                reader,
                maximum) ||
            !IsFiniteArray3(
                mesh.pivot_) ||
            !IsFiniteArray3(
                minimum) ||
            !IsFiniteArray3(
                maximum))
        {
            return AssetResult::CorruptData;
        }

        for (std::size_t axis = 0U;
             axis < 3U;
             ++axis)
        {
            if (
                minimum[axis] >
                maximum[axis])
            {
                return AssetResult::CorruptData;
            }
        }

        try
        {
            mesh.vertices_.resize(
                vertexCount);

            mesh.indices_.resize(
                indexCount);

            mesh.sections_.resize(
                sectionCount);

            mesh.debugName_ =
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

        if (
            !reader.ReadBytes(
                mesh.vertices_.data(),
                static_cast<std::size_t>(
                    vertexBytes)) ||
            !reader.ReadBytes(
                mesh.indices_.data(),
                static_cast<std::size_t>(
                    indexBytes)))
        {
            return AssetResult::CorruptData;
        }

        std::uint32_t maximumMaterialSlot = 0U;

        for (std::size_t sectionIndex = 0U;
             sectionIndex <
                mesh.sections_.size();
             ++sectionIndex)
        {
            SkeletalMeshSection& section =
                mesh.sections_[sectionIndex];

            if (
                !reader.Read(
                    section.firstIndex) ||
                !reader.Read(
                    section.indexCount) ||
                !reader.Read(
                    section.materialSlot) ||
                !reader.ReadString(
                    section.materialAssetPath))
            {
                return AssetResult::CorruptData;
            }

            if (
                section.materialSlot >=
                MaximumSectionCount)
            {
                return AssetResult::CorruptData;
            }

            maximumMaterialSlot =
                (std::max)(
                    maximumMaterialSlot,
                    section.materialSlot);
        }

        if (!reader.IsAtEnd())
        {
            return AssetResult::CorruptData;
        }

        mesh.materialSlotCount_ =
            maximumMaterialSlot + 1U;

        mesh.bounds_.minimum = minimum;
        mesh.bounds_.maximum = maximum;

        float radiusSquared = 0.0F;

        for (std::size_t axis = 0U;
             axis < 3U;
             ++axis)
        {
            mesh.bounds_.sphereCenter[axis] =
                (
                    minimum[axis] +
                    maximum[axis]
                ) *
                0.5F;

            const float extent =
                (
                    maximum[axis] -
                    minimum[axis]
                ) *
                0.5F;

            radiusSquared +=
                extent * extent;
        }

        mesh.bounds_.sphereRadius =
            std::sqrt(radiusSquared);

        if (!mesh.IsValid())
        {
            return AssetResult::CorruptData;
        }

        try
        {
            outAsset =
                std::make_unique<
                    SkeletalMeshLoadedAsset>(
                        std::move(mesh));
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