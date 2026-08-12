#include "LegacyLevelDataLoader.h"

#include <Assets/AssetResult.h>
#include <Assets/LegacyMeshImporter.h>

#include <pugixml.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace studio::editor
{
    namespace
    {
        constexpr std::uint32_t ScbSignature =
            0xFADC0038U;

        constexpr std::size_t MaximumReportedErrors =
            12U;

        struct MeshReference final
        {
            bool sourceReference = false;

            std::filesystem::path sourcePath;
            std::filesystem::path meshPath;
            std::filesystem::path logicalMeshPath;
        };

        struct CachedMesh final
        {
            bool available = false;

            std::filesystem::path path;
        };

        struct PendingObject final
        {
            pugi::xml_node node;

            std::wstring originalName;

            std::filesystem::path meshPath;
            std::filesystem::path logicalMeshPath;

            engine::scene::SceneTransform transform;

            std::size_t objectIndex = 0U;

            bool rewriteXml = false;
        };

        [[nodiscard]]
        std::wstring ToWide(
            const char* const text)
        {
            if (
                text == nullptr ||
                *text == '\0')
            {
                return {};
            }

            return
                std::filesystem::u8path(text).
                    generic_wstring();
        }

        [[nodiscard]]
        std::wstring Lowercase(
            std::wstring value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(
                        std::towlower(character));
                });

            return value;
        }

        [[nodiscard]]
        bool IsSafeRelativePath(
            const std::filesystem::path& path) noexcept
        {
            if (
                path.empty() ||
                path.is_absolute() ||
                path.has_root_path())
            {
                return false;
            }

            for (
                const std::filesystem::path& component :
                path)
            {
                if (component == L"..")
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool ExtractRelativePath(
            const std::filesystem::path& logicalPath,
            const std::wstring_view rootName,
            std::filesystem::path& relativePath)
        {
            relativePath.clear();

            if (!IsSafeRelativePath(logicalPath))
            {
                return false;
            }

            auto component =
                logicalPath.begin();

            if (
                component == logicalPath.end() ||
                Lowercase(component->wstring()) !=
                    L"data")
            {
                return false;
            }

            ++component;

            if (
                component == logicalPath.end() ||
                Lowercase(component->wstring()) !=
                    rootName)
            {
                return false;
            }

            ++component;

            for (
                ;
                component != logicalPath.end();
                ++component)
            {
                relativePath /= *component;
            }

            return
                IsSafeRelativePath(relativePath);
        }

        [[nodiscard]]
        bool BuildMeshReference(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& logicalPath,
            MeshReference& reference)
        {
            reference = {};

            const std::filesystem::path normalized =
                logicalPath.lexically_normal();

            const std::wstring extension =
                Lowercase(
                    normalized.extension().wstring());

            std::filesystem::path relativePath;

            /*
             * LevelData.xml уже был мигрирован.
             * Runtime читает только StaticMeshes/*.mesh.
             */
            if (extension == L".mesh")
            {
                if (!ExtractRelativePath(
                        normalized,
                        L"staticmeshes",
                        relativePath))
                {
                    return false;
                }

                reference.meshPath =
                    workspaceRoot /
                    L"bin" /
                    L"Data" /
                    L"StaticMeshes" /
                    relativePath;

                reference.logicalMeshPath =
                    std::filesystem::path(L"Data") /
                    L"StaticMeshes" /
                    relativePath;

                return true;
            }

            /*
             * В старом XML может быть написано .sco или .scb.
             * В обоих случаях физически читается только SCB.
             */
            if (
                extension != L".sco" &&
                extension != L".scb")
            {
                return false;
            }

            if (!ExtractRelativePath(
                    normalized,
                    L"objectsdepot",
                    relativePath))
            {
                return false;
            }

            std::filesystem::path sourceRelativePath =
                relativePath;

            sourceRelativePath.replace_extension(
                L".scb");

            std::filesystem::path meshRelativePath =
                relativePath;

            meshRelativePath.replace_extension(
                L".mesh");

            reference.sourceReference = true;

            reference.sourcePath =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"ObjectsDepot" /
                sourceRelativePath;

            reference.meshPath =
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"StaticMeshes" /
                meshRelativePath;

            reference.logicalMeshPath =
                std::filesystem::path(L"Data") /
                L"StaticMeshes" /
                meshRelativePath;

            return true;
        }

        [[nodiscard]]
        bool HasScbSignature(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::ifstream input(
                    path,
                    std::ios::binary);

                std::uint32_t signature = 0U;

                return
                    input.read(
                        reinterpret_cast<char*>(
                            &signature),
                        sizeof(signature)).
                        good() &&
                    signature == ScbSignature;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        float AttributeFloat(
            const pugi::xml_attribute& attribute,
            const float fallback) noexcept
        {
            return
                attribute
                    ? attribute.as_float()
                    : fallback;
        }

        void ReadTransform(
            const pugi::xml_node& objectNode,
            engine::scene::SceneTransform& transform) noexcept
        {
            const pugi::xml_node position =
                objectNode.child("position");

            transform.position =
            {
                AttributeFloat(
                    position.attribute("x"),
                    0.0F),

                AttributeFloat(
                    position.attribute("y"),
                    0.0F),

                AttributeFloat(
                    position.attribute("z"),
                    0.0F)
            };

            const pugi::xml_node gameObject =
                objectNode.child("gameObject");

            pugi::xml_node rotation =
                gameObject.child("rotation");

            if (!rotation)
            {
                rotation =
                    objectNode.child("rotation");
            }

            transform.rotationDegrees =
            {
                AttributeFloat(
                    rotation.attribute("x"),
                    0.0F),

                AttributeFloat(
                    rotation.attribute("y"),
                    0.0F),

                AttributeFloat(
                    rotation.attribute("z"),
                    0.0F)
            };

            pugi::xml_node scale =
                gameObject.child("scale");

            if (!scale)
            {
                scale =
                    objectNode.child("scale");
            }

            if (scale)
            {
                transform.scale =
                {
                    AttributeFloat(
                        scale.attribute("x"),
                        1.0F),

                    AttributeFloat(
                        scale.attribute("y"),
                        1.0F),

                    AttributeFloat(
                        scale.attribute("z"),
                        1.0F)
                };
            }
        }

        void AddError(
            std::vector<std::string>& errors,
            const std::string_view message,
            const std::filesystem::path& path)
        {
            if (
                errors.size() >=
                MaximumReportedErrors)
            {
                return;
            }

            std::string text(message);

            text += " ";
            text += path.generic_u8string();

            errors.push_back(
                std::move(text));
        }

        [[nodiscard]]
        std::filesystem::path FindSourceTexturesDirectory(
            const std::filesystem::path& sourceMeshPath)
        {
            std::filesystem::path directory =
                sourceMeshPath.parent_path();

            while (!directory.empty())
            {
                if (
                    Lowercase(
                        directory.filename().wstring()) ==
                    L"objectsdepot")
                {
                    break;
                }

                std::error_code error;

                const std::filesystem::path textures =
                    directory / L"Textures";

                if (
                    std::filesystem::is_directory(
                        textures,
                        error) &&
                    !error)
                {
                    return textures;
                }

                const std::filesystem::path parent =
                    directory.parent_path();

                if (parent == directory)
                {
                    break;
                }

                directory = parent;
            }

            return {};
        }

        [[nodiscard]]
        bool PreparePackage(
            const MeshReference& reference,
            std::unordered_set<std::wstring>&
                copiedTextureDirectories,
            std::vector<std::string>& errors)
        {
            std::error_code filesystemError;

            const std::filesystem::path packageDirectory =
                reference.meshPath.parent_path();

            for (
                const wchar_t* const directoryName :
                {
                    L"Materials",
                    L"Textures",
                    L"Physics"
                })
            {
                filesystemError.clear();

                std::filesystem::create_directories(
                    packageDirectory /
                        directoryName,
                    filesystemError);

                if (filesystemError)
                {
                    AddError(
                        errors,
                        "Cannot create output directory:",
                        packageDirectory /
                            directoryName);

                    return false;
                }
            }

            const std::filesystem::path sourceTextures =
                FindSourceTexturesDirectory(
                    reference.sourcePath);

            if (sourceTextures.empty())
            {
                return true;
            }

            const std::wstring textureKey =
                Lowercase(
                    sourceTextures.
                        lexically_normal().
                        wstring());

            if (
                !copiedTextureDirectories.
                    insert(textureKey).
                    second)
            {
                return true;
            }

            filesystemError.clear();

            std::filesystem::copy(
                sourceTextures,
                packageDirectory /
                    L"Textures",
                std::filesystem::copy_options::recursive |
                    std::filesystem::copy_options::update_existing,
                filesystemError);

            if (filesystemError)
            {
                AddError(
                    errors,
                    "Cannot copy original textures:",
                    sourceTextures);

                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool MeshFileExists(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::error_code error;

                if (
                    !std::filesystem::is_regular_file(
                        path,
                        error) ||
                    error)
                {
                    return false;
                }

                const std::uintmax_t fileSize =
                    std::filesystem::file_size(
                        path,
                        error);

                return
                    !error &&
                    fileSize >= 160U;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool MaterialSetExists(const std::filesystem::path& meshPath) noexcept
        {
            try
            {
                const std::filesystem::path directory =
                    meshPath.parent_path() /
                    L"Materials";

                std::error_code error;

                if (
                    !std::filesystem::is_directory(
                        directory,
                        error) ||
                    error)
                {
                    return false;
                }

                const std::wstring prefix =
                    Lowercase(
                        meshPath.stem().wstring() +
                        L"_");

                for (
                    std::filesystem::directory_iterator
                        iterator(directory, error),
                        end;

                    !error &&
                    iterator != end;

                    iterator.increment(error))
                {
                    if (
                        !iterator->is_regular_file(error) ||
                        error)
                    {
                        error.clear();
                        continue;
                    }

                    const std::filesystem::path file =
                        iterator->path();

                    if (
                        Lowercase(
                            file.extension().wstring()) !=
                            L".material")
                    {
                        continue;
                    }

                    const std::wstring filename =
                        Lowercase(
                            file.filename().wstring());

                    if (
                        filename.rfind(
                            prefix,
                            0U) != 0U)
                    {
                        continue;
                    }

                    const std::uintmax_t size =
                        std::filesystem::file_size(
                            file,
                            error);

                    if (
                        !error &&
                        size >= 192U)
                    {
                        return true;
                    }

                    error.clear();
                }
            }
            catch (...)
            {
            }

            return false;
        }

        [[nodiscard]]
        CachedMesh ResolveMesh(
            const MeshReference& reference,
            LegacyLevelLoadStats& stats,
            std::unordered_map<
                std::wstring,
                CachedMesh>& meshCache,
            std::unordered_set<std::wstring>&
                copiedTextureDirectories,
            std::vector<std::string>& errors)
        {
            const std::wstring key =
                Lowercase(
                    reference.logicalMeshPath.
                        generic_wstring());

            const auto existing =
                meshCache.find(key);

            if (existing != meshCache.end())
            {
                return existing->second;
            }

            ++stats.uniqueMeshes;

            CachedMesh result;
            result.path = reference.meshPath;

            /*
             * XML уже указывает на готовый .mesh.
             */
            if (!reference.sourceReference)
            {
                result.available =
                    MeshFileExists(
                        reference.meshPath);

                if (result.available)
                {
                    ++stats.cachedMeshes;
                }
                else
                {
                    ++stats.failedMeshes;

                    AddError(
                        errors,
                        "Missing or invalid .mesh:",
                        reference.meshPath);
                }

                meshCache.emplace(
                    key,
                    result);

                return result;
            }

            std::error_code filesystemError;

            if (!std::filesystem::is_regular_file(reference.sourcePath, filesystemError) || filesystemError || !HasScbSignature(reference.sourcePath))
            {
                if (
                    MeshFileExists(
                        reference.meshPath) &&
                    MaterialSetExists(
                        reference.meshPath))
                {
                    result.available = true;

                    ++stats.cachedMeshes;

                    meshCache.emplace(
                        key,
                        result);

                    return result;
                }

                ++stats.missingMeshes;

                AddError(
                    errors,
                    "Missing or invalid SCB:",
                    reference.sourcePath);

                meshCache.emplace(
                    key,
                    result);

                return result;
            }

            if (!PreparePackage(
                    reference,
                    copiedTextureDirectories,
                    errors))
            {
                ++stats.failedMeshes;

                meshCache.emplace(
                    key,
                    result);

                return result;
            }

            bool needsImport = !MeshFileExists(reference.meshPath) || !MaterialSetExists(reference.meshPath);
            filesystemError.clear();

            if (
                MeshFileExists(
                    reference.meshPath))
            {
                const auto sourceTime =
                    std::filesystem::last_write_time(
                        reference.sourcePath,
                        filesystemError);

                if (!filesystemError)
                {
                    const auto meshTime =
                        std::filesystem::last_write_time(
                            reference.meshPath,
                            filesystemError);

                    if (
                        !filesystemError &&
                        meshTime >= sourceTime)
                    {
                        needsImport = false;
                    }
                }
            }

            if (needsImport)
            {
                std::wstring importError;

                const engine::assets::AssetResult importResult =
                    engine::assets::LegacyMeshImporter::Import(
                        reference.sourcePath,
                        reference.meshPath,
                        importError);

                if (
                    engine::assets::Failed(
                        importResult) ||
                    !MeshFileExists(
                        reference.meshPath))
                {
                    ++stats.failedMeshes;

                    AddError(
                        errors,
                        "SCB conversion failed:",
                        reference.sourcePath);

                    meshCache.emplace(
                        key,
                        result);

                    return result;
                }

                ++stats.convertedMeshes;
            }
            else
            {
                ++stats.cachedMeshes;
            }

            result.available = true;

            meshCache.emplace(
                key,
                result);

            return result;
        }

        [[nodiscard]]
        bool SaveLevelData(
            const std::filesystem::path& levelDataPath,
            const pugi::xml_document& document,
            std::string& error)
        {
            std::filesystem::path temporaryPath =
                levelDataPath;

            temporaryPath +=
                L".mesh_migration.tmp";

            std::filesystem::path backupPath =
                levelDataPath;

            backupPath +=
                L".before_mesh_migration.bak";

            std::error_code filesystemError;

            std::filesystem::remove(
                temporaryPath,
                filesystemError);

            const std::string temporaryUtf8 =
                temporaryPath.u8string();

            if (!document.save_file(
                    temporaryUtf8.c_str(),
                    "\t",
                    pugi::format_default,
                    pugi::encoding_utf8))
            {
                error =
                    "Cannot write temporary LevelData.xml.";

                return false;
            }

            /*
             * Backup создаётся только один раз.
             */
            if (!CopyFileW(
                    levelDataPath.c_str(),
                    backupPath.c_str(),
                    TRUE))
            {
                const DWORD copyError =
                    GetLastError();

                if (copyError != ERROR_FILE_EXISTS)
                {
                    std::filesystem::remove(
                        temporaryPath,
                        filesystemError);

                    error =
                        "Cannot create LevelData.xml backup.";

                    return false;
                }
            }

            /*
             * Оригинальный XML заменяется только после полной
             * конвертации всех уникальных SCB.
             */
            if (!MoveFileExW(
                    temporaryPath.c_str(),
                    levelDataPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(
                    temporaryPath,
                    filesystemError);

                error =
                    "Cannot replace LevelData.xml.";

                return false;
            }

            return true;
        }

        [[nodiscard]]
        std::string BuildWarningMessage(
            const LegacyLevelLoadStats& stats,
            const std::vector<std::string>& errors)
        {
            if (
                stats.missingMeshes == 0U &&
                stats.failedMeshes == 0U)
            {
                return {};
            }

            std::string message =
                "Static mesh migration completed with warnings. ";

            message += "Missing SCB: ";
            message +=
                std::to_string(
                    stats.missingMeshes);

            message += ", failed: ";
            message +=
                std::to_string(
                    stats.failedMeshes);

            for (const std::string& item : errors)
            {
                message += "\n";
                message += item;
            }

            return message;
        }
    }

    LegacyLevelLoadResult LoadLegacyLevelData(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelDataPath,
        const std::wstring& mapName) noexcept
    {
        LegacyLevelLoadResult result;

        /*
         * mapName раньше использовался для
         * game/Cache/LegacyLevels.
         * Этого cache-каталога больше нет.
         */
        static_cast<void>(mapName);

        try
        {
            if (
                workspaceRoot.empty() ||
                levelDataPath.empty())
            {
                result.error =
                    "Workspace or LevelData.xml path is empty.";

                return result;
            }

            pugi::xml_document document;

            const std::string levelDataUtf8 =
                levelDataPath.u8string();

            const pugi::xml_parse_result parseResult =
                document.load_file(
                    levelDataUtf8.c_str());

            if (!parseResult)
            {
                result.error =
                    "Cannot parse LevelData.xml: ";

                result.error +=
                    parseResult.description();

                return result;
            }

            const pugi::xml_node level =
                document.child("level");

            if (!level)
            {
                result.error =
                    "LevelData.xml has no <level> root element.";

                return result;
            }

            std::vector<PendingObject>
                pendingObjects;

            pendingObjects.reserve(20000U);

            std::unordered_map<
                std::wstring,
                CachedMesh> meshCache;

            meshCache.reserve(1024U);

            std::unordered_set<std::wstring>
                copiedTextureDirectories;

            copiedTextureDirectories.reserve(
                256U);

            std::vector<std::string>
                reportedErrors;

            reportedErrors.reserve(
                MaximumReportedErrors);

            std::size_t objectIndex = 0U;

            for (
                pugi::xml_node object =
                    level.child("object");

                object;

                object =
                    object.next_sibling("object"))
            {
                ++objectIndex;
                ++result.stats.totalObjects;

                const std::wstring className =
                    ToWide(
                        object.attribute(
                            "className").
                            value());

                if (className != L"obj_Building")
                {
                    continue;
                }

                const std::wstring fileName =
                    ToWide(
                        object.attribute(
                            "fileName").
                            value());

                MeshReference reference;

                if (
                    fileName.empty() ||
                    !BuildMeshReference(
                        workspaceRoot,
                        std::filesystem::path(
                            fileName),
                        reference))
                {
                    ++result.stats.failedMeshes;

                    AddError(
                        reportedErrors,
                        "Unsupported obj_Building path:",
                        std::filesystem::path(
                            fileName));

                    continue;
                }

                const CachedMesh mesh =
                    ResolveMesh(
                        reference,
                        result.stats,
                        meshCache,
                        copiedTextureDirectories,
                        reportedErrors);

                bool available = false;

                PendingObject pending;

                pending.node = object;
                pending.originalName = fileName;
                pending.meshPath = mesh.path;
                pending.available = mesh.available;
                pending.logicalMeshPath = reference.logicalMeshPath;
                pending.objectIndex = objectIndex;
                pending.rewriteXml = reference.sourceReference;

                ReadTransform(object, pending.transform);
                pendingObjects.push_back(std::move(pending));
            }

            bool xmlChanged = false;

            for (
                const PendingObject& pending :
                pendingObjects)
            {
                if (!pending.rewriteXml)
                {
                    continue;
                }

                pugi::xml_attribute fileName =
                    pending.node.attribute(
                        "fileName");

                const std::string meshPath =
                    pending.logicalMeshPath.
                        generic_u8string();

                if (
                    !fileName ||
                    !fileName.set_value(
                        meshPath.c_str()))
                {
                    result.error =
                        "Cannot update obj_Building path. "
                        "LevelData.xml was not changed.";

                    return result;
                }

                xmlChanged = true;
            }

            if (
                xmlChanged &&
                !SaveLevelData(
                    levelDataPath,
                    document,
                    result.error))
            {
                return result;
            }

            result.entities.reserve(
                pendingObjects.size());

            for (
                const PendingObject& pending :
                pendingObjects)
            {
                engine::scene::SceneEntity entity;

                entity.kind =
                    engine::scene::
                        SceneEntityKind::Empty;

                if (!pending.available)
                {
                    continue;
                }

                entity.transform =
                    pending.transform;

                entity.name =
                    std::filesystem::path(
                        pending.originalName).
                        stem().
                        wstring();

                entity.name +=
                    L" #" +
                    std::to_wstring(
                        pending.objectIndex);

                entity.editorFolder =
                    L"LevelData/obj_Building";

                entity.staticMesh.emplace();

                entity.staticMesh->assetPath =
                    pending.meshPath.
                        lexically_normal().
                        generic_wstring();

                entity.staticMesh->visible = true;

                entity.staticMesh->castShadows =
                    true;

                result.entities.push_back(
                    std::move(entity));
            }

            result.stats.importedObjects =
                result.entities.size();

            result.stats.staticMeshObjects =
                result.entities.size();

            result.warning =BuildWarningMessage(result.stats, reportedErrors);
            result.succeeded = true;

            return result;
        }
        catch (const std::exception& exception)
        {
            result.error =
                "LevelData.xml import failed: ";

            result.error +=
                exception.what();

            return result;
        }
        catch (...)
        {
            result.error =
                "Unexpected LevelData.xml import failure.";

            return result;
        }
    }
}