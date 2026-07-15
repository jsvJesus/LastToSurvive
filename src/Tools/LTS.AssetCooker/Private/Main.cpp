#include "Assets/DdsTextureDecoder.h"
#include "Assets/LtsMaterialWriter.h"
#include "Assets/LtsMeshWriter.h"
#include "Assets/LtsStaticModelWriter.h"
#include "Assets/StaticModelAsset.h"
#include "AssetCooker/CookerTransaction.h"
#include "Legacy/Assets/AsciiCaseInsensitive.h"
#include "Legacy/Assets/LegacyMaterialConverter.h"
#include "Legacy/Assets/LegacyMaterialLibraryDecoder.h"
#include "Legacy/Assets/LegacyMaterialTextureResolver.h"
#include "Legacy/Assets/LegacyScbMeshDecoder.h"
#include "Platform/File.h"

#include <Windows.h>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace
{
using engine::assets::AssetData;
using engine::assets::AssetResult;

AssetResult ReadFile(const std::filesystem::path& path, AssetData& output) noexcept
{
    engine::platform::File file(path);
    const auto size = file.GetSize();
    if (!file || !size) return AssetResult::IoError;
    if (*size > (std::numeric_limits<std::size_t>::max)()) return AssetResult::FileTooLarge;
    const AssetResult resize = output.Resize(static_cast<std::size_t>(*size));
    if (engine::assets::Failed(resize)) return resize;
    const auto read = file.Read(output.GetData(), output.GetSize());
    if (!read || read.bytesTransferred != output.GetSize()) { output.Clear(); return AssetResult::IoError; }
    return AssetResult::Success;
}

AssetResult WriteAtomic(const std::filesystem::path& path, const AssetData& data, const bool force) noexcept
{
    std::error_code error;
    if (std::filesystem::exists(path, error) && !force) return AssetResult::AlreadyExists;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error) return AssetResult::IoError;
    std::filesystem::path temporary = path; temporary += L".tmp";
    std::filesystem::remove(temporary, error);
    engine::platform::File file(temporary, engine::platform::FileAccess::Write, engine::platform::FileCreation::CreateNew);
    if (!file) return AssetResult::IoError;
    const auto written = file.Write(data.GetData(), data.GetSize());
    if (!written || written.bytesTransferred != data.GetSize() || !file.Flush())
    { file.Close(); std::filesystem::remove(temporary, error); return AssetResult::IoError; }
    file.Close();
    const DWORD flags = MOVEFILE_WRITE_THROUGH | (force ? MOVEFILE_REPLACE_EXISTING : 0U);
    if (!MoveFileExW(temporary.c_str(), path.c_str(), flags))
    { std::filesystem::remove(temporary, error); return AssetResult::IoError; }
    return AssetResult::Success;
}

void PrintMesh(const engine::legacy::assets::LegacyStaticMeshData& data, const std::filesystem::path& source)
{
    const auto& mesh = data.mesh;
    std::wprintf(L"source: %ls\nversion: 0x%08X\n", source.c_str(), engine::legacy::assets::LegacyScbMeshDecoder::SupportedVersion);
    std::printf("mesh: %s\nvertices: %zu\nindices: %zu\ntriangles: %zu\nsubmeshes: %zu\n", data.sourceName.c_str(),
                mesh.GetVertexCount(), mesh.GetIndexCount(), mesh.GetIndexCount() / 3U, mesh.GetSubmeshCount());
    for (std::size_t i = 0U; i < data.materialSlotNames.size(); ++i) std::printf("material[%zu]: %s\n", i, data.materialSlotNames[i].c_str());
    const auto& b = mesh.GetBounds();
    std::printf("bounds: min %.3f %.3f %.3f, max %.3f %.3f %.3f, radius %.3f\n", b.minimum[0], b.minimum[1], b.minimum[2], b.maximum[0], b.maximum[1], b.maximum[2], b.sphereRadius);
}

