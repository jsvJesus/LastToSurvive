#pragma once

#include "Assets/AssetData.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetResult.h"

namespace engine::assets
{
    class AssetSource
    {
    public:
        virtual ~AssetSource() noexcept = default;

        AssetSource(const AssetSource&) = delete;
        AssetSource& operator=(const AssetSource&) = delete;

        AssetSource(AssetSource&&) = delete;
        AssetSource& operator=(AssetSource&&) = delete;

        [[nodiscard]] virtual AssetResult Read(
            const AssetPath& path,
            AssetData& outData) noexcept = 0;

        [[nodiscard]] virtual bool Exists(
            const AssetPath& path) const noexcept = 0;

    protected:
        AssetSource() noexcept = default;
    };
}