#include "Editor/Tools/Import/WarZImporterWindow.h"
#include "Editor/Tools/Import/WarZAssetConverter.h"

#include <imgui.h>

#include <Windows.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <cstdio>
#include <cmath>
#include <type_traits>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        struct LegacyBinaryHeader final
        {
            std::uint32_t fileId = 0U;
            std::uint32_t assetId = 0U;
            std::uint32_t version = 0U;
        };

        static_assert(sizeof(LegacyBinaryHeader) == 12U);

        [[nodiscard]]
        constexpr std::uint32_t MakeLegacyId(
            const char first,
            const char second,
            const char third,
            const char fourth) noexcept
        {
            return
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(first)) << 24U) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(second)) << 16U) |
                (static_cast<std::uint32_t>(
                    static_cast<unsigned char>(third)) << 8U) |
                static_cast<std::uint32_t>(
                    static_cast<unsigned char>(fourth));
        }

        constexpr std::uint32_t WarZBinaryFileId =
            MakeLegacyId('2', 'd', '3', 'r');

        constexpr std::uint32_t WarZSkeletonId =
            MakeLegacyId('t', 'l', 'k', 's');

        constexpr std::uint32_t WarZAnimationId =
            MakeLegacyId('d', 'm', 'n', 'a');

        constexpr std::uint32_t WarZSkeletonVersion = 1U;
        constexpr std::uint32_t WarZAnimationVersion = 3U;

        [[nodiscard]]
        std::string PathToUtf8(
            const std::filesystem::path& path)
        {
            return path.generic_u8string();
        }

        [[nodiscard]]
        std::string ToLowerAscii(std::string value)
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
        std::wstring ToLowerWide(std::wstring value)
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
        bool HasExtension(
            const std::filesystem::path& path,
            const wchar_t* expectedExtension)
        {
            return
                ToLowerWide(path.extension().wstring()) ==
                expectedExtension;
        }

        [[nodiscard]]
        bool DirectoryExists(
            const std::filesystem::path& path) noexcept
        {
            std::error_code error;

            return
                std::filesystem::is_directory(path, error) &&
                !error;
        }

        [[nodiscard]]
        bool ReadLegacyHeader(
            const std::filesystem::path& path,
            LegacyBinaryHeader& header) noexcept
        {
            try
            {
                std::ifstream stream(path, std::ios::binary);

                if (!stream)
                {
                    return false;
                }

                stream.read(
                    reinterpret_cast<char*>(&header),
                    sizeof(header));

                return
                    stream.good() ||
                    stream.gcount() ==
                        static_cast<std::streamsize>(
                            sizeof(header));
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool IsWarZSkeleton(
            const std::filesystem::path& path) noexcept
        {
            LegacyBinaryHeader header;

            return
                ReadLegacyHeader(path, header) &&
                header.fileId == WarZBinaryFileId &&
                header.assetId == WarZSkeletonId &&
                header.version == WarZSkeletonVersion;
        }

        [[nodiscard]]
        bool IsWarZAnimation(
            const std::filesystem::path& path) noexcept
        {
            LegacyBinaryHeader header;

            return
                ReadLegacyHeader(path, header) &&
                header.fileId == WarZBinaryFileId &&
                header.assetId == WarZAnimationId &&
                header.version == WarZAnimationVersion;
        }

        [[nodiscard]]
        std::filesystem::path NormalizeSourceRoot(
            const std::filesystem::path& selected)
        {
            if (DirectoryExists(selected / L"ObjectsDepot"))
            {
                return selected.lexically_normal();
            }

            if (DirectoryExists(
                    selected / L"Data" / L"ObjectsDepot"))
            {
                return
                    (selected / L"Data").
                    lexically_normal();
            }

            if (DirectoryExists(
                    selected /
                    L"bin" /
                    L"Data" /
                    L"ObjectsDepot"))
            {
                return
                    (selected / L"bin" / L"Data").
                    lexically_normal();
            }

            return selected.lexically_normal();
        }

        [[nodiscard]]
        std::filesystem::path FindDefaultSourceRoot() noexcept
        {
            try
            {
                std::filesystem::path cursor =
                    std::filesystem::current_path();

                for (std::uint32_t depth = 0U;
                     depth < 8U;
                     ++depth)
                {
                    const std::filesystem::path candidate =
                        cursor / L"bin" / L"Data";

                    if (DirectoryExists(
                            candidate / L"ObjectsDepot"))
                    {
                        return candidate.lexically_normal();
                    }

                    if (ToLowerWide(
                            cursor.filename().wstring()) ==
                            L"bin" &&
                        DirectoryExists(
                            cursor /
                            L"Data" /
                            L"ObjectsDepot"))
                    {
                        return
                            (cursor / L"Data").
                            lexically_normal();
                    }

                    if (ToLowerWide(
                            cursor.filename().wstring()) ==
                            L"data" &&
                        DirectoryExists(
                            cursor / L"ObjectsDepot"))
                    {
                        return cursor.lexically_normal();
                    }

                    const std::filesystem::path parent =
                        cursor.parent_path();

                    if (parent.empty() || parent == cursor)
                    {
                        break;
                    }

                    cursor = parent;
                }

                return
                    std::filesystem::current_path() /
                    L"bin" /
                    L"Data";
            }
            catch (...)
            {
                return {};
            }
        }

        [[nodiscard]]
        std::size_t CountFilesWithExtension(
            const std::filesystem::path& root,
            const wchar_t* extension)
        {
            if (!DirectoryExists(root))
            {
                return 0U;
            }

            std::size_t count = 0U;

            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::recursive_directory_iterator(
                     root,
                     std::filesystem::directory_options::
                         skip_permission_denied))
            {
                std::error_code error;

                if (!entry.is_regular_file(error) || error)
                {
                    continue;
                }

                if (HasExtension(entry.path(), extension))
                {
                    ++count;
                }
            }

            return count;
        }

        [[nodiscard]]
        bool BrowseForFolder(
            const std::filesystem::path& initialFolder,
            std::filesystem::path& selectedFolder) noexcept
        {
            const HRESULT initializeResult =
                CoInitializeEx(
                    nullptr,
                    COINIT_APARTMENTTHREADED |
                        COINIT_DISABLE_OLE1DDE);

            const bool shouldUninitialize =
                SUCCEEDED(initializeResult);

            if (FAILED(initializeResult) &&
                initializeResult != RPC_E_CHANGED_MODE)
            {
                return false;
            }

            ComPtr<IFileDialog> dialog;

            HRESULT result = CoCreateInstance(
                CLSID_FileOpenDialog,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(dialog.GetAddressOf()));

            if (FAILED(result))
            {
                if (shouldUninitialize)
                {
                    CoUninitialize();
                }

                return false;
            }

            DWORD options = 0U;

            result = dialog->GetOptions(&options);

            if (SUCCEEDED(result))
            {
                result = dialog->SetOptions(
                    options |
                    FOS_PICKFOLDERS |
                    FOS_FORCEFILESYSTEM |
                    FOS_PATHMUSTEXIST);
            }

            if (SUCCEEDED(result))
            {
                static_cast<void>(
                    dialog->SetTitle(
                        L"Select WarZ bin\\Data folder"));
            }

            if (SUCCEEDED(result) &&
                !initialFolder.empty() &&
                DirectoryExists(initialFolder))
            {
                ComPtr<IShellItem> initialItem;

                if (SUCCEEDED(
                        SHCreateItemFromParsingName(
                            initialFolder.c_str(),
                            nullptr,
                            IID_PPV_ARGS(
                                initialItem.GetAddressOf()))))
                {
                    static_cast<void>(
                        dialog->SetFolder(initialItem.Get()));
                }
            }

            if (SUCCEEDED(result))
            {
                result = dialog->Show(nullptr);
            }

            if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                if (shouldUninitialize)
                {
                    CoUninitialize();
                }

                return false;
            }

            if (FAILED(result))
            {
                if (shouldUninitialize)
                {
                    CoUninitialize();
                }

                return false;
            }

            ComPtr<IShellItem> selectedItem;

            result = dialog->GetResult(
                selectedItem.GetAddressOf());

            if (FAILED(result))
            {
                if (shouldUninitialize)
                {
                    CoUninitialize();
                }

                return false;
            }

            PWSTR selectedPath = nullptr;

            result = selectedItem->GetDisplayName(
                SIGDN_FILESYSPATH,
                &selectedPath);

            if (SUCCEEDED(result) && selectedPath != nullptr)
            {
                selectedFolder = selectedPath;
                CoTaskMemFree(selectedPath);
            }

            if (shouldUninitialize)
            {
                CoUninitialize();
            }

            return
                SUCCEEDED(result) &&
                !selectedFolder.empty();
        }
    }

    void WarZImporterWindow::Open() noexcept
    {
        open_ = true;
    }

    void WarZImporterWindow::SetOpen(
        const bool open) noexcept
    {
        open_ = open;
    }

    bool WarZImporterWindow::IsOpen() const noexcept
    {
        return open_;
    }

    void WarZImporterWindow::
        InitializeDefaultSource() noexcept
    {
        if (initialized_)
        {
            return;
        }

        initialized_ = true;
        sourceRoot_ = FindDefaultSourceRoot();

        if (DirectoryExists(sourceRoot_))
        {
            status_ =
                "WarZ source root detected. Press Scan Source.";
        }
        else
        {
            status_ =
                "WarZ bin/Data folder was not detected.";
        }
    }

    bool WarZImporterWindow::
        SelectSourceFolder() noexcept
    {
        std::filesystem::path selected;

        if (!BrowseForFolder(sourceRoot_, selected))
        {
            return false;
        }

        sourceRoot_ =
            NormalizeSourceRoot(selected);

        ScanSource();
        return true;
    }

    bool WarZImporterWindow::MatchesFilter(
        const SourcePackage& package) const noexcept
    {
        if (packageFilter_[0] == '\0')
        {
            return true;
        }

        const std::string packageName =
            ToLowerAscii(
                PathToUtf8(package.relativePath));

        const std::string filter =
            ToLowerAscii(packageFilter_.data());

        return
            packageName.find(filter) !=
            std::string::npos;
    }

    void WarZImporterWindow::ResetAnimation(
    const bool clearSelection) noexcept
    {
        selectedAnimationData_ = {};
        selectedAnimationPose_ = {};

        animationStatus_.clear();

        animationFrame_ = 0.0F;
        animationPlaybackFps_ = 30.0F;

        animationLoaded_ = false;
        animationCompatible_ = false;
        animationPlaying_ = false;

        if (clearSelection)
        {
            selectedAnimation_ = -1;
        }
    }

    bool WarZImporterWindow::MatchesAnimationFilter(
        const std::filesystem::path& path) const noexcept
    {
        if (animationFilter_[0] == '\0')
        {
            return true;
        }

        const std::filesystem::path animationsRoot =
            sourceRoot_ / L"Animations";

        const std::string animationName =
            ToLowerAscii(
                PathToUtf8(
                    path.lexically_relative(
                        animationsRoot)));

        const std::string filter =
            ToLowerAscii(
                animationFilter_.data());

        return
            animationName.find(filter) !=
            std::string::npos;
    }

    void WarZImporterWindow::LoadSelectedAnimation() noexcept
    {
        ResetAnimation(false);

        if (!analysisSucceeded_)
        {
            animationStatus_ =
                "Analyze the selected mesh and skeleton first.";

            return;
        }

        if (selectedAnimation_ < 0 ||
            selectedAnimation_ >=
                static_cast<int>(
                    animations_.size()))
        {
            animationStatus_ =
                "No animation is selected.";

            return;
        }

        if (!LegacyAnimationReader::Read(
                animations_[
                    static_cast<std::size_t>(
                        selectedAnimation_)],
                &selectedSkeletonData_,
                selectedAnimationData_))
        {
            animationStatus_ =
                selectedAnimationData_.error;

            return;
        }

        animationLoaded_ = true;

        animationCompatible_ =
            selectedAnimationData_.IsCompatible() &&
            selectedSkeletonData_.bones.size() <=
                LegacyAnimationMaximumBones;

        animationPlaybackFps_ =
            selectedAnimationData_.frameRate;

        if (!animationCompatible_)
        {
            if (selectedAnimationData_.
                    mappedTrackCount == 0U)
            {
                animationStatus_ =
                    "Animation contains no tracks for the selected skeleton.";
            }
            else if (selectedSkeletonData_.
                         bones.size() >
                     LegacyAnimationMaximumBones)
            {
                animationStatus_ =
                    "The selected skeleton exceeds the GPU preview bone limit.";
            }
            else
            {
                animationStatus_ =
                    "Animation cannot be used by the preview.";
            }

            return;
        }

        if (selectedAnimationData_.skeletonIdMismatch)
        {
            animationStatus_ =
                "Skeleton ID differs, but WarZ bone-name remapping succeeded: " +
                std::to_string(
                    selectedAnimationData_.mappedTrackCount) +
                " / " +
                std::to_string(
                    selectedAnimationData_.tracks.size()) +
                " tracks mapped.";
        }
        else if (selectedAnimationData_.
                     missingBoneTrackCount != 0U)
        {
            animationStatus_ =
                "Animation loaded with " +
                std::to_string(
                    selectedAnimationData_.
                        missingBoneTrackCount) +
                " unmapped tracks.";
        }
        else
        {
            animationStatus_ =
                "Animation is compatible with the selected skeleton.";
        }

        UpdateAnimationPose();
    }

    void WarZImporterWindow::UpdateAnimationPose() noexcept
    {
        if (!animationLoaded_ ||
            !animationCompatible_)
        {
            selectedAnimationPose_ = {};
            return;
        }

        std::string error;

        if (!LegacyAnimationReader::Sample(
                selectedAnimationData_,
                selectedSkeletonData_,
                selectedMeshData_.pivot,
                animationFrame_,
                animationLoop_,
                animationLockRoot_,
                selectedAnimationPose_,
                error))
        {
            animationCompatible_ = false;
            animationPlaying_ = false;

            selectedAnimationPose_ = {};

            animationStatus_ =
                error.empty()
                    ? "Failed to sample animation pose."
                    : error;
        }
    }

    void WarZImporterWindow::
        UpdateAnimationPlayback() noexcept
    {
        if (!animationPlaying_ ||
            !animationLoaded_ ||
            !animationCompatible_ ||
            selectedAnimationData_.frameCount <= 1U)
        {
            return;
        }

        const float deltaSeconds =
            std::clamp(
                ImGui::GetIO().DeltaTime,
                0.0F,
                0.1F);

        animationFrame_ +=
            deltaSeconds *
            animationPlaybackFps_;

        const float finalFrame =
            static_cast<float>(
                selectedAnimationData_.
                    frameCount -
                1U);

        if (animationLoop_)
        {
            if (finalFrame > 0.0F &&
                animationFrame_ >= finalFrame)
            {
                animationFrame_ =
                    std::fmod(
                        animationFrame_,
                        finalFrame);
            }
        }
        else if (animationFrame_ >= finalFrame)
        {
            animationFrame_ = finalFrame;
            animationPlaying_ = false;
        }

        UpdateAnimationPose();
    }

    void WarZImporterWindow::ResetAnalysis() noexcept
    {
        selectedSkeletonData_ = {};
        selectedWeightData_ = {};
        selectedMeshData_ = {};
        selectedMaterialSet_ = {};
        ResetAnimation(false);

        meshPreview_.Reset();

        analysisStatus_.clear();

        analysisAttempted_ = false;
        analysisSucceeded_ = false;

        usingEmbeddedWeights_ = false;
        vertexWeightCountMismatch_ = false;

        showLegacyPreview_ = false;
    }

    void WarZImporterWindow::AnalyzeSelection() noexcept
    {
        ResetAnalysis();
        analysisAttempted_ = true;

        if (selectedPackage_ < 0 ||
            selectedPackage_ >=
                static_cast<int>(packages_.size()))
        {
            analysisStatus_ =
                "No asset package is selected.";

            return;
        }

        if (selectedSkeleton_ < 0 ||
            selectedSkeleton_ >=
                static_cast<int>(skeletons_.size()))
        {
            analysisStatus_ =
                "No bind skeleton is selected.";

            return;
        }

        const SourcePackage& package =
            packages_[
                static_cast<std::size_t>(
                    selectedPackage_)];

        const std::filesystem::path& skeletonPath =
            skeletons_[
                static_cast<std::size_t>(
                    selectedSkeleton_)];

        if (!LegacySkeletalReader::ReadSkeleton(
                skeletonPath,
                selectedSkeletonData_))
        {
            analysisStatus_ =
                selectedSkeletonData_.error;

            return;
        }

        if (!LegacyMeshReader::Read(
                package.scbPath,
                package.scoPath,
                &selectedSkeletonData_,
                selectedMeshData_))
        {
            analysisStatus_ =
                selectedMeshData_.error;

            return;
        }

        const std::filesystem::path sourceMeshPath =
            !package.scbPath.empty()
        ? package.scbPath
        : package.scoPath;

        const std::filesystem::path packageDirectory =
            sourceMeshPath.parent_path();

        std::string materialAnalysisError;

        if (!LegacyMaterialReader::ReadForMesh(
                sourceRoot_,
                packageDirectory,
                selectedMeshData_.materialChunks,
                selectedMaterialSet_))
        {
            materialAnalysisError =
                selectedMaterialSet_.error;
        }

        std::string externalWeightError;

        if (!package.wgtPath.empty())
        {
            if (!LegacySkeletalReader::ReadWeights(
                    package.wgtPath,
                    &selectedSkeletonData_,
                    selectedWeightData_))
            {
                externalWeightError =
                    selectedWeightData_.error;

                selectedWeightData_ = {};
            }
        }

        if (selectedWeightData_.vertices.empty())
        {
            if (selectedMeshData_.hasEmbeddedWeights)
            {
                selectedWeightData_ =
                    selectedMeshData_.embeddedWeights;

                usingEmbeddedWeights_ = true;
            }
            else
            {
                analysisStatus_ =
                    externalWeightError.empty()
                        ? "Neither external WGT nor embedded SCB weights were found."
                        : externalWeightError;

                return;
            }
        }

        vertexWeightCountMismatch_ =
            selectedMeshData_.vertices.size() !=
            selectedWeightData_.vertices.size();

        const bool meshWarnings =
            selectedMeshData_.invalidIndexCount != 0U ||
            selectedMeshData_.degenerateTriangleCount != 0U ||
            selectedMeshData_.nonFiniteVertexCount != 0U ||
            selectedMeshData_.uvConflictCount != 0U ||
            selectedMeshData_.invalidMaterialRangeCount != 0U ||
            selectedMeshData_.indexCountNotTriangleList ||
            selectedMeshData_.
                embeddedWeightVertexCountMismatch ||
            selectedMeshData_.usedScoFallback;

        const bool skeletonWarnings =
            selectedSkeletonData_.rootCount != 1U ||
            selectedSkeletonData_.emptyNameCount != 0U ||
            selectedSkeletonData_.duplicateNameCount != 0U ||
            selectedSkeletonData_.invalidParentCount != 0U;

        const bool weightWarnings =
            selectedWeightData_.zeroWeightVertexCount != 0U ||
            selectedWeightData_.nonNormalizedVertexCount != 0U ||
            selectedWeightData_.invalidWeightValueCount != 0U ||
            selectedWeightData_.invalidBoneReferenceCount != 0U ||
            selectedWeightData_.skeletonIdMismatch ||
            vertexWeightCountMismatch_;

        const bool materialWarnings =
            !materialAnalysisError.empty() ||
            selectedMaterialSet_.missingMaterialCount != 0U ||
            selectedMaterialSet_.missingTextureCount != 0U ||
            selectedMaterialSet_.invalidDdsCount != 0U ||
            selectedMaterialSet_.parseWarningCount != 0U;

        analysisSucceeded_ = true;

        analysisStatus_ =
            meshWarnings ||
            skeletonWarnings ||
            weightWarnings ||
            materialWarnings
                ? "Geometry, skeleton, weights and materials were loaded with warnings."
                : "Geometry, skeleton, weights and materials are valid.";

        if (!externalWeightError.empty() &&
            usingEmbeddedWeights_)
        {
            analysisStatus_ +=
                " External WGT failed; embedded SCB weights are used.";
        }

        if (!materialAnalysisError.empty())
        {
            analysisStatus_ +=
                " " +
                materialAnalysisError;
        }

        if (selectedAnimation_ >= 0 && selectedAnimation_ <static_cast<int>(animations_.size()))
        {
            LoadSelectedAnimation();
        }
    }

    void WarZImporterWindow::ConvertSelection() noexcept
    {
        conversionStatus_.clear();
        conversionSucceeded_ = false;

        if (
            !analysisSucceeded_ ||
            selectedPackage_ < 0 ||
            selectedPackage_ >=
                static_cast<int>(
                    packages_.size()))
        {
            conversionStatus_ =
                "Analyze a package before conversion.";

            return;
        }

        if (
            vertexWeightCountMismatch_ ||
            selectedMeshData_.vertices.empty() ||
            selectedWeightData_.vertices.empty())
        {
            conversionStatus_ =
                "Mesh and skin weights are not compatible.";

            return;
        }

        const SourcePackage& package =
            packages_[
                static_cast<std::size_t>(
                    selectedPackage_)];

        std::error_code currentPathError;

        const std::filesystem::path projectRoot =
            std::filesystem::current_path(
                currentPathError);

        if (currentPathError)
        {
            conversionStatus_ =
                "Failed to resolve Editor working directory.";

            return;
        }

        WarZConversionRequest request;

        request.dataRoot =
            projectRoot /
            L"Data";

        request.sourceRoot =
            sourceRoot_;

        request.packageRelativePath =
            package.relativePath;

        request.sourceSkeletonPath =
            selectedSkeletonData_.sourcePath;

        request.mesh =
            &selectedMeshData_;

        request.skeleton =
            &selectedSkeletonData_;

        request.weights =
            &selectedWeightData_;

        request.materials =
            &selectedMaterialSet_;

        request.animationPaths =
            &animations_;

        request.writeSkeletalMesh =
            importSkeletalMesh_;

        request.writeSkeleton =
            importSkeleton_;

        request.writeMaterials =
            importMaterials_;

        request.writeTextures =
            importTextures_;

        request.writeAnimations =
            importAnimations_;

        WarZConversionResult conversion;

        if (!WarZAssetConverter::Convert(
                request,
                conversion))
        {
            conversionStatus_ =
                conversion.error.empty()
                    ? "Asset conversion failed."
                    : conversion.error;

            return;
        }

        std::ostringstream status;

        status <<
            "Conversion completed. "
            "Materials: " <<
            conversion.materialCount <<
            ", textures: " <<
            conversion.textureCount <<
            ", animations: " <<
            conversion.animationCount <<
            ".";

        if (!conversion.warnings.empty())
        {
            status <<
                " Warnings: " <<
                conversion.warnings.size() <<
                ".";
        }

        conversionStatus_ =
            status.str();

        conversionSucceeded_ = true;
    }

    void WarZImporterWindow::DrawMaterialAnalysis() noexcept
    {
        ImGui::SeparatorText("WarZ Materials");

        ImGui::Text(
            "Resolved: %llu | Missing MAT: %llu | "
            "Missing DDS: %llu | Invalid DDS: %llu",
            static_cast<unsigned long long>(
                selectedMaterialSet_.materials.size()),
            static_cast<unsigned long long>(
                selectedMaterialSet_.
                    missingMaterialCount),
            static_cast<unsigned long long>(
                selectedMaterialSet_.
                    missingTextureCount),
            static_cast<unsigned long long>(
                selectedMaterialSet_.
                    invalidDdsCount));

        if (selectedMaterialSet_.materials.empty())
        {
            ImGui::TextDisabled(
                "The selected mesh has no material chunks.");

            return;
        }

        for (std::size_t materialIndex = 0U;
             materialIndex <
                 selectedMaterialSet_.materials.size();
             ++materialIndex)
        {
            const LegacyMaterialData& material =
                selectedMaterialSet_.
                    materials[materialIndex];

            ImGui::PushID(
                static_cast<int>(
                    materialIndex));

            const std::string label =
                material.name.empty()
                    ? "<unnamed material>"
                    : material.name;

            if (ImGui::TreeNode(
                    "LegacyMaterial",
                    "%s",
                    label.c_str()))
            {
                ImGui::ColorButton(
                    "##MaterialColor",
                    ImVec4(
                        material.diffuseColor[0],
                        material.diffuseColor[1],
                        material.diffuseColor[2],
                        1.0F),
                    ImGuiColorEditFlags_NoTooltip,
                    ImVec2(36.0F, 20.0F));

                ImGui::SameLine();

                ImGui::Text(
                    "Color24: %.3f %.3f %.3f",
                    material.diffuseColor[0],
                    material.diffuseColor[1],
                    material.diffuseColor[2]);

                if (!material.sourcePath.empty())
                {
                    ImGui::TextWrapped(
                        "MAT: %s",
                        PathToUtf8(
                            material.sourcePath).
                            c_str());
                }

                if (!material.imagesDirectory.empty())
                {
                    ImGui::TextWrapped(
                        "Images: %s",
                        PathToUtf8(
                            material.imagesDirectory).
                            c_str());
                }

                ImGui::Text(
                    "Type: %s",
                    material.typeName.empty()
                        ? "<default>"
                        : material.typeName.c_str());

                ImGui::Text(
                    "Specular: %.3f | Secondary: %.3f | Reflection: %.3f",
                    material.specularPower,
                    material.specularPower1,
                    material.reflectionPower);

                ImGui::Text(
                    "Double Sided: %s | Transparent: %s | Force Alpha: %s",
                    material.doubleSided
                        ? "Yes"
                        : "No",
                    material.transparent
                        ? "Yes"
                        : "No",
                    material.forceAlpha
                        ? "Yes"
                        : "No");

                if (!material.error.empty())
                {
                    ImGui::TextColored(
                        ImVec4(
                            0.95F,
                            0.32F,
                            0.24F,
                            1.0F),
                        "%s",
                        material.error.c_str());
                }

                if (material.parseWarningCount != 0U)
                {
                    ImGui::TextColored(
                        ImVec4(
                            0.95F,
                            0.65F,
                            0.20F,
                            1.0F),
                        "MAT parse warnings: %llu",
                        static_cast<unsigned long long>(
                            material.
                                parseWarningCount));
                }

                if (ImGui::BeginTable(
                        "MaterialTextures",
                        4,
                        ImGuiTableFlags_Borders |
                            ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_Resizable |
                            ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn(
                        "Slot");

                    ImGui::TableSetupColumn(
                        "Source");

                    ImGui::TableSetupColumn(
                        "DDS");

                    ImGui::TableSetupColumn(
                        "Status");

                    ImGui::TableHeadersRow();

                    for (const LegacyMaterialTexture& texture :
                         material.textures)
                    {
                        if (texture.sourceName.empty())
                        {
                            continue;
                        }

                        ImGui::TableNextRow();

                        ImGui::TableNextColumn();

                        ImGui::TextUnformatted(
                            ToString(texture.slot));

                        ImGui::TableNextColumn();

                        ImGui::TextWrapped(
                            "%s",
                            texture.sourceName.c_str());

                        ImGui::TableNextColumn();

                        if (texture.dds.valid)
                        {
                            ImGui::Text(
                                "%ux%u | %s | Mips %u",
                                texture.dds.width,
                                texture.dds.height,
                                texture.dds.format.c_str(),
                                texture.dds.mipCount);

                            if (texture.dds.arraySize > 1U)
                            {
                                ImGui::TextDisabled(
                                    "Array: %u",
                                    texture.dds.arraySize);
                            }

                            if (texture.dds.isCubeMap)
                            {
                                ImGui::TextDisabled(
                                    "Cube Map");
                            }
                        }
                        else
                        {
                            ImGui::TextDisabled("-");
                        }

                        ImGui::TableNextColumn();

                        if (texture.dds.valid)
                        {
                            ImGui::TextColored(
                                ImVec4(
                                    0.35F,
                                    0.85F,
                                    0.45F,
                                    1.0F),
                                "Ready");
                        }
                        else
                        {
                            ImGui::TextColored(
                                ImVec4(
                                    0.95F,
                                    0.35F,
                                    0.25F,
                                    1.0F),
                                "%s",
                                texture.dds.error.empty()
                                    ? "Missing"
                                    : texture.dds.error.c_str());
                        }

                        if (!texture.dds.path.empty() &&
                            ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip(
                                "%s",
                                PathToUtf8(
                                    texture.dds.path).
                                    c_str());
                        }
                    }

                    ImGui::EndTable();
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }
    }

    void WarZImporterWindow::DrawAnimationControls() noexcept
    {
        ImGui::SeparatorText("Animation Preview");

        ImGui::InputTextWithHint(
            "##WarZAnimationFilter",
            "Filter animations...",
            animationFilter_.data(),
            animationFilter_.size());

        const std::filesystem::path animationsRoot =
            sourceRoot_ / L"Animations";

        std::string selectedLabel =
            "<select WarZ animation>";

        if (selectedAnimation_ >= 0 &&
            selectedAnimation_ <
                static_cast<int>(
                    animations_.size()))
        {
            selectedLabel =
                PathToUtf8(
                    animations_[
                        static_cast<std::size_t>(
                            selectedAnimation_)].
                        lexically_relative(
                            animationsRoot));
        }

        if (ImGui::BeginCombo(
                "Animation",
                selectedLabel.c_str()))
        {
            for (std::size_t index = 0U;
                 index < animations_.size();
                 ++index)
            {
                if (!MatchesAnimationFilter(
                        animations_[index]))
                {
                    continue;
                }

                const std::string label =
                    PathToUtf8(
                        animations_[index].
                            lexically_relative(
                                animationsRoot));

                const bool selected =
                    selectedAnimation_ ==
                    static_cast<int>(index);

                if (ImGui::Selectable(
                        label.c_str(),
                        selected))
                {
                    selectedAnimation_ =
                        static_cast<int>(index);

                    LoadSelectedAnimation();
                }

                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (selectedAnimation_ < 0)
        {
            ImGui::TextDisabled(
                "Select an animation from bin/Data/Animations.");

            return;
        }

        if (!animationLoaded_)
        {
            if (ImGui::Button("Load Animation"))
            {
                LoadSelectedAnimation();
            }
        }

        if (!animationStatus_.empty())
        {
            const bool hasCompatibilityWarning =
                animationCompatible_ &&
                (
                    selectedAnimationData_.skeletonIdMismatch ||
                    selectedAnimationData_.missingBoneTrackCount != 0U
                );

            const ImVec4 color =
                !animationCompatible_
                    ? ImVec4(
                        0.95F,
                        0.35F,
                        0.25F,
                        1.0F)
                    : hasCompatibilityWarning
                        ? ImVec4(
                            0.95F,
                            0.65F,
                            0.20F,
                            1.0F)
                        : ImVec4(
                            0.35F,
                            0.85F,
                            0.45F,
                            1.0F);

            ImGui::TextColored(
                color,
                "%s",
                animationStatus_.c_str());
        }

        if (!animationLoaded_)
        {
            return;
        }

        if (ImGui::BeginTable(
                "WarZAnimationInfo",
                2,
                ImGuiTableFlags_Borders |
                    ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_SizingStretchProp))
        {
            const auto drawRow =
                [](const char* property,
                   const std::string& value)
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(property);

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(
                    value.c_str());
            };

            char animationSkeletonId[32]{};
            char selectedSkeletonId[32]{};

            std::snprintf(
                animationSkeletonId,
                sizeof(animationSkeletonId),
                "0x%08X",
                selectedAnimationData_.
                    skeletonId);

            std::snprintf(
                selectedSkeletonId,
                sizeof(selectedSkeletonId),
                "0x%08X",
                selectedSkeletonData_.
                    skeletonId);

            drawRow(
                "Skeleton ID",
                std::string(animationSkeletonId) +
                    " / selected " +
                    selectedSkeletonId);

            drawRow(
                "Tracks",
                std::to_string(
                    selectedAnimationData_.
                        tracks.size()));

            drawRow(
                "Mapped Tracks",
                std::to_string(
                    selectedAnimationData_.
                        mappedTrackCount));

            drawRow(
                "Unmapped Tracks",
                std::to_string(
                    selectedAnimationData_.
                        missingBoneTrackCount));

            drawRow(
                "Frames",
                std::to_string(
                    selectedAnimationData_.
                        frameCount));

            drawRow(
                "Source FPS",
                std::to_string(
                    selectedAnimationData_.
                        frameRate));

            drawRow(
                "Duration",
                std::to_string(
                    selectedAnimationData_.
                        durationSeconds) +
                    " sec");

            drawRow(
                "Root Tracks",
                std::to_string(
                    selectedAnimationData_.
                        rootTrackCount));

            drawRow(
                "Invalid Quaternions",
                std::to_string(
                    selectedAnimationData_.
                        invalidQuaternionCount));

            drawRow(
                "Non-normalized Quaternions",
                std::to_string(
                    selectedAnimationData_.
                        nonNormalizedQuaternionCount));

            drawRow(
                "Invalid Translations",
                std::to_string(
                    selectedAnimationData_.
                        invalidTranslationCount));

            drawRow(
                "Trailing Bytes",
                std::to_string(
                    selectedAnimationData_.
                        trailingByteCount));

            ImGui::EndTable();
        }

        if (!animationCompatible_)
        {
            return;
        }

        UpdateAnimationPlayback();

        if (ImGui::Button(
                animationPlaying_
                    ? "Pause"
                    : "Play",
                ImVec2(80.0F, 28.0F)))
        {
            const float finalFrame =
                static_cast<float>(
                    selectedAnimationData_.
                        frameCount -
                    1U);

            if (!animationPlaying_ &&
                !animationLoop_ &&
                animationFrame_ >= finalFrame)
            {
                animationFrame_ = 0.0F;
                UpdateAnimationPose();
            }

            animationPlaying_ =
                !animationPlaying_;
        }

        ImGui::SameLine();

        if (ImGui::Button(
                "Stop",
                ImVec2(70.0F, 28.0F)))
        {
            animationPlaying_ = false;
            animationFrame_ = 0.0F;

            UpdateAnimationPose();
        }

        ImGui::SameLine();

        ImGui::Checkbox(
            "Loop",
            &animationLoop_);

        ImGui::SameLine();

        if (ImGui::Checkbox(
                "Lock Root X/Z",
                &animationLockRoot_))
        {
            UpdateAnimationPose();
        }

        if (ImGui::DragFloat(
                "Playback FPS",
                &animationPlaybackFps_,
                0.25F,
                1.0F,
                240.0F,
                "%.2f"))
        {
            animationPlaybackFps_ =
                std::clamp(
                    animationPlaybackFps_,
                    1.0F,
                    240.0F);
        }

        const float finalFrame =
            static_cast<float>(
                selectedAnimationData_.
                    frameCount -
                1U);

        if (ImGui::SliderFloat(
                "Timeline",
                &animationFrame_,
                0.0F,
                finalFrame,
                "Frame %.2f"))
        {
            animationPlaying_ = false;
            UpdateAnimationPose();
        }

        if (ImGui::Button("-1 Frame"))
        {
            animationPlaying_ = false;

            animationFrame_ =
                (std::max)(
                    animationFrame_ -
                        1.0F,
                    0.0F);

            UpdateAnimationPose();
        }

        ImGui::SameLine();

        if (ImGui::Button("+1 Frame"))
        {
            animationPlaying_ = false;

            animationFrame_ =
                (std::min)(
                    animationFrame_ +
                        1.0F,
                    finalFrame);

            UpdateAnimationPose();
        }

        const float currentSeconds =
            selectedAnimationData_.
                frameRate > 0.0F
                    ? animationFrame_ /
                        selectedAnimationData_.
                            frameRate
                    : 0.0F;

        ImGui::SameLine();

        ImGui::Text(
            "%.3f / %.3f sec",
            currentSeconds,
            selectedAnimationData_.
                durationSeconds);
    }

    void WarZImporterWindow::ScanSource() noexcept
    {
        packages_.clear();
        skeletons_.clear();
        animations_.clear();
        ResetAnalysis();

        selectedPackage_ = -1;
        selectedSkeleton_ = -1;
        selectedAnimation_ = -1;

        scbCount_ = 0U;
        scoCount_ = 0U;
        wgtCount_ = 0U;
        materialCount_ = 0U;
        textureCount_ = 0U;

        scanSucceeded_ = false;

        try
        {
            sourceRoot_ = NormalizeSourceRoot(sourceRoot_);

            const std::filesystem::path objectsDepotRoot =
                sourceRoot_ / L"ObjectsDepot";

            const std::filesystem::path animationsRoot =
                sourceRoot_ / L"Animations";

            if (!DirectoryExists(sourceRoot_))
            {
                status_ = "Source root does not exist.";
                return;
            }

            if (!DirectoryExists(objectsDepotRoot))
            {
                status_ = "ObjectsDepot folder was not found.";
                return;
            }
            
            std::unordered_map<std::string, std::size_t> packageLookup;

            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::recursive_directory_iterator(
                     objectsDepotRoot,
                     std::filesystem::directory_options::
                         skip_permission_denied))
            {
                std::error_code entryError;

                if (!entry.is_regular_file(entryError) || entryError)
                {
                    continue;
                }

                const std::filesystem::path filePath = entry.path();

                const bool isScb =
                    HasExtension(filePath, L".scb");

                const bool isSco =
                    HasExtension(filePath, L".sco");

                const bool isWgt =
                    HasExtension(filePath, L".wgt");

                const bool isMaterial =
                    HasExtension(filePath, L".mat");

                const bool isTexture =
                    HasExtension(filePath, L".dds");

                const bool isSkeletonFile =
                    HasExtension(filePath, L".skl");

                if (isMaterial)
                {
                    ++materialCount_;
                }

                if (isTexture)
                {
                    ++textureCount_;
                }
                
                if (isSkeletonFile && IsWarZSkeleton(filePath))
                {
                    skeletons_.push_back(filePath);
                }

                if (!isScb && !isSco && !isWgt)
                {
                    continue;
                }

                std::filesystem::path relativePath =
                    std::filesystem::relative(
                        filePath,
                        objectsDepotRoot,
                        entryError);

                if (entryError)
                {
                    continue;
                }

                relativePath.replace_extension();
                relativePath = relativePath.lexically_normal();

                const std::string packageKey =
                    ToLowerAscii(PathToUtf8(relativePath));

                std::size_t packageIndex = 0U;

                const auto existingPackage =
                    packageLookup.find(packageKey);

                if (existingPackage == packageLookup.end())
                {
                    packageIndex = packages_.size();

                    SourcePackage package;
                    package.relativePath = relativePath;

                    packages_.push_back(std::move(package));

                    packageLookup.emplace(
                        packageKey,
                        packageIndex);
                }
                else
                {
                    packageIndex = existingPackage->second;
                }

                SourcePackage& package =
                    packages_[packageIndex];

                if (isScb)
                {
                    package.scbPath = filePath;
                    ++scbCount_;
                }
                else if (isSco)
                {
                    package.scoPath = filePath;
                    ++scoCount_;
                }
                else
                {
                    package.wgtPath = filePath;
                    ++wgtCount_;
                }
            }
            
            packages_.erase(
                std::remove_if(
                    packages_.begin(),
                    packages_.end(),
                    [](const SourcePackage& package)
                    {
                        return
                            package.scbPath.empty() &&
                            package.scoPath.empty();
                    }),
                packages_.end());
            
            for (SourcePackage& package : packages_)
            {
                const std::filesystem::path sourceFile =
                    !package.scbPath.empty()
                        ? package.scbPath
                        : package.scoPath;

                const std::filesystem::path packageDirectory =
                    sourceFile.parent_path();

                const std::filesystem::path materialsDirectory =
                    packageDirectory / L"Materials";

                const std::filesystem::path texturesDirectory =
                    packageDirectory / L"Textures";

                if (DirectoryExists(materialsDirectory))
                {
                    package.materialCount =
                        CountFilesWithExtension(
                            materialsDirectory,
                            L".mat");
                }
                else
                {
                    package.materialCount =
                        CountFilesWithExtension(
                            packageDirectory,
                            L".mat");
                }

                if (DirectoryExists(texturesDirectory))
                {
                    package.textureCount =
                        CountFilesWithExtension(
                            texturesDirectory,
                            L".dds");
                }
                else
                {
                    package.textureCount =
                        CountFilesWithExtension(
                            packageDirectory,
                            L".dds");
                }
            }

            std::sort(
                packages_.begin(),
                packages_.end(),
                [](const SourcePackage& left,
                   const SourcePackage& right)
                {
                    return
                        ToLowerAscii(
                            PathToUtf8(left.relativePath)) <
                        ToLowerAscii(
                            PathToUtf8(right.relativePath));
                });

            std::sort(
                skeletons_.begin(),
                skeletons_.end(),
                [](const std::filesystem::path& left,
                   const std::filesystem::path& right)
                {
                    return
                        ToLowerWide(left.wstring()) <
                        ToLowerWide(right.wstring());
                });

            skeletons_.erase(
                std::unique(
                    skeletons_.begin(),
                    skeletons_.end()),
                skeletons_.end());
            
            if (DirectoryExists(animationsRoot))
            {
                for (const std::filesystem::directory_entry& entry :
                     std::filesystem::recursive_directory_iterator(
                         animationsRoot,
                         std::filesystem::directory_options::
                             skip_permission_denied))
                {
                    std::error_code entryError;

                    if (!entry.is_regular_file(entryError) ||
                        entryError)
                    {
                        continue;
                    }

                    if (IsWarZAnimation(entry.path()))
                    {
                        animations_.push_back(entry.path());
                    }
                }
            }

            std::sort(
                animations_.begin(),
                animations_.end(),
                [](const std::filesystem::path& left,
                   const std::filesystem::path& right)
                {
                    return
                        ToLowerWide(left.wstring()) <
                        ToLowerWide(right.wstring());
                });

            animations_.erase(
                std::unique(
                    animations_.begin(),
                    animations_.end()),
                animations_.end());

            if (!packages_.empty())
            {
                selectedPackage_ = 0;
            }

            if (!skeletons_.empty())
            {
                selectedSkeleton_ = 0;
                
                for (std::size_t index = 0U;
                     index < skeletons_.size();
                     ++index)
                {
                    const std::wstring fileName =
                        ToLowerWide(
                            skeletons_[index].
                                filename().
                                wstring());

                    if (fileName == L"properscale_andbiped_new.skl")
                    {
                        selectedSkeleton_ =
                            static_cast<int>(index);

                        break;
                    }
                }
            }

            scanSucceeded_ = true;

            status_ =
                "Scan completed: " +
                std::to_string(packages_.size()) +
                " packages, " +
                std::to_string(skeletons_.size()) +
                " skeletons, " +
                std::to_string(animations_.size()) +
                " animations.";
        }
        catch (const std::exception& exception)
        {
            status_ =
                "Source scan failed: " +
                std::string(exception.what());
        }
        catch (...)
        {
            status_ =
                "Source scan failed with an unknown error.";
        }
    }

    void WarZImporterWindow::Draw() noexcept
    {
        InitializeDefaultSource();

        if (!open_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(1100.0F, 700.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "WarZ Importer",
                &open_,
                ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Import WarZ assets into project formats.");

        ImGui::SeparatorText("Source");

        const std::string sourceRootText =
            PathToUtf8(sourceRoot_);

        ImGui::TextWrapped(
            "Source Root: %s",
            sourceRootText.empty()
                ? "<not selected>"
                : sourceRootText.c_str());

        if (ImGui::Button("Browse Folder..."))
        {
            static_cast<void>(
                SelectSourceFolder());
        }

        ImGui::SameLine();

        if (ImGui::Button("Scan Source"))
        {
            ScanSource();
        }

        ImGui::SameLine();

        ImGui::TextDisabled(
            "Expected root: bin/Data");

        if (!status_.empty())
        {
            const ImVec4 statusColor =
                scanSucceeded_
                    ? ImVec4(
                        0.35F,
                        0.85F,
                        0.45F,
                        1.0F)
                    : ImVec4(
                        0.95F,
                        0.72F,
                        0.28F,
                        1.0F);

            ImGui::TextColored(
                statusColor,
                "%s",
                status_.c_str());
        }

        ImGui::SeparatorText("Scan Summary");

        if (ImGui::BeginTable(
                "WarZScanSummary",
                4,
                ImGuiTableFlags_Borders |
                    ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Packages");
            ImGui::TextDisabled(
                "%llu",
                static_cast<unsigned long long>(
                    packages_.size()));

            ImGui::TableNextColumn();
            ImGui::Text("Geometry");
            ImGui::TextDisabled(
                "SCB: %llu | SCO: %llu",
                static_cast<unsigned long long>(
                    scbCount_),
                static_cast<unsigned long long>(
                    scoCount_));

            ImGui::TableNextColumn();
            ImGui::Text("Skin / Skeleton");
            ImGui::TextDisabled(
                "WGT: %llu | SKL: %llu",
                static_cast<unsigned long long>(
                    wgtCount_),
                static_cast<unsigned long long>(
                    skeletons_.size()));

            ImGui::TableNextColumn();
            ImGui::Text("Content");
            ImGui::TextDisabled(
                "MAT: %llu | DDS: %llu | ANM: %llu",
                static_cast<unsigned long long>(
                    materialCount_),
                static_cast<unsigned long long>(
                    textureCount_),
                static_cast<unsigned long long>(
                    animations_.size()));

            ImGui::EndTable();
        }

        ImGui::Separator();

        if (ImGui::BeginTable(
                "WarZImporterLayout",
                2,
                ImGuiTableFlags_BordersInnerV |
                    ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn(
                "Packages",
                ImGuiTableColumnFlags_WidthFixed,
                390.0F);

            ImGui::TableSetupColumn(
                "Package Details",
                ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Skeletal Packages");

            ImGui::InputTextWithHint(
                "##WarZPackageSearch",
                "Search packages...",
                packageFilter_.data(),
                packageFilter_.size());

            if (ImGui::BeginChild(
                    "WarZPackageList",
                    ImVec2(0.0F, 480.0F),
                    true))
            {
                if (packages_.empty())
                {
                    ImGui::TextDisabled(
                        "No .scb or .sco packages found.");
                }

                for (std::size_t index = 0U;
                     index < packages_.size();
                     ++index)
                {
                    const SourcePackage& package =
                        packages_[index];

                    if (!MatchesFilter(package))
                    {
                        continue;
                    }

                    std::string label =
                        PathToUtf8(
                            package.relativePath);

                    if (package.wgtPath.empty())
                    {
                        label += "  [no external WGT]";
                    }

                    const bool selected =
                        selectedPackage_ ==
                        static_cast<int>(index);

                    if (ImGui::Selectable(label.c_str(), selected))
                    {
                        const int newSelection =
                            static_cast<int>(index);

                        if (selectedPackage_ != newSelection)
                        {
                            selectedPackage_ = newSelection;
                            ResetAnalysis();
                        }
                    }

                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
            }

            ImGui::EndChild();

            ImGui::TableNextColumn();

            ImGui::SeparatorText("Selected Package");

            if (selectedPackage_ < 0 ||
                selectedPackage_ >=
                    static_cast<int>(
                        packages_.size()))
            {
                ImGui::TextDisabled(
                    "Select a package from the list.");
            }
            else
            {
                const SourcePackage& package =
                    packages_[
                        static_cast<std::size_t>(
                            selectedPackage_)];

                const std::string packageName =
                    PathToUtf8(
                        package.relativePath);

                ImGui::TextWrapped(
                    "Package: %s",
                    packageName.c_str());

                const auto drawSourceFile =
                    [](const char* label,
                       const std::filesystem::path& path)
                {
                    if (path.empty())
                    {
                        ImGui::TextColored(
                            ImVec4(
                                0.95F,
                                0.35F,
                                0.28F,
                                1.0F),
                            "%s: Missing",
                            label);

                        return;
                    }

                    const std::string pathText =
                        PathToUtf8(path);

                    ImGui::TextWrapped(
                        "%s: %s",
                        label,
                        pathText.c_str());
                };

                drawSourceFile(
                    "Binary Mesh",
                    package.scbPath);

                drawSourceFile(
                    "Source Mesh",
                    package.scoPath);

                drawSourceFile(
                    "Skin Weights",
                    package.wgtPath);

                ImGui::Text(
                    "Materials: %llu",
                    static_cast<unsigned long long>(
                        package.materialCount));

                ImGui::Text(
                    "Textures: %llu",
                    static_cast<unsigned long long>(
                        package.textureCount));

                ImGui::SeparatorText("Skeleton");

                std::string skeletonPreview =
                    "<no valid WarZ skeleton found>";

                if (selectedSkeleton_ >= 0 &&
                    selectedSkeleton_ <
                        static_cast<int>(
                            skeletons_.size()))
                {
                    skeletonPreview =
                        PathToUtf8(
                            skeletons_[
                                static_cast<std::size_t>(
                                    selectedSkeleton_)].
                                lexically_relative(
                                    sourceRoot_));
                }

                if (ImGui::BeginCombo(
                        "Bind Skeleton",
                        skeletonPreview.c_str()))
                {
                    for (std::size_t index = 0U;
                         index < skeletons_.size();
                         ++index)
                    {
                        const std::string label =
                            PathToUtf8(
                                skeletons_[index].
                                    lexically_relative(
                                        sourceRoot_));

                        const bool selected =
                            selectedSkeleton_ ==
                            static_cast<int>(index);

                        if (ImGui::Selectable(label.c_str(), selected))
                        {
                            const int newSelection =
                                static_cast<int>(index);

                            if (selectedSkeleton_ != newSelection)
                            {
                                selectedSkeleton_ = newSelection;
                                ResetAnalysis();
                            }
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text(
                    "Available animations: %llu",
                    static_cast<unsigned long long>(
                        animations_.size()));

                ImGui::TextDisabled("Animation-to-package binding will be added later.");
                ImGui::SeparatorText("Legacy Analysis");

                const bool canAnalyze =
                (
                    !package.scbPath.empty() ||
                    !package.scoPath.empty()
                ) &&
                selectedSkeleton_ >= 0 &&
                selectedSkeleton_ <
                    static_cast<int>(
                        skeletons_.size());

                if (!canAnalyze)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button(
                        "Analyze Selected",
                        ImVec2(170.0F, 30.0F)))
                {
                    AnalyzeSelection();
                }

                if (!canAnalyze)
                {
                    ImGui::EndDisabled();
                }

                if (analysisAttempted_)
                {
                    const ImVec4 analysisColor =
                        analysisSucceeded_
                            ? ImVec4(
                                0.35F,
                                0.85F,
                                0.45F,
                                1.0F)
                            : ImVec4(
                                0.95F,
                                0.35F,
                                0.28F,
                                1.0F);

                    ImGui::TextColored(
                        analysisColor,
                        "%s",
                        analysisStatus_.c_str());
                }

                if (analysisSucceeded_)
                {
                    const char* meshFormatName = "Unknown";

                    switch (selectedMeshData_.format)
                    {
                        case LegacyMeshFormat::Scb:
                            meshFormatName = "SCB";
                            break;

                        case LegacyMeshFormat::Sco:
                            meshFormatName = "SCO";
                            break;

                        case LegacyMeshFormat::Unknown:
                            break;
                    }

                    ImGui::Text(
                        "Geometry Source: %s",
                        meshFormatName);

                    ImGui::SameLine();

                    ImGui::TextDisabled(
                        "%s",
                        PathToUtf8(
                            selectedMeshData_.sourcePath).
                            c_str());

                    ImGui::Text(
                        "Weight Source: %s",
                        usingEmbeddedWeights_
                            ? "Embedded SCB"
                            : "External WGT");

                    if (!selectedMeshData_.warning.empty())
                    {
                        ImGui::TextColored(
                            ImVec4(
                                0.95F,
                                0.65F,
                                0.20F,
                                1.0F),
                            "%s",
                            selectedMeshData_.warning.c_str());
                    }

                    if (ImGui::BeginTable(
                            "LegacyMeshAnalysis",
                            2,
                            ImGuiTableFlags_Borders |
                                ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Mesh Property");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        const auto drawMeshRow =
                            [](const char* property,
                               const std::string& value)
                        {
                            ImGui::TableNextRow();

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(property);

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(value.c_str());
                        };

                        drawMeshRow(
                            "Name",
                            selectedMeshData_.name.empty()
                                ? "<unnamed>"
                                : selectedMeshData_.name);

                        drawMeshRow(
                            "Vertices",
                            std::to_string(
                                selectedMeshData_.vertices.size()));

                        drawMeshRow(
                            "Weight Vertices",
                            std::to_string(
                                selectedWeightData_.vertices.size()));

                        drawMeshRow(
                            "Indices",
                            std::to_string(
                                selectedMeshData_.indices.size()));

                        drawMeshRow(
                            "Triangles",
                            std::to_string(
                                selectedMeshData_.indices.size() /
                                3U));

                        drawMeshRow(
                            "Material Chunks",
                            std::to_string(
                                selectedMeshData_.
                                    materialChunks.size()));

                        drawMeshRow(
                            "Invalid Indices",
                            std::to_string(
                                selectedMeshData_.
                                    invalidIndexCount));

                        drawMeshRow(
                            "Degenerate Triangles",
                            std::to_string(
                                selectedMeshData_.
                                    degenerateTriangleCount));

                        drawMeshRow(
                            "Non-finite Vertices",
                            std::to_string(
                                selectedMeshData_.
                                    nonFiniteVertexCount));

                        drawMeshRow(
                            "SCO UV Conflicts",
                            std::to_string(
                                selectedMeshData_.
                                    uvConflictCount));

                        drawMeshRow(
                            "Invalid Material Ranges",
                            std::to_string(
                                selectedMeshData_.
                                    invalidMaterialRangeCount));

                        drawMeshRow(
                            "Vertex Colors",
                            selectedMeshData_.hasVertexColors
                                ? "Yes"
                                : "No");

                        drawMeshRow(
                            "Embedded Weights",
                            selectedMeshData_.hasEmbeddedWeights
                                ? "Yes"
                                : "No");

                        drawMeshRow(
                            "Trailing Bytes",
                            std::to_string(
                                selectedMeshData_.
                                    trailingByteCount));

                        ImGui::EndTable();
                    }

                    if (vertexWeightCountMismatch_)
                    {
                        ImGui::TextColored(
                            ImVec4(
                                0.95F,
                                0.30F,
                                0.22F,
                                1.0F),
                            "Error: mesh vertex count does not match weight vertex count.");
                    }

                    if (selectedMeshData_.
                            embeddedWeightVertexCountMismatch)
                    {
                        ImGui::TextColored(
                            ImVec4(
                                0.95F,
                                0.30F,
                                0.22F,
                                1.0F),
                            "Error: embedded SCB weight count does not match mesh vertices.");
                    }

                    if (ImGui::TreeNode(
                            "Material Slots"))
                    {
                        if (selectedMeshData_.
                                materialChunks.empty())
                        {
                            ImGui::TextDisabled(
                                "No material chunks.");
                        }

                        for (std::size_t chunkIndex = 0U;
                             chunkIndex <
                                 selectedMeshData_.
                                     materialChunks.size();
                             ++chunkIndex)
                        {
                            const LegacyMaterialChunk& chunk =
                                selectedMeshData_.
                                    materialChunks[chunkIndex];

                            ImGui::Text(
                                "%llu: %s | First=%u | Count=%u",
                                static_cast<unsigned long long>(
                                    chunkIndex),
                                chunk.materialName.empty()
                                    ? "<unnamed>"
                                    : chunk.materialName.c_str(),
                                chunk.firstIndex,
                                chunk.indexCount);
                        }

                        ImGui::TreePop();
                    }

                    DrawMaterialAnalysis();

                    ImGui::Spacing();
                    
                    if (ImGui::BeginTable(
                            "LegacySkeletalAnalysis",
                            3,
                            ImGuiTableFlags_Borders |
                                ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableSetupColumn("Property");
                        ImGui::TableSetupColumn("Skeleton");
                        ImGui::TableSetupColumn("Weights");
                        ImGui::TableHeadersRow();

                        const auto drawRow =
                            [](const char* property,
                               const std::string& skeletonValue,
                               const std::string& weightValue)
                        {
                            ImGui::TableNextRow();

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(property);

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(
                                skeletonValue.c_str());

                            ImGui::TableNextColumn();
                            ImGui::TextUnformatted(
                                weightValue.c_str());
                        };

                        char skeletonIdText[32]{};
                        char weightSkeletonIdText[32]{};

                        std::snprintf(
                            skeletonIdText,
                            sizeof(skeletonIdText),
                            "0x%08X",
                            static_cast<unsigned int>(
                                selectedSkeletonData_.skeletonId));

                        std::snprintf(
                            weightSkeletonIdText,
                            sizeof(weightSkeletonIdText),
                            "0x%08X",
                            static_cast<unsigned int>(
                                selectedWeightData_.skeletonId));

                        drawRow(
                            "Skeleton ID",
                            skeletonIdText,
                            weightSkeletonIdText);

                        drawRow(
                            "Elements",
                            std::to_string(
                                selectedSkeletonData_.bones.size()) +
                                " bones",

                            std::to_string(
                                selectedWeightData_.vertices.size()) +
                                " vertices");

                        drawRow(
                            "Hierarchy",
                            std::to_string(
                                selectedSkeletonData_.rootCount) +
                                " roots",

                            "Max Bone ID: " +
                                std::to_string(
                                    selectedWeightData_.
                                        maximumBoneIndex));

                        drawRow(
                            "Invalid references",
                            std::to_string(
                                selectedSkeletonData_.
                                    invalidParentCount) +
                                " parents",

                            std::to_string(
                                selectedWeightData_.
                                    invalidBoneReferenceCount) +
                                " influences");

                        drawRow(
                            "Names / values",
                            std::to_string(
                                selectedSkeletonData_.
                                    duplicateNameCount) +
                                " duplicate names",

                            std::to_string(
                                selectedWeightData_.
                                    invalidWeightValueCount) +
                                " invalid weights");

                        drawRow(
                            "Weight normalization",
                            "-",

                            std::to_string(
                                selectedWeightData_.
                                    nonNormalizedVertexCount) +
                                " vertices");

                        drawRow(
                            "Zero weights",
                            "-",

                            std::to_string(
                                selectedWeightData_.
                                    zeroWeightVertexCount) +
                                " vertices");

                        drawRow(
                            "Trailing bytes",
                            std::to_string(
                                selectedSkeletonData_.
                                    trailingByteCount),

                            std::to_string(
                                selectedWeightData_.
                                    trailingByteCount));

                        ImGui::EndTable();
                    }

                    if (selectedWeightData_.skeletonIdMismatch)
                    {
                        ImGui::TextColored(
                            ImVec4(
                                0.95F,
                                0.45F,
                                0.20F,
                                1.0F),
                            "Warning: WGT skeleton ID does not match the selected SKL.");
                    }

                    if (ImGui::TreeNode(
                            "Bone Hierarchy Preview"))
                    {
                        const std::size_t visibleBoneCount =
                            (std::min)(
                                selectedSkeletonData_.bones.size(),
                                static_cast<std::size_t>(128U));

                        if (ImGui::BeginChild(
                                "LegacyBoneList",
                                ImVec2(0.0F, 220.0F),
                                true))
                        {
                            for (std::size_t boneIndex = 0U;
                                 boneIndex < visibleBoneCount;
                                 ++boneIndex)
                            {
                                const LegacyBone& bone =
                                    selectedSkeletonData_.
                                        bones[boneIndex];

                                ImGui::Text(
                                    "%llu: %s  Parent=%d  Length=%.3f",
                                    static_cast<unsigned long long>(
                                        boneIndex),
                                    bone.name.empty()
                                        ? "<unnamed>"
                                        : bone.name.c_str(),
                                    bone.parentIndex,
                                    bone.length);
                            }

                            if (visibleBoneCount <
                                selectedSkeletonData_.bones.size())
                            {
                                ImGui::TextDisabled(
                                    "... %llu more bones",
                                    static_cast<unsigned long long>(
                                        selectedSkeletonData_.
                                            bones.size() -
                                        visibleBoneCount));
                            }
                        }

                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                }

                ImGui::SeparatorText("Import");

                ImGui::Checkbox(
                    "Skeletal Mesh",
                    &importSkeletalMesh_);

                ImGui::Checkbox(
                    "Skeleton",
                    &importSkeleton_);

                ImGui::Checkbox(
                    "Materials",
                    &importMaterials_);

                ImGui::Checkbox(
                    "Textures",
                    &importTextures_);

                ImGui::Checkbox(
                    "Animations",
                    &importAnimations_);

                ImGui::SeparatorText("Output");

                std::filesystem::path skeletalMeshOutput =
                    std::filesystem::path(L"Data") /
                    L"SkeletalMeshes" /
                    package.relativePath;

                skeletalMeshOutput.replace_extension(L".skm");

                const std::string outputText =
                    PathToUtf8(
                        skeletalMeshOutput);

                ImGui::TextWrapped(
                    "Skeletal Mesh: %s",
                    outputText.c_str());

                const std::filesystem::path skeletonOutput =std::filesystem::path(L"Data") / WarZAssetConverter::BuildSkeletonRelativePath(selectedSkeletonData_.sourcePath);
                ImGui::TextWrapped("Skeleton: %s", PathToUtf8(skeletonOutput).c_str());

                ImGui::TextWrapped(
                    "Materials: Data/Materials/%s",
                    PathToUtf8(
                        package.relativePath.
                            parent_path()).
                        c_str());

                ImGui::TextWrapped(
                    "Textures: Data/Textures/%s",
                    PathToUtf8(
                        package.relativePath.
                            parent_path()).
                        c_str());

                ImGui::TextWrapped(
                    "Animations: Data/Animations");

                ImGui::Spacing();

                const bool canPreview = analysisSucceeded_ && !selectedMeshData_.vertices.empty() && !selectedMeshData_.indices.empty();
                if (!canPreview)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button(
                        showLegacyPreview_
                            ? "Hide Preview"
                            : "Preview Legacy",
                        ImVec2(145.0F, 32.0F)))
                {
                    showLegacyPreview_ =
                        !showLegacyPreview_;

                    if (showLegacyPreview_)
                    {
                        meshPreview_.Frame(
                            selectedMeshData_);
                    }
                }

                if (!canPreview)
                {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();

                const bool canConvert =
                    analysisSucceeded_ &&
                    !selectedMeshData_.vertices.empty() &&
                    !selectedMeshData_.indices.empty() &&
                    !selectedWeightData_.vertices.empty() &&
                    !vertexWeightCountMismatch_;

                if (!canConvert)
                {
                    ImGui::BeginDisabled();
                }

                if (ImGui::Button(
                        "Convert Assets",
                        ImVec2(145.0F, 32.0F)))
                {
                    ConvertSelection();
                }

                if (!canConvert)
                {
                    ImGui::EndDisabled();
                }

                ImGui::SameLine();

                ImGui::BeginDisabled();

                ImGui::Button(
                    "Export Editable FBX",
                    ImVec2(175.0F, 32.0F));

                ImGui::EndDisabled();

                if (!conversionStatus_.empty())
                {
                    ImGui::TextColored(
                        conversionSucceeded_
                            ? ImVec4(
                                0.35F,
                                0.85F,
                                0.45F,
                                1.0F)
                            : ImVec4(
                                0.95F,
                                0.35F,
                                0.25F,
                                1.0F),
                        "%s",
                        conversionStatus_.c_str());
                }

                if (analysisSucceeded_)
                {
                    DrawAnimationControls();
                }

                const LegacyAnimationPose* animationPose = animationLoaded_ && animationCompatible_ ? &selectedAnimationPose_: nullptr;

                if (showLegacyPreview_ &&
                    canPreview)
                {
                    ImGui::SeparatorText(animationLoaded_ && animationCompatible_ ? "3D Animation Preview": "3D Bind Pose Preview");

                    if (ImGui::Button("Frame"))
                    {
                        meshPreview_.Frame(
                            selectedMeshData_);
                    }

                    ImGui::SameLine();

                    ImGui::Checkbox(
                        "Skeleton",
                        &previewShowSkeleton_);

                    ImGui::SameLine();

                    ImGui::Checkbox(
                        "Wireframe",
                        &previewWireframe_);

                    ImGui::TextDisabled("WarZ Color24 is applied. DDS files are resolved and validated below.");

                    const float previewWidth =
                        (std::max)(
                            ImGui::GetContentRegionAvail().x,
                            320.0F);

                    meshPreview_.Draw(
                        selectedMeshData_,
                        &selectedSkeletonData_,
                        &selectedWeightData_,
                        &selectedMaterialSet_,
                        animationPose,
                        previewWidth,
                        430.0F,
                        previewShowSkeleton_,
                        previewWireframe_);
                }

                ImGui::TextDisabled(animationPose != nullptr ? "D3D11 WarZ animation preview with GPU skinning.": "D3D11 bind-pose preview with WarZ DDS materials.");
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }

    void WarZImporterWindow::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) noexcept
    {
        meshPreview_.Initialize(device, context);
    }

    void WarZImporterWindow::Shutdown() noexcept
    {
        meshPreview_.Shutdown();
    }
}
