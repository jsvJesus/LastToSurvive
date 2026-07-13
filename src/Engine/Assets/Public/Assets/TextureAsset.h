#pragma once

#include "Assets/AssetResult.h"

#include "Graphics/Texture.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace engine::assets
{
    struct TextureSubresourceLayout final
    {
        std::uint32_t mipLevel = 0U;
        std::uint32_t arrayLayer = 0U;

        std::size_t offset = 0U;
        std::size_t dataSize = 0U;
        std::size_t rowPitch = 0U;
        std::size_t slicePitch = 0U;

        [[nodiscard]] bool IsValid(
            std::size_t totalDataSize) const noexcept;
    };

    class TextureAsset final
    {
    public:
        TextureAsset() = default;

        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;

        TextureAsset(TextureAsset&&) noexcept = default;
        TextureAsset& operator=(TextureAsset&&) noexcept = default;

        void Clear() noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] const engine::graphics::TextureDesc&
            GetDesc() const noexcept;

        [[nodiscard]] std::size_t
            GetDataSize() const noexcept;

        [[nodiscard]] std::size_t
            GetSubresourceCount() const noexcept;

        [[nodiscard]] const TextureSubresourceLayout*
            GetSubresourceLayout(
                std::size_t index) const noexcept;

        [[nodiscard]] AssetResult GetSubresourceData(
            std::size_t index,
            engine::graphics::TextureSubresourceData&
                outData) const noexcept;

    private:
        friend class DdsTextureDecoder;

        engine::graphics::TextureDesc desc_;

        std::vector<std::byte> bytes_;

        std::vector<TextureSubresourceLayout>
            subresources_;
    };
}