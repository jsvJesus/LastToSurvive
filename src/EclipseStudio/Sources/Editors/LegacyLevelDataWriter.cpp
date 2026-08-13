#include "LegacyLevelDataWriter.h"

#include <pugixml.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace studio::editor
{
    namespace
    {
        struct ExistingBuilding final
        {
            std::size_t objectIndex = 0U;
            pugi::xml_node node;
        };

        struct SavedNodeBinding final
        {
            engine::scene::SceneEntityId entityId = 0U;
            pugi::xml_node node;
        };

        [[nodiscard]]
        std::wstring Lowercase(
            std::wstring value)
        {
            for (wchar_t& character : value)
            {
                character =
                    static_cast<wchar_t>(
                        std::towlower(character));
            }

            return value;
        }

        [[nodiscard]]
        bool IsFiniteTransform(
            const engine::scene::SceneTransform& transform) noexcept
        {
            for (const float value : transform.position)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            for (const float value : transform.rotationDegrees)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            for (const float value : transform.scale)
            {
                if (!std::isfinite(value))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool IsManagedStaticMesh(
            const engine::scene::SceneEntity& entity)
        {
            if (!entity.staticMesh.has_value())
            {
                return false;
            }

            const std::wstring folder =
                Lowercase(entity.editorFolder);

            if (
                folder == L"leveldata/obj_road" ||
                folder == L"leveldata/obj_waterplane")
            {
                return false;
            }

            const std::filesystem::path assetPath =
                std::filesystem::path(
                    entity.staticMesh->assetPath).
                    lexically_normal();

            if (
                Lowercase(assetPath.extension().wstring()) !=
                L".mesh")
            {
                return false;
            }

            auto component = assetPath.begin();

            if (
                component == assetPath.end() ||
                Lowercase(component->wstring()) != L"data")
            {
                return false;
            }

            ++component;

            if (
                component == assetPath.end() ||
                Lowercase(component->wstring()) !=
                    L"staticmeshes")
            {
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool TryParseObjectIndex(
            const std::wstring& entityName,
            std::size_t& objectIndex) noexcept
        {
            objectIndex = 0U;

            const std::size_t marker =
                entityName.rfind(L" #");

            if (marker == std::wstring::npos)
            {
                return false;
            }

            const std::size_t firstDigit =
                marker + 2U;

            if (firstDigit >= entityName.size())
            {
                return false;
            }

            std::size_t value = 0U;

            for (
                std::size_t index = firstDigit;
                index < entityName.size();
                ++index)
            {
                const wchar_t character =
                    entityName[index];

                if (
                    character < L'0' ||
                    character > L'9')
                {
                    return false;
                }

                const std::size_t digit =
                    static_cast<std::size_t>(
                        character - L'0');

                if (
                    value >
                    (
                        (std::numeric_limits<std::size_t>::max)() -
                        digit
                    ) /
                    10U)
                {
                    return false;
                }

                value =
                    value * 10U +
                    digit;
            }

            if (value == 0U)
            {
                return false;
            }

            objectIndex = value;

            return true;
        }

        [[nodiscard]]
        pugi::xml_attribute EnsureAttribute(
            pugi::xml_node node,
            const char* const name)
        {
            pugi::xml_attribute attribute =
                node.attribute(name);

            if (!attribute)
            {
                attribute =
                    node.append_attribute(name);
            }

            return attribute;
        }

        [[nodiscard]]
        pugi::xml_node EnsureChild(
            pugi::xml_node parent,
            const char* const name)
        {
            pugi::xml_node child =
                parent.child(name);

            if (!child)
            {
                child =
                    parent.append_child(name);
            }

            return child;
        }

        [[nodiscard]]
        bool SetStringAttribute(
            pugi::xml_node node,
            const char* const name,
            const char* const value)
        {
            const pugi::xml_attribute attribute =
                EnsureAttribute(
                    node,
                    name);

            return
                attribute &&
                attribute.set_value(value);
        }

        [[nodiscard]]
        bool SetFloatAttribute(
            pugi::xml_node node,
            const char* const name,
            const float value)
        {
            const pugi::xml_attribute attribute =
                EnsureAttribute(
                    node,
                    name);

            return
                attribute &&
                attribute.set_value(value);
        }

        [[nodiscard]]
        bool SetUnsignedAttribute(
            pugi::xml_node node,
            const char* const name,
            const std::uint32_t value)
        {
            const pugi::xml_attribute attribute =
                EnsureAttribute(
                    node,
                    name);

            return
                attribute &&
                attribute.set_value(value);
        }

        [[nodiscard]]
        bool WriteVector(
            pugi::xml_node node,
            const std::array<float, 3U>& value)
        {
            return
                SetFloatAttribute(
                    node,
                    "x",
                    value[0]) &&
                SetFloatAttribute(
                    node,
                    "y",
                    value[1]) &&
                SetFloatAttribute(
                    node,
                    "z",
                    value[2]);
        }

        [[nodiscard]]
        std::uint32_t BuildObjectHash(
            const engine::scene::SceneEntity& entity)
        {
            std::uint32_t hash =
                2166136261U;

            const std::string assetPath =
                std::filesystem::path(
                    entity.staticMesh->assetPath).
                    lexically_normal().
                    generic_u8string();

            for (const unsigned char value : assetPath)
            {
                hash ^= value;
                hash *= 16777619U;
            }

            std::uint64_t entityId =
                entity.id;

            for (std::uint32_t byte = 0U;
                 byte < 8U;
                 ++byte)
            {
                hash ^=
                    static_cast<std::uint8_t>(
                        entityId & 0xFFU);

                hash *= 16777619U;
                entityId >>= 8U;
            }

            return hash == 0U
                ? 1U
                : hash;
        }

        [[nodiscard]]
        bool WriteBuilding(
            pugi::xml_node object,
            const engine::scene::SceneEntity& entity,
            const bool newObject)
        {
            if (
                !entity.staticMesh.has_value() ||
                !IsFiniteTransform(entity.transform))
            {
                return false;
            }

            const std::string assetPath =
                std::filesystem::path(
                    entity.staticMesh->assetPath).
                    lexically_normal().
                    generic_u8string();

            if (
                !SetStringAttribute(
                    object,
                    "className",
                    "obj_Building") ||
                !SetStringAttribute(
                    object,
                    "fileName",
                    assetPath.c_str()))
            {
                return false;
            }

            pugi::xml_node position =
                EnsureChild(
                    object,
                    "position");

            pugi::xml_node gameObject =
                EnsureChild(
                    object,
                    "gameObject");

            if (
                !position ||
                !gameObject ||
                !WriteVector(
                    position,
                    entity.transform.position))
            {
                return false;
            }

            if (newObject)
            {
                if (
                    !SetUnsignedAttribute(
                        gameObject,
                        "hash",
                        BuildObjectHash(entity)) ||
                    !SetStringAttribute(
                        gameObject,
                        "PhysEnable",
                        "true") ||
                    !SetStringAttribute(
                        gameObject,
                        "MinQuality",
                        "1") ||
                    !SetStringAttribute(
                        gameObject,
                        "BulletPierceable",
                        "0"))
                {
                    return false;
                }
            }

            pugi::xml_node rotation =
                EnsureChild(
                    gameObject,
                    "rotation");

            pugi::xml_node scale =
                EnsureChild(
                    gameObject,
                    "scale");

            if (
                !rotation ||
                !scale ||
                !WriteVector(
                    rotation,
                    entity.transform.rotationDegrees) ||
                !WriteVector(
                    scale,
                    entity.transform.scale))
            {
                return false;
            }

            static_cast<void>(
                EnsureChild(
                    object,
                    "building"));

            return true;
        }

        [[nodiscard]]
        bool SaveDocumentAtomic(
            const std::filesystem::path& levelDataPath,
            const pugi::xml_document& document,
            std::string& error)
        {
            std::filesystem::path temporaryPath =
                levelDataPath;

            temporaryPath +=
                L".studio_save.tmp";

            std::filesystem::path backupPath =
                levelDataPath;

            backupPath +=
                L".before_studio_save.bak";

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
    }

    LegacyLevelSaveResult SaveLegacyLevelData(
        const std::filesystem::path& levelDataPath,
        const std::vector<engine::scene::SceneEntity>& entities,
        const std::vector<std::size_t>& managedObjectIndices) noexcept
    {
        LegacyLevelSaveResult result;

        try
        {
            if (levelDataPath.empty())
            {
                result.error =
                    "LevelData.xml path is empty.";

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

            pugi::xml_node level =
                document.child("level");

            if (!level)
            {
                result.error =
                    "LevelData.xml has no <level> root element.";

                return result;
            }

            std::vector<ExistingBuilding>
                existingBuildings;

            std::unordered_map<
                std::size_t,
                std::size_t>
                buildingByIndex;

            std::size_t objectIndex = 0U;

            for (
                pugi::xml_node object =
                    level.child("object");

                object;

                object =
                    object.next_sibling("object"))
            {
                ++objectIndex;

                if (std::strcmp(
                        object.attribute(
                            "className").
                            value(),
                        "obj_Building") != 0)
                {
                    continue;
                }

                const std::size_t buildingIndex =
                    existingBuildings.size();

                existingBuildings.push_back(
                    {
                        objectIndex,
                        object
                    });

                buildingByIndex.emplace(
                    objectIndex,
                    buildingIndex);
            }

            const std::unordered_set<std::size_t>
                managedIndices(
                    managedObjectIndices.begin(),
                    managedObjectIndices.end());

            std::unordered_set<std::size_t>
                claimedIndices;

            std::vector<SavedNodeBinding>
                bindings;

            bindings.reserve(
                entities.size());

            for (
                const engine::scene::SceneEntity& entity :
                entities)
            {
                if (!IsManagedStaticMesh(entity))
                {
                    continue;
                }

                pugi::xml_node object;
                bool newObject = true;

                std::size_t sourceObjectIndex = 0U;

                if (
                    TryParseObjectIndex(
                        entity.name,
                        sourceObjectIndex) &&
                    managedIndices.find(
                        sourceObjectIndex) !=
                        managedIndices.end())
                {
                    const auto existing =
                        buildingByIndex.find(
                            sourceObjectIndex);

                    if (
                        existing != buildingByIndex.end() &&
                        claimedIndices.insert(
                            sourceObjectIndex).
                            second)
                    {
                        object =
                            existingBuildings[
                                existing->second].
                                node;

                        newObject = false;
                    }
                }

                if (!object)
                {
                    object =
                        level.append_child(
                            "object");

                    if (!object)
                    {
                        result.error =
                            "Cannot append obj_Building.";

                        return result;
                    }
                }

                if (!WriteBuilding(
                        object,
                        entity,
                        newObject))
                {
                    result.error =
                        "Cannot serialize obj_Building.";

                    return result;
                }

                bindings.push_back(
                    {
                        entity.id,
                        object
                    });

                if (newObject)
                {
                    ++result.addedObjects;
                }
                else
                {
                    ++result.updatedObjects;
                }
            }

            for (
                const ExistingBuilding& existing :
                existingBuildings)
            {
                if (
                    managedIndices.find(
                        existing.objectIndex) ==
                        managedIndices.end() ||
                    claimedIndices.find(
                        existing.objectIndex) !=
                        claimedIndices.end())
                {
                    continue;
                }

                if (!level.remove_child(existing.node))
                {
                    result.error =
                        "Cannot remove deleted obj_Building.";

                    return result;
                }

                ++result.removedObjects;
            }

            std::unordered_map<
                std::size_t,
                engine::scene::SceneEntityId>
                entityByNode;

            entityByNode.reserve(
                bindings.size());

            for (
                const SavedNodeBinding& binding :
                bindings)
            {
                const bool inserted =
                    entityByNode.emplace(
                        binding.node.hash_value(),
                        binding.entityId).
                        second;

                if (!inserted)
                {
                    result.error =
                        "Duplicate XML node binding.";

                    return result;
                }
            }

            objectIndex = 0U;

            for (
                pugi::xml_node object =
                    level.child("object");

                object;

                object =
                    object.next_sibling("object"))
            {
                ++objectIndex;

                const auto entity =
                    entityByNode.find(
                        object.hash_value());

                if (entity ==
                    entityByNode.end())
                {
                    continue;
                }

                result.managedObjectIndices.push_back(
                    objectIndex);

                result.identities.push_back(
                    {
                        entity->second,
                        objectIndex
                    });
            }

            if (
                result.identities.size() !=
                bindings.size())
            {
                result.error =
                    "Cannot rebuild saved object identities.";

                return result;
            }

            if (!SaveDocumentAtomic(
                    levelDataPath,
                    document,
                    result.error))
            {
                return result;
            }

            result.succeeded = true;

            return result;
        }
        catch (const std::exception& exception)
        {
            result.error =
                "LevelData.xml save failed: ";

            result.error +=
                exception.what();

            return result;
        }
        catch (...)
        {
            result.error =
                "Unexpected LevelData.xml save failure.";

            return result;
        }
    }
}