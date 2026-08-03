#include "Editor/Tools/Import/FbxAssetImporter.h"

#include "Assets/AssetResult.h"
#include "Assets/FbxAnimationsImporter.h"
#include "Assets/FbxSkeletalMeshImporter.h"
#include "Assets/FbxStaticMeshImporter.h"

#include <imgui.h>

#include <Windows.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <utility>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        std::string ToUtf8(
            const std::wstring& value)
        {
            if (value.empty())
            {
                return {};
            }

            const int required =
                WideCharToMultiByte(
                    CP_UTF8,
                    0,
                    value.data(),
                    static_cast<int>(
                        value.size()),
                    nullptr,
                    0,
                    nullptr,
                    nullptr);

            if (required <= 0)
            {
                return {};
            }

            std::string output(
                static_cast<std::size_t>(
                    required),
                '\0');

            WideCharToMultiByte(
                CP_UTF8,
                0,
                value.data(),
                static_cast<int>(
                    value.size()),
                output.data(),
                required,
                nullptr,
                nullptr);

            return output;
        }

        class ComScope final
        {
        public:
            ComScope() noexcept
            {
                const HRESULT result =
                    CoInitializeEx(
                        nullptr,
                        COINIT_APARTMENTTHREADED |
                        COINIT_DISABLE_OLE1DDE);

                uninitialize_ =
                    result == S_OK ||
                    result == S_FALSE;
            }

            ~ComScope()
            {
                if (uninitialize_)
                {
                    CoUninitialize();
                }
            }

        private:
            bool uninitialize_ = false;
        };

        [[nodiscard]]
        bool SelectFile(
            const wchar_t* title,
            const COMDLG_FILTERSPEC* filters,
            const UINT filterCount,
            std::filesystem::path& output)
        {
            ComScope comScope;

            Microsoft::WRL::ComPtr<
                IFileOpenDialog>
                dialog;

            if (FAILED(
                    CoCreateInstance(
                        CLSID_FileOpenDialog,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&dialog))))
            {
                return false;
            }

            if (title != nullptr)
            {
                dialog->SetTitle(title);
            }

            if (
                filters != nullptr &&
                filterCount != 0U)
            {
                dialog->SetFileTypes(
                    filterCount,
                    filters);
            }

            if (FAILED(dialog->Show(nullptr)))
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IShellItem>
                item;

            if (FAILED(
                    dialog->GetResult(&item)))
            {
                return false;
            }

            PWSTR path = nullptr;

            if (FAILED(
                    item->GetDisplayName(
                        SIGDN_FILESYSPATH,
                        &path)))
            {
                return false;
            }

            output =
                std::filesystem::path(path).
                    lexically_normal();

            CoTaskMemFree(path);

            return true;
        }

        [[nodiscard]]
        bool SelectFolder(
            std::filesystem::path& output)
        {
            ComScope comScope;

            Microsoft::WRL::ComPtr<
                IFileOpenDialog>
                dialog;

            if (FAILED(
                    CoCreateInstance(
                        CLSID_FileOpenDialog,
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&dialog))))
            {
                return false;
            }

            DWORD options = 0U;

            if (FAILED(
                    dialog->GetOptions(&options)))
            {
                return false;
            }

            dialog->SetOptions(
                options |
                FOS_PICKFOLDERS |
                FOS_FORCEFILESYSTEM);

            dialog->SetTitle(
                L"Select LTS asset output directory");

            if (FAILED(dialog->Show(nullptr)))
            {
                return false;
            }

            Microsoft::WRL::ComPtr<IShellItem>
                item;

            if (FAILED(
                    dialog->GetResult(&item)))
            {
                return false;
            }

            PWSTR path = nullptr;

            if (FAILED(
                    item->GetDisplayName(
                        SIGDN_FILESYSPATH,
                        &path)))
            {
                return false;
            }

            output =
                std::filesystem::path(path).
                    lexically_normal();

            CoTaskMemFree(path);

            return true;
        }

        void AppendReport(
            const engine::assets::FbxImportReport&
                report,
            std::vector<std::filesystem::path>&
                files,
            std::vector<std::wstring>& warnings)
        {
            files.insert(
                files.end(),
                report.writtenFiles.begin(),
                report.writtenFiles.end());

            warnings.insert(
                warnings.end(),
                report.warnings.begin(),
                report.warnings.end());
        }
    }

    void FbxAssetImporter::SetError(
        std::wstring message) noexcept
    {
        status_ = std::move(message);
        operationSucceeded_ = false;
    }

    void FbxAssetImporter::SelectSource() noexcept
    {
        constexpr COMDLG_FILTERSPEC filters[]
        {
            {
                L"Autodesk FBX (*.fbx)",
                L"*.fbx"
            }
        };

        std::filesystem::path selected;

        if (
            SelectFile(
                L"Select FBX source",
                filters,
                1U,
                selected))
        {
            sourceFile_ =
                std::move(selected);

            analyzed_ = false;
            sourceInfo_ = {};

            Analyze();
        }
    }

    void FbxAssetImporter::
        SelectDestination() noexcept
    {
        std::filesystem::path selected;

        if (SelectFolder(selected))
        {
            destinationDirectory_ =
                std::move(selected);
        }
    }

    void FbxAssetImporter::
        SelectExistingSkeleton() noexcept
    {
        constexpr COMDLG_FILTERSPEC filters[]
        {
            {
                L"LTS Skeleton (*.skeleton)",
                L"*.skeleton"
            }
        };

        std::filesystem::path selected;

        if (
            SelectFile(
                L"Select existing skeleton",
                filters,
                1U,
                selected))
        {
            existingSkeletonFile_ =
                std::move(selected);
        }
    }

    void FbxAssetImporter::Analyze() noexcept
    {
        sourceInfo_ = {};
        status_.clear();
        writtenFiles_.clear();
        warnings_.clear();

        const engine::assets::AssetResult result =
            engine::assets::InspectFbxSource(
                sourceFile_,
                sourceInfo_,
                status_);

        analyzed_ =
            engine::assets::Succeeded(result);

        operationSucceeded_ = analyzed_;

        if (!analyzed_)
        {
            return;
        }

        importStaticMeshes_ =
            sourceInfo_.HasStaticMeshes();

        importSkeletalMeshes_ =
            sourceInfo_.HasSkeletalMeshes();

        importSkeleton_ =
            sourceInfo_.HasSkeleton() &&
            (
                importSkeletalMeshes_ ||
                sourceInfo_.HasAnimations()
            );

        importAnimations_ =
            sourceInfo_.HasAnimations();

        status_ =
            L"FBX analysis completed.";
    }

    void FbxAssetImporter::Import() noexcept
    {
        writtenFiles_.clear();
        warnings_.clear();
        status_.clear();
        operationSucceeded_ = false;

        if (!analyzed_)
        {
            SetError(
                L"FBX must be analyzed before import.");

            return;
        }

        if (destinationDirectory_.empty())
        {
            SetError(
                L"Select an output directory "
                L"inside game/Data.");

            return;
        }

        if (
            !importStaticMeshes_ &&
            !importSkeletalMeshes_ &&
            !importSkeleton_ &&
            !importAnimations_)
        {
            SetError(
                L"No asset type is selected.");

            return;
        }

        engine::assets::FbxImportReport report;

        if (importStaticMeshes_)
        {
            engine::assets::
                FbxStaticMeshImportOptions options;

            options.destinationDirectory =
                destinationDirectory_;

            options.overwriteExisting = true;

            std::wstring error;

            const auto result =
                engine::assets::
                    FbxStaticMeshImporter::Import(
                        sourceFile_,
                        options,
                        report,
                        error);

            if (engine::assets::Failed(result))
            {
                SetError(std::move(error));
                return;
            }

            AppendReport(
                report,
                writtenFiles_,
                warnings_);
        }

        std::filesystem::path skeletonFile =
            existingSkeletonFile_;

        if (
            importSkeletalMeshes_ ||
            importSkeleton_)
        {
            engine::assets::
                FbxSkeletalMeshImportOptions options;

            options.destinationDirectory =
                destinationDirectory_;

            options.skeletonFile =
                importSkeleton_
                    ? std::filesystem::path{}
                    : existingSkeletonFile_;

            options.writeSkeleton =
                importSkeleton_;

            options.writeMeshes =
                importSkeletalMeshes_;

            options.maximumBoneInfluences =
                static_cast<std::uint32_t>(
                    std::clamp(
                        maximumBoneInfluences_,
                        1,
                        4));

            options.overwriteExisting = true;

            std::filesystem::path
                generatedSkeletonFile;

            std::string
                generatedSkeletonAssetPath;

            std::wstring error;

            const auto result =
                engine::assets::
                    FbxSkeletalMeshImporter::Import(
                        sourceFile_,
                        options,
                        report,
                        generatedSkeletonFile,
                        generatedSkeletonAssetPath,
                        error);

            if (engine::assets::Failed(result))
            {
                SetError(std::move(error));
                return;
            }

            skeletonFile =
                generatedSkeletonFile;

            AppendReport(
                report,
                writtenFiles_,
                warnings_);
        }

        if (importAnimations_)
        {
            if (skeletonFile.empty())
            {
                SetError(
                    L"Animations require an imported "
                    L"or existing .skeleton file.");

                return;
            }

            engine::assets::
                FbxAnimationImportOptions options;

            options.destinationDirectory =
                destinationDirectory_;

            options.skeletonFile =
                skeletonFile;

            options.sampleRate =
                animationSampleRate_;

            options.overwriteExisting = true;

            std::wstring error;

            const auto result =
                engine::assets::
                    FbxAnimationsImporter::Import(
                        sourceFile_,
                        options,
                        report,
                        error);

            if (engine::assets::Failed(result))
            {
                SetError(std::move(error));
                return;
            }

            AppendReport(
                report,
                writtenFiles_,
                warnings_);
        }

        operationSucceeded_ = true;

        status_ =
            L"Import completed successfully.";
    }

    void FbxAssetImporter::Draw() noexcept
    {
        if (!open_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(760.0F, 680.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "FBX Model Importer",
                &open_,
                ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        ImGui::TextUnformatted(
            "Unified LTS model and animation importer");

        ImGui::Separator();

        ImGui::SeparatorText("Source FBX");

        if (ImGui::Button(
                "Select FBX...",
                ImVec2(150.0F, 30.0F)))
        {
            SelectSource();
        }

        ImGui::SameLine();

        if (sourceFile_.empty())
        {
            ImGui::TextDisabled(
                "No FBX selected");
        }
        else
        {
            const std::string source =
                sourceFile_.u8string();

            ImGui::TextWrapped(
                "%s",
                source.c_str());
        }

        if (
            !sourceFile_.empty() &&
            ImGui::Button("Analyze FBX"))
        {
            Analyze();
        }

        ImGui::SeparatorText("Output");

        if (ImGui::Button(
                "Select Output...",
                ImVec2(150.0F, 30.0F)))
        {
            SelectDestination();
        }

        ImGui::SameLine();

        if (destinationDirectory_.empty())
        {
            ImGui::TextDisabled(
                "Select a directory inside game/Data");
        }
        else
        {
            const std::string destination =
                destinationDirectory_.u8string();

            ImGui::TextWrapped(
                "%s",
                destination.c_str());
        }

        ImGui::SeparatorText("Detected Content");

        if (!analyzed_)
        {
            ImGui::TextDisabled(
                "Select and analyze an FBX file.");
        }
        else
        {
            ImGui::Text(
                "Static meshes: %zu",
                sourceInfo_.staticMeshCount);

            ImGui::Text(
                "Skeletal meshes: %zu",
                sourceInfo_.skeletalMeshCount);

            ImGui::Text(
                "Skeleton bones: %zu",
                sourceInfo_.skeletonBoneCount);

            ImGui::Text(
                "Animation clips: %zu",
                sourceInfo_.animationClipCount);
        }

        ImGui::SeparatorText("Assets To Import");

        ImGui::BeginDisabled(
            !sourceInfo_.HasStaticMeshes());

        ImGui::Checkbox(
            "Static Meshes (.sm)",
            &importStaticMeshes_);

        ImGui::EndDisabled();

        ImGui::BeginDisabled(
            !sourceInfo_.HasSkeletalMeshes());

        ImGui::Checkbox(
            "Skeletal Meshes (.skm)",
            &importSkeletalMeshes_);

        ImGui::EndDisabled();

        ImGui::BeginDisabled(
            !sourceInfo_.HasSkeleton());

        ImGui::Checkbox(
            "Skeleton (.skeleton)",
            &importSkeleton_);

        ImGui::EndDisabled();

        ImGui::BeginDisabled(
            !sourceInfo_.HasAnimations());

        ImGui::Checkbox(
            "Animations (.anim)",
            &importAnimations_);

        ImGui::EndDisabled();

        if (
            (
                importSkeletalMeshes_ ||
                importAnimations_
            ) &&
            !importSkeleton_)
        {
            ImGui::SeparatorText(
                "Existing Skeleton");

            if (ImGui::Button(
                    "Select Skeleton...",
                    ImVec2(160.0F, 30.0F)))
            {
                SelectExistingSkeleton();
            }

            if (!existingSkeletonFile_.empty())
            {
                const std::string skeleton =
                    existingSkeletonFile_.
                        u8string();

                ImGui::TextWrapped(
                    "%s",
                    skeleton.c_str());
            }
            else
            {
                ImGui::TextDisabled(
                    "A .skeleton file is required.");
            }
        }

        ImGui::SeparatorText("Import Settings");

        ImGui::SliderInt(
            "Bone Influences",
            &maximumBoneInfluences_,
            1,
            4);

        ImGui::DragFloat(
            "Animation Sample Rate",
            &animationSampleRate_,
            1.0F,
            1.0F,
            240.0F,
            "%.0f FPS");

        const bool hasSelection =
            importStaticMeshes_ ||
            importSkeletalMeshes_ ||
            importSkeleton_ ||
            importAnimations_;

        const bool needsExistingSkeleton =
            (
                importSkeletalMeshes_ ||
                importAnimations_
            ) &&
            !importSkeleton_;

        const bool canImport =
            analyzed_ &&
            hasSelection &&
            !destinationDirectory_.empty() &&
            (
                !needsExistingSkeleton ||
                !existingSkeletonFile_.empty()
            );

        ImGui::BeginDisabled(!canImport);

        if (ImGui::Button(
                "Import Selected Assets",
                ImVec2(210.0F, 36.0F)))
        {
            Import();
        }

        ImGui::EndDisabled();

        if (!status_.empty())
        {
            ImGui::SeparatorText("Status");

            const std::string status =
                ToUtf8(status_);

            if (operationSucceeded_)
            {
                ImGui::TextWrapped(
                    "%s",
                    status.c_str());
            }
            else
            {
                ImGui::TextWrapped(
                    "Error: %s",
                    status.c_str());
            }
        }

        if (!writtenFiles_.empty())
        {
            ImGui::SeparatorText("Written Files");

            for (
                const auto& file :
                writtenFiles_)
            {
                const std::string path =
                    file.u8string();

                ImGui::BulletText(
                    "%s",
                    path.c_str());
            }
        }

        if (!warnings_.empty())
        {
            ImGui::SeparatorText("Warnings");

            for (
                const std::wstring& warning :
                warnings_)
            {
                const std::string text =
                    ToUtf8(warning);

                ImGui::BulletText(
                    "%s",
                    text.c_str());
            }
        }

        ImGui::End();
    }
}