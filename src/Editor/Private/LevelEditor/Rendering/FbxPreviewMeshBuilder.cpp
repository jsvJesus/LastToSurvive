#include "Editor/LevelEditor/Rendering/FbxPreviewMeshBuilder.h"

#include <Assets/FbxAssetData.h>
#include <Assets/FbxAssetImporter.h>
#include <Assets/MeshAssetBuilder.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        std::array<float, 3U> TransformPosition(
            const engine::assets::FbxMatrix4& matrix,
            const std::array<float, 3U>& value) noexcept
        {
            const auto& m = matrix.values;

            return
            {
                value[0U] * m[0U] +
                    value[1U] * m[4U] +
                    value[2U] * m[8U] +
                    m[12U],

                value[0U] * m[1U] +
                    value[1U] * m[5U] +
                    value[2U] * m[9U] +
                    m[13U],

                value[0U] * m[2U] +
                    value[1U] * m[6U] +
                    value[2U] * m[10U] +
                    m[14U]
            };
        }

        [[nodiscard]]
        std::array<float, 3U> TransformDirection(
            const engine::assets::FbxMatrix4& matrix,
            const std::array<float, 3U>& value,
            const std::array<float, 3U>& fallback) noexcept
        {
            const auto& m = matrix.values;

            std::array<float, 3U> result
            {
                value[0U] * m[0U] +
                    value[1U] * m[4U] +
                    value[2U] * m[8U],

                value[0U] * m[1U] +
                    value[1U] * m[5U] +
                    value[2U] * m[9U],

                value[0U] * m[2U] +
                    value[1U] * m[6U] +
                    value[2U] * m[10U]
            };

            const float lengthSquared =
                result[0U] * result[0U] +
                result[1U] * result[1U] +
                result[2U] * result[2U];

            if (
                !std::isfinite(lengthSquared) ||
                lengthSquared <= 0.0000001F)
            {
                return fallback;
            }

            const float inverseLength =
                1.0F / std::sqrt(lengthSquared);

            result[0U] *= inverseLength;
            result[1U] *= inverseLength;
            result[2U] *= inverseLength;

            return result;
        }

        template<typename SourceVertex>
        [[nodiscard]]
        engine::assets::StaticMeshVertex ConvertVertex(
            const SourceVertex& source,
            const engine::assets::FbxMatrix4& transform)
            noexcept
        {
            engine::assets::StaticMeshVertex result;

            result.position =
                TransformPosition(
                    transform,
                    source.position);

            result.normal =
                TransformDirection(
                    transform,
                    source.normal,
                    {0.0F, 1.0F, 0.0F});

            const std::array<float, 3U> tangent =
                TransformDirection(
                    transform,
                    {
                        source.tangent[0U],
                        source.tangent[1U],
                        source.tangent[2U]
                    },
                    {1.0F, 0.0F, 0.0F});

            result.tangent =
            {
                tangent[0U],
                tangent[1U],
                tangent[2U],
                source.tangent[3U] < 0.0F
                    ? -1.0F
                    : 1.0F
            };

            result.texcoord0 =
                source.texcoord0;

            return result;
        }

        template<typename SourceMesh>
        [[nodiscard]]
        engine::assets::AssetResult AppendMesh(
            const SourceMesh& source,
            std::vector<
                engine::assets::StaticMeshVertex>&
                    vertices,
            std::vector<std::uint32_t>& indices,
            std::vector<engine::assets::MeshSubmesh>&
                submeshes,
            std::uint32_t& materialSlotCount)
        {
            if (
                source.vertices.empty() ||
                source.indices.empty() ||
                source.sections.empty())
            {
                return
                    engine::assets::AssetResult::
                        CorruptData;
            }

            if (
                source.vertices.size() >
                    static_cast<std::size_t>(
                        (std::numeric_limits<
                            std::uint32_t>::max)()) -
                    vertices.size() ||
                source.indices.size() >
                    static_cast<std::size_t>(
                        (std::numeric_limits<
                            std::uint32_t>::max)()) -
                    indices.size())
            {
                return
                    engine::assets::AssetResult::
                        FileTooLarge;
            }

            const std::uint32_t baseVertex =
                static_cast<std::uint32_t>(
                    vertices.size());

            const std::uint32_t firstSourceIndex =
                static_cast<std::uint32_t>(
                    indices.size());

            vertices.reserve(
                vertices.size() +
                source.vertices.size());

            for (const auto& sourceVertex :
                 source.vertices)
            {
                vertices.push_back(
                    ConvertVertex(
                        sourceVertex,
                        source.localToWorld));
            }

            indices.reserve(
                indices.size() +
                source.indices.size());

            for (const std::uint32_t sourceIndex :
                 source.indices)
            {
                if (
                    sourceIndex >=
                        source.vertices.size())
                {
                    return
                        engine::assets::AssetResult::
                            CorruptData;
                }

                indices.push_back(
                    baseVertex +
                    sourceIndex);
            }

            const std::uint32_t localMaterialCount =
                static_cast<std::uint32_t>(
                    (std::max)(
                        source.materials.size(),
                        std::size_t{1U}));

            if (
                localMaterialCount >
                    (std::numeric_limits<
                        std::uint32_t>::max)() -
                    materialSlotCount)
            {
                return
                    engine::assets::AssetResult::
                        FileTooLarge;
            }

            for (const auto& sourceSection :
                 source.sections)
            {
                if (
                    sourceSection.indexCount == 0U ||
                    sourceSection.firstIndex >
                        source.indices.size() ||
                    sourceSection.indexCount >
                        source.indices.size() -
                            sourceSection.firstIndex)
                {
                    return
                        engine::assets::AssetResult::
                            CorruptData;
                }

                engine::assets::MeshSubmesh section;

                section.firstIndex =
                    firstSourceIndex +
                    sourceSection.firstIndex;

                section.indexCount =
                    sourceSection.indexCount;

                section.baseVertex = 0;

                section.materialSlot =
                    materialSlotCount +
                    (std::min)(
                        sourceSection.materialSlot,
                        localMaterialCount - 1U);

                submeshes.push_back(section);
            }

            materialSlotCount +=
                localMaterialCount;

            return
                engine::assets::AssetResult::Success;
        }
    }

    engine::assets::AssetResult
        FbxPreviewMeshBuilder::Build(
            const std::filesystem::path& sourcePath,
            engine::assets::MeshAsset& output,
            std::wstring& error,
            std::vector<std::wstring>* const warnings)
            noexcept
    {
        output.Clear();
        error.clear();

        std::vector<std::wstring> localWarnings;

        std::vector<std::wstring>& warningOutput =
            warnings != nullptr
                ? *warnings
                : localWarnings;

        warningOutput.clear();

        try
        {
            engine::assets::FbxImportOptions options;

            options.importStaticMeshes = true;
            options.importSkeletalMeshes = true;

            /*
             * Mesh preview does not evaluate FBX
             * animation stacks. Animation playback
             * is connected by Stage 3.
             */
            options.importAnimations = false;

            engine::assets::FbxImportedScene imported;

            engine::assets::AssetResult result =
                engine::assets::FbxAssetImporter::Import(
                    sourcePath,
                    options,
                    imported,
                    error,
                    &warningOutput);

            if (engine::assets::Failed(result))
            {
                return result;
            }

            std::vector<
                engine::assets::StaticMeshVertex>
                    vertices;

            std::vector<std::uint32_t>
                indices;

            std::vector<
                engine::assets::MeshSubmesh>
                    submeshes;

            std::uint32_t materialSlotCount = 0U;

            for (const auto& mesh :
                 imported.staticMeshes)
            {
                result =
                    AppendMesh(
                        mesh,
                        vertices,
                        indices,
                        submeshes,
                        materialSlotCount);

                if (engine::assets::Failed(result))
                {
                    output.Clear();
                    return result;
                }
            }

            /*
             * Skeletal FBX is visible immediately in
             * bind pose without any WarZ skeleton or
             * character classes.
             */
            for (const auto& mesh :
                 imported.skeletalMeshes)
            {
                result =
                    AppendMesh(
                        mesh,
                        vertices,
                        indices,
                        submeshes,
                        materialSlotCount);

                if (engine::assets::Failed(result))
                {
                    output.Clear();
                    return result;
                }
            }

            if (
                vertices.empty() ||
                indices.empty() ||
                submeshes.empty())
            {
                error =
                    L"FBX contains no renderable static "
                    L"or skeletal mesh.";

                return
                    engine::assets::AssetResult::
                        UnsupportedFeature;
            }

            materialSlotCount =
                (std::max)(
                    materialSlotCount,
                    1U);

            const std::string debugName =
                sourcePath.stem().u8string();

            result =
                engine::assets::MeshAssetBuilder::Build(
                    vertices.data(),
                    vertices.size(),
                    indices.data(),
                    indices.size(),
                    submeshes.data(),
                    submeshes.size(),
                    materialSlotCount,
                    debugName,
                    output);

            if (engine::assets::Failed(result))
            {
                output.Clear();

                error =
                    L"Failed to build FBX preview mesh.";

                return result;
            }

            return
                engine::assets::AssetResult::Success;
        }
        catch (const std::bad_alloc&)
        {
            output.Clear();

            error =
                L"Not enough memory for FBX preview.";

            return
                engine::assets::AssetResult::OutOfMemory;
        }
        catch (...)
        {
            output.Clear();

            error =
                L"Unexpected FBX preview failure.";

            return
                engine::assets::AssetResult::InternalError;
        }
    }
}