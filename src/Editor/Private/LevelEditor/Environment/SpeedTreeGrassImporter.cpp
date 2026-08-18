#include "Editor/LevelEditor/Environment/SpeedTreeGrassImporter.h"

#include <Assets/AssetData.h>
#include <Assets/AssetPath.h>
#include <Assets/AssetResult.h>
#include <Assets/LtsMeshWriter.h>
#include <Assets/MaterialAsset.h>
#include <Assets/MaterialAssetWriter.h>
#include <Assets/MeshAssetBuilder.h>

#include <Core/Core.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <limits>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        bool WriteAssetData(
            const std::filesystem::path& destination,
            const engine::assets::AssetData& data,
            std::string& error)
        {
            std::error_code filesystemError;

            std::filesystem::create_directories(
                destination.parent_path(),
                filesystemError);

            if (filesystemError)
            {
                error =
                    "Cannot create directory: " +
                    destination.parent_path().u8string();

                return false;
            }

            std::ofstream stream(
                destination,
                std::ios::binary |
                    std::ios::trunc);

            if (!stream)
            {
                error =
                    "Cannot create file: " +
                    destination.u8string();

                return false;
            }

            stream.write(
                reinterpret_cast<const char*>(
                    data.GetData()),
                static_cast<std::streamsize>(
                    data.GetSize()));

            if (!stream)
            {
                error =
                    "Cannot write file: " +
                    destination.u8string();

                return false;
            }

            return true;
        }

        [[nodiscard]]
        bool IsSrtFile(
            const std::filesystem::path& path)
        {
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

            return extension == L".srt";
        }

        [[nodiscard]]
        std::filesystem::path FindTexture(
            const std::filesystem::path& sourceDirectory,
            const char* textureName,
            const std::filesystem::path& fallbackFilename)
        {
            std::vector<std::filesystem::path>
                requestedNames;

            const auto appendUniqueName =
                [&requestedNames](
                    const std::filesystem::path& value)
                {
                    const std::filesystem::path filename =
                        value.filename();

                    if (filename.empty())
                    {
                        return;
                    }

                    for (
                        const std::filesystem::path& existing :
                            requestedNames)
                    {
                        if (_wcsicmp(
                                existing.c_str(),
                                filename.c_str()) == 0)
                        {
                            return;
                        }
                    }

                    requestedNames.push_back(filename);
                };

            /*
             * The texture names stored in the SRT are authoritative.
             *
             * For example:
             * Grass_Blades_green_wide.srt contains:
             * Grass_Blades_green_d.dds
             * Grass_Blades_green_n.dds
             */
            if (
                textureName != nullptr &&
                textureName[0] != '\0')
            {
                const std::filesystem::path requested =
                    std::filesystem::u8path(textureName).
                        filename();

                appendUniqueName(requested);

                /*
                 * Some older SRT files reference TGA files while the
                 * shipped runtime asset is the converted DDS file.
                 */
                std::filesystem::path ddsName = requested;

                if (!ddsName.empty())
                {
                    ddsName.replace_extension(L".dds");
                    appendUniqueName(ddsName);
                }
            }

            /*
             * This name is used only when the name stored inside the
             * SRT cannot be resolved.
             */
            appendUniqueName(fallbackFilename);

            if (requestedNames.empty())
            {
                return {};
            }

            std::error_code error;

            for (
                const std::filesystem::path& requested :
                    requestedNames)
            {
                const std::filesystem::path directPath =
                    sourceDirectory /
                    requested;

                if (
                    std::filesystem::is_regular_file(
                        directPath,
                        error) &&
                    !error)
                {
                    return directPath.lexically_normal();
                }

                error.clear();
            }

            for (
                std::filesystem::directory_iterator iterator(
                    sourceDirectory,
                    error),
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

                const std::filesystem::path candidate =
                    iterator->path();

                for (
                    const std::filesystem::path& requested :
                        requestedNames)
                {
                    if (_wcsicmp(
                            candidate.filename().c_str(),
                            requested.c_str()) == 0)
                    {
                        return candidate.lexically_normal();
                    }
                }
            }

            return {};
        }

        [[nodiscard]]
        std::optional<engine::assets::AssetPath>
        MakeTextureAssetPath(
            const std::filesystem::path& dataRoot,
            const std::filesystem::path& sourceSrtPath,
            const char* textureName,
            const wchar_t* fallbackSuffix)
        {
            std::filesystem::path fallbackFilename;

            if (
                fallbackSuffix != nullptr &&
                fallbackSuffix[0] != L'\0')
            {
                fallbackFilename =
                    sourceSrtPath.stem().wstring() +
                    std::wstring(fallbackSuffix);
            }

            const std::filesystem::path texturePath =
                FindTexture(
                    sourceSrtPath.parent_path(),
                    textureName,
                    fallbackFilename);

            if (texturePath.empty())
            {
                return std::nullopt;
            }

            std::error_code error;

            const std::filesystem::path relativeToData =
                std::filesystem::relative(
                    texturePath,
                    dataRoot,
                    error);

            if (
                error ||
                relativeToData.empty())
            {
                return std::nullopt;
            }

            /*
             * StaticMeshRenderer resolves material texture paths
             * relative to the bin directory, not bin/Data.
             *
             * Therefore the material must contain:
             * Data/SpeedTree/Grass/.../texture.dds
             */
            const std::filesystem::path logicalPath =
                std::filesystem::path(L"Data") /
                relativeToData;

            engine::assets::AssetPath assetPath;

            if (engine::assets::Failed(
                    engine::assets::AssetPath::TryCreate(
                        logicalPath.generic_u8string(),
                        assetPath)))
            {
                return std::nullopt;
            }

            return assetPath;
        }

        [[nodiscard]]
        std::wstring MakeMaterialFilename(
            const std::wstring& meshStem,
            const std::size_t materialIndex)
        {
            std::wostringstream stream;

            stream
                << meshStem
                << L"_"
                << std::setw(4)
                << std::setfill(L'0')
                << materialIndex
                << L"_grass.material";

            return stream.str();
        }

        [[nodiscard]]
        bool WriteMaterial(
            const std::filesystem::path& materialPath,
            const std::filesystem::path& dataRoot,
            const std::filesystem::path& sourceSrtPath,
            const SpeedTree::SRenderState* renderState,
            const std::size_t materialIndex,
            std::string& error)
        {
            engine::assets::MaterialAssetDesc description;

            description.debugName =
                "SpeedTree Grass " +
                std::to_string(materialIndex);

            description.baseColorFactor =
            {
                1.0F,
                1.0F,
                1.0F,
                1.0F
            };

            if (renderState != nullptr)
            {
                const float diffuseScalar =
                    (std::max)(
                        0.0F,
                        renderState->m_fDiffuseScalar);

                description.baseColorFactor =
                {
                    std::clamp(
                        renderState->m_vDiffuseColor.x *
                            diffuseScalar,
                        0.0F,
                        1.0F),

                    std::clamp(
                        renderState->m_vDiffuseColor.y *
                            diffuseScalar,
                        0.0F,
                        1.0F),

                    std::clamp(
                        renderState->m_vDiffuseColor.z *
                            diffuseScalar,
                        0.0F,
                        1.0F),

                    std::clamp(
                        renderState->m_fAlphaScalar,
                        0.0F,
                        1.0F)
                };

                const char* const diffuseTexture =
                    static_cast<const char*>(
                        renderState->
                            m_apTextures[
                                SpeedTree::TL_DIFFUSE]);

                const char* const normalTexture =
                    static_cast<const char*>(
                        renderState->
                            m_apTextures[
                                SpeedTree::TL_NORMAL]);

                description.baseColorTexture =
                    MakeTextureAssetPath(
                        dataRoot,
                        sourceSrtPath,
                        diffuseTexture,
                        L"_d.dds");

                description.normalTexture =
                    MakeTextureAssetPath(
                        dataRoot,
                        sourceSrtPath,
                        normalTexture,
                        L"_n.dds");

                if (!description.baseColorTexture.has_value())
                {
                    error =
                        "Cannot find the SpeedTree Grass diffuse "
                        "texture for: " +
                        sourceSrtPath.filename().u8string();

                    return false;
                }
            }

            description.metallicFactor = 0.0F;
            description.roughnessFactor = 0.82F;

            /*
             * SpeedTree grass normally uses alpha-test,
             * not alpha blending.
             */
            description.alphaMode =
                engine::assets::MaterialAlphaMode::Mask;

            description.alphaCutoff = 0.35F;
            description.doubleSided = true;

            description.normalScale = 1.0F;
            description.specularIntensity = 0.15F;
            description.specularPower = 16.0F;

            description.sampler.filter =
                engine::graphics::TextureFilter::Anisotropic;

            description.sampler.addressU =
                engine::graphics::TextureAddressMode::Wrap;

            description.sampler.addressV =
                engine::graphics::TextureAddressMode::Wrap;

            description.sampler.addressW =
                engine::graphics::TextureAddressMode::Wrap;

            description.sampler.maximumAnisotropy = 16U;

            engine::assets::MaterialAsset material;

            if (engine::assets::Failed(
                    material.Initialize(
                        std::move(description))))
            {
                error =
                    "Generated SpeedTree material is invalid.";

                return false;
            }

            engine::assets::AssetData encodedMaterial;

            if (engine::assets::Failed(
                    engine::assets::MaterialAssetWriter::Encode(
                        material,
                        encodedMaterial)))
            {
                error =
                    "Cannot encode SpeedTree material.";

                return false;
            }

            return WriteAssetData(
                materialPath,
                encodedMaterial,
                error);
        }

        [[nodiscard]]
        bool ReadVertex(
            const SpeedTree::SDrawCall& drawCall,
            const std::int32_t vertexIndex,
            engine::assets::StaticMeshVertex& output)
        {
            float values[4]
            {
                0.0F,
                0.0F,
                0.0F,
                0.0F
            };

            if (!drawCall.GetProperty(
                    SpeedTree::VERTEX_PROPERTY_POSITION,
                    vertexIndex,
                    values))
            {
                return false;
            }

            output.position =
            {
                values[0],
                values[1],
                values[2]
            };

            values[0] = 0.0F;
            values[1] = 1.0F;
            values[2] = 0.0F;
            values[3] = 0.0F;

            if (drawCall.GetProperty(
                    SpeedTree::VERTEX_PROPERTY_NORMAL,
                    vertexIndex,
                    values))
            {
                output.normal =
                {
                    values[0],
                    values[1],
                    values[2]
                };
            }
            else
            {
                output.normal =
                {
                    0.0F,
                    1.0F,
                    0.0F
                };
            }

            values[0] = 1.0F;
            values[1] = 0.0F;
            values[2] = 0.0F;
            values[3] = 1.0F;

            if (drawCall.GetProperty(
                    SpeedTree::VERTEX_PROPERTY_TANGENT,
                    vertexIndex,
                    values))
            {
                output.tangent =
                {
                    values[0],
                    values[1],
                    values[2],
                    1.0F
                };
            }
            else
            {
                output.tangent =
                {
                    1.0F,
                    0.0F,
                    0.0F,
                    1.0F
                };
            }

            values[0] = 0.0F;
            values[1] = 0.0F;
            values[2] = 0.0F;
            values[3] = 0.0F;

            if (drawCall.GetProperty(
                    SpeedTree::
                        VERTEX_PROPERTY_DIFFUSE_TEXCOORDS,
                    vertexIndex,
                    values))
            {
                output.texcoord0 =
                {
                    values[0],
                    values[1]
                };
            }
            else
            {
                output.texcoord0 =
                {
                    0.0F,
                    0.0F
                };
            }

            return
                std::isfinite(output.position[0]) &&
                std::isfinite(output.position[1]) &&
                std::isfinite(output.position[2]);
        }

        [[nodiscard]]
        bool AppendIndices(
            const SpeedTree::SDrawCall& drawCall,
            std::vector<std::uint32_t>& indices)
        {
            if (
                drawCall.m_nNumIndices <= 0 ||
                drawCall.m_pIndexData == nullptr)
            {
                return false;
            }

            indices.reserve(
                indices.size() +
                static_cast<std::size_t>(
                    drawCall.m_nNumIndices));

            if (drawCall.m_b32BitIndices)
            {
                const auto* const source =
                    reinterpret_cast<const std::uint32_t*>(
                        static_cast<const SpeedTree::st_byte*>(
                            drawCall.m_pIndexData));

                indices.insert(
                    indices.end(),
                    source,
                    source + drawCall.m_nNumIndices);
            }
            else
            {
                const auto* const source =
                    reinterpret_cast<const std::uint16_t*>(
                        static_cast<const SpeedTree::st_byte*>(
                            drawCall.m_pIndexData));

                for (
                    std::int32_t index = 0;
                    index < drawCall.m_nNumIndices;
                    ++index)
                {
                    indices.push_back(
                        source[index]);
                }
            }

            return true;
        }

        [[nodiscard]]
        bool MoveGrassPivotToGround(
            std::vector<engine::assets::StaticMeshVertex>& vertices) noexcept
        {
            if (vertices.empty())
            {
                return false;
            }

            float minimumY =
                (std::numeric_limits<float>::max)();

            for (const engine::assets::StaticMeshVertex& vertex :
                 vertices)
            {
                if (!std::isfinite(vertex.position[1]))
                {
                    return false;
                }

                minimumY =
                    (std::min)(
                        minimumY,
                        vertex.position[1]);
            }

            if (!std::isfinite(minimumY))
            {
                return false;
            }

            /*
             * После этого нижняя точка растения будет Y = 0.
             * Instance.y можно ставить точно на поверхность Terrain.
             */
            for (engine::assets::StaticMeshVertex& vertex :
                 vertices)
            {
                vertex.position[1] -= minimumY;
            }

            return true;
        }
    }

    SpeedTreeGrassImportResult
        SpeedTreeGrassImporter::Import(
            const std::filesystem::path& workspaceRoot,
            const std::filesystem::path& sourceSrtPath) const noexcept
    {
        SpeedTreeGrassImportResult result;

        try
        {
            const std::filesystem::path dataRoot =
                workspaceRoot /
                L"bin" /
                L"Data";

            const std::filesystem::path grassRoot =
                dataRoot /
                L"SpeedTree" /
                L"Grass";

            std::error_code filesystemError;

            if (
                workspaceRoot.empty() ||
                !std::filesystem::is_regular_file(
                    sourceSrtPath,
                    filesystemError) ||
                filesystemError ||
                !IsSrtFile(sourceSrtPath))
            {
                result.error =
                    "Selected Grass asset is not a valid .srt file.";

                return result;
            }

            filesystemError.clear();

            const std::filesystem::path relativeSrtPath =
                std::filesystem::relative(
                    sourceSrtPath,
                    grassRoot,
                    filesystemError);

            if (
                filesystemError ||
                relativeSrtPath.empty() ||
                relativeSrtPath.native().find(L"..") == 0U)
            {
                result.error =
                    "Grass .srt must be inside "
                    "bin/Data/SpeedTree/Grass.";

                return result;
            }

            SpeedTree::CCore tree;

            const std::string sourceName =
                sourceSrtPath.u8string();

            /*
             * true означает, что ресурс загружается
             * именно как Grass model.
             */
            if (!tree.LoadTree(
                    sourceName.c_str(),
                    true,
                    1.0F))
            {
                result.error =
                    "SpeedTree Core cannot load: " +
                    sourceName;

                return result;
            }

            const SpeedTree::SGeometry* const geometry =
                tree.GetGeometry();

            if (
                geometry == nullptr ||
                geometry->m_nNumLods <= 0 ||
                geometry->m_pLods == nullptr)
            {
                result.error =
                    "The .srt file contains no 3D LOD geometry.";

                return result;
            }

            const SpeedTree::SLod* const lods =
                geometry->m_pLods;

            const SpeedTree::SLod& lod =
                lods[0];

            if (
                lod.m_nNumDrawCalls <= 0 ||
                lod.m_pDrawCalls == nullptr)
            {
                result.error =
                    "SpeedTree LOD 0 contains no draw calls.";

                return result;
            }

            const SpeedTree::SDrawCall* const drawCalls =
                lod.m_pDrawCalls;

            std::vector<engine::assets::StaticMeshVertex>
                vertices;

            std::vector<std::uint32_t> indices;

            std::vector<engine::assets::MeshSubmesh>
                submeshes;

            submeshes.reserve(
                static_cast<std::size_t>(
                    lod.m_nNumDrawCalls));

            for (
                std::int32_t drawIndex = 0;
                drawIndex < lod.m_nNumDrawCalls;
                ++drawIndex)
            {
                const SpeedTree::SDrawCall& drawCall =
                    drawCalls[drawIndex];

                if (
                    drawCall.m_nNumVertices <= 0 ||
                    drawCall.m_nNumIndices <= 0)
                {
                    continue;
                }

                const std::uint32_t firstVertex =
                    static_cast<std::uint32_t>(
                        vertices.size());

                const std::uint32_t firstIndex =
                    static_cast<std::uint32_t>(
                        indices.size());

                for (
                    std::int32_t vertexIndex = 0;
                    vertexIndex <
                        drawCall.m_nNumVertices;
                    ++vertexIndex)
                {
                    engine::assets::StaticMeshVertex vertex;

                    if (!ReadVertex(
                            drawCall,
                            vertexIndex,
                            vertex))
                    {
                        result.error =
                            "Cannot read SpeedTree vertex data.";

                        return result;
                    }

                    vertices.push_back(vertex);
                }

                if (!AppendIndices(
                        drawCall,
                        indices))
                {
                    result.error =
                        "Cannot read SpeedTree index data.";

                    return result;
                }

                engine::assets::MeshSubmesh submesh;

                submesh.firstIndex = firstIndex;

                submesh.indexCount =
                    static_cast<std::uint32_t>(
                        indices.size()) -
                    firstIndex;

                submesh.baseVertex =
                    static_cast<std::int32_t>(
                        firstVertex);

                submesh.materialSlot =
                    static_cast<std::uint32_t>(
                        drawIndex);

                submeshes.push_back(submesh);
            }

            if (
                vertices.empty() ||
                indices.empty() ||
                submeshes.empty())
            {
                result.error =
                    "SpeedTree conversion produced an empty mesh.";

                return result;
            }

            if (!MoveGrassPivotToGround(vertices))
            {
                result.error =
                    "Cannot move the Grass mesh pivot to ground.";

                return result;
            }

            engine::assets::MeshAsset mesh;

            if (engine::assets::Failed(
                    engine::assets::MeshAssetBuilder::Build(
                        vertices.data(),
                        vertices.size(),
                        indices.data(),
                        indices.size(),
                        submeshes.data(),
                        submeshes.size(),
                        static_cast<std::uint32_t>(
                            lod.m_nNumDrawCalls),
                        sourceSrtPath.filename().u8string(),
                        mesh)))
            {
                result.error =
                    "MeshAssetBuilder cannot build Grass mesh.";

                return result;
            }

            engine::assets::AssetData encodedMesh;

            if (engine::assets::Failed(
                    engine::assets::LtsMeshWriter::Encode(
                        mesh,
                        encodedMesh)))
            {
                result.error =
                    "Cannot encode converted Grass mesh.";

                return result;
            }

            std::filesystem::path relativeMeshPath =
                relativeSrtPath;

            relativeMeshPath.replace_extension(L".mesh");

            const std::filesystem::path meshPath =
                dataRoot /
                L"StaticMeshes" /
                L"SpeedTree" /
                L"Grass" /
                relativeMeshPath;

            if (!WriteAssetData(
                    meshPath,
                    encodedMesh,
                    result.error))
            {
                return result;
            }

            const std::filesystem::path materialDirectory =
                meshPath.parent_path() /
                L"Materials";

            for (
                std::size_t materialIndex = 0U;
                materialIndex <
                    static_cast<std::size_t>(
                        lod.m_nNumDrawCalls);
                ++materialIndex)
            {
                const SpeedTree::SDrawCall& drawCall =
                    drawCalls[materialIndex];

                const SpeedTree::SRenderState* const renderState =
                    drawCall.m_pRenderState;

                const std::filesystem::path materialPath =
                    materialDirectory /
                    MakeMaterialFilename(
                        meshPath.stem().wstring(),
                        materialIndex);

                if (!WriteMaterial(
                        materialPath,
                        dataRoot,
                        sourceSrtPath,
                        renderState,
                        materialIndex,
                        result.error))
                {
                    return result;
                }
            }

            filesystemError.clear();

            const std::filesystem::path logicalMeshPath =
                std::filesystem::relative(
                    meshPath,
                    workspaceRoot / L"bin",
                    filesystemError);

            if (
                filesystemError ||
                logicalMeshPath.empty())
            {
                result.error =
                    "Cannot create logical Grass mesh path.";

                return result;
            }

            result.logicalMeshPath =
                logicalMeshPath.generic_wstring();

            result.succeeded = true;
            result.error.clear();

            return result;
        }
        catch (const std::exception& exception)
        {
            result.error =
                std::string(
                    "SpeedTree Grass import failed: ") +
                exception.what();

            return result;
        }
        catch (...)
        {
            result.error =
                "SpeedTree Grass import failed.";

            return result;
        }
    }
}
