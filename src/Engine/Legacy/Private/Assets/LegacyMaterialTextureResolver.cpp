#include "Legacy/Assets/LegacyMaterialTextureResolver.h"

#include <cwchar>
#include <system_error>

namespace engine::legacy::assets
{
namespace
{
bool EqualPath(const std::filesystem::path& left, const std::filesystem::path& right) noexcept
{ return _wcsicmp(left.lexically_normal().c_str(), right.lexically_normal().c_str()) == 0; }
bool Contained(const std::filesystem::path& root, const std::filesystem::path& candidate) noexcept
{
    const auto r = root.lexically_normal(); const auto c = candidate.lexically_normal();
    auto ri = r.begin(); auto ci = c.begin();
    for (; ri != r.end(); ++ri, ++ci) if (ci == c.end() || _wcsicmp(ri->c_str(), ci->c_str()) != 0) return false;
    return true;
}
void Add(std::vector<std::filesystem::path>& values, const std::filesystem::path& value)
{
    const auto normalized = value.lexically_normal();
    for (const auto& existing : values) if (EqualPath(existing, normalized)) return;
    values.push_back(normalized);
}
bool HasParentTraversal(const std::filesystem::path& value) noexcept
{
    for (const auto& component : value) if (component == L"..") return true;
    return false;
}
std::filesystem::path ImagesBase(const std::filesystem::path& root, std::filesystem::path value)
{
    value = value.lexically_normal();
    if (value.is_absolute()) return value;
    auto iterator = value.begin();
    if (iterator != value.end() && _wcsicmp(iterator->c_str(), L"Data") == 0)
    {
        std::filesystem::path stripped;
        for (++iterator; iterator != value.end(); ++iterator) stripped /= *iterator;
        return root / stripped;
    }
    return root / value;
}
}

engine::assets::AssetResult LegacyMaterialTextureResolver::ResolveDiffuse(
    const std::filesystem::path& dataRoot, const std::filesystem::path& materialLibraryPath,
    const std::filesystem::path& meshPath, const std::string& imagesDir, const std::string& texture,
    const LegacyTextureLookupPolicy policy, LegacyTextureResolution& outResolution) noexcept
{
    outResolution.Clear(); outResolution.policy = policy;
    if (dataRoot.empty() || materialLibraryPath.empty() || meshPath.empty() || texture.empty())
        return engine::assets::AssetResult::InvalidArgument;
    try
    {
        std::error_code error;
        const auto root = std::filesystem::weakly_canonical(dataRoot, error);
        if (error) return engine::assets::AssetResult::InvalidPath;
        const auto texturePath = std::filesystem::u8path(texture);
        if (texturePath.is_absolute() || HasParentTraversal(texturePath)) return engine::assets::AssetResult::InvalidPath;
        std::filesystem::path strictBase;
        if (!imagesDir.empty())
        {
            const auto imagePath = std::filesystem::u8path(imagesDir);
            if (HasParentTraversal(imagePath)) return engine::assets::AssetResult::InvalidPath;
            strictBase = ImagesBase(root, imagePath);
        }
        else strictBase = meshPath.parent_path() / L"Textures";
        Add(outResolution.candidates, strictBase / texturePath);
        auto dds = strictBase / texturePath; dds.replace_extension(L".dds"); Add(outResolution.candidates, dds);
        const std::size_t strictCount = outResolution.candidates.size();
        if (policy == LegacyTextureLookupPolicy::Relaxed)
        {
            Add(outResolution.candidates, materialLibraryPath.parent_path() / texturePath);
            auto relaxedDds = materialLibraryPath.parent_path() / texturePath; relaxedDds.replace_extension(L".dds"); Add(outResolution.candidates, relaxedDds);
        }
        for (std::size_t index = 0U; index < outResolution.candidates.size(); ++index)
        {
            const auto absolute = std::filesystem::weakly_canonical(outResolution.candidates[index], error);
            if (error) { error.clear(); continue; }
            if (!Contained(root, absolute)) return engine::assets::AssetResult::InvalidPath;
            if (!std::filesystem::is_regular_file(absolute, error) || error) { error.clear(); continue; }
            const auto relative = std::filesystem::relative(absolute, root, error);
            if (error) return engine::assets::AssetResult::InvalidPath;
            const auto result = engine::assets::AssetPath::TryCreate(relative.generic_u8string(), outResolution.path);
            if (engine::assets::Succeeded(result)) outResolution.usedRelaxedFallback = index >= strictCount;
            return result;
        }
    }
    catch (const std::bad_alloc&) { return engine::assets::AssetResult::OutOfMemory; }
    catch (...) { return engine::assets::AssetResult::InvalidPath; }
    return engine::assets::AssetResult::NotFound;
}
}
