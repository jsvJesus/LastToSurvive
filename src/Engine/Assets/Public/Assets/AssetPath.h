#pragma once

#include "Assets/AssetId.h"
#include "Assets/AssetResult.h"

#include <string>
#include <string_view>

namespace engine::assets
{
    class AssetPath final
    {
    public:
        static constexpr std::size_t MaximumLength = 1024U;

        AssetPath() = default;

        [[nodiscard]] static AssetResult TryCreate(
            std::string_view source,
            AssetPath& outPath) noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] const std::string& String() const noexcept;

        [[nodiscard]] std::string_view View() const noexcept;

        [[nodiscard]] AssetId GetId() const noexcept;

        void Clear() noexcept;

        friend bool operator==(
            const AssetPath& left,
            const AssetPath& right) noexcept
        {
            return left.value_ == right.value_;
        }

        friend bool operator!=(
            const AssetPath& left,
            const AssetPath& right) noexcept
        {
            return !(left == right);
        }

        friend bool operator<(
            const AssetPath& left,
            const AssetPath& right) noexcept
        {
            return left.value_ < right.value_;
        }

    private:
        std::string value_;
    };
}