#include "Assets/StaticModelAsset.h"

#include <string_view>

namespace engine::assets
{
namespace
{
bool HasSuffix(const AssetPath& path, const std::string_view suffix) noexcept
{
    const auto value = path.View();
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}
}

AssetResult StaticModelAsset::Initialize(
    AssetPath meshPath,
    std::vector<AssetPath> materialPaths,
    std::string debugName) noexcept
{
    Clear();
    if (!meshPath.IsValid() ||
        !HasSuffix(meshPath, ".sm") ||
        materialPaths.empty() ||
        materialPaths.size() > MaximumMaterialCount ||
        debugName.size() > MaximumDebugNameLength)
    {
        return AssetResult::InvalidArgument;
    }
    for (const AssetPath& path : materialPaths)
        if (!path.IsValid() || !HasSuffix(path, ".material"))
            return AssetResult::InvalidArgument;
    meshPath_ = std::move(meshPath);
    materialPaths_ = std::move(materialPaths);
    debugName_ = std::move(debugName);
    initialized_ = true;
    return AssetResult::Success;
}

void StaticModelAsset::Clear() noexcept
{
    meshPath_.Clear();
    materialPaths_.clear();
    debugName_.clear();
    initialized_ = false;
}

bool StaticModelAsset::IsValid() const noexcept
{
    return initialized_ && meshPath_.IsValid() && !materialPaths_.empty();
}

const AssetPath& StaticModelAsset::GetMaterialPath(const std::size_t index) const noexcept
{
    static const AssetPath empty;
    return index < materialPaths_.size() ? materialPaths_[index] : empty;
}
}
