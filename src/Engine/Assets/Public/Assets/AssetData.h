#pragma once

#include "Assets/AssetResult.h"

#include <cstddef>
#include <vector>

namespace engine::assets
{
    class AssetData final
    {
    public:
        AssetData() = default;

        AssetData(const AssetData&) = delete;
        AssetData& operator=(const AssetData&) = delete;

        AssetData(AssetData&&) noexcept = default;
        AssetData& operator=(AssetData&&) noexcept = default;

        [[nodiscard]] AssetResult Resize(
            std::size_t size) noexcept;

        void Clear() noexcept;

        void Swap(
            AssetData& other) noexcept;

        [[nodiscard]] bool IsEmpty() const noexcept;

        [[nodiscard]] std::size_t GetSize() const noexcept;

        [[nodiscard]] std::byte* GetData() noexcept;

        [[nodiscard]] const std::byte* GetData() const noexcept;

    private:
        std::vector<std::byte> bytes_;
    };
}