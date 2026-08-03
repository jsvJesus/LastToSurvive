#include "Editor/LevelEditor/UI/ContentBrowserPanel.h"

#include "Editor/LevelEditor/Viewport/CameraController.h"
#include "Editor/Commands/CommandHistory.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Rendering/StaticMeshRenderer.h"
#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"

#include <imgui.h>

#include <DirectXMath.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <string>
#include <system_error>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        std::filesystem::path DiscoverGameRoot()
        {
            std::filesystem::path gameRoot =
                std::filesystem::current_path();

            if (gameRoot.filename() != L"game")
            {
                gameRoot /= L"game";
            }

            return gameRoot.lexically_normal();
        }

        [[nodiscard]]
        std::string ToLower(
            std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](const unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        [[nodiscard]]
        bool IsStaticMeshFile(
            const std::filesystem::path& path) noexcept
        {
            try
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

                return extension == L".sm";
            }
            catch (...)
            {
                return false;
            }
        }
    }

    void ContentBrowserPanel::
        Refresh() noexcept
    {
        const std::filesystem::path
            previousDirectory =
                selectedDirectory_;

        meshFiles_.clear();

        try
        {
            const std::filesystem::path gameRoot =
                DiscoverGameRoot();

            meshesRoot_ =
                (
                    gameRoot /
                    L"Data" /
                    L"Meshes"
                ).
                lexically_normal();

            selectedDirectory_ =
                previousDirectory.empty()
                    ? meshesRoot_
                    : previousDirectory;

            std::error_code error;

            for (
                std::filesystem::
                    recursive_directory_iterator
                        iterator
                        {
                            meshesRoot_,
                            std::filesystem::
                                directory_options::
                                    skip_permission_denied,
                            error
                        },
                        end;

                iterator != end;

                iterator.increment(error))
            {
                if (error)
                {
                    error.clear();
                    continue;
                }

                if (
                    iterator->is_regular_file(error) && !error && IsStaticMeshFile(iterator->path()))
                {
                    meshFiles_.push_back(iterator->path().lexically_normal());
                }
            }

            std::sort(
                meshFiles_.begin(),
                meshFiles_.end());

            error.clear();

            if (
                !std::filesystem::is_directory(
                    selectedDirectory_,
                    error) ||
                error)
            {
                selectedDirectory_ =
                    meshesRoot_;
            }
        }
        catch (...)
        {
            meshFiles_.clear();

            selectedDirectory_.clear();
            selectedAsset_.clear();
        }
    }

    void ContentBrowserPanel::
        DrawDirectoryTree(
            const std::filesystem::path& directory,
            const char* const label) noexcept
    {
        std::vector<std::filesystem::path>
            children;

        std::error_code error;

        for (
            std::filesystem::directory_iterator
                iterator
                {
                    directory,
                    std::filesystem::
                        directory_options::
                            skip_permission_denied,
                    error
                },
                end;

            iterator != end;

            iterator.increment(error))
        {
            if (error)
            {
                error.clear();
                continue;
            }

            if (
                iterator->is_directory(error) &&
                !error)
            {
                children.push_back(
                    iterator->path().
                        lexically_normal());
            }
        }

        std::sort(
            children.begin(),
            children.end());

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        if (children.empty())
        {
            flags |=
                ImGuiTreeNodeFlags_Leaf;
        }

        if (
            directory ==
            selectedDirectory_)
        {
            flags |=
                ImGuiTreeNodeFlags_Selected;
        }

        if (directory == meshesRoot_)
        {
            flags |=
                ImGuiTreeNodeFlags_DefaultOpen;
        }

        const std::string identifier =
            directory.generic_u8string();

        const bool open =
            ImGui::TreeNodeEx(
                identifier.c_str(),
                flags,
                "%s",
                label);

        if (
            ImGui::IsItemClicked() &&
            !ImGui::IsItemToggledOpen())
        {
            selectedDirectory_ =
                directory;
        }

        if (!open)
        {
            return;
        }

        for (
            const std::filesystem::path& child :
                children)
        {
            const std::string childLabel =
                child.filename().u8string();

            DrawDirectoryTree(
                child,
                childLabel.c_str());
        }

        ImGui::TreePop();
    }

    bool ContentBrowserPanel::
        PlaceAssetAtViewportCenter(
            const std::filesystem::path& file,
            EditorContentBrowserContext& context) noexcept
    {
        try
        {
            const std::filesystem::path gameRoot =
                meshesRoot_.
                    parent_path().
                    parent_path();

            std::error_code relativeError;

            const std::filesystem::path
                relativePath =
                    std::filesystem::relative(
                        file,
                        gameRoot,
                        relativeError);

            if (relativeError)
            {
                return false;
            }

            const EditorSceneSnapshot before =
                context.sceneDocument.
                    CreateSnapshot();

            EditorTransform transform{};
            EditorPickRay ray{};

            const std::uint32_t viewportWidth =
                static_cast<std::uint32_t>(
                    (std::max)(
                        context.viewportWidth,
                        1.0F));

            const std::uint32_t viewportHeight =
                static_cast<std::uint32_t>(
                    (std::max)(
                        context.viewportHeight,
                        1.0F));

            if (context.cameraController.
                    BuildPickRay(
                        viewportWidth / 2U,
                        viewportHeight / 2U,
                        viewportWidth,
                        viewportHeight,
                        ray))
            {
                float distance = 10.0F;

                if (
                    std::abs(
                        ray.direction.y) >
                    0.00001F)
                {
                    const float planeDistance =
                        -ray.origin.y /
                        ray.direction.y;

                    if (planeDistance >= 0.0F)
                    {
                        distance =
                            planeDistance;
                    }

                    float terrainHeight = 0.0F;

                    for (
                        std::uint32_t iteration = 0U;
                        iteration < 8U;
                        ++iteration)
                    {
                        const float x =
                            ray.origin.x +
                            ray.direction.x *
                                distance;

                        const float z =
                            ray.origin.z +
                            ray.direction.z *
                                distance;

                        if (!context.terrainRenderer.
                                TryGetSurfaceHeight(
                                    context.sceneDocument,
                                    x,
                                    z,
                                    terrainHeight))
                        {
                            break;
                        }

                        const float refinedDistance =
                            (
                                terrainHeight -
                                ray.origin.y
                            ) /
                            ray.direction.y;

                        if (refinedDistance < 0.0F)
                        {
                            break;
                        }

                        distance =
                            refinedDistance;
                    }
                }

                transform.position =
                {
                    ray.origin.x +
                        ray.direction.x *
                            distance,

                    ray.origin.y +
                        ray.direction.y *
                            distance,

                    ray.origin.z +
                        ray.direction.z *
                            distance
                };

                float terrainHeight = 0.0F;

                if (context.terrainRenderer.
                        TryGetSurfaceHeight(
                            context.sceneDocument,
                            transform.position[0],
                            transform.position[2],
                            terrainHeight))
                {
                    transform.position[1] =
                        terrainHeight;
                }

                DirectX::XMFLOAT3
                    boundsMinimum{};

                DirectX::XMFLOAT3
                    boundsMaximum{};

                if (context.staticMeshRenderer.
                        TryGetMeshBounds(
                            relativePath.
                                generic_wstring(),
                            boundsMinimum,
                            boundsMaximum))
                {
                    transform.position[1] -=
                        boundsMinimum.y;
                }
            }

            if (!context.sceneDocument.
                    CreateStaticMeshEntity(
                        file.stem().wstring(),
                        relativePath.
                            generic_wstring(),
                        transform))
            {
                return false;
            }

            static_cast<void>(
                context.commandHistory.Push(
                    before,
                    context.sceneDocument.
                        CreateSnapshot()));

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    void ContentBrowserPanel::Draw(
        EditorContentBrowserContext& context) noexcept
    {
        if (!ImGui::Begin(
                "Content Browser"))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Refresh"))
        {
            Refresh();
        }

        ImGui::SameLine();

        ImGui::SetNextItemWidth(
            260.0F);

        ImGui::InputTextWithHint(
            "##ContentSearch",
            "Search meshes...",
            search_.data(),
            search_.size());

        ImGui::SameLine();

        if (ImGui::Button(
                gridView_
                    ? "List"
                    : "Tiles"))
        {
            gridView_ =
                !gridView_;
        }

        ImGui::Separator();

        ImGui::BeginChild(
            "ContentFolders",
            ImVec2(210.0F, 0.0F),
            ImGuiChildFlags_Borders);

        DrawDirectoryTree(
            meshesRoot_,
            "Meshes");

        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild(
            "ContentAssets",
            ImVec2(0.0F, 0.0F),
            ImGuiChildFlags_Borders);

        std::error_code breadcrumbError;

        const std::filesystem::path
            relativeDirectory =
                std::filesystem::relative(
                    selectedDirectory_,
                    meshesRoot_,
                    breadcrumbError);

        if (ImGui::SmallButton("Meshes"))
        {
            selectedDirectory_ =
                meshesRoot_;
        }

        if (
            !breadcrumbError &&
            relativeDirectory != L".")
        {
            std::filesystem::path current =
                meshesRoot_;

            for (
                const auto& part :
                    relativeDirectory)
            {
                current /= part;

                ImGui::SameLine();
                ImGui::TextDisabled(">");
                ImGui::SameLine();

                const std::string partLabel =
                    part.u8string();

                const std::string identifier =
                    current.generic_u8string();

                ImGui::PushID(
                    identifier.c_str());

                if (ImGui::SmallButton(
                        partLabel.c_str()))
                {
                    selectedDirectory_ =
                        current;
                }

                ImGui::PopID();
            }
        }

        ImGui::Separator();

        const std::string search =
            ToLower(
                std::string(
                    search_.data()));

        bool refreshRequested = false;
        std::size_t visibleMeshCount = 0U;

        for (const std::filesystem::path& file :meshFiles_)
        {
            if (
                selectedDirectory_ !=
                    meshesRoot_ &&
                file.parent_path() !=
                    selectedDirectory_)
            {
                continue;
            }

            const std::string name =
                file.stem().u8string();

            std::error_code displayPathError;

            const std::string displayPath =
                std::filesystem::relative(
                    file,
                    meshesRoot_,
                    displayPathError).
                    generic_u8string();

            const std::string searchableName =
                ToLower(
                    displayPathError
                        ? name
                        : displayPath);

            if (
                !search.empty() &&
                searchableName.find(search) ==
                    std::string::npos)
            {
                continue;
            }

            const std::string identifier =
                file.generic_u8string();

            ++visibleMeshCount;

            ImGui::PushID(
                identifier.c_str());

            const float itemWidth =
                gridView_
                    ? 150.0F
                    : -1.0F;

            const float itemHeight =
                gridView_
                    ? 58.0F
                    : 24.0F;

            const bool selected =
                ImGui::Selectable(
                    name.c_str(),
                    selectedAsset_ == file, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(itemWidth, itemHeight));

            if (selected)
            {
                selectedAsset_ =
                    file;
            }

            /*
             * Payload оставляем совместимым
             * с существующим Viewport.
             */
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                std::error_code relativeError;

                const std::filesystem::path
                    relativePath =
                        std::filesystem::relative(
                            file,
                            meshesRoot_.
                                parent_path().
                                parent_path(),
                            relativeError);

                if (!relativeError)
                {
                    const std::string payload =
                        relativePath.
                            generic_u8string();

                    ImGui::SetDragDropPayload(
                        "LTS_MESH_ASSET",
                        payload.c_str(),
                        payload.size() + 1U);

                    ImGui::Text(
                        "Place %s",
                        name.c_str());
                }

                ImGui::EndDragDropSource();
            }

            if (
                selected &&
                ImGui::IsMouseDoubleClicked(
                    ImGuiMouseButton_Left))
            {
                static_cast<void>(
                    PlaceAssetAtViewportCenter(
                        file,
                        context));
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(
                    "%s\nDouble-click to add to scene",
                    identifier.c_str());
            }

            if (ImGui::BeginPopupContextItem(
                    "AssetContext"))
            {
                selectedAsset_ =
                    file;

                if (ImGui::MenuItem(
                        "Copy Asset Path"))
                {
                    ImGui::SetClipboardText(
                        identifier.c_str());
                }

                if (ImGui::MenuItem(
                        "Copy Game Path"))
                {
                    std::error_code relativeError;

                    const std::filesystem::path
                        relativePath =
                            std::filesystem::relative(
                                file,
                                meshesRoot_.
                                    parent_path().
                                    parent_path(),
                                relativeError);

                    if (!relativeError)
                    {
                        const std::string path =
                            relativePath.
                                generic_u8string();

                        ImGui::SetClipboardText(
                            path.c_str());
                    }
                }

                if (ImGui::MenuItem("Refresh"))
                {
                    refreshRequested = true;
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();

            if (
                gridView_ &&
                ImGui::GetContentRegionAvail().x >
                    310.0F)
            {
                ImGui::SameLine();
            }
        }

        if (refreshRequested)
        {
            Refresh();
        }

        if (visibleMeshCount == 0U)
        {
            ImGui::TextDisabled(
                "No .sm files found "
                "in the selected folder");
        }

        ImGui::EndChild();
        ImGui::End();
    }
}