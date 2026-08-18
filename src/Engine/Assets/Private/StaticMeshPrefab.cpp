#include "Assets/StaticMeshPrefab.h"

#include <algorithm>
#include <cstddef>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <new>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace engine::assets
{
    namespace
    {
        constexpr std::uint32_t
            CurrentPrefabVersion = 1U;

        constexpr std::size_t
            MaximumPrefabParts = 4096U;

        constexpr std::size_t
            MaximumPrefabNameLength = 256U;

        [[nodiscard]]
        bool HasStaticMeshSuffix(
            const AssetPath& path) noexcept
        {
            const std::string_view value =
                path.View();

            constexpr std::string_view suffix =
                ".mesh";

            return
                value.size() >= suffix.size() &&
                value.substr(
                    value.size() -
                        suffix.size()) ==
                    suffix;
        }
    }

    void StaticMeshPrefab::Clear() noexcept
    {
        name.clear();
        parts.clear();
    }

    bool StaticMeshPrefab::IsValid() const noexcept
    {
        if (
            name.empty() ||
            name.size() >
                MaximumPrefabNameLength ||
            parts.empty() ||
            parts.size() >
                MaximumPrefabParts)
        {
            return false;
        }

        std::unordered_set<std::string>
            meshPaths;

        try
        {
            meshPaths.reserve(parts.size());
        }
        catch (...)
        {
            return false;
        }

        for (
            const StaticMeshPrefabPart& part :
            parts)
        {
            if (
                part.name.empty() ||
                part.name.size() >
                    MaximumPrefabNameLength ||
                !part.meshPath.IsValid() ||
                !HasStaticMeshSuffix(
                    part.meshPath))
            {
                return false;
            }

            if (
                !meshPaths.insert(
                    part.meshPath.String()).
                    second)
            {
                /*
                 * Один и тот же .mesh может иметь
                 * несколько FBX instances.
                 *
                 * Пока импортёр создаёт отдельный
                 * .mesh для каждого instance, поэтому
                 * дубликат пути считается ошибкой.
                 */
                return false;
            }
        }

        return true;
    }

    AssetResult StaticMeshPrefabCodec::Save(
        const std::filesystem::path& path,
        const StaticMeshPrefab& prefab,
        const bool overwriteExisting,
        std::wstring& error) noexcept
    {
        error.clear();

        try
        {
            if (
                path.empty() ||
                !prefab.IsValid())
            {
                error =
                    L"Invalid prefab output data.";

                return AssetResult::InvalidArgument;
            }

            std::wstring extension =
                path.extension().wstring();

            std::transform(
                extension.begin(),
                extension.end(),
                extension.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            if (extension != L".prefab")
            {
                error =
                    L"Prefab output file must use "
                    L"the .prefab extension.";

                return AssetResult::InvalidPath;
            }

            std::error_code filesystemError;

            std::filesystem::create_directories(
                path.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    L"Failed to create prefab "
                    L"output directory.";

                return AssetResult::IoError;
            }

            filesystemError.clear();

            if (
                !overwriteExisting &&
                std::filesystem::exists(
                    path,
                    filesystemError) &&
                !filesystemError)
            {
                error =
                    L"Prefab file already exists.";

                return AssetResult::AlreadyExists;
            }

            std::filesystem::path temporary =
                path;

            temporary += L".tmp";

            filesystemError.clear();

            std::filesystem::remove(
                temporary,
                filesystemError);

            std::ofstream output(
                temporary,
                std::ios::binary |
                std::ios::trunc);

            if (!output)
            {
                error =
                    L"Failed to create temporary "
                    L"prefab file.";

                return AssetResult::IoError;
            }

            output
                << "LTS_PREFAB "
                << CurrentPrefabVersion
                << '\n';

            output
                << "name "
                << std::quoted(prefab.name)
                << '\n';

            for (
                const StaticMeshPrefabPart& part :
                prefab.parts)
            {
                output
                    << "part "
                    << std::quoted(part.name)
                    << ' '
                    << std::quoted(
                        part.meshPath.String())
                    << '\n';
            }

            output.flush();
            output.close();

            if (!output)
            {
                filesystemError.clear();

                std::filesystem::remove(
                    temporary,
                    filesystemError);

                error =
                    L"Failed to write complete "
                    L"prefab file.";

                return AssetResult::IoError;
            }

            if (overwriteExisting)
            {
                filesystemError.clear();

                std::filesystem::remove(
                    path,
                    filesystemError);
            }

            filesystemError.clear();

            std::filesystem::rename(
                temporary,
                path,
                filesystemError);

            if (filesystemError)
            {
                std::error_code cleanupError;

                std::filesystem::remove(
                    temporary,
                    cleanupError);

                error =
                    L"Failed to replace destination "
                    L"prefab file.";

                return AssetResult::IoError;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            error =
                L"Not enough memory to save prefab.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            error =
                L"Unexpected prefab save failure.";

            return AssetResult::InternalError;
        }
    }

    AssetResult StaticMeshPrefabCodec::Load(
        const std::filesystem::path& path,
        StaticMeshPrefab& prefab,
        std::wstring& error) noexcept
    {
        prefab.Clear();
        error.clear();

        try
        {
            std::ifstream input(
                path,
                std::ios::binary);

            if (!input)
            {
                error =
                    L"Failed to open prefab file.";

                return AssetResult::NotFound;
            }

            std::string magic;
            std::uint32_t version = 0U;

            if (
                !(input >> magic >> version) ||
                magic != "LTS_PREFAB")
            {
                error =
                    L"Prefab file has an invalid "
                    L"header.";

                return AssetResult::CorruptData;
            }

            if (
                version !=
                    CurrentPrefabVersion)
            {
                error =
                    L"Prefab version is not "
                    L"supported.";

                return AssetResult::
                    UnsupportedFormat;
            }

            std::string command;

            while (input >> command)
            {
                if (command == "name")
                {
                    if (
                        !(input >>
                            std::quoted(
                                prefab.name)))
                    {
                        prefab.Clear();

                        error =
                            L"Prefab name is invalid.";

                        return AssetResult::
                            CorruptData;
                    }

                    continue;
                }

                if (command == "part")
                {
                    std::string partName;
                    std::string meshPathString;

                    if (
                        !(input >>
                            std::quoted(partName) >>
                            std::quoted(
                                meshPathString)))
                    {
                        prefab.Clear();

                        error =
                            L"Prefab part is invalid.";

                        return AssetResult::
                            CorruptData;
                    }

                    AssetPath meshPath;

                    const AssetResult pathResult =
                        AssetPath::TryCreate(
                            meshPathString,
                            meshPath);

                    if (Failed(pathResult))
                    {
                        prefab.Clear();

                        error =
                            L"Prefab contains an "
                            L"invalid mesh path.";

                        return pathResult;
                    }

                    StaticMeshPrefabPart part;

                    part.name =
                        std::move(partName);

                    part.meshPath =
                        std::move(meshPath);

                    prefab.parts.push_back(
                        std::move(part));

                    if (
                        prefab.parts.size() >
                            MaximumPrefabParts)
                    {
                        prefab.Clear();

                        error =
                            L"Prefab contains too many "
                            L"parts.";

                        return AssetResult::
                            FileTooLarge;
                    }

                    continue;
                }

                prefab.Clear();

                error =
                    L"Prefab contains an unknown "
                    L"command.";

                return AssetResult::CorruptData;
            }

            if (!prefab.IsValid())
            {
                prefab.Clear();

                error =
                    L"Prefab failed validation.";

                return AssetResult::CorruptData;
            }

            return AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            prefab.Clear();

            error =
                L"Not enough memory to load prefab.";

            return AssetResult::OutOfMemory;
        }
        catch (...)
        {
            prefab.Clear();

            error =
                L"Unexpected prefab load failure.";

            return AssetResult::InternalError;
        }
    }
}