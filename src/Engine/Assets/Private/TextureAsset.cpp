#include "Assets/TextureAsset.h"

#include <limits>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] bool TryAdd(
            const std::size_t left,
            const std::size_t right,
            std::size_t& outValue) noexcept
        {
            if (
                left >
                (std::numeric_limits<std::size_t>::max)() -
                    right
            )
            {
                outValue = 0U;
                return false;
            }

            outValue = left + right;
            return true;
        }

        [[nodiscard]] bool TryMultiply(
            const std::size_t left,
            const std::size_t right,
            std::size_t& outValue) noexcept
        {
            if (left == 0U || right == 0U)
            {
                outValue = 0U;
                return true;
            }

            if (
                left >
                (std::numeric_limits<std::size_t>::max)() /
                    right
            )
            {
                outValue = 0U;
                return false;
            }

            outValue = left * right;
            return true;
        }
    }

    bool TextureSubresourceLayout::IsValid(
        const std::size_t totalDataSize) const noexcept
    {
        if (
            dataSize == 0U ||
            rowPitch == 0U ||
            slicePitch == 0U ||
            slicePitch < rowPitch
        )
        {
            return false;
        }

        std::size_t endOffset = 0U;

        if (!TryAdd(offset, dataSize, endOffset))
        {
            return false;
        }

        return endOffset <= totalDataSize;
    }

    void TextureAsset::Clear() noexcept
    {
        desc_ = engine::graphics::TextureDesc{};

        bytes_.clear();
        subresources_.clear();
    }

    bool TextureAsset::IsValid() const noexcept
    {
        if (
            !desc_.IsValid() ||
            bytes_.empty() ||
            subresources_.empty()
        )
        {
            return false;
        }

        std::size_t expectedSubresourceCount = 0U;

        if (
            desc_.dimension ==
            engine::graphics::TextureDimension::Texture3D
        )
        {
            expectedSubresourceCount =
                static_cast<std::size_t>(
                    desc_.mipLevels);
        }
        else
        {
            if (
                !TryMultiply(
                    static_cast<std::size_t>(
                        desc_.mipLevels),
                    static_cast<std::size_t>(
                        desc_.arrayLayers),
                    expectedSubresourceCount)
            )
            {
                return false;
            }
        }

        if (
            subresources_.size() !=
            expectedSubresourceCount
        )
        {
            return false;
        }

        for (const TextureSubresourceLayout& layout :
             subresources_)
        {
            if (!layout.IsValid(bytes_.size()))
            {
                return false;
            }
        }

        return true;
    }

    const engine::graphics::TextureDesc&
    TextureAsset::GetDesc() const noexcept
    {
        return desc_;
    }

    std::size_t TextureAsset::GetDataSize() const noexcept
    {
        return bytes_.size();
    }

    std::size_t
    TextureAsset::GetSubresourceCount() const noexcept
    {
        return subresources_.size();
    }

    const TextureSubresourceLayout*
    TextureAsset::GetSubresourceLayout(
        const std::size_t index) const noexcept
    {
        if (index >= subresources_.size())
        {
            return nullptr;
        }

        return &subresources_[index];
    }

    AssetResult TextureAsset::GetSubresourceData(
        const std::size_t index,
        engine::graphics::TextureSubresourceData&
            outData) const noexcept
    {
        outData =
            engine::graphics::TextureSubresourceData{};

        if (index >= subresources_.size())
        {
            return AssetResult::InvalidArgument;
        }

        const TextureSubresourceLayout& layout =
            subresources_[index];

        if (!layout.IsValid(bytes_.size()))
        {
            return AssetResult::CorruptData;
        }

        outData.data =
            bytes_.data() + layout.offset;

        outData.dataSize = layout.dataSize;
        outData.rowPitch = layout.rowPitch;
        outData.slicePitch = layout.slicePitch;

        return outData.IsValid()
            ? AssetResult::Success
            : AssetResult::CorruptData;
    }
}