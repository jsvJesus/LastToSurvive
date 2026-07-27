#include "Editor/Tools/Import/WarZImporterWindow.h"

#include <imgui.h>

#include <Windows.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>

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

    void WarZImporterWindow::ScanSource() noexcept
    {
        packages_.clear();
        skeletons_.clear();
        animations_.clear();

        selectedPackage_ = -1;
        selectedSkeleton_ = -1;

        scbCount_ = 0U;
        scoCount_ = 0U;
        wgtCount_ = 0U;
        materialCount_ = 0U;
        textureCount_ = 0U;

        scanSucceeded_ = false;

        try
        {
            sourceRoot_ =
                NormalizeSourceRoot(sourceRoot_);

            const std::filesystem::path charactersRoot =
                sourceRoot_ /
                L"ObjectsDepot" /
                L"CharactersNew";

            const std::filesystem::path animationsRoot =
                sourceRoot_ / L"Animations";

            if (!DirectoryExists(sourceRoot_))
            {
                status_ =
                    "Source root does not exist.";

                return;
            }

            if (!DirectoryExists(charactersRoot))
            {
                status_ =
                    "ObjectsDepot/CharactersNew was not found.";

                return;
            }

            std::unordered_map<
                std::string,
                std::size_t> packageLookup;

            for (const std::filesystem::directory_entry& entry :
                 std::filesystem::recursive_directory_iterator(
                     charactersRoot,
                     std::filesystem::directory_options::
                         skip_permission_denied))
            {
                std::error_code error;

                if (!entry.is_regular_file(error) || error)
                {
                    continue;
                }

                const std::filesystem::path& filePath =
                    entry.path();

                const bool isScb =
                    HasExtension(filePath, L".scb");

                const bool isSco =
                    HasExtension(filePath, L".sco");

                const bool isWgt =
                    HasExtension(filePath, L".wgt");

                if (!isScb && !isSco && !isWgt)
                {
                    continue;
                }

                std::filesystem::path relativePath =
                    std::filesystem::relative(
                        filePath,
                        charactersRoot,
                        error);

                if (error)
                {
                    continue;
                }

                relativePath.replace_extension();

                const std::string key =
                    ToLowerAscii(
                        PathToUtf8(relativePath));

                std::size_t packageIndex = 0U;

                const auto found =
                    packageLookup.find(key);

                if (found == packageLookup.end())
                {
                    packageIndex = packages_.size();

                    SourcePackage package;
                    package.relativePath =
                        relativePath.lexically_normal();

                    packages_.push_back(
                        std::move(package));

                    packageLookup.emplace(
                        key,
                        packageIndex);
                }
                else
                {
                    packageIndex = found->second;
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

                package.materialCount =
                    CountFilesWithExtension(
                        packageDirectory / L"Materials",
                        L".mat");

                package.textureCount =
                    CountFilesWithExtension(
                        packageDirectory / L"Textures",
                        L".dds");

                materialCount_ +=
                    package.materialCount;

                textureCount_ +=
                    package.textureCount;
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

            const std::filesystem::path skeletonRoots[]
            {
                sourceRoot_ /
                    L"ObjectsDepot" /
                    L"Characters",

                sourceRoot_ /
                    L"ObjectsDepot" /
                    L"CharactersNew"
            };

            for (const std::filesystem::path& skeletonRoot :
                 skeletonRoots)
            {
                if (!DirectoryExists(skeletonRoot))
                {
                    continue;
                }

                for (const std::filesystem::directory_entry& entry :
                     std::filesystem::recursive_directory_iterator(
                         skeletonRoot,
                         std::filesystem::directory_options::
                             skip_permission_denied))
                {
                    std::error_code error;

                    if (!entry.is_regular_file(error) ||
                        error)
                    {
                        continue;
                    }

                    if (IsWarZSkeleton(entry.path()))
                    {
                        skeletons_.push_back(
                            entry.path());
                    }
                }
            }

            std::sort(
                skeletons_.begin(),
                skeletons_.end());

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
                    std::error_code error;

                    if (!entry.is_regular_file(error) ||
                        error)
                    {
                        continue;
                    }

                    if (IsWarZAnimation(entry.path()))
                    {
                        animations_.push_back(
                            entry.path());
                    }
                }
            }

            std::sort(
                animations_.begin(),
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
                    if (ToLowerWide(
                            skeletons_[index].
                                filename().
                                wstring()) ==
                        L"properscale_andbiped_new.skl")
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
                " skeletal packages, " +
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
            "Import WarZ skeletal assets and animations into LTS.");

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
                        label += "  [missing WGT]";
                    }

                    const bool selected =
                        selectedPackage_ ==
                        static_cast<int>(index);

                    if (ImGui::Selectable(
                            label.c_str(),
                            selected))
                    {
                        selectedPackage_ =
                            static_cast<int>(index);
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

                        if (ImGui::Selectable(
                                label.c_str(),
                                selected))
                        {
                            selectedSkeleton_ =
                                static_cast<int>(index);
                        }

                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text(
                    "Available animations: %llu",
                    static_cast<unsigned long long>(
                        animations_.size()));

                ImGui::TextDisabled(
                    "Animation-to-package binding will be added with the legacy parser.");

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

                skeletalMeshOutput +=
                    L".ltsskeletalmesh";

                const std::string outputText =
                    PathToUtf8(
                        skeletalMeshOutput);

                ImGui::TextWrapped(
                    "Skeletal Mesh: %s",
                    outputText.c_str());

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

                ImGui::BeginDisabled();

                ImGui::Button(
                    "Preview Legacy",
                    ImVec2(145.0F, 32.0F));

                ImGui::SameLine();

                ImGui::Button(
                    "Convert to LTS",
                    ImVec2(145.0F, 32.0F));

                ImGui::SameLine();

                ImGui::Button(
                    "Export Editable FBX",
                    ImVec2(175.0F, 32.0F));

                ImGui::EndDisabled();

                ImGui::TextDisabled(
                    "Source scanner is ready. Legacy mesh parsing and conversion are not connected yet.");
            }

            ImGui::EndTable();
        }

        ImGui::End();
    }
}