bool SamePath(const std::filesystem::path& left, const std::filesystem::path& right) noexcept
{
    return _wcsicmp(left.lexically_normal().c_str(), right.lexically_normal().c_str()) == 0;
}
std::string FileStem(const std::string& name)
{
    std::string output;
    for (const unsigned char c : name)
    {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) output.push_back(static_cast<char>(c));
        else if (c >= 'A' && c <= 'Z') output.push_back(static_cast<char>(c - 'A' + 'a'));
        else if (output.empty() || output.back() != '_') output.push_back('_');
    }
    while (!output.empty() && output.back() == '_') output.pop_back();
    return output.empty() ? "material" : output;
}
const char* AlphaName(const engine::assets::MaterialAlphaMode mode) noexcept
{
    switch (mode) { case engine::assets::MaterialAlphaMode::Opaque: return "Opaque"; case engine::assets::MaterialAlphaMode::Mask: return "Mask"; case engine::assets::MaterialAlphaMode::Blend: return "Blend"; default: return "Invalid"; }
}

struct ModelOptions final
{
    std::filesystem::path input;
    std::filesystem::path dataRoot;
    std::filesystem::path outputRoot;
    bool force = false;
    bool allowMissingTextures = false;
    bool relaxedTextureLookup = false;
};
bool ParseModelOptions(const int argc, wchar_t** argv, ModelOptions& options)
{
    for (int index = 2; index < argc; ++index)
    {
        const std::wstring_view key(argv[index]);
        if (key == L"--force") { options.force = true; continue; }
        if (key == L"--allow-missing-textures") { options.allowMissingTextures = true; continue; }
        if (key == L"--relaxed-texture-lookup") { options.relaxedTextureLookup = true; continue; }
        if (index + 1 >= argc) return false;
        const std::filesystem::path value(argv[++index]);
        if (key == L"--input") options.input = value;
        else if (key == L"--data-root") options.dataRoot = value;
        else if (key == L"--output-root") options.outputRoot = value;
        else return false;
    }
    return !options.input.empty() && !options.dataRoot.empty() && !options.outputRoot.empty();
}

AssetResult DecodeDds(const std::filesystem::path& dataRoot, const engine::assets::AssetPath& path,
                      engine::assets::TextureAsset& texture) noexcept
{
    AssetData bytes;
    AssetResult result = ReadFile(dataRoot / std::filesystem::u8path(path.String()), bytes);
    if (engine::assets::Failed(result)) return result;
    return engine::assets::DdsTextureDecoder::Decode(bytes, {}, texture);
}

