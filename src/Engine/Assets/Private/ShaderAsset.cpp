#include "Assets/ShaderAsset.h"

#include <utility>

namespace engine::assets
{
namespace
{
bool HasNull(const std::string& value) noexcept { return value.find('\0') != std::string::npos; }
bool ProfileMatches(const engine::graphics::ShaderStage stage, const std::string& profile) noexcept
{
    const char* prefix = nullptr;
    switch (stage)
    {
    case engine::graphics::ShaderStage::Vertex: prefix = "vs_"; break;
    case engine::graphics::ShaderStage::Pixel: prefix = "ps_"; break;
    case engine::graphics::ShaderStage::Geometry: prefix = "gs_"; break;
    case engine::graphics::ShaderStage::Hull: prefix = "hs_"; break;
    case engine::graphics::ShaderStage::Domain: prefix = "ds_"; break;
    case engine::graphics::ShaderStage::Compute: prefix = "cs_"; break;
    default: return false;
    }
    return profile.size() >= 5U && profile.compare(0U, 3U, prefix) == 0;
}
bool Valid(const ShaderAssetDesc& desc) noexcept
{
    return ProfileMatches(desc.stage, desc.targetProfile) && !desc.bytecode.empty() &&
        desc.bytecode.size() <= ShaderAsset::MaximumBytecodeSize && !desc.entryPoint.empty() &&
        desc.targetProfile.size() <= ShaderAsset::MaximumMetadataLength &&
        desc.entryPoint.size() <= ShaderAsset::MaximumMetadataLength &&
        desc.debugName.size() <= ShaderAsset::MaximumMetadataLength && !HasNull(desc.targetProfile) &&
        !HasNull(desc.entryPoint) && !HasNull(desc.debugName) && desc.sourceHash != 0U;
}
}
AssetResult ShaderAsset::Initialize(ShaderAssetDesc desc) noexcept
{
    if (!Valid(desc)) return AssetResult::InvalidArgument;
    Clear(); desc_ = std::move(desc); initialized_ = true; return AssetResult::Success;
}
void ShaderAsset::Clear() noexcept { desc_ = ShaderAssetDesc{}; initialized_ = false; }
bool ShaderAsset::IsValid() const noexcept { return initialized_ && Valid(desc_); }
}
