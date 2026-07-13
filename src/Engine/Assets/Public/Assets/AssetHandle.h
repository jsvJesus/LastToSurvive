#pragma once

#include <cstdint>
#include <type_traits>

namespace engine::assets
{
    class AssetHandle final
    {
    public:
        using ValueType = std::uint64_t;

        constexpr AssetHandle() noexcept = default;

        [[nodiscard]] static constexpr AssetHandle FromParts(
            const std::uint32_t index,
            const std::uint32_t generation) noexcept
        {
            if (index == 0U || generation == 0U)
            {
                return AssetHandle{};
            }

            return AssetHandle(
                (static_cast<ValueType>(generation) << 32U) |
                static_cast<ValueType>(index));
        }

        [[nodiscard]] static constexpr AssetHandle FromValue(
            const ValueType value) noexcept
        {
            return AssetHandle(value);
        }

        [[nodiscard]] constexpr ValueType Value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] constexpr std::uint32_t Index() const noexcept
        {
            return static_cast<std::uint32_t>(
                value_ & 0xFFFFFFFFULL);
        }

        [[nodiscard]] constexpr std::uint32_t Generation() const noexcept
        {
            return static_cast<std::uint32_t>(
                value_ >> 32U);
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return
                Index() != 0U &&
                Generation() != 0U;
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept
        {
            return IsValid();
        }

        friend constexpr bool operator==(
            const AssetHandle left,
            const AssetHandle right) noexcept
        {
            return left.value_ == right.value_;
        }

        friend constexpr bool operator!=(
            const AssetHandle left,
            const AssetHandle right) noexcept
        {
            return !(left == right);
        }

    private:
        explicit constexpr AssetHandle(
            const ValueType value) noexcept
            : value_(value)
        {
        }

        ValueType value_ = 0U;
    };

    static_assert(
        std::is_trivially_copyable_v<AssetHandle>);

    static_assert(
        sizeof(AssetHandle) == sizeof(std::uint64_t));
}