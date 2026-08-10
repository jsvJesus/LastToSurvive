#include "LegacyLevelDataLoader.h"

#include <Assets/AssetResult.h>
#include <Assets/LegacyMeshImporter.h>

#include <pugixml.hpp>

#include <algorithm>
#include <cwctype>
#include <system_error>
#include <unordered_map>
#include <utility>

namespace studio::editor
{
    namespace
    {
        [[nodiscard]] std::wstring ToWide(const char* const text)
        {
            if (text == nullptr || *text == '\0')
            {
                return {};
            }

            return std::filesystem::u8path(text).generic_wstring();
        }

        [[nodiscard]] std::wstring Lowercase(std::wstring value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const wchar_t character)
                {
                    return static_cast<wchar_t>(std::towlower(character));
                });

            return value;
        }

        [[nodiscard]] bool IsSafeRelativePath(
            const std::filesystem::path& path) noexcept
        {
            if (path.empty() || path.is_absolute() || path.has_root_path())
            {
                return false;
            }

            for (const std::filesystem::path& part : path)
            {
                if (part == L"..")
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] float AttributeFloat(
            const pugi::xml_attribute& attribute,
            const float fallback) noexcept
        {
            return attribute ? attribute.as_float() : fallback;
        }

        void ReadTransform(
            const pugi::xml_node& objectNode,
            engine::scene::SceneTransform& transform) noexcept
        {
            const pugi::xml_node position = objectNode.child("position");
            transform.position =
            {
                AttributeFloat(position.attribute("x"), 0.0F),
                AttributeFloat(position.attribute("y"), 0.0F),
                AttributeFloat(position.attribute("z"), 0.0F)
            };

            const pugi::xml_node gameObject = objectNode.child("gameObject");
            pugi::xml_node rotation = gameObject.child("rotation");

            if (!rotation)
            {
                rotation = objectNode.child("rotation");
            }

            transform.rotationDegrees =
            {
                AttributeFloat(rotation.attribute("x"), 0.0F),
                AttributeFloat(rotation.attribute("y"), 0.0F),
                AttributeFloat(rotation.attribute("z"), 0.0F)
            };

            pugi::xml_node scale = gameObject.child("scale");

            if (!scale)
            {
                scale = objectNode.child("scale");
            }

            if (scale)
            {
                transform.scale =
                {
                    AttributeFloat(scale.attribute("x"), 1.0F),
                    AttributeFloat(scale.attribute("y"), 1.0F),
                    AttributeFloat(scale.attribute("z"), 1.0F)
                };
            }
        }

        struct CachedMeshPath final
        {
            bool available = false;
            std::wstring path;
        };

        [[nodiscard]] CachedMeshPath ResolveMesh(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& cacheRoot,
            const std::filesystem::path& logicalPath,
            LegacyLevelLoadStats& stats,
            std::unordered_map<std::wstring, CachedMeshPath>& meshCache)
        {
            const std::wstring key = Lowercase(logicalPath.generic_wstring());
            const auto existing = meshCache.find(key);

            if (existing != meshCache.end())
            {
                return existing->second;
            }

            ++stats.uniqueMeshes;
            CachedMeshPath resolved;
            std::filesystem::path sourcePath =
                workspaceRoot / L"bin" / logicalPath;
            std::error_code error;

            if (!std::filesystem::is_regular_file(sourcePath, error) || error)
            {
                error.clear();
                std::filesystem::path alternateSource = sourcePath;
                const std::wstring extension =
                    Lowercase(alternateSource.extension().wstring());
                alternateSource.replace_extension(
                    extension == L".sco" ? L".scb" : L".sco");

                if (
                    !std::filesystem::is_regular_file(alternateSource, error) ||
                    error)
                {
                    ++stats.missingMeshes;
                    meshCache.emplace(key, resolved);
                    return resolved;
                }

                sourcePath = std::move(alternateSource);
            }

            std::filesystem::path destinationPath =
                cacheRoot / L"Meshes" / logicalPath;
            destinationPath.replace_extension(L".ltsmesh");

            bool needsImport = true;
            error.clear();

            if (std::filesystem::is_regular_file(destinationPath, error) && !error)
            {
                std::filesystem::path materialSidecar = destinationPath;
                materialSidecar += L".materials";

                if (!std::filesystem::is_regular_file(materialSidecar, error) || error)
                {
                    error.clear();
                }
                else
                {
                const auto sourceTime =
                    std::filesystem::last_write_time(sourcePath, error);

                if (!error)
                {
                    const auto destinationTime =
                        std::filesystem::last_write_time(destinationPath, error);

                    if (!error && destinationTime >= sourceTime)
                    {
                        needsImport = false;
                    }
                }
                }
            }

            if (needsImport)
            {
                std::wstring importError;
                const engine::assets::AssetResult importResult =
                    engine::assets::LegacyMeshImporter::Import(
                        sourcePath,
                        destinationPath,
                        importError);

                if (engine::assets::Failed(importResult))
                {
                    ++stats.failedMeshes;
                    meshCache.emplace(key, resolved);
                    return resolved;
                }

                ++stats.convertedMeshes;
            }
            else
            {
                ++stats.cachedMeshes;
            }

            resolved.available = true;
            resolved.path = destinationPath.lexically_normal().generic_wstring();
            meshCache.emplace(key, resolved);
            return resolved;
        }
    }

    LegacyLevelLoadResult LoadLegacyLevelData(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelDataPath,
        const std::wstring& mapName) noexcept
    {
        LegacyLevelLoadResult result;

        try
        {
            if (workspaceRoot.empty() || levelDataPath.empty())
            {
                result.error = "Workspace or LevelData.xml path is empty.";
                return result;
            }

            pugi::xml_document document;
            const std::string levelDataUtf8 = levelDataPath.u8string();
            const pugi::xml_parse_result parseResult =
                document.load_file(levelDataUtf8.c_str());

            if (!parseResult)
            {
                result.error = "Cannot parse LevelData.xml: ";
                result.error += parseResult.description();
                return result;
            }

            const pugi::xml_node level = document.child("level");

            if (!level)
            {
                result.error = "LevelData.xml has no <level> root element.";
                return result;
            }

            std::size_t objectCount = 0U;

            for (pugi::xml_node object = level.child("object"); object;
                 object = object.next_sibling("object"))
            {
                ++objectCount;
            }

            result.entities.reserve(objectCount);
            std::unordered_map<std::wstring, CachedMeshPath> meshCache;
            meshCache.reserve(1024U);
            const std::filesystem::path cacheRoot =
                workspaceRoot / L"game" / L"Cache" / L"LegacyLevels" / mapName;
            std::size_t objectIndex = 0U;

            for (pugi::xml_node object = level.child("object"); object;
                 object = object.next_sibling("object"))
            {
                ++objectIndex;
                ++result.stats.totalObjects;
                const std::wstring className =
                    ToWide(object.attribute("className").value());
                const std::wstring fileName =
                    ToWide(object.attribute("fileName").value());

                if (className != L"obj_Building")
                {
                    continue;
                }

                engine::scene::SceneEntity entity;
                entity.kind = engine::scene::SceneEntityKind::Empty;
                ReadTransform(object, entity.transform);
                entity.name = fileName.empty() ? className :
                    std::filesystem::path(fileName).stem().wstring();
                entity.name += L" #" + std::to_wstring(objectIndex);
                entity.editorFolder = L"LevelData/" + className;

                if (!fileName.empty())
                {
                    const std::filesystem::path logicalPath =
                        std::filesystem::path(fileName).lexically_normal();

                    if (
                        IsSafeRelativePath(logicalPath) &&
                        engine::assets::LegacyMeshImporter::IsSupportedSource(
                            logicalPath))
                    {
                        const CachedMeshPath mesh = ResolveMesh(
                            workspaceRoot,
                            cacheRoot,
                            logicalPath,
                            result.stats,
                            meshCache);

                        if (mesh.available)
                        {
                            entity.staticMesh.emplace();
                            entity.staticMesh->assetPath = mesh.path;
                            entity.staticMesh->visible = true;
                            entity.staticMesh->castShadows = true;
                            ++result.stats.staticMeshObjects;
                        }
                    }
                }

                result.entities.push_back(std::move(entity));
                ++result.stats.importedObjects;
            }

            result.succeeded = true;
            return result;
        }
        catch (const std::exception& exception)
        {
            result.error = "LevelData.xml import failed: ";
            result.error += exception.what();
            return result;
        }
        catch (...)
        {
            result.error = "Unexpected LevelData.xml import failure.";
            return result;
        }
    }
}
