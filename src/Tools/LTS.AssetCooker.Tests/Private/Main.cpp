#include "AssetCooker/CookerTransaction.h"
#include "Legacy/Assets/LegacyMaterialTextureResolver.h"

#include <Windows.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
bool Check(const bool value, const char* message)
{
    if (value) return true;
    std::fprintf(stderr, "FAILED: %s\n", message); return false;
}
bool Write(const std::filesystem::path& path, const char* value = "x")
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream << value; return stream.good();
}
std::string Read(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary); return {std::istreambuf_iterator<char>(stream), {}};
}
}

int main()
{
    namespace fs = std::filesystem;
    using engine::assets::AssetResult;
    const fs::path base = fs::temp_directory_path() / (L"lts-cooker-tests-" + std::to_wstring(GetCurrentProcessId()));
    std::error_code error; fs::remove_all(base, error);
    const fs::path data = base / L"Data";
    const fs::path mesh = data / L"ObjectsDepot" / L"Depot With Spaces" / L"model.scb";
    const fs::path output = data / L"CookedPreview" / L"Model";
    if (!Check(Write(mesh), "fixture setup")) return 1;
    lts::asset_cooker::CookPaths paths; std::string message;
    const auto validate = [&](const fs::path& candidate) { return lts::asset_cooker::ValidateCookPaths(mesh, data, candidate, L"test", paths, message); };
    if (!Check(validate(data) == AssetResult::InvalidPath, "reject output=data root") ||
        !Check(validate(mesh.parent_path()) == AssetResult::InvalidPath, "reject input parent") ||
        !Check(validate(data / L"ObjectsDepot") == AssetResult::InvalidPath, "reject output ancestor of input") ||
        !Check(validate(output) == AssetResult::Success, "accept nested sibling output")) return 1;
    const fs::path outputFile = data / L"CookedPreview" / L"file-output"; Write(outputFile);
    if (!Check(validate(outputFile) == AssetResult::InvalidPath, "reject output regular file")) return 1;
    fs::remove(outputFile);
    if (!Check(validate(output) == AssetResult::Success, "restore validated transaction paths")) return 1;

    if (!Check(lts::asset_cooker::PrepareTemporaryDirectory(paths, message) == AssetResult::Success && Write(paths.temporary / L"new.txt", "new"), "prepare transaction")) return 1;
    fs::create_directories(paths.outputRoot); Write(paths.outputRoot / L"old.txt", "old");
    if (!Check(lts::asset_cooker::CommitDirectory(paths, false, message) == AssetResult::AlreadyExists &&
        Read(paths.outputRoot / L"old.txt") == "old" && !fs::exists(paths.temporary), "existing output without force")) return 1;
    (void)lts::asset_cooker::PrepareTemporaryDirectory(paths, message); Write(paths.temporary / L"new.txt", "new");
    if (!Check(lts::asset_cooker::CommitDirectory(paths, true, message) == AssetResult::Success &&
        Read(paths.outputRoot / L"new.txt") == "new" && !fs::exists(paths.backup), "existing output force replace")) return 1;
    fs::remove_all(paths.outputRoot); fs::create_directories(paths.outputRoot); Write(paths.outputRoot / L"old.txt", "old");
    (void)lts::asset_cooker::PrepareTemporaryDirectory(paths, message); Write(paths.temporary / L"new.txt", "new");
    if (!Check(lts::asset_cooker::CommitDirectory(paths, true, message, lts::asset_cooker::TransactionTestFailure::AfterBackup) == AssetResult::IoError &&
        Read(paths.outputRoot / L"old.txt") == "old" && !fs::exists(paths.temporary) && !fs::exists(paths.backup), "rollback restores old output")) return 1;
    fs::remove_all(paths.outputRoot); fs::create_directories(paths.outputRoot); Write(paths.outputRoot / L"old.txt", "old");
    (void)lts::asset_cooker::PrepareTemporaryDirectory(paths, message); Write(paths.temporary / L"new.txt", "new");
    if (!Check(lts::asset_cooker::CommitDirectory(paths, true, message, lts::asset_cooker::TransactionTestFailure::AfterBackupAndRollback) == AssetResult::IoError &&
        !fs::exists(paths.outputRoot) && !fs::exists(paths.temporary) && Read(paths.backup / L"old.txt") == "old", "failed rollback preserves backup")) return 1;
    fs::rename(paths.backup, paths.outputRoot);

    const fs::path reparseTarget = base / L"reparse-target"; fs::create_directories(reparseTarget); Write(reparseTarget / L"sentinel.txt", "safe");
    error.clear(); fs::create_directory_symlink(reparseTarget, paths.temporary, error);
    if (!Check(!error, "create temporary directory symlink") ||
        !Check(lts::asset_cooker::PrepareTemporaryDirectory(paths, message) == AssetResult::IoError &&
            Read(reparseTarget / L"sentinel.txt") == "safe" && fs::is_symlink(paths.temporary), "reject stale temporary reparse point")) return 1;
    fs::remove(paths.temporary); error.clear();
    (void)lts::asset_cooker::PrepareTemporaryDirectory(paths, message); Write(paths.temporary / L"new.txt", "new");
    fs::create_directory_symlink(reparseTarget, paths.backup, error);
    if (!Check(!error, "create backup directory symlink") ||
        !Check(lts::asset_cooker::CommitDirectory(paths, true, message) == AssetResult::IoError &&
            Read(reparseTarget / L"sentinel.txt") == "safe" && fs::is_symlink(paths.backup), "reject stale backup reparse point")) return 1;
    fs::remove(paths.backup); error.clear();

    const fs::path material = mesh.parent_path() / L"Materials" / L"Mat.mat"; Write(material);
    const fs::path depotTexture = mesh.parent_path() / L"Textures" / L"Diffuse.DDS"; Write(depotTexture);
    engine::legacy::assets::LegacyTextureResolution resolution;
    using Policy = engine::legacy::assets::LegacyTextureLookupPolicy;
    if (!Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "", "diffuse.tga", Policy::Strict, resolution) == AssetResult::Success &&
        resolution.path.View() == "objectsdepot/depot with spaces/textures/diffuse.dds" && !resolution.usedRelaxedFallback, "strict depot DDS replacement and Windows case")) return 1;
    const fs::path depotOriginal = mesh.parent_path() / L"Textures" / L"diffuse.tga"; Write(depotOriginal);
    if (!Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "", "diffuse.tga", Policy::Strict, resolution) == AssetResult::Success && resolution.path.View().substr(resolution.path.View().size()-11U)=="diffuse.tga", "original extension wins over DDS")) return 1;
    fs::remove(depotOriginal);
    const fs::path imageOriginal = data / L"Images With Spaces" / L"original.tga"; Write(imageOriginal);
    if (!Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "Data/Images With Spaces", "original.tga", Policy::Strict, resolution) == AssetResult::Success &&
        resolution.path.View() == "images with spaces/original.tga", "ImagesDir original wins")) return 1;
    if (!Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "Data/Missing", "Diffuse.DDS", Policy::Strict, resolution) == AssetResult::NotFound, "invalid ImagesDir has no strict depot fallback")) return 1;
    Write(material.parent_path() / L"relaxed.dds");
    if (!Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "", "relaxed.tga", Policy::Relaxed, resolution) == AssetResult::Success && resolution.usedRelaxedFallback, "explicit relaxed material fallback")) return 1;
    if (!Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "", "../escape.dds", Policy::Strict, resolution) == AssetResult::InvalidPath, "reject traversal") ||
        !Check(engine::legacy::assets::LegacyMaterialTextureResolver::ResolveDiffuse(data, material, mesh, "", "C:/absolute.dds", Policy::Strict, resolution) == AssetResult::InvalidPath, "reject absolute texture")) return 1;
    fs::remove_all(base, error);
    std::puts("LTS.AssetCooker safety and resolver tests passed"); return 0;
}
