#include "Legacy/Assets/LegacyMaterialTextureResolver.h"

#include <algorithm>
#include <cwchar>
#include <system_error>

namespace engine::legacy::assets
{
namespace
{
std::filesystem::path StripDataPrefix(std::filesystem::path path)
{
    path = path.lexically_normal();
    auto first = path.begin();
    if (first != path.end() && _wcsicmp(first->c_str(), L"Data") == 0)
    {
        std::filesystem::path stripped;
        for (++first; first != path.end(); ++first) stripped /= *first;
        return stripped;
    }
    return path;
}
bool Contained(const std::filesystem::path& root, const std::filesystem::path& candidate) noexcept
{
    const auto r = root.lexically_normal();
    const auto c = candidate.lexically_normal();
    auto ri = r.begin(); auto ci = c.begin();
    for (; ri != r.end(); ++ri, ++ci) if (ci == c.end() || _wcsicmp(ri->c_str(), ci->c_str()) != 0) return false;
    return true;
}
void AddCandidate(std::vector<std::filesystem::path>& output, const std::filesystem::path& path)
{
    const auto normalized = path.lexically_normal();
    for (const auto& existing : output) if (_wcsicmp(existing.c_str(), normalized.c_str()) == 0) return;
    output.push_back(normalized);
}
}

engine::assets::AssetResult LegacyMaterialTextureResolver::ResolveDiffuse(
    const std::filesystem::path& dataRoot, const std::filesystem::path& materialLibraryPath,
    const std::filesystem::path& meshPath, const std::string& imagesDir, const std::string& texture,
    LegacyTextureResolution& outResolution) noexcept
{
    outResolution.Clear();
    if (dataRoot.empty() || materialLibraryPath.empty() || meshPath.empty() || texture.empty())
        return engine::assets::AssetResult::InvalidArgument;
    try
    {
        std::error_code error;
        const auto root = std::filesystem::absolute(dataRoot, error).lexically_normal();
        if (error) return engine::assets::AssetResult::InvalidPath;
        const std::filesystem::path texturePath = std::filesystem::u8path(texture);
        if (texturePath.is_absolute()) return engine::assets::AssetResult::InvalidPath;
        if (!imagesDir.empty())
        {
            auto base = StripDataPrefix(std::filesystem::u8path(imagesDir));
            if (base.is_absolute()) AddCandidate(outResolution.candidates, base / texturePath);
            else AddCandidate(outResolution.candidates, root / base / texturePath);
        }
        const auto depot = meshPath.parent_path();
        AddCandidate(outResolution.candidates, depot / L"Textures" / texturePath);
        AddCandidate(outResolution.candidates, materialLibraryPath.parent_path() / texturePath);
        const std::size_t originalCount = outResolution.candidates.size();
        for (std::size_t index = 0U; index < originalCount; ++index)
        {
            auto dds = outResolution.candidates[index];
            dds.replace_extension(L".dds");
            AddCandidate(outResolution.candidates, dds);
        }
        for (const auto& candidate : outResolution.candidates)
        {
            const auto absolute = std::filesystem::absolute(candidate, error).lexically_normal();
            if (error || !Contained(root, absolute)) continue;
            if (!std::filesystem::is_regular_file(absolute, error) || error) { error.clear(); continue; }
            const auto relative = std::filesystem::relative(absolute, root, error);
            if (error) return engine::assets::AssetResult::InvalidPath;
            const auto result = engine::assets::AssetPath::TryCreate(relative.generic_u8string(), outResolution.path);
            return result;
        }
    }
    catch (const std::bad_alloc&) { return engine::assets::AssetResult::OutOfMemory; }
    catch (...) { return engine::assets::AssetResult::InvalidPath; }
    return engine::assets::AssetResult::NotFound;
}
}
