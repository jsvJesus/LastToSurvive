#pragma once

#include <cstdint>
#include <string_view>

namespace engine::assets
{
    class AssetId final
    {
    public:
        using ValueType = std::uint64_t;

        constexpr AssetId() noexcept = default;

        [[nodiscard]] static constexpr AssetId FromValue(
            const ValueType value) noexcept
        {
            return AssetId(value);
        }

        // Использовать только с уже нормализованным AssetPath.
        [[nodiscard]] static AssetId FromNormalizedPath(
            std::string_view normalizedPath) noexcept;

        [[nodiscard]] constexpr ValueType Value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return value_ != 0U;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return IsValid();
        }

        friend constexpr bool operator==(
            const AssetId left,
            const AssetId right) noexcept
        {
            return left.value_ == right.value_;
        }

        friend constexpr bool operator!=(
            const AssetId left,
            const AssetId right) noexcept
        {
            return !(left == right);
        }

        friend constexpr bool operator<(
            const AssetId left,
            const AssetId right) noexcept
        {
            return left.value_ < right.value_;
        }

    private:
        explicit constexpr AssetId(
            const ValueType value) noexcept
            : value_(value)
        {
        }

        ValueType value_ = 0U;
    };
}