int CookModel(const ModelOptions& rawOptions)
{
    const auto start = std::chrono::steady_clock::now();
    std::error_code error;
    lts::asset_cooker::CookPaths paths;
    std::string pathError;
    AssetResult result = lts::asset_cooker::ValidateCookPaths(rawOptions.input, rawOptions.dataRoot, rawOptions.outputRoot,
        std::to_wstring(GetCurrentProcessId()), paths, pathError);
    if (engine::assets::Failed(result)) { std::fprintf(stderr, "unsafe output path: %s\n", pathError.c_str()); return 2; }
    const auto& input = paths.input; const auto& dataRoot = paths.dataRoot; const auto& outputRoot = paths.outputRoot;

    AssetData source;
    result = ReadFile(input, source);
    engine::legacy::assets::LegacyStaticMeshData decoded;
    if (engine::assets::Succeeded(result)) result = engine::legacy::assets::LegacyScbMeshDecoder::Decode(source, decoded);
    if (engine::assets::Failed(result)) { std::fprintf(stderr, "SCB decode failed: %s\n", engine::assets::ToString(result)); return 3; }

    result = lts::asset_cooker::PrepareTemporaryDirectory(paths, pathError);
    if (engine::assets::Failed(result)) { std::fprintf(stderr, "temporary output failed: %s\n", pathError.c_str()); return 4; }
    const auto& temporary = paths.temporary;
    const auto fail = [&](const char* phase, const AssetResult failure) -> int
    {
        std::fprintf(stderr, "%s failed: %s\n", phase, engine::assets::ToString(failure));
        std::error_code ignored; std::filesystem::remove_all(temporary, ignored); return 4;
    };

    AssetData meshBytes;
    result = engine::assets::LtsMeshWriter::Encode(decoded.mesh, meshBytes);
    if (engine::assets::Succeeded(result)) result = WriteAtomic(temporary / L"model.ltsmesh", meshBytes, true);
    if (engine::assets::Failed(result)) return fail("mesh write", result);

    engine::assets::AssetPath meshAssetPath;
    const auto meshRelative = std::filesystem::relative(outputRoot / L"model.ltsmesh", dataRoot, error).generic_u8string();
    result = error ? AssetResult::InvalidPath : engine::assets::AssetPath::TryCreate(meshRelative, meshAssetPath);
    if (engine::assets::Failed(result)) return fail("mesh asset path", result);

    std::vector<engine::assets::AssetPath> materialPaths;
    std::set<std::string> filenames;
    std::size_t missingCount = 0U;
    try { materialPaths.reserve(decoded.materialSlotNames.size()); }
    catch (...) { return fail("material allocation", AssetResult::OutOfMemory); }
    PrintMesh(decoded, input);
    for (std::size_t slot = 0U; slot < decoded.materialSlotNames.size(); ++slot)
    {
        const std::string& materialName = decoded.materialSlotNames[slot];
        const auto materialFile = input.parent_path() / L"Materials" / std::filesystem::u8path(materialName + ".mat");
        AssetData materialSource;
        result = ReadFile(materialFile, materialSource);
        if (engine::assets::Failed(result)) return fail("material library read", result);
        engine::legacy::assets::LegacyMaterialLibraryData library;
        result = engine::legacy::assets::LegacyMaterialLibraryDecoder::Decode(materialSource, library);
        if (engine::assets::Failed(result)) return fail("material library decode", result);
        const auto* record = library.FindMaterial(materialName);
        if (record == nullptr) return fail("material name lookup", AssetResult::NotFound);

        engine::legacy::assets::LegacyTextureResolution resolution;
        const engine::assets::AssetPath* texturePath = nullptr;
        auto textureAlpha = engine::legacy::assets::LegacyTextureAlpha::NoAlpha;
        if (!record->texture.empty())
        {
            result = engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(
                dataRoot, materialFile, input, record->imagesDir, record->texture,
                rawOptions.relaxedTextureLookup ? engine::legacy::assets::LegacyTextureLookupPolicy::Relaxed : engine::legacy::assets::LegacyTextureLookupPolicy::Strict,
                resolution);
            if (engine::assets::Succeeded(result))
            {
                engine::assets::TextureAsset decodedTexture;
                result = DecodeDds(dataRoot, resolution.path, decodedTexture);
                if (engine::assets::Failed(result)) return fail("DDS validation", result);
                textureAlpha = engine::legacy::assets::LegacyMaterialConverter::DetectTextureAlpha(decodedTexture);
                texturePath = &resolution.path;
                if (resolution.usedRelaxedFallback) std::fprintf(stderr, "warning: relaxed texture fallback used for %s\n", materialName.c_str());
            }
            else if (result == AssetResult::NotFound && rawOptions.allowMissingTextures)
            { ++missingCount; std::fprintf(stderr, "warning: missing diffuse texture for %s\n", materialName.c_str()); }
            else return fail("diffuse texture resolution", result);
        }

        engine::assets::MaterialAsset material;
        std::vector<std::string> diagnostics;
        result = engine::legacy::assets::LegacyMaterialConverter::Convert(*record, texturePath, textureAlpha, material, diagnostics);
        if (engine::assets::Failed(result)) return fail("material conversion", result);
        std::string stem = FileStem(materialName);
        if (!filenames.insert(stem).second)
        {
            char suffix[24]{};
            std::snprintf(suffix, sizeof(suffix), "_%08llx", static_cast<unsigned long long>(engine::legacy::assets::AsciiCaseInsensitiveHash{}(materialName) & 0xffffffffULL));
            stem += suffix;
            if (!filenames.insert(stem).second) return fail("deterministic material filename collision", AssetResult::AlreadyExists);
        }
        const std::string filename = stem + ".ltsmat";
        AssetData materialBytes;
        result = engine::assets::LtsMaterialWriter::Encode(material, materialBytes);
        if (engine::assets::Succeeded(result)) result = WriteAtomic(temporary / std::filesystem::u8path(filename), materialBytes, true);
        if (engine::assets::Failed(result)) return fail("material write", result);
        engine::assets::AssetPath cookedMaterialPath;
        const auto relative = std::filesystem::relative(outputRoot / std::filesystem::u8path(filename), dataRoot, error).generic_u8string();
        result = error ? AssetResult::InvalidPath : engine::assets::AssetPath::TryCreate(relative, cookedMaterialPath);
        if (engine::assets::Failed(result)) return fail("material asset path", result);
        materialPaths.push_back(cookedMaterialPath);
        std::printf("slot[%zu]: %s -> %s -> %s, alpha=%s, doubleSided=%s\n", slot, materialName.c_str(),
                    cookedMaterialPath.String().c_str(), texturePath ? texturePath->String().c_str() : "<fallback>",
                    AlphaName(material.GetDesc().alphaMode), material.GetDesc().doubleSided ? "true" : "false");
        for (const auto& diagnostic : diagnostics) std::printf("  diagnostic: %s\n", diagnostic.c_str());
        std::wprintf(L"  material library: %ls\n", materialFile.c_str());
    }

    engine::assets::StaticModelAsset model;
    result = model.Initialize(std::move(meshAssetPath), std::move(materialPaths), decoded.sourceName);
    if (engine::assets::Failed(result)) return fail("model creation", result);
    AssetData modelBytes;
    result = engine::assets::LtsStaticModelWriter::Encode(model, modelBytes);
    if (engine::assets::Succeeded(result)) result = WriteAtomic(temporary / L"model.ltsmodel", modelBytes, true);
    if (engine::assets::Failed(result)) return fail("model write", result);
    result = lts::asset_cooker::CommitDirectory(paths, rawOptions.force, pathError);
    if (engine::assets::Failed(result)) { std::fprintf(stderr, "output commit failed: %s (%s)\n", pathError.c_str(), engine::assets::ToString(result)); return 4; }
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::wprintf(L"output model: %ls\n", (outputRoot / L"model.ltsmodel").c_str());
    std::printf("unique materials: %zu\nmissing textures: %zu\nelapsed ms: %.3f\n", decoded.materialSlotNames.size(), missingCount, elapsed);
    return 0;
}
}

