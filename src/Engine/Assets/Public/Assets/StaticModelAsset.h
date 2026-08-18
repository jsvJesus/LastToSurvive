#pragma once

#include "Assets/AssetPath.h"

#include <cstddef>
#include <string>
#include <vector>

namespace engine::assets
{
    class StaticModelAsset final
    {
    public:
        static constexpr std::size_t MaximumMaterialCount = 256U;
        static constexpr std::size_t MaximumDebugNameLength = 1024U;

        [[nodiscard]] AssetResult Initialize(
            AssetPath meshPath,
            std::vector<AssetPath> materialPaths,
            std::string debugName = {}) noexcept;
        void Clear() noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] const AssetPath& GetMeshPath() const noexcept { return meshPath_; }
        [[nodiscard]] std::size_t GetMaterialCount() const noexcept { return materialPaths_.size(); }
        [[nodiscard]] const AssetPath& GetMaterialPath(std::size_t index) const noexcept;
        [[nodiscard]] const std::string& GetDebugName() const noexcept { return debugName_; }

    private:
        AssetPath meshPath_;
        std::vector<AssetPath> materialPaths_;
        std::string debugName_;
        bool initialized_ = false;
    };
}
