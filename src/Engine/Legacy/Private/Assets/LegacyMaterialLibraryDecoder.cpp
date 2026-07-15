#include "Legacy/Assets/LegacyMaterialLibraryDecoder.h"
#include "Legacy/Assets/AsciiCaseInsensitive.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <new>
#include <string_view>

namespace engine::legacy::assets
{
namespace
{
std::string_view Trim(std::string_view value) noexcept
{
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t' || value.front() == '\r')) value.remove_prefix(1U);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r')) value.remove_suffix(1U);
    return value;
}
bool Equal(const std::string_view a, const std::string_view b) noexcept
{
    return AsciiCaseInsensitiveEqual{}(a, b);
}
template<typename T> bool Number(const std::string_view source, T& value) noexcept
{
    const auto trimmed = Trim(source);
    if (trimmed.empty()) return false;
    const char* const begin = trimmed.data();
    const char* const end = begin + trimmed.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}
bool Float(const std::string_view source, float& value) noexcept { return Number(source, value) && std::isfinite(value); }
bool Boolean(const std::string_view source, bool& value) noexcept
{
    int parsed = 0;
    if (!Number(source, parsed) || (parsed != 0 && parsed != 1)) return false;
    value = parsed != 0;
    return true;
}
bool Color(const std::string_view source, std::array<std::uint8_t, 3U>& color) noexcept
{
    std::size_t cursor = 0U;
    for (std::size_t component = 0U; component < 3U; ++component)
    {
        while (cursor < source.size() && (source[cursor] == ' ' || source[cursor] == '\t')) ++cursor;
        const std::size_t begin = cursor;
        while (cursor < source.size() && source[cursor] != ' ' && source[cursor] != '\t') ++cursor;
        int value = 0;
        if (begin == cursor || !Number(source.substr(begin, cursor - begin), value) || value < 0 || value > 255) return false;
        color[component] = static_cast<std::uint8_t>(value);
    }
    return Trim(source.substr(cursor)).empty();
}
bool DeferredTextureKey(const std::string_view key) noexcept
{
    constexpr std::string_view keys[] = {"NormalMap", "SpecularMap", "EnvMap", "GlowMap", "DetailNMap",
        "DensityMap", "CamoMask", "DistortionMap", "SpecPowMap"};
    for (const auto candidate : keys) if (Equal(key, candidate)) return true;
    return false;
}
engine::assets::AssetResult Diagnostic(LegacyMaterialRecord& material, const std::string_view text) noexcept
{
    try { material.diagnostics.emplace_back(text); }
    catch (...) { return engine::assets::AssetResult::OutOfMemory; }
    return engine::assets::AssetResult::Success;
}
engine::assets::AssetResult ParseField(LegacyMaterialRecord& material, const std::string_view key,
                                       const std::string_view rawValue) noexcept
{
    const auto value = Trim(rawValue);
    if (Equal(key, "Name")) { if (value.empty()) return engine::assets::AssetResult::CorruptData; material.name.assign(value); return engine::assets::AssetResult::Success; }
    if (Equal(key, "Color24")) return Color(value, material.color24) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "DoubleSided")) return Boolean(value, material.doubleSided) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "AlphaTransparent")) return Boolean(value, material.alphaTransparent) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "ForceTransparent")) return Boolean(value, material.forceTransparent) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "TransparentShadows")) return Boolean(value, material.transparentShadows) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "Texture")) { material.texture = Equal(value, "NONE") ? std::string{} : std::string(value); return engine::assets::AssetResult::Success; }
    if (Equal(key, "ImagesDir")) { material.imagesDir.assign(value); return engine::assets::AssetResult::Success; }
    if (Equal(key, "SelfIllumMultiplier")) return Float(value, material.selfIllumMultiplier) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "lowQMetallness")) return Float(value, material.lowQMetallness) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "SpecularPower")) return Float(value, material.specularPower) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (Equal(key, "ReflectionPower")) return Float(value, material.reflectionPower) ? engine::assets::AssetResult::Success : engine::assets::AssetResult::CorruptData;
    if (DeferredTextureKey(key))
    {
        if (!value.empty() && !Equal(value, "NONE"))
            try { material.deferredTextureSlots.emplace_back(std::string(key) + "=" + std::string(value)); }
            catch (...) { return engine::assets::AssetResult::OutOfMemory; }
        return engine::assets::AssetResult::Success;
    }
    return Diagnostic(material, std::string("ignored key: ") + std::string(key));
}
}

const LegacyMaterialRecord* LegacyMaterialLibraryData::GetMaterial(const std::size_t index) const noexcept
{
    return index < materials_.size() ? &materials_[index] : nullptr;
}
const LegacyMaterialRecord* LegacyMaterialLibraryData::FindMaterial(const std::string& name) const noexcept
{
    for (const auto& material : materials_) if (Equal(material.name, name)) return &material;
    return nullptr;
}

engine::assets::AssetResult LegacyMaterialLibraryDecoder::Decode(
    const engine::assets::AssetData& source, LegacyMaterialLibraryData& outData) noexcept
{
    outData.Clear();
    if (source.IsEmpty()) return engine::assets::AssetResult::CorruptData;
    const std::string_view text(reinterpret_cast<const char*>(source.GetData()), source.GetSize());
    LegacyMaterialLibraryData candidate;
    LegacyMaterialRecord current;
    bool inside = false;
    std::size_t cursor = 0U;
    try
    {
        while (cursor < text.size())
        {
            const std::size_t newline = text.find('\n', cursor);
            const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
            if (end - cursor > MaximumLineLength) return engine::assets::AssetResult::CorruptData;
            const auto line = Trim(text.substr(cursor, end - cursor));
            cursor = newline == std::string_view::npos ? text.size() : newline + 1U;
            if (line.empty() || line.front() == ';' || line.front() == '#') continue;
            if (Equal(line, "[MaterialBegin]"))
            {
                if (inside || candidate.materials_.size() >= MaximumMaterialCount) return engine::assets::AssetResult::CorruptData;
                current = {};
                inside = true;
                continue;
            }
            if (Equal(line, "[MaterialEnd]"))
            {
                if (!inside || current.name.empty()) return engine::assets::AssetResult::CorruptData;
                for (const auto& existing : candidate.materials_) if (Equal(existing.name, current.name)) return engine::assets::AssetResult::AlreadyExists;
                candidate.materials_.push_back(std::move(current));
                inside = false;
                continue;
            }
            if (!inside) return engine::assets::AssetResult::CorruptData;
            const std::size_t equals = line.find('=');
            if (equals == std::string_view::npos || Trim(line.substr(0U, equals)).empty()) return engine::assets::AssetResult::CorruptData;
            const auto result = ParseField(current, Trim(line.substr(0U, equals)), line.substr(equals + 1U));
            if (engine::assets::Failed(result)) return result;
        }
    }
    catch (const std::bad_alloc&) { return engine::assets::AssetResult::OutOfMemory; }
    catch (...) { return engine::assets::AssetResult::InternalError; }
    if (inside || candidate.materials_.empty()) return engine::assets::AssetResult::CorruptData;
    outData = std::move(candidate);
    return engine::assets::AssetResult::Success;
}
}