int wmain(const int argc, wchar_t** argv)
{
    if (argc >= 2 && std::wstring_view(argv[1]) == L"model")
    {
        ModelOptions options;
        if (!ParseModelOptions(argc, argv, options))
        {
            std::fwprintf(stderr, L"usage: LTS.AssetCooker model --input <mesh.scb> --data-root <Data> --output-root <directory> [--force] [--allow-missing-textures] [--relaxed-texture-lookup]\n");
            return 2;
        }
        return CookModel(options);
    }
    if (argc < 3)
    {
        std::fwprintf(stderr, L"usage: LTS.AssetCooker inspect-mesh <input.scb> | mesh <input.scb> <output.ltsmesh> [--force]\n");
        return 2;
    }
    const bool inspect = std::wstring_view(argv[1]) == L"inspect-mesh";
    const bool convert = std::wstring_view(argv[1]) == L"mesh";
    if ((!inspect && !convert) || (inspect && argc != 3) || (convert && (argc < 4 || argc > 5))) return 2;
    const std::filesystem::path input = std::filesystem::absolute(argv[2]).lexically_normal();
    const std::filesystem::path output = convert ? std::filesystem::absolute(argv[3]).lexically_normal() : std::filesystem::path{};
    const bool force = argc == 5 && std::wstring_view(argv[4]) == L"--force";
    if (convert && SamePath(input, output)) { std::fwprintf(stderr, L"input and output must differ\n"); return 2; }
    const auto start = std::chrono::steady_clock::now();
    AssetData source;
    AssetResult result = ReadFile(input, source);
    engine::legacy::assets::LegacyStaticMeshData decoded;
    if (engine::assets::Succeeded(result)) result = engine::legacy::assets::LegacyScbMeshDecoder::Decode(source, decoded);
    if (engine::assets::Failed(result)) { std::fprintf(stderr, "decode failed: %s\n", engine::assets::ToString(result)); return 3; }
    PrintMesh(decoded, input);
    if (convert)
    {
        AssetData cooked;
        result = engine::assets::LtsMeshWriter::Encode(decoded.mesh, cooked);
        if (engine::assets::Succeeded(result)) result = WriteAtomic(output, cooked, force);
        if (engine::assets::Failed(result)) { std::fprintf(stderr, "write failed: %s\n", engine::assets::ToString(result)); return 4; }
        std::printf("output bytes: %zu\n", cooked.GetSize());
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::printf("elapsed ms: %.3f\n", elapsed);
    return 0;
}
