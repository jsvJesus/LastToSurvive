#include "Editor/LevelEditor/Environment/WaterPlaneEditor.h"

#include <Assets/AssetData.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/LtsMeshWriter.h>
#include <Assets/MaterialAsset.h>
#include <Assets/MaterialAssetWriter.h>
#include <Assets/MeshAssetBuilder.h>

#include <pugixml.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        constexpr std::uint16_t WaterDataVersion = 4U;
        constexpr std::uint64_t MaximumWaterCellCount = 16000000U;

        [[nodiscard]]
        std::filesystem::path BuildRelativeMeshPath(
            const std::filesystem::path& levelRoot,
            const std::wstring& sourceName)
        {
            return
                std::filesystem::path(L"LevelData") /
                levelRoot.filename() /
                L"WaterPlanes" /
                (sourceName + L".mesh");
        }

        [[nodiscard]]
        std::filesystem::path BuildMeshPath(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelRoot,
            const std::wstring& sourceName)
        {
            return
                workspaceRoot /
                L"bin" /
                L"Data" /
                L"StaticMeshes" /
                BuildRelativeMeshPath(
                    levelRoot,
                    sourceName);
        }

        [[nodiscard]]
        std::wstring BuildLogicalMeshPath(
            const std::filesystem::path& levelRoot,
            const std::wstring& sourceName)
        {
            return
                (
                    std::filesystem::path(L"Data") /
                    L"StaticMeshes" /
                    BuildRelativeMeshPath(
                        levelRoot,
                        sourceName)
                ).generic_wstring();
        }

        [[nodiscard]]
        bool IsValidSourceName(
            const std::wstring& value) noexcept
        {
            if (
                value.empty() ||
                value.size() > 96U)
            {
                return false;
            }

            for (const wchar_t character : value)
            {
                const bool valid =
                    (character >= L'a' && character <= L'z') ||
                    (character >= L'A' && character <= L'Z') ||
                    (character >= L'0' && character <= L'9') ||
                    character == L'_' ||
                    character == L'-';

                if (!valid)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool IsValidComponent(
            const engine::scene::WaterPlaneComponent& water) noexcept
        {
            const std::uint64_t cellCount =
                static_cast<std::uint64_t>(water.gridWidth) *
                water.gridHeight;

            return
                IsValidSourceName(water.sourceName) &&
                water.gridWidth > 0U &&
                water.gridHeight > 0U &&
                cellCount <= MaximumWaterCellCount &&
                water.cells.size() ==
                    static_cast<std::size_t>(cellCount) &&
                std::isfinite(water.waterHeight) &&
                std::isfinite(water.cellSize) &&
                std::isfinite(water.planeWidth) &&
                std::isfinite(water.planeDepth) &&
                std::isfinite(water.centerX) &&
                std::isfinite(water.centerZ) &&
                water.cellSize >= 1.0F &&
                water.planeWidth > 0.0F &&
                water.planeDepth > 0.0F &&
                water.coastSmoothLevels >= 0 &&
                water.coastSmoothLevels <= 6;
        }

        [[nodiscard]]
        bool WriteAssetData(
            const std::filesystem::path& path,
            const engine::assets::AssetData& data,
            std::string& error)
        {
            std::error_code filesystemError;
            std::filesystem::create_directories(
                path.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error = "Cannot create water asset directory.";
                return false;
            }

            std::ofstream output(
                path,
                std::ios::binary |
                    std::ios::trunc);

            if (!output)
            {
                error = "Cannot open generated water asset.";
                return false;
            }

            output.write(
                reinterpret_cast<const char*>(data.GetData()),
                static_cast<std::streamsize>(data.GetSize()));

            if (!output.good())
            {
                error = "Cannot write generated water asset.";
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool CopyWaterTexture(
            const std::filesystem::path& sourcePath,
            const std::filesystem::path& destinationMeshPath,
            const std::filesystem::path& dataRoot,
            std::optional<engine::assets::AssetPath>& outputPath)
        {
            outputPath.reset();

            std::error_code error;

            if (!std::filesystem::is_regular_file(sourcePath, error) || error)
            {
                return true;
            }

            const std::filesystem::path destinationPath =
                destinationMeshPath.parent_path() /
                L"Textures" /
                sourcePath.filename();

            std::filesystem::create_directories(
                destinationPath.parent_path(),
                error);

            if (error)
            {
                return false;
            }

            std::filesystem::copy_file(
                sourcePath,
                destinationPath,
                std::filesystem::copy_options::update_existing,
                error);

            if (error)
            {
                return false;
            }

            const std::filesystem::path logicalPath =
                std::filesystem::relative(
                    destinationPath,
                    dataRoot,
                    error);

            if (error)
            {
                return false;
            }

            engine::assets::AssetPath assetPath;

            if (engine::assets::Failed(
                    engine::assets::AssetPath::TryCreate(
                        logicalPath.generic_u8string(),
                        assetPath)))
            {
                return false;
            }

            outputPath = std::move(assetPath);
            return true;
        }

        [[nodiscard]]
        bool WriteWaterMaterial(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& meshPath,
            const engine::scene::WaterPlaneComponent& water,
            std::string& error)
        {
            engine::assets::MaterialAssetDesc description;

            description.baseColorFactor =
            {
                water.waterColor[0],
                water.waterColor[1],
                water.waterColor[2],
                0.78F
            };

            description.metallicFactor =
                (std::clamp)(water.fresnelPower / 32.0F, 0.0F, 1.0F);
            description.roughnessFactor = 0.12F;
            description.alphaMode =
                engine::assets::MaterialAlphaMode::Blend;
            description.alphaCutoff =
                (std::clamp)(water.colorTiling, 0.001F, 0.1F);
            description.doubleSided = true;
            description.normalScale =
                (std::clamp)(water.fresnelBumpiness, 0.125F, 16.0F);
            description.specularIntensity =
                (std::clamp)(water.sunIntensity, 0.0F, 16.0F);
            description.specularPower =
                (std::clamp)(
                    water.sunBumpiness * 32.0F,
                    1.0F,
                    960.0F);
            description.reflectionFactor =
                (std::clamp)(water.reflectionStrength, 0.0F, 16.0F);
            description.emissiveStrength =
                (std::clamp)(water.bumpTiling, 0.001F, 0.2F);
            description.debugName = "water";
            description.sampler.filter =
                engine::graphics::TextureFilter::Anisotropic;
            description.sampler.addressU =
                engine::graphics::TextureAddressMode::Wrap;
            description.sampler.addressV =
                engine::graphics::TextureAddressMode::Wrap;
            description.sampler.addressW =
                engine::graphics::TextureAddressMode::Wrap;
            description.sampler.maximumAnisotropy = 16U;

            const std::filesystem::path dataRoot =
                workspaceRoot / L"bin" / L"Data";

            if (
                !CopyWaterTexture(
                    dataRoot / L"Water" / L"LakeColor.dds",
                    meshPath,
                    dataRoot,
                    description.baseColorTexture) ||
                !CopyWaterTexture(
                    dataRoot / L"Water" / L"waves_00.dds",
                    meshPath,
                    dataRoot,
                    description.normalTexture))
            {
                error = "Cannot copy water textures.";
                return false;
            }

            engine::assets::MaterialAsset material;

            if (engine::assets::Failed(
                    material.Initialize(std::move(description))))
            {
                error = "Generated water material is invalid.";
                return false;
            }

            engine::assets::AssetData encoded;

            if (engine::assets::Failed(
                    engine::assets::MaterialAssetWriter::Encode(
                        material,
                        encoded)))
            {
                error = "Cannot encode water material.";
                return false;
            }

            const std::filesystem::path materialPath =
                meshPath.parent_path() /
                L"Materials" /
                (meshPath.stem().wstring() +
                    L"_0000_water.material");

            return WriteAssetData(materialPath, encoded, error);
        }

        [[nodiscard]]
        bool BuildWaterAsset(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelRoot,
            const engine::scene::WaterPlaneComponent& water,
            std::wstring& logicalPath,
            std::string& error)
        {
            logicalPath.clear();

            if (!IsValidComponent(water))
            {
                error = "Water plane data is invalid.";
                return false;
            }

            const std::size_t activeCellCount =
                static_cast<std::size_t>(
                    std::count_if(
                        water.cells.begin(),
                        water.cells.end(),
                        [](const std::uint8_t value)
                        {
                            return value != 0U;
                        }));

            if (activeCellCount == 0U)
            {
                const std::filesystem::path emptyMeshPath =
                    BuildMeshPath(
                        workspaceRoot,
                        levelRoot,
                        water.sourceName);
                std::error_code removeError;

                std::filesystem::remove(
                    emptyMeshPath,
                    removeError);
                removeError.clear();
                std::filesystem::remove(
                    emptyMeshPath.parent_path() /
                        L"Materials" /
                        (water.sourceName + L"_0000_water.material"),
                    removeError);

                return true;
            }

            if (activeCellCount >
                (std::numeric_limits<std::uint32_t>::max)() / 6U)
            {
                error = "Water plane contains too many cells.";
                return false;
            }

            std::vector<engine::assets::StaticMeshVertex> vertices;
            std::vector<std::uint32_t> indices;

            vertices.reserve(activeCellCount * 4U);
            indices.reserve(activeCellCount * 6U);

            const float offsetX =
                water.centerX - water.planeWidth * 0.5F;
            const float offsetZ =
                water.centerZ - water.planeDepth * 0.5F;

            for (std::uint32_t z = 0U; z < water.gridHeight; ++z)
            {
                for (std::uint32_t x = 0U; x < water.gridWidth; ++x)
                {
                    const std::size_t cellIndex =
                        static_cast<std::size_t>(z) *
                            water.gridWidth +
                        x;

                    if (water.cells[cellIndex] == 0U)
                    {
                        continue;
                    }

                    const float minimumX =
                        offsetX + static_cast<float>(x) * water.cellSize;
                    const float minimumZ =
                        offsetZ + static_cast<float>(z) * water.cellSize;
                    const float maximumX = minimumX + water.cellSize;
                    const float maximumZ = minimumZ + water.cellSize;

                    const std::uint32_t firstVertex =
                        static_cast<std::uint32_t>(vertices.size());

                    for (const std::array<float, 2U>& position :
                        {
                            std::array<float, 2U>{minimumX, minimumZ},
                            std::array<float, 2U>{minimumX, maximumZ},
                            std::array<float, 2U>{maximumX, maximumZ},
                            std::array<float, 2U>{maximumX, minimumZ}
                        })
                    {
                        engine::assets::StaticMeshVertex vertex;

                        vertex.position =
                        {
                            position[0] - water.centerX,
                            0.0F,
                            position[1] - water.centerZ
                        };

                        vertex.normal = {0.0F, 1.0F, 0.0F};
                        vertex.tangent = {1.0F, 0.0F, 0.0F, 0.0F};
                        vertex.texcoord0 =
                        {
                            position[0] * water.colorTiling,
                            position[1] * water.colorTiling
                        };

                        vertices.push_back(vertex);
                    }

                    indices.insert(
                        indices.end(),
                        {
                            firstVertex,
                            firstVertex + 1U,
                            firstVertex + 2U,
                            firstVertex,
                            firstVertex + 2U,
                            firstVertex + 3U
                        });
                }
            }

            const engine::assets::MeshSubmesh submesh
            {
                0U,
                static_cast<std::uint32_t>(indices.size()),
                0,
                0U
            };

            engine::assets::MeshAsset mesh;

            if (engine::assets::Failed(
                    engine::assets::MeshAssetBuilder::Build(
                        vertices.data(),
                        vertices.size(),
                        indices.data(),
                        indices.size(),
                        &submesh,
                        1U,
                        1U,
                        std::filesystem::path(water.sourceName).u8string(),
                        mesh)))
            {
                error = "Cannot build water mesh.";
                return false;
            }

            engine::assets::AssetData encoded;

            if (engine::assets::Failed(
                    engine::assets::LtsMeshWriter::Encode(
                        mesh,
                        encoded)))
            {
                error = "Cannot encode water mesh.";
                return false;
            }

            const std::filesystem::path meshPath =
                BuildMeshPath(
                    workspaceRoot,
                    levelRoot,
                    water.sourceName);

            if (
                !WriteAssetData(meshPath, encoded, error) ||
                !WriteWaterMaterial(
                    workspaceRoot,
                    meshPath,
                    water,
                    error))
            {
                return false;
            }

            logicalPath =
                BuildLogicalMeshPath(
                    levelRoot,
                    water.sourceName);

            return true;
        }

        [[nodiscard]]
        pugi::xml_attribute EnsureAttribute(
            pugi::xml_node node,
            const char* const name)
        {
            pugi::xml_attribute attribute = node.attribute(name);

            if (!attribute)
            {
                attribute = node.append_attribute(name);
            }

            return attribute;
        }

        [[nodiscard]]
        pugi::xml_node EnsureChild(
            pugi::xml_node parent,
            const char* const name)
        {
            pugi::xml_node child = parent.child(name);

            if (!child)
            {
                child = parent.append_child();

                if (child)
                {
                    static_cast<void>(child.set_name(name));
                }
            }

            return child;
        }

        [[nodiscard]]
        std::uint32_t PackColor(
            const std::array<float, 3U>& color) noexcept
        {
            const auto channel = [](const float value)
            {
                return static_cast<std::uint32_t>(
                    std::lround(
                        (std::clamp)(value, 0.0F, 1.0F) * 255.0F));
            };

            return
                0xFF000000U |
                (channel(color[0]) << 16U) |
                (channel(color[1]) << 8U) |
                channel(color[2]);
        }

        [[nodiscard]]
        bool SetFloat(
            pugi::xml_node node,
            const char* const name,
            const float value)
        {
            return
                EnsureAttribute(node, name).
                    set_value(static_cast<double>(value));
        }

        [[nodiscard]]
        bool SetInteger(
            pugi::xml_node node,
            const char* const name,
            const std::int32_t value)
        {
            return EnsureAttribute(node, name).set_value(value);
        }

        [[nodiscard]]
        bool SetUnsigned(
            pugi::xml_node node,
            const char* const name,
            const std::uint32_t value)
        {
            return EnsureAttribute(node, name).set_value(value);
        }

        [[nodiscard]]
        bool WriteWaterNode(
            pugi::xml_node object,
            const engine::scene::WaterPlaneComponent& water)
        {
            const std::string sourceName =
                std::filesystem::path(water.sourceName).u8string();

            if (
                !EnsureAttribute(object, "className").
                    set_value("obj_WaterPlane") ||
                !EnsureAttribute(object, "fileName").
                    set_value(sourceName.c_str()))
            {
                return false;
            }

            pugi::xml_node position = EnsureChild(object, "position");
            pugi::xml_node gameObject = EnsureChild(object, "gameObject");
            pugi::xml_node lake = EnsureChild(object, "lake");
            pugi::xml_node settings = EnsureChild(object, "new_lake");

            static_cast<void>(lake);

            if (
                !position ||
                !gameObject ||
                !settings ||
                !SetFloat(position, "x", 0.0F) ||
                !SetFloat(position, "y", 0.0F) ||
                !SetFloat(position, "z", 0.0F))
            {
                return false;
            }

            if (!gameObject.attribute("hash"))
            {
                const std::uint32_t hash =
                    static_cast<std::uint32_t>(
                        std::hash<std::wstring>{}(water.sourceName));

                if (!SetUnsigned(gameObject, "hash", hash == 0U ? 1U : hash))
                {
                    return false;
                }
            }

            EnsureAttribute(gameObject, "PhysEnable").set_value("true");
            EnsureAttribute(gameObject, "MinQuality").set_value("1");
            EnsureAttribute(gameObject, "BulletPierceable").set_value("0");
            EnsureAttribute(gameObject, "DisableShadows").set_value("true");

            if (!settings.attribute("wave_tex"))
            {
                EnsureAttribute(settings, "wave_tex").
                    set_value("data/water/waves_");
            }

            return
                SetFloat(settings, "waterplaneheight", water.waterHeight) &&
                SetFloat(settings, "cellgridsize", water.cellSize) &&
                SetInteger(settings, "coastsmoothlevels", water.coastSmoothLevels) &&
                SetFloat(settings, "total_x_size", water.planeWidth) &&
                SetFloat(settings, "total_z_size", water.planeDepth) &&
                SetFloat(settings, "center_x", water.centerX) &&
                SetFloat(settings, "center_z", water.centerZ) &&
                SetUnsigned(settings, "deep_color", PackColor(water.waterColor)) &&
                SetUnsigned(settings, "shallow_color", PackColor(water.lightColor)) &&
                SetUnsigned(settings, "atten_color", PackColor(water.surfaceColor)) &&
                SetFloat(settings, "farTileScale", water.farTileScale) &&
                SetFloat(settings, "farTileFadeStart", water.farFadeStart) &&
                SetFloat(settings, "farTileFadeEnd", water.farFadeEnd) &&
                SetFloat(settings, "farTileAmmount", water.farTileAmount) &&
                SetFloat(settings, "farTileBumpiness", water.farTileBumpiness) &&
                SetFloat(settings, "reflectionIntensity", water.reflectionStrength) &&
                SetFloat(settings, "fresnelPow", water.fresnelPower) &&
                SetFloat(settings, "fresnelBumpiness", water.fresnelBumpiness) &&
                SetFloat(settings, "refraction_index", water.refractionIndex) &&
                SetFloat(settings, "refractionPerturbation", water.refractionPerturbation) &&
                SetFloat(settings, "caustic_strength", water.causticStrength) &&
                SetFloat(settings, "caustic_depth", water.causticDepth) &&
                SetFloat(settings, "caustic_tile", water.causticTiling) &&
                SetFloat(settings, "editorMaxAttDist", water.maximumAttenuationDistance) &&
                SetFloat(settings, "atten_dist", water.attenuationDistance) &&
                SetFloat(settings, "waterColorTile", water.colorTiling) &&
                SetFloat(settings, "waterColorBlend", water.colorBlend) &&
                SetFloat(settings, "bumpness", water.bumpiness) &&
                SetFloat(settings, "tile_size", water.bumpTiling) &&
                SetFloat(settings, "specular", water.sunCosinePower) &&
                SetFloat(settings, "specBumpiness", water.sunBumpiness) &&
                SetFloat(settings, "specIntensity", water.sunIntensity) &&
                SetFloat(settings, "specularTiling", water.specularTiling) &&
                SetFloat(settings, "shallow_depth", water.coastlineWidth);
        }

        [[nodiscard]]
        bool WriteGridFile(
            const std::filesystem::path& path,
            const engine::scene::WaterPlaneComponent& water,
            std::string& error)
        {
            std::error_code filesystemError;
            std::filesystem::create_directories(
                path.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error = "Cannot create water_planes directory.";
                return false;
            }

            std::filesystem::path temporaryPath = path;
            temporaryPath += L".studio.tmp";

            std::ofstream output(
                temporaryPath,
                std::ios::binary |
                    std::ios::trunc);

            if (!output)
            {
                error = "Cannot open temporary water .dat.";
                return false;
            }

            output.write(
                reinterpret_cast<const char*>(&WaterDataVersion),
                sizeof(WaterDataVersion));
            output.write(
                reinterpret_cast<const char*>(&water.gridWidth),
                sizeof(water.gridWidth));
            output.write(
                reinterpret_cast<const char*>(&water.gridHeight),
                sizeof(water.gridHeight));
            output.write(
                reinterpret_cast<const char*>(water.cells.data()),
                static_cast<std::streamsize>(water.cells.size()));
            output.close();

            if (!output.good())
            {
                std::filesystem::remove(temporaryPath, filesystemError);
                error = "Cannot write temporary water .dat.";
                return false;
            }

            if (!MoveFileExW(
                    temporaryPath.c_str(),
                    path.c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(temporaryPath, filesystemError);
                error = "Cannot replace water .dat.";
                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool SaveDocumentAtomic(
            const std::filesystem::path& levelDataPath,
            const pugi::xml_document& document,
            std::string& error)
        {
            std::filesystem::path temporaryPath = levelDataPath;
            temporaryPath += L".water_save.tmp";

            std::filesystem::path backupPath = levelDataPath;
            backupPath += L".before_water_save.bak";

            std::error_code filesystemError;
            std::filesystem::remove(temporaryPath, filesystemError);

            const std::string temporaryUtf8 = temporaryPath.u8string();

            if (!document.save_file(
                    temporaryUtf8.c_str(),
                    "\t",
                    pugi::format_default,
                    pugi::encoding_utf8))
            {
                error = "Cannot write temporary LevelData.xml.";
                return false;
            }

            if (!CopyFileW(
                    levelDataPath.c_str(),
                    backupPath.c_str(),
                    FALSE))
            {
                std::filesystem::remove(temporaryPath, filesystemError);
                error = "Cannot create LevelData.xml water backup.";
                return false;
            }

            if (!MoveFileExW(
                    temporaryPath.c_str(),
                    levelDataPath.c_str(),
                    MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            {
                std::filesystem::remove(temporaryPath, filesystemError);
                error = "Cannot replace LevelData.xml.";
                return false;
            }

            return true;
        }

        void RemoveGeneratedFiles(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& levelRoot,
            const std::wstring& sourceName) noexcept
        {
            if (!IsValidSourceName(sourceName))
            {
                return;
            }

            std::error_code error;

            std::filesystem::remove(
                levelRoot /
                    L"water_planes" /
                    (sourceName + L".dat"),
                error);

            const std::filesystem::path meshPath =
                BuildMeshPath(
                    workspaceRoot,
                    levelRoot,
                    sourceName);

            error.clear();
            std::filesystem::remove(meshPath, error);

            error.clear();
            std::filesystem::remove(
                meshPath.parent_path() /
                    L"Materials" /
                    (sourceName + L"_0000_water.material"),
                error);
        }

        [[nodiscard]]
        std::wstring MakeUniqueWaterName(
            const SceneDocument& document)
        {
            const auto ticks =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).
                    count();

            const std::wstring base =
                L"water_plane_" +
                std::to_wstring(ticks);

            std::wstring candidate = base;
            std::uint32_t suffix = 1U;

            const auto exists =
                [&document](const std::wstring& name)
                {
                    for (const EditorSceneEntity& entity : document.GetEntities())
                    {
                        if (
                            entity.waterPlane.has_value() &&
                            entity.waterPlane->sourceName == name)
                        {
                            return true;
                        }
                    }

                    return false;
                };

            while (exists(candidate))
            {
                candidate =
                    base + L"_" + std::to_wstring(suffix++);
            }

            return candidate;
        }
    }

    bool WaterPlaneEditor::IsWaterPlaneEntity(
        const EditorSceneEntity& entity) noexcept
    {
        return entity.waterPlane.has_value();
    }

    bool WaterPlaneEditor::AddWaterPlane(
        const std::filesystem::path& levelDataPath,
        SceneDocument& document,
        std::string& status) const noexcept
    {
        try
        {
            pugi::xml_document levelDocument;

            if (!levelDocument.load_file(levelDataPath.u8string().c_str()))
            {
                status = "Cannot read LevelData.xml.";
                return false;
            }

            const pugi::xml_node level = levelDocument.child("level");

            if (!level)
            {
                status = "LevelData.xml has no <level>.";
                return false;
            }

            const pugi::xml_attribute originXAttribute =
                level.attribute("minimapOrigin.x");
            const pugi::xml_attribute originZAttribute =
                level.attribute("minimapOrigin.z");
            const pugi::xml_attribute widthAttribute =
                level.attribute("minimapSize.x");
            const pugi::xml_attribute depthAttribute =
                level.attribute("minimapSize.z");
            const float originX =
                originXAttribute ? originXAttribute.as_float() : 0.0F;
            const float originZ =
                originZAttribute ? originZAttribute.as_float() : 0.0F;
            const float width =
                widthAttribute ? widthAttribute.as_float() : 0.0F;
            const float depth =
                depthAttribute ? depthAttribute.as_float() : 0.0F;

            if (
                !std::isfinite(width) ||
                !std::isfinite(depth) ||
                width <= 0.0F ||
                depth <= 0.0F)
            {
                status = "Level minimap bounds are invalid.";
                return false;
            }

            engine::scene::WaterPlaneComponent water;
            water.sourceName = MakeUniqueWaterName(document);
            water.cellSize = 50.0F;
            water.planeWidth = width;
            water.planeDepth = depth;
            water.centerX = originX + width * 0.5F;
            water.centerZ = originZ + depth * 0.5F;
            water.gridWidth =
                static_cast<std::uint32_t>(width / water.cellSize) + 1U;
            water.gridHeight =
                static_cast<std::uint32_t>(depth / water.cellSize) + 1U;

            const std::uint64_t cellCount =
                static_cast<std::uint64_t>(water.gridWidth) *
                water.gridHeight;

            if (cellCount == 0U || cellCount > MaximumWaterCellCount)
            {
                status = "Default water grid is too large.";
                return false;
            }

            water.cells.assign(
                static_cast<std::size_t>(cellCount),
                0U);

            EditorTransform transform;
            transform.position =
            {
                water.centerX,
                water.waterHeight,
                water.centerZ
            };

            const EditorEntityId entityId =
                document.CreateEntity(
                    water.sourceName,
                    EditorEntityKind::Empty,
                    transform);

            EditorSceneEntity* const entity =
                document.FindEntityMutable(entityId);

            if (entity == nullptr)
            {
                status = "Cannot create water scene entity.";
                return false;
            }

            entity->editorFolder = L"LevelData/obj_WaterPlane";
            entity->waterPlane = std::move(water);
            entity->staticMesh.reset();
            document.MarkModified();

            status = "Water plane added. Paint cells and save.";
            return true;
        }
        catch (...)
        {
            status = "Unexpected Add Water failure.";
            return false;
        }
    }

    bool WaterPlaneEditor::DeleteSelectedWaterPlane(
        SceneDocument& document,
        std::string& status) const noexcept
    {
        const EditorSceneEntity* const selected =
            document.GetSelectedEntity();

        if (selected == nullptr || !selected->waterPlane.has_value())
        {
            status = "Select a water plane first.";
            return false;
        }

        if (!document.DeleteSelectedEntity())
        {
            status = "Cannot delete selected water plane.";
            return false;
        }

        status = "Water plane removed from the scene. Save to commit.";
        return true;
    }

    bool WaterPlaneEditor::ResizeSelectedGrid(
        SceneDocument& document,
        const float cellSize,
        std::string& status) const noexcept
    {
        EditorSceneEntity* const entity =
            document.GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !entity->waterPlane.has_value() ||
            !std::isfinite(cellSize) ||
            cellSize < 1.0F ||
            cellSize > 500.0F)
        {
            status = "Cell Size is invalid.";
            return false;
        }

        auto& water = *entity->waterPlane;

        if (std::abs(water.cellSize - cellSize) < 0.0001F)
        {
            return true;
        }

        const std::uint32_t newWidth =
            static_cast<std::uint32_t>(water.planeWidth / cellSize) + 1U;
        const std::uint32_t newHeight =
            static_cast<std::uint32_t>(water.planeDepth / cellSize) + 1U;
        const std::uint64_t newCellCount =
            static_cast<std::uint64_t>(newWidth) * newHeight;

        if (newCellCount == 0U || newCellCount > MaximumWaterCellCount)
        {
            status = "Resized water grid is too large.";
            return false;
        }

        std::vector<std::uint8_t> resized(
            static_cast<std::size_t>(newCellCount),
            0U);

        for (std::uint32_t z = 0U; z < newHeight; ++z)
        {
            for (std::uint32_t x = 0U; x < newWidth; ++x)
            {
                const float localX =
                    (static_cast<float>(x) + 0.5F) * cellSize;
                const float localZ =
                    (static_cast<float>(z) + 0.5F) * cellSize;
                const std::uint32_t oldX =
                    static_cast<std::uint32_t>(localX / water.cellSize);
                const std::uint32_t oldZ =
                    static_cast<std::uint32_t>(localZ / water.cellSize);

                if (oldX < water.gridWidth && oldZ < water.gridHeight)
                {
                    resized[
                        static_cast<std::size_t>(z) * newWidth + x] =
                        water.cells[
                            static_cast<std::size_t>(oldZ) *
                                water.gridWidth +
                            oldX];
                }
            }
        }

        water.cellSize = cellSize;
        water.gridWidth = newWidth;
        water.gridHeight = newHeight;
        water.cells = std::move(resized);
        document.MarkModified();

        status = "Water grid resized.";
        return true;
    }

    bool WaterPlaneEditor::PaintSelected(
        SceneDocument& document,
        const float worldX,
        const float worldZ,
        const float radius,
        const bool erase) const noexcept
    {
        EditorSceneEntity* const entity =
            document.GetSelectedEntityMutable();

        if (
            entity == nullptr ||
            !entity->waterPlane.has_value() ||
            !std::isfinite(worldX) ||
            !std::isfinite(worldZ) ||
            !std::isfinite(radius) ||
            radius <= 0.0F)
        {
            return false;
        }

        auto& water = *entity->waterPlane;

        if (!IsValidComponent(water))
        {
            return false;
        }

        const float offsetX =
            water.centerX - water.planeWidth * 0.5F;
        const float offsetZ =
            water.centerZ - water.planeDepth * 0.5F;
        const float radiusSquared = radius * radius;
        bool changed = false;

        const std::int32_t centerX =
            static_cast<std::int32_t>(
                std::floor((worldX - offsetX) / water.cellSize));
        const std::int32_t centerZ =
            static_cast<std::int32_t>(
                std::floor((worldZ - offsetZ) / water.cellSize));
        const std::int32_t cellRadius =
            static_cast<std::int32_t>(
                std::ceil(radius / water.cellSize)) + 1;

        for (
            std::int32_t z = centerZ - cellRadius;
            z <= centerZ + cellRadius;
            ++z)
        {
            if (z < 0 || z >= static_cast<std::int32_t>(water.gridHeight))
            {
                continue;
            }

            for (
                std::int32_t x = centerX - cellRadius;
                x <= centerX + cellRadius;
                ++x)
            {
                if (x < 0 || x >= static_cast<std::int32_t>(water.gridWidth))
                {
                    continue;
                }

                const float cellCenterX =
                    offsetX +
                    (static_cast<float>(x) + 0.5F) * water.cellSize;
                const float cellCenterZ =
                    offsetZ +
                    (static_cast<float>(z) + 0.5F) * water.cellSize;
                const float deltaX = cellCenterX - worldX;
                const float deltaZ = cellCenterZ - worldZ;

                if (deltaX * deltaX + deltaZ * deltaZ > radiusSquared)
                {
                    continue;
                }

                const std::size_t index =
                    static_cast<std::size_t>(z) *
                        water.gridWidth +
                    static_cast<std::size_t>(x);
                const std::uint8_t value = erase ? 0U : 1U;

                if (water.cells[index] != value)
                {
                    water.cells[index] = value;
                    changed = true;
                }
            }
        }

        if (changed)
        {
            document.MarkModified();
        }

        return changed;
    }

    bool WaterPlaneEditor::RebuildSelectedAsset(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelDataPath,
        SceneDocument& document,
        std::string& status) const noexcept
    {
        EditorSceneEntity* const entity =
            document.GetSelectedEntityMutable();

        if (entity == nullptr || !entity->waterPlane.has_value())
        {
            status = "Select a water plane first.";
            return false;
        }

        std::wstring logicalPath;

        if (!BuildWaterAsset(
                workspaceRoot,
                levelDataPath.parent_path(),
                *entity->waterPlane,
                logicalPath,
                status))
        {
            return false;
        }

        if (logicalPath.empty())
        {
            entity->staticMesh.reset();
            status = "Water plane has no painted cells.";
            return true;
        }

        entity->staticMesh.emplace();
        entity->staticMesh->assetPath = logicalPath;
        entity->staticMesh->visible = true;
        entity->staticMesh->castShadows = false;
        entity->staticMesh->disableDistanceCulling = true;
        entity->staticMesh->renderOrder = 100;
        entity->transform.position =
        {
            entity->waterPlane->centerX,
            entity->waterPlane->waterHeight,
            entity->waterPlane->centerZ
        };

        document.MarkModified();
        status = "Water preview rebuilt.";
        return true;
    }

    WaterPlaneSaveResult WaterPlaneEditor::Save(
        const std::filesystem::path& workspaceRoot,
        const std::filesystem::path& levelDataPath,
        SceneDocument& document,
        const std::vector<std::size_t>& managedObjectIndices) const noexcept
    {
        WaterPlaneSaveResult result;

        try
        {
            pugi::xml_document xml;
            const pugi::xml_parse_result parseResult =
                xml.load_file(levelDataPath.u8string().c_str());

            if (!parseResult)
            {
                result.error = "Cannot parse LevelData.xml: ";
                result.error += parseResult.description();
                return result;
            }

            pugi::xml_node level = xml.child("level");

            if (!level)
            {
                result.error = "LevelData.xml has no <level>.";
                return result;
            }

            std::map<std::wstring, pugi::xml_node> existingWater;
            std::vector<std::pair<std::size_t, pugi::xml_node>> managedNodes;
            std::size_t objectIndex = 0U;

            const std::unordered_set<std::size_t> managedSet(
                managedObjectIndices.begin(),
                managedObjectIndices.end());

            for (
                pugi::xml_node object = level.child("object");
                object;
                object = object.next_sibling("object"))
            {
                ++objectIndex;

                if (managedSet.find(objectIndex) != managedSet.end())
                {
                    managedNodes.emplace_back(objectIndex, object);
                }

                if (std::strcmp(
                        object.attribute("className").value(),
                        "obj_WaterPlane") == 0)
                {
                    existingWater.emplace(
                        std::filesystem::u8path(
                            object.attribute("fileName").value()).wstring(),
                        object);
                }
            }

            std::unordered_set<std::wstring> currentNames;
            std::vector<pugi::xml_node> claimedNodes;
            std::vector<std::wstring> renamedNames;

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (!entity.waterPlane.has_value())
                {
                    continue;
                }

                EditorSceneEntity* const mutableEntity =
                    document.FindEntityMutable(entity.id);

                if (
                    mutableEntity == nullptr ||
                    !mutableEntity->waterPlane.has_value())
                {
                    result.error = "Cannot update water scene entity.";
                    return result;
                }

                auto& water = *mutableEntity->waterPlane;

                if (!IsValidComponent(water))
                {
                    result.error = "Water plane contains invalid settings or name.";
                    return result;
                }

                if (!currentNames.insert(water.sourceName).second)
                {
                    result.error = "Water plane names must be unique.";
                    return result;
                }

                const std::wstring previousAssetPath =
                    mutableEntity->staticMesh.has_value()
                        ? mutableEntity->staticMesh->assetPath
                        : std::wstring{};
                std::wstring logicalAssetPath;

                if (!BuildWaterAsset(
                        workspaceRoot,
                        levelDataPath.parent_path(),
                        water,
                        logicalAssetPath,
                        result.error))
                {
                    return result;
                }

                if (!previousAssetPath.empty())
                {
                    result.meshAssetsToReload.push_back(
                        previousAssetPath);
                }

                if (logicalAssetPath.empty())
                {
                    mutableEntity->staticMesh.reset();
                }
                else
                {
                    mutableEntity->staticMesh.emplace();
                    mutableEntity->staticMesh->assetPath =
                        logicalAssetPath;
                    mutableEntity->staticMesh->visible = true;
                    mutableEntity->staticMesh->castShadows = false;
                    mutableEntity->staticMesh->disableDistanceCulling = true;
                    mutableEntity->staticMesh->renderOrder = 100;

                    result.meshAssetsToReload.push_back(
                        logicalAssetPath);
                }

                mutableEntity->transform.position =
                {
                    water.centerX,
                    water.waterHeight,
                    water.centerZ
                };

                pugi::xml_node object;

                if (!water.savedSourceName.empty())
                {
                    const auto found = existingWater.find(water.savedSourceName);

                    if (found != existingWater.end())
                    {
                        object = found->second;
                    }
                }

                const bool newPlane = !object;

                if (newPlane)
                {
                    object = level.append_child();

                    if (!object || !object.set_name("object"))
                    {
                        result.error = "Cannot append obj_WaterPlane.";
                        return result;
                    }

                    ++result.addedPlanes;
                }
                else
                {
                    ++result.updatedPlanes;
                }

                if (!WriteWaterNode(object, water))
                {
                    result.error = "Cannot serialize obj_WaterPlane.";
                    return result;
                }

                claimedNodes.push_back(object);

                const std::filesystem::path dataPath =
                    levelDataPath.parent_path() /
                    L"water_planes" /
                    (water.sourceName + L".dat");

                if (!WriteGridFile(dataPath, water, result.error))
                {
                    return result;
                }

                if (
                    !water.savedSourceName.empty() &&
                    water.savedSourceName != water.sourceName)
                {
                    renamedNames.push_back(water.savedSourceName);
                }
            }

            std::vector<std::wstring> obsoleteNames;

            for (const auto& [name, node] : existingWater)
            {
                if (std::find(
                        claimedNodes.begin(),
                        claimedNodes.end(),
                        node) != claimedNodes.end())
                {
                    continue;
                }

                if (!level.remove_child(node))
                {
                    result.error = "Cannot remove deleted obj_WaterPlane.";
                    return result;
                }

                obsoleteNames.push_back(name);
                ++result.removedPlanes;
            }

            if (!SaveDocumentAtomic(levelDataPath, xml, result.error))
            {
                return result;
            }

            for (const std::wstring& name : obsoleteNames)
            {
                RemoveGeneratedFiles(
                    workspaceRoot,
                    levelDataPath.parent_path(),
                    name);
            }

            for (const std::wstring& name : renamedNames)
            {
                RemoveGeneratedFiles(
                    workspaceRoot,
                    levelDataPath.parent_path(),
                    name);
            }

            std::sort(
                result.meshAssetsToReload.begin(),
                result.meshAssetsToReload.end());
            result.meshAssetsToReload.erase(
                std::unique(
                    result.meshAssetsToReload.begin(),
                    result.meshAssetsToReload.end()),
                result.meshAssetsToReload.end());

            objectIndex = 0U;

            for (
                pugi::xml_node object = level.child("object");
                object;
                object = object.next_sibling("object"))
            {
                ++objectIndex;

                for (const auto& [oldIndex, managedNode] : managedNodes)
                {
                    if (object == managedNode)
                    {
                        result.managedObjectIndices.push_back(objectIndex);
                        result.objectIndexRemap.emplace_back(
                            oldIndex,
                            objectIndex);
                        break;
                    }
                }
            }

            for (const EditorSceneEntity& entity : document.GetEntities())
            {
                if (!entity.waterPlane.has_value())
                {
                    continue;
                }

                EditorSceneEntity* const mutableEntity =
                    document.FindEntityMutable(entity.id);

                if (
                    mutableEntity != nullptr &&
                    mutableEntity->waterPlane.has_value())
                {
                    mutableEntity->waterPlane->savedSourceName =
                        mutableEntity->waterPlane->sourceName;
                    mutableEntity->name =
                        mutableEntity->waterPlane->sourceName;
                }
            }

            result.succeeded = true;
            return result;
        }
        catch (const std::exception& exception)
        {
            result.error = "Water plane save failed: ";
            result.error += exception.what();
            return result;
        }
        catch (...)
        {
            result.error = "Unexpected Water Plane save failure.";
            return result;
        }
    }
}