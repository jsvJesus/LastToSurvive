#include "AssetCooker/CookerTransaction.h"

#include <cwchar>
#include <system_error>

namespace lts::asset_cooker
{
namespace
{
bool Equal(const std::filesystem::path& left, const std::filesystem::path& right) noexcept
{ return _wcsicmp(left.c_str(), right.c_str()) == 0; }
bool Contains(const std::filesystem::path& parent, const std::filesystem::path& child) noexcept
{
    auto pi = parent.begin(); auto ci = child.begin();
    for (; pi != parent.end(); ++pi, ++ci) if (ci == child.end() || _wcsicmp(pi->c_str(), ci->c_str()) != 0) return false;
    return true;
}
engine::assets::AssetResult Fail(
    std::string& message,
    const char* value,
    const engine::assets::AssetResult result) noexcept
{
    try { message = value; } catch (...) {}
    return result;
}
bool UnsafeDerived(const CookPaths& paths, const std::filesystem::path& value) noexcept
{
    return Equal(value, paths.dataRoot) || Equal(value, paths.input.parent_path()) ||
        Contains(value, paths.input) || Contains(paths.input, value);
}
}

engine::assets::AssetResult ValidateCookPaths(
    const std::filesystem::path& rawInput, const std::filesystem::path& rawDataRoot,
    const std::filesystem::path& rawOutputRoot, const std::wstring_view nonce,
    CookPaths& outPaths, std::string& outError) noexcept
{
    outPaths = {}; outError.clear();
    try
    {
        std::error_code error;
        CookPaths candidate;
        candidate.dataRoot = std::filesystem::canonical(rawDataRoot, error);
        if (error || !std::filesystem::is_directory(candidate.dataRoot, error)) return Fail(outError, "data root must be an existing directory", engine::assets::AssetResult::InvalidPath);
        candidate.input = std::filesystem::canonical(rawInput, error);
        if (error || !std::filesystem::is_regular_file(candidate.input, error)) return Fail(outError, "input SCB must be an existing file", engine::assets::AssetResult::InvalidPath);
        if (!Contains(candidate.dataRoot, candidate.input)) return Fail(outError, "input SCB must remain inside data root", engine::assets::AssetResult::InvalidPath);

        const auto absoluteOutput = std::filesystem::absolute(rawOutputRoot, error);
        if (error) return Fail(outError, "output path cannot be made absolute", engine::assets::AssetResult::InvalidPath);
        if (std::filesystem::exists(absoluteOutput, error))
        {
            if (error || !std::filesystem::is_directory(absoluteOutput, error)) return Fail(outError, "output root must not be a regular file", engine::assets::AssetResult::InvalidPath);
            candidate.outputRoot = std::filesystem::canonical(absoluteOutput, error);
        }
        else
        {
            const auto parent = std::filesystem::weakly_canonical(absoluteOutput.parent_path(), error);
            if (error) return Fail(outError, "output parent cannot be canonicalized", engine::assets::AssetResult::InvalidPath);
            candidate.outputRoot = (parent / absoluteOutput.filename()).lexically_normal();
        }
        if (error || !Contains(candidate.dataRoot, candidate.outputRoot)) return Fail(outError, "output root must remain inside canonical data root", engine::assets::AssetResult::InvalidPath);
        if (Equal(candidate.outputRoot, candidate.dataRoot)) return Fail(outError, "output root cannot equal data root", engine::assets::AssetResult::InvalidPath);
        if (Equal(candidate.outputRoot, candidate.input.parent_path())) return Fail(outError, "output root cannot equal input directory", engine::assets::AssetResult::InvalidPath);
        if (Contains(candidate.outputRoot, candidate.input)) return Fail(outError, "output root cannot contain input SCB", engine::assets::AssetResult::InvalidPath);
        if (Equal(candidate.outputRoot.parent_path(), candidate.dataRoot)) return Fail(outError, "output root must be a dedicated nested model directory", engine::assets::AssetResult::InvalidPath);

        candidate.temporary = candidate.outputRoot; candidate.temporary += L".tmp."; candidate.temporary += nonce;
        candidate.backup = candidate.outputRoot; candidate.backup += L".backup."; candidate.backup += nonce;
        if (UnsafeDerived(candidate, candidate.temporary) || UnsafeDerived(candidate, candidate.backup))
            return Fail(outError, "temporary or backup directory intersects source assets", engine::assets::AssetResult::InvalidPath);
        outPaths = std::move(candidate);
        return engine::assets::AssetResult::Success;
    }
    catch (const std::bad_alloc&) { return engine::assets::AssetResult::OutOfMemory; }
    catch (...) { return Fail(outError, "unexpected filesystem validation failure", engine::assets::AssetResult::InvalidPath); }
}

engine::assets::AssetResult PrepareTemporaryDirectory(const CookPaths& paths, std::string& outError) noexcept
{
    outError.clear(); std::error_code error;
    std::filesystem::remove_all(paths.temporary, error);
    if (error) return Fail(outError, "failed to remove stale temporary directory", engine::assets::AssetResult::IoError);
    std::filesystem::create_directories(paths.temporary, error);
    if (error) return Fail(outError, "failed to create temporary output directory", engine::assets::AssetResult::IoError);
    return engine::assets::AssetResult::Success;
}

engine::assets::AssetResult CommitDirectory(const CookPaths& paths, const bool force, std::string& outError,
                                             const TransactionTestFailure testFailure) noexcept
{
    outError.clear(); std::error_code error;
    const auto cleanupTemporary = [&]() noexcept { std::error_code ignored; std::filesystem::remove_all(paths.temporary, ignored); };
    if (!std::filesystem::exists(paths.temporary, error) || error) { cleanupTemporary(); return Fail(outError, "temporary output is missing", engine::assets::AssetResult::IoError); }
    if (!std::filesystem::exists(paths.outputRoot, error))
    {
        std::filesystem::rename(paths.temporary, paths.outputRoot, error);
        if (error) { cleanupTemporary(); return Fail(outError, "failed to publish new output directory", engine::assets::AssetResult::IoError); }
        return engine::assets::AssetResult::Success;
    }
    if (!force) { cleanupTemporary(); return engine::assets::AssetResult::AlreadyExists; }
    std::filesystem::remove_all(paths.backup, error);
    if (error) { cleanupTemporary(); return Fail(outError, "failed to remove stale backup directory", engine::assets::AssetResult::IoError); }
    std::filesystem::rename(paths.outputRoot, paths.backup, error);
    if (error) { cleanupTemporary(); return Fail(outError, "failed to preserve existing output as backup", engine::assets::AssetResult::IoError); }

    if (testFailure != TransactionTestFailure::AfterBackup)
        std::filesystem::rename(paths.temporary, paths.outputRoot, error);
    else error = std::make_error_code(std::errc::permission_denied);
    if (error)
    {
        std::error_code rollbackError;
        if (std::filesystem::exists(paths.outputRoot, rollbackError)) std::filesystem::remove_all(paths.outputRoot, rollbackError);
        rollbackError.clear();
        std::filesystem::rename(paths.backup, paths.outputRoot, rollbackError);
        cleanupTemporary();
        if (rollbackError) return Fail(outError, "publish failed and rollback could not restore old output; backup was preserved", engine::assets::AssetResult::IoError);
        return Fail(outError, "publish failed; old output was restored", engine::assets::AssetResult::IoError);
    }
    std::filesystem::remove_all(paths.backup, error);
    if (error) return Fail(outError, "new output published but old backup cleanup failed", engine::assets::AssetResult::IoError);
    return engine::assets::AssetResult::Success;
}
}
