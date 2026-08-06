#include "Editor/Tools/Character/CharacterEditor.h"

#include <Graphics/CommandContext.h>
#include <Graphics/GraphicsResult.h>
#include <Graphics/RenderDevice.h>

#include <imgui.h>

#include <Windows.h>
#include <ShObjIdl.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace lts::editor
{
    namespace
    {
        constexpr const char* CharacterModuleNames[]
        {
            "Head",
            "Body",
            "Legs",
            "Shoes",
            "Hands"
        };

        constexpr const char* ArmorSlotNames[]
        {
            "Helmet",
            "Mask",
            "Armor"
        };

        constexpr const char* WeaponSlotNames[]
        {
            "Primary Weapon",
            "Secondary Weapon"
        };

        constexpr const wchar_t* SkinnedMeshFilter = L"Skinned Mesh (*.skm)";
        constexpr const wchar_t* SkeletonFilter = L"Skeleton (*.skeleton)";
        constexpr const wchar_t* CharacterFilter = L"Character Definition (*.character)";
        constexpr const wchar_t* CharacterFilePattern = L"*.character";
        constexpr const wchar_t* CharacterDefaultExtension = L"character";

        [[nodiscard]]
        std::filesystem::path SelectOpenFile(
            const wchar_t* title,
            const wchar_t* filterName,
            const wchar_t* extension) noexcept
        {
            IFileOpenDialog* dialog = nullptr;

            const HRESULT createResult =
                CoCreateInstance(
                    CLSID_FileOpenDialog,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&dialog));

            if (FAILED(createResult) ||
                dialog == nullptr)
            {
                return {};
            }

            dialog->SetTitle(title);

            COMDLG_FILTERSPEC filter
            {
                filterName,
                extension
            };

            dialog->SetFileTypes(
                1,
                &filter);

            const HRESULT showResult =
                dialog->Show(nullptr);

            if (FAILED(showResult))
            {
                dialog->Release();
                return {};
            }

            IShellItem* item = nullptr;

            const HRESULT resultResult =
                dialog->GetResult(&item);

            if (FAILED(resultResult) ||
                item == nullptr)
            {
                dialog->Release();
                return {};
            }

            PWSTR pathText = nullptr;

            const HRESULT pathResult =
                item->GetDisplayName(
                    SIGDN_FILESYSPATH,
                    &pathText);

            std::filesystem::path selectedPath;

            if (SUCCEEDED(pathResult) &&
                pathText != nullptr)
            {
                selectedPath = pathText;
                CoTaskMemFree(pathText);
            }

            item->Release();
            dialog->Release();

            return selectedPath;
        }

        [[nodiscard]]
        std::filesystem::path SelectSaveFile(
            const wchar_t* title,
            const wchar_t* filterName,
            const wchar_t* extension,
            const wchar_t* defaultExtension) noexcept
        {
            IFileSaveDialog* dialog = nullptr;

            const HRESULT createResult =
                CoCreateInstance(
                    CLSID_FileSaveDialog,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&dialog));

            if (FAILED(createResult) ||
                dialog == nullptr)
            {
                return {};
            }

            dialog->SetTitle(title);
            dialog->SetDefaultExtension(
                defaultExtension);

            COMDLG_FILTERSPEC filter
            {
                filterName,
                extension
            };

            dialog->SetFileTypes(
                1,
                &filter);

            const HRESULT showResult =
                dialog->Show(nullptr);

            if (FAILED(showResult))
            {
                dialog->Release();
                return {};
            }

            IShellItem* item = nullptr;

            const HRESULT resultResult =
                dialog->GetResult(&item);

            if (FAILED(resultResult) ||
                item == nullptr)
            {
                dialog->Release();
                return {};
            }

            PWSTR pathText = nullptr;

            const HRESULT pathResult =
                item->GetDisplayName(
                    SIGDN_FILESYSPATH,
                    &pathText);

            std::filesystem::path selectedPath;

            if (SUCCEEDED(pathResult) &&
                pathText != nullptr)
            {
                selectedPath = pathText;
                CoTaskMemFree(pathText);
            }

            item->Release();
            dialog->Release();

            return selectedPath;
        }

        [[nodiscard]]
        std::string PathToUtf8(
            const std::filesystem::path& path)
        {
            return path.generic_string();
        }

        [[nodiscard]]
        std::string DisplayPath(
            const std::filesystem::path& path)
        {
            if (path.empty())
            {
                return "Not selected";
            }

            return path.filename().string();
        }

        void DrawPathValue(
            const std::filesystem::path& path)
        {
            const std::string value =
                DisplayPath(path);

            ImGui::TextWrapped(
                "%s",
                value.c_str());

            if (!path.empty() &&
                ImGui::IsItemHovered())
            {
                const std::string completePath =
                    PathToUtf8(path);

                ImGui::SetTooltip(
                    "%s",
                    completePath.c_str());
            }
        }

        void WritePath(
            std::ostream& stream,
            const char* key,
            const std::filesystem::path& path)
        {
            stream
                << key
                << '='
                << std::quoted(
                    PathToUtf8(path))
                << '\n';
        }

        [[nodiscard]]
        bool ReadQuotedValue(
            const std::string& line,
            std::string& key,
            std::string& value)
        {
            const std::size_t separator =
                line.find('=');

            if (separator == std::string::npos)
            {
                return false;
            }

            key = line.substr(
                0,
                separator);

            std::istringstream valueStream(
                line.substr(separator + 1));

            valueStream >> std::quoted(value);

            return !key.empty();
        }

        void WriteTransform(
            std::ostream& stream,
            const char* prefix,
            const CharacterTransform& transform)
        {
            stream
                << prefix
                << ".position="
                << transform.position.x
                << ','
                << transform.position.y
                << ','
                << transform.position.z
                << '\n';

            stream
                << prefix
                << ".rotation="
                << transform.rotation.x
                << ','
                << transform.rotation.y
                << ','
                << transform.rotation.z
                << '\n';

            stream
                << prefix
                << ".scale="
                << transform.scale.x
                << ','
                << transform.scale.y
                << ','
                << transform.scale.z
                << '\n';
        }

        [[nodiscard]]
        bool ParseVector(
            const std::string& value,
            CharacterVector3& destination)
        {
            std::istringstream stream(value);
            char firstSeparator = '\0';
            char secondSeparator = '\0';

            stream
                >> destination.x
                >> firstSeparator
                >> destination.y
                >> secondSeparator
                >> destination.z;

            return !stream.fail() &&
                firstSeparator == ',' &&
                secondSeparator == ',';
        }

        enum class TransformParseResult : std::uint8_t
        {
            NotMatched = 0,
            Parsed,
            Invalid
        };

        [[nodiscard]]
        TransformParseResult ParseTransformField(
            const std::string& key,
            const std::string& prefix,
            const std::string& value,
            CharacterTransform& transform)
        {
            CharacterVector3* destination = nullptr;

            if (key == prefix + ".position")
            {
                destination =
                    &transform.position;
            }
            else if (key == prefix + ".rotation")
            {
                destination =
                    &transform.rotation;
            }
            else if (key == prefix + ".scale")
            {
                destination =
                    &transform.scale;
            }
            else
            {
                return TransformParseResult::NotMatched;
            }

            if (!ParseVector(
                    value,
                    *destination))
            {
                return TransformParseResult::Invalid;
            }

            return TransformParseResult::Parsed;
        }

        [[nodiscard]]
        std::size_t ToIndex(
            const CharacterModuleType type) noexcept
        {
            return static_cast<std::size_t>(type);
        }

        [[nodiscard]]
        std::size_t ToIndex(
            const CharacterArmorType type) noexcept
        {
            return static_cast<std::size_t>(type);
        }

        [[nodiscard]]
        std::size_t ToIndex(
            const CharacterWeaponSlot slot) noexcept
        {
            return static_cast<std::size_t>(slot);
        }
    }

    CharacterEditor::CharacterEditor() noexcept
    {
        character_.armor[
            ToIndex(CharacterArmorType::Helmet)]
            .attachmentBone = "head";

        character_.armor[
            ToIndex(CharacterArmorType::Mask)]
            .attachmentBone = "head";

        character_.armor[
            ToIndex(CharacterArmorType::Armor)]
            .attachmentBone = "spine_03";
    }

    void CharacterEditor::SetOpen(
        const bool open) noexcept
    {
        open_ = open;
    }

    bool CharacterEditor::IsOpen() const noexcept
    {
        return open_;
    }

    void CharacterEditor::Draw(
        engine::graphics::RenderDevice& device,
        engine::graphics::CommandContext& context) noexcept
    {
        if (!open_)
        {
            return;
        }

        ImGui::SetNextWindowSize(
            ImVec2(1280.0F, 760.0F),
            ImGuiCond_FirstUseEver);

        if (!ImGui::Begin(
                "Character Editor",
                &open_,
                ImGuiWindowFlags_MenuBar |
                    ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        DrawToolbar();
        DrawContent(
            device,
            context);

        DrawStatusBar();

        ImGui::End();
    }

    void CharacterEditor::Shutdown(
        engine::graphics::RenderDevice& device) noexcept
    {
        if (!previewInitialized_)
        {
            return;
        }

        previewRenderer_.Shutdown(
            device);

        previewInitialized_ = false;
        previewDirty_ = true;
    }

    void CharacterEditor::DrawToolbar() noexcept
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem(
                        "New Character",
                        "Ctrl+N"))
                {
                    NewCharacter();
                }

                if (ImGui::MenuItem(
                        "Open Character",
                        "Ctrl+O"))
                {
                    OpenCharacter();
                }

                ImGui::Separator();

                if (ImGui::MenuItem(
                        "Save",
                        "Ctrl+S"))
                {
                    SaveCharacter();
                }

                if (ImGui::MenuItem(
                        "Save As"))
                {
                    SaveCharacterAs();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                ImGui::MenuItem(
                    "Grid",
                    nullptr,
                    &showGrid_);

                ImGui::MenuItem(
                    "Skeleton",
                    nullptr,
                    &showSkeleton_);

                ImGui::MenuItem(
                    "Bones",
                    nullptr,
                    &showBones_);

                ImGui::MenuItem(
                    "Sockets",
                    nullptr,
                    &showSockets_);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Character"))
            {
                if (ImGui::MenuItem(
                        "Validate Character"))
                {
                    ValidateCharacter();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void CharacterEditor::DrawContent(
        engine::graphics::RenderDevice& device,
        engine::graphics::CommandContext& context) noexcept
    {
        const ImGuiTableFlags flags =
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_SizingStretchProp;

        if (!ImGui::BeginTable(
                "CharacterEditorContent",
                3,
                flags))
        {
            return;
        }

        ImGui::TableSetupColumn(
            "Hierarchy",
            ImGuiTableColumnFlags_WidthFixed,
            230.0F);

        ImGui::TableSetupColumn(
            "Preview",
            ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableSetupColumn(
            "Inspector",
            ImGuiTableColumnFlags_WidthFixed,
            350.0F);

        ImGui::TableNextColumn();
        DrawHierarchy();

        ImGui::TableNextColumn();
        DrawPreview(device, context);

        ImGui::TableNextColumn();
        DrawInspector();

        ImGui::EndTable();
    }

    void CharacterEditor::DrawHierarchy() noexcept
    {
        ImGui::BeginChild(
            "CharacterHierarchy",
            ImVec2(0.0F, 0.0F),
            false);

        ImGui::SeparatorText("Character");

        if (ImGui::TreeNodeEx(
                "Modules",
                ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (std::size_t index = 0;
                 index <
                 static_cast<std::size_t>(
                     CharacterModuleType::Count);
                 ++index)
            {
                const CharacterModuleType type =
                    static_cast<CharacterModuleType>(
                        index);

                const bool selected =
                    selectedSection_ ==
                        InspectorSection::Character &&
                    selectedModule_ == type;

                if (ImGui::Selectable(
                        CharacterModuleNames[index],
                        selected))
                {
                    selectedSection_ =
                        InspectorSection::Character;

                    selectedModule_ = type;
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
                "Armor",
                ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (std::size_t index = 0;
                 index <
                 static_cast<std::size_t>(
                     CharacterArmorType::Count);
                 ++index)
            {
                const CharacterArmorType type =
                    static_cast<CharacterArmorType>(
                        index);

                const bool selected =
                    selectedSection_ ==
                        InspectorSection::Armor &&
                    selectedArmor_ == type;

                if (ImGui::Selectable(
                        ArmorSlotNames[index],
                        selected))
                {
                    selectedSection_ =
                        InspectorSection::Armor;

                    selectedArmor_ = type;
                }
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
                "Weapons",
                ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (std::size_t index = 0;
                 index <
                 static_cast<std::size_t>(
                     CharacterWeaponSlot::Count);
                 ++index)
            {
                const CharacterWeaponSlot slot =
                    static_cast<CharacterWeaponSlot>(
                        index);

                const bool selected =
                    selectedSection_ ==
                        InspectorSection::Weapons &&
                    selectedWeapon_ == slot;

                if (ImGui::Selectable(
                        WeaponSlotNames[index],
                        selected))
                {
                    selectedSection_ =
                        InspectorSection::Weapons;

                    selectedWeapon_ = slot;
                }
            }

            ImGui::TreePop();
        }

        ImGui::Separator();

        if (ImGui::Selectable(
                "Skeleton",
                selectedSection_ ==
                    InspectorSection::Skeleton))
        {
            selectedSection_ =
                InspectorSection::Skeleton;
        }

        if (ImGui::Selectable(
                "Validation",
                selectedSection_ ==
                    InspectorSection::Validation))
        {
            selectedSection_ =
                InspectorSection::Validation;
        }

        ImGui::EndChild();
    }

    void CharacterEditor::DrawInspector() noexcept
    {
        ImGui::BeginChild(
            "CharacterInspector",
            ImVec2(0.0F, 0.0F),
            false);

        switch (selectedSection_)
        {
            case InspectorSection::Character:
            {
                DrawCharacterInspector();
                break;
            }

            case InspectorSection::Armor:
            {
                DrawArmorInspector();
                break;
            }

            case InspectorSection::Weapons:
            {
                DrawWeaponInspector();
                break;
            }

            case InspectorSection::Skeleton:
            {
                DrawSkeletonInspector();
                break;
            }

            case InspectorSection::Validation:
            {
                DrawValidationInspector();
                break;
            }
        }

        ImGui::EndChild();
    }

    void CharacterEditor::DrawPreview(
        engine::graphics::RenderDevice& device,
        engine::graphics::CommandContext& context) noexcept
    {
        ImGui::BeginChild(
            "CharacterPreview",
            ImVec2(0.0F, 0.0F),
            true,
            ImGuiWindowFlags_NoScrollbar);

        ImGui::TextUnformatted(
            "Character Preview");

        ImGui::SameLine();

        if (ImGui::SmallButton("Front"))
        {
            previewYaw_ = 180.0F;
            previewPitch_ = 0.0F;
        }

        ImGui::SameLine();

        if (ImGui::SmallButton("Back"))
        {
            previewYaw_ = 0.0F;
            previewPitch_ = 0.0F;
        }

        ImGui::SameLine();

        if (ImGui::SmallButton("Left"))
        {
            previewYaw_ = 90.0F;
            previewPitch_ = 0.0F;
        }

        ImGui::SameLine();

        if (ImGui::SmallButton("Right"))
        {
            previewYaw_ = -90.0F;
            previewPitch_ = 0.0F;
        }

        ImGui::Separator();

        if (!previewInitialized_)
        {
            previewInitialized_ =
                previewRenderer_.Initialize(
                    device);

            if (!previewInitialized_)
            {
                SetStatus(
                    "Failed to initialize Character Preview.",
                    true);
            }
        }

        const ImVec2 available =
            ImGui::GetContentRegionAvail();

        const float footerHeight =
            ImGui::GetTextLineHeightWithSpacing() +
            12.0F;

        const ImVec2 previewSize
        {
            std::max(
                available.x,
                64.0F),

            std::max(
                available.y - footerHeight,
                128.0F)
        };

        bool previewReady = false;

        if (previewInitialized_)
        {
            previewRenderer_.Update(
                ImGui::GetIO().DeltaTime);

            if (previewDirty_)
            {
                previewDirty_ = false;

                if (character_.bodySkeletonFile.empty())
                {
                    SetStatus(
                        "Select body.skeleton to build preview.");
                }
                else if (!previewRenderer_.LoadCharacter(
                             device,
                             character_))
                {
                    SetStatus(
                        previewRenderer_.GetStatus(),
                        true);
                }
                else
                {
                    SetStatus(
                        previewRenderer_.GetStatus());
                }
            }

            const auto resizeResult =
                previewRenderer_.Resize(
                    device,
                    static_cast<std::uint32_t>(
                        previewSize.x),
                    static_cast<std::uint32_t>(
                        previewSize.y));

            if (engine::graphics::Succeeded(
                    resizeResult))
            {
                const auto renderResult =
                    previewRenderer_.Render(
                        context,
                        previewYaw_,
                        previewPitch_,
                        previewDistance_);

                previewReady =
                    engine::graphics::Succeeded(
                        renderResult);
            }
        }

        void* const textureId =
            previewReady
                ? previewRenderer_.GetImGuiTextureId(
                    device)
                : nullptr;

        if (textureId != nullptr)
        {
            ImGui::Image(
                reinterpret_cast<ImTextureID>(
                    textureId),
                previewSize,
                ImVec2(0.0F, 0.0F),
                ImVec2(1.0F, 1.0F));

            const ImVec2 imageMinimum = ImGui::GetItemRectMin();

            const ImVec2 imageMaximum =
                ImGui::GetItemRectMax();

            if ((showSkeleton_ || showBones_) &&
                previewRenderer_.BuildDebugBones(
                    previewDebugBones_))
            {
                ImDrawList* drawList =
                    ImGui::GetWindowDrawList();

                const float imageWidth =
                    imageMaximum.x -
                    imageMinimum.x;

                const float imageHeight =
                    imageMaximum.y -
                    imageMinimum.y;

                for (const CharacterPreviewDebugBone& bone :
                     previewDebugBones_)
                {
                    const ImVec2 bonePosition
                    {
                        imageMinimum.x +
                            bone.x *
                            imageWidth,

                        imageMinimum.y +
                            bone.y *
                            imageHeight
                    };

                    if (showSkeleton_ &&
                        bone.hasParent)
                    {
                        const ImVec2 parentPosition
                        {
                            imageMinimum.x +
                                bone.parentX *
                                imageWidth,

                            imageMinimum.y +
                                bone.parentY *
                                imageHeight
                        };

                        drawList->AddLine(
                            parentPosition,
                            bonePosition,
                            IM_COL32(
                                235,
                                155,
                                70,
                                230),
                            1.5F);
                    }

                    if (showBones_)
                    {
                        drawList->AddCircleFilled(
                            bonePosition,
                            2.5F,
                            IM_COL32(
                                255,
                                205,
                                110,
                                255));
                    }
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGuiIO& io =
                    ImGui::GetIO();

                if (ImGui::IsMouseDragging(
                        ImGuiMouseButton_Left))
                {
                    previewYaw_ +=
                        io.MouseDelta.x *
                        0.4F;

                    previewPitch_ +=
                        io.MouseDelta.y *
                        0.4F;

                    previewPitch_ =
                        std::clamp(
                            previewPitch_,
                            -80.0F,
                            80.0F);
                }

                if (io.MouseWheel != 0.0F)
                {
                    previewDistance_ -=
                        io.MouseWheel *
                        0.2F;

                    previewDistance_ =
                        std::clamp(
                            previewDistance_,
                            0.5F,
                            20.0F);
                }
            }
        }
        else
        {
            const ImVec2 previewStart =
                ImGui::GetCursorScreenPos();

            const ImVec2 previewEnd
            {
                previewStart.x +
                    previewSize.x,

                previewStart.y +
                    previewSize.y
            };

            ImDrawList* const drawList =
                ImGui::GetWindowDrawList();

            drawList->AddRectFilled(
                previewStart,
                previewEnd,
                IM_COL32(
                    20,
                    22,
                    25,
                    255));

            drawList->AddRect(
                previewStart,
                previewEnd,
                IM_COL32(
                    70,
                    75,
                    82,
                    255));

            ImGui::InvisibleButton(
                "CharacterPreviewUnavailable",
                previewSize);

            const std::string& previewStatus =
                previewRenderer_.GetStatus();

            const char* const message =
                previewStatus.empty()
                    ? "Character Preview is not ready."
                    : previewStatus.c_str();

            drawList->AddText(
                ImVec2(
                    previewStart.x + 12.0F,
                    previewStart.y + 12.0F),
                IM_COL32(
                    190,
                    195,
                    205,
                    255),
                message);
        }

        ImGui::Text(
            "Yaw %.1f | Pitch %.1f | Distance %.2f",
            previewYaw_,
            previewPitch_,
            previewDistance_);

        ImGui::EndChild();
    }

    void CharacterEditor::DrawStatusBar() noexcept
    {
        ImGui::Separator();

        const ImVec4 color =
            statusIsError_
            ? ImVec4(
                0.95F,
                0.35F,
                0.30F,
                1.0F)
            : ImVec4(
                0.60F,
                0.75F,
                0.60F,
                1.0F);

        ImGui::TextColored(
            color,
            "%s",
            status_.c_str());

        if (IsDirty())
        {
            ImGui::SameLine();
            ImGui::TextDisabled("* Modified");
        }
    }

    void CharacterEditor::
        DrawCharacterInspector() noexcept
    {
        const std::size_t index =
            ToIndex(selectedModule_);

        CharacterMeshSlot& slot =
            character_.modules[index];

        ImGui::SeparatorText(
            CharacterModuleNames[index]);

        DrawMeshSlot(
            CharacterModuleNames[index],
            slot);

        ImGui::Spacing();
        ImGui::SeparatorText("Shared skeleton");

        DrawPathValue(
            character_.bodySkeletonFile);

        if (ImGui::Button(
                "Select body.skeleton",
                ImVec2(-1.0F, 0.0F)))
        {
            SelectBodySkeleton();
        }

        ImGui::TextWrapped(
            "All modular parts use the Body skeleton "
            "as the common character skeleton.");
    }

    void CharacterEditor::
        DrawArmorInspector() noexcept
    {
        const std::size_t index =
            ToIndex(selectedArmor_);

        CharacterArmorSlot& slot =
            character_.armor[index];

        const char* defaultBone = "spine_03";
        if (selectedArmor_ == CharacterArmorType::Helmet || selectedArmor_ == CharacterArmorType::Mask)
        {
            defaultBone = "head";
        }

        DrawArmorSlot(
            ArmorSlotNames[index],
            slot,
            defaultBone);
    }

    void CharacterEditor::
        DrawWeaponInspector() noexcept
    {
        const std::size_t index =
            ToIndex(selectedWeapon_);

        CharacterWeapon& weapon =
            character_.weapons[index];

        DrawWeaponSlot(
            WeaponSlotNames[index],
            weapon);
    }

    void CharacterEditor::
        DrawSkeletonInspector() noexcept
    {
        ImGui::SeparatorText("Common skeleton");

        DrawPathValue(
            character_.bodySkeletonFile);

        if (ImGui::Button(
                "Select body.skeleton",
                ImVec2(-1.0F, 0.0F)))
        {
            SelectBodySkeleton();
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Debug drawing");

        ImGui::Checkbox(
            "Show Skeleton",
            &showSkeleton_);

        ImGui::Checkbox(
            "Show Bones",
            &showBones_);

        ImGui::Checkbox(
            "Show Sockets",
            &showSockets_);

        ImGui::Checkbox(
            "Show Grid",
            &showGrid_);

        ImGui::Spacing();

        ImGui::TextWrapped(
            "The Body skeleton is authoritative. "
            "Head, Legs, Shoes, Armor and weapons "
            "must resolve their skinning or attachment "
            "bones against this skeleton.");
    }

    void CharacterEditor::
        DrawValidationInspector() noexcept
    {
        ImGui::SeparatorText("Validation");

        if (ImGui::Button(
                "Validate Character",
                ImVec2(-1.0F, 32.0F)))
        {
            ValidateCharacter();
        }

        ImGui::Spacing();

        if (validationErrors_.empty())
        {
            ImGui::TextColored(
                ImVec4(
                    0.50F,
                    0.85F,
                    0.50F,
                    1.0F),
                "No validation errors.");
        }
        else
        {
            ImGui::TextColored(
                ImVec4(
                    0.95F,
                    0.35F,
                    0.30F,
                    1.0F),
                "Errors");

            for (const std::string& error :
                 validationErrors_)
            {
                ImGui::BulletText(
                    "%s",
                    error.c_str());
            }
        }

        if (!validationWarnings_.empty())
        {
            ImGui::Spacing();

            ImGui::TextColored(
                ImVec4(
                    0.95F,
                    0.75F,
                    0.30F,
                    1.0F),
                "Warnings");

            for (const std::string& warning :
                 validationWarnings_)
            {
                ImGui::BulletText(
                    "%s",
                    warning.c_str());
            }
        }
    }

    void CharacterEditor::DrawMeshSlot(
        const char* label,
        CharacterMeshSlot& slot) noexcept
    {
        bool changed = false;

        changed |= ImGui::Checkbox(
            "Visible",
            &slot.visible);

        ImGui::SeparatorText("Skinned mesh");

        DrawPathValue(slot.meshFile);

        std::string buttonText =
            "Select ";

        buttonText += label;
        buttonText += " .skm";

        if (ImGui::Button(
                buttonText.c_str(),
                ImVec2(-1.0F, 0.0F)))
        {
            SelectMeshFile(slot.meshFile);
            changed = true;
        }

        if (!slot.meshFile.empty() &&
            ImGui::Button(
                "Clear mesh",
                ImVec2(-1.0F, 0.0F)))
        {
            slot.meshFile.clear();
            changed = true;
        }

        if (changed)
        {
            MarkDirty();
        }
    }

    void CharacterEditor::DrawArmorSlot(
        const char* label,
        CharacterArmorSlot& slot,
        const char* defaultBone) noexcept
    {
        bool changed = false;

        ImGui::SeparatorText(label);

        changed |= ImGui::Checkbox(
            "Visible",
            &slot.visible);

        ImGui::SeparatorText("Mesh");

        DrawPathValue(slot.meshFile);

        if (ImGui::Button(
                "Select armor .skm",
                ImVec2(-1.0F, 0.0F)))
        {
            SelectMeshFile(slot.meshFile);
            changed = true;
        }

        ImGui::SeparatorText("Attachment");

        std::array<char, 128> boneBuffer {};

        const std::size_t copyLength =
            std::min(
                slot.attachmentBone.size(),
                boneBuffer.size() - 1);

        std::copy_n(
            slot.attachmentBone.data(),
            copyLength,
            boneBuffer.data());

        if (ImGui::InputText(
                "Bone",
                boneBuffer.data(),
                boneBuffer.size()))
        {
            slot.attachmentBone =
                boneBuffer.data();

            changed = true;
        }

        if (ImGui::Button(
                "Use default bone",
                ImVec2(-1.0F, 0.0F)))
        {
            slot.attachmentBone =
                defaultBone;

            changed = true;
        }

        DrawTransform(
            "ArmorTransform",
            slot.localTransform);

        if (changed)
        {
            MarkDirty();
        }
    }

    void CharacterEditor::DrawWeaponSlot(
        const char* label,
        CharacterWeapon& weapon) noexcept
    {
        bool changed = false;

        ImGui::SeparatorText(label);

        changed |= ImGui::Checkbox(
            "Visible",
            &weapon.visible);

        ImGui::SeparatorText("Weapon mesh");

        DrawPathValue(weapon.meshFile);

        if (ImGui::Button(
                "Select weapon .skm",
                ImVec2(-1.0F, 0.0F)))
        {
            SelectMeshFile(weapon.meshFile);
            changed = true;
        }

        ImGui::SeparatorText("IK");

        changed |= ImGui::Checkbox(
            "Enable IK",
            &weapon.ik.enabled);

        auto drawBoneName =
            [&changed](
                const char* labelText,
                std::string& value)
            {
                std::array<char, 128> buffer {};

                const std::size_t copyLength =
                    std::min(
                        value.size(),
                        buffer.size() - 1);

                std::copy_n(
                    value.data(),
                    copyLength,
                    buffer.data());

                if (ImGui::InputText(
                        labelText,
                        buffer.data(),
                        buffer.size()))
                {
                    value = buffer.data();
                    changed = true;
                }
            };

        drawBoneName(
            "Attachment Bone",
            weapon.ik.attachmentBone);

        drawBoneName(
            "Right Hand Bone",
            weapon.ik.rightHandBone);

        drawBoneName(
            "Left Hand Bone",
            weapon.ik.leftHandBone);

        if (ImGui::CollapsingHeader(
                "Weapon transform",
                ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawTransform(
                "Weapon",
                weapon.ik.weaponTransform);
        }

        if (ImGui::CollapsingHeader(
                "Right hand IK",
                ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawTransform(
                "RightHand",
                weapon.ik.rightHandTransform);
        }

        if (ImGui::CollapsingHeader(
                "Left hand IK",
                ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawTransform(
                "LeftHand",
                weapon.ik.leftHandTransform);
        }

        if (changed)
        {
            MarkDirty();
        }
    }

    void CharacterEditor::DrawTransform(
        const char* identifier,
        CharacterTransform& transform) noexcept
    {
        ImGui::PushID(identifier);

        bool changed = false;

        changed |= ImGui::DragFloat3(
            "Position",
            &transform.position.x,
            0.001F);

        changed |= ImGui::DragFloat3(
            "Rotation",
            &transform.rotation.x,
            0.1F);

        changed |= ImGui::DragFloat3(
            "Scale",
            &transform.scale.x,
            0.01F,
            0.001F,
            100.0F);

        if (ImGui::Button(
                "Reset Transform",
                ImVec2(-1.0F, 0.0F)))
        {
            transform = CharacterTransform {};
            changed = true;
        }

        if (changed)
        {
            MarkDirty();
        }

        ImGui::PopID();
    }

    void CharacterEditor::NewCharacter() noexcept
    {
        character_ = CharacterDefinition {};

        character_.armor[
            ToIndex(CharacterArmorType::Helmet)]
            .attachmentBone = "head";

        character_.armor[
            ToIndex(CharacterArmorType::Mask)]
            .attachmentBone = "head";

        character_.armor[
            ToIndex(CharacterArmorType::Armor)]
            .attachmentBone = "spine_03";

        dirty_ = false;

        validationErrors_.clear();
        validationWarnings_.clear();

        SetStatus(
            "Created new character definition.");

        previewDirty_ = true;
    }

    void CharacterEditor::OpenCharacter() noexcept
    {
        const std::filesystem::path selected =
            SelectOpenFile(
                L"Open Character Definition",
                CharacterFilter,
                CharacterFilePattern);

        if (selected.empty())
        {
            return;
        }

        if (!DeserializeCharacter(selected))
        {
            SetStatus(
                "Failed to open character definition.",
                true);

            return;
        }

        character_.sourceFile = selected;
        dirty_ = false;

        SetStatus(
            "Character definition opened.");
    }

    void CharacterEditor::SaveCharacter() noexcept
    {
        if (character_.sourceFile.empty())
        {
            SaveCharacterAs();
            return;
        }

        if (!SerializeCharacter(
                character_.sourceFile))
        {
            SetStatus(
                "Failed to save character definition.",
                true);

            return;
        }

        dirty_ = false;

        SetStatus(
            "Character definition saved.");
    }

    void CharacterEditor::SaveCharacterAs() noexcept
    {
        const std::filesystem::path selected =
            SelectSaveFile(
                L"Save Character Definition",
                CharacterFilter,
                CharacterFilePattern,
                CharacterDefaultExtension);

        if (selected.empty())
        {
            return;
        }

        character_.sourceFile = selected;

        SaveCharacter();
    }

    void CharacterEditor::
        SelectBodySkeleton() noexcept
    {
        const std::filesystem::path selected =
            SelectOpenFile(
                L"Select Body Skeleton",
                SkeletonFilter,
                L"*.skeleton");

        if (selected.empty())
        {
            return;
        }

        character_.bodySkeletonFile =
            selected;

        MarkDirty();

        SetStatus(
            "Body skeleton selected.");
    }

    void CharacterEditor::SelectMeshFile(
        std::filesystem::path& destination) noexcept
    {
        const std::filesystem::path selected =
            SelectOpenFile(
                L"Select Skinned Mesh",
                SkinnedMeshFilter,
                L"*.skm");

        if (selected.empty())
        {
            return;
        }

        destination = selected;

        MarkDirty();

        SetStatus(
            "Skinned mesh selected.");
    }

    void CharacterEditor::
        ValidateCharacter() noexcept
    {
        validationErrors_.clear();
        validationWarnings_.clear();

        if (character_.bodySkeletonFile.empty())
        {
            validationErrors_.emplace_back(
                "Body skeleton is not selected.");
        }
        else if (!std::filesystem::exists(
                     character_.bodySkeletonFile))
        {
            validationErrors_.emplace_back(
                "Body skeleton file does not exist.");
        }

        const CharacterMeshSlot& body =
            character_.modules[
                ToIndex(CharacterModuleType::Body)];

        if (body.meshFile.empty())
        {
            validationErrors_.emplace_back(
                "Body mesh is not selected.");
        }

        for (std::size_t index = 0;
             index < character_.modules.size();
             ++index)
        {
            const CharacterMeshSlot& slot =
                character_.modules[index];

            if (!slot.meshFile.empty() &&
                !std::filesystem::exists(
                    slot.meshFile))
            {
                std::string error =
                    CharacterModuleNames[index];

                error +=
                    " mesh file does not exist.";

                validationErrors_.push_back(
                    std::move(error));
            }
        }

        for (std::size_t index = 0;
             index < character_.armor.size();
             ++index)
        {
            const CharacterArmorSlot& slot =
                character_.armor[index];

            if (!slot.meshFile.empty() &&
                slot.attachmentBone.empty())
            {
                std::string error =
                    ArmorSlotNames[index];

                error +=
                    " attachment bone is empty.";

                validationErrors_.push_back(
                    std::move(error));
            }
        }

        for (std::size_t index = 0;
             index < character_.weapons.size();
             ++index)
        {
            const CharacterWeapon& weapon =
                character_.weapons[index];

            if (weapon.meshFile.empty())
            {
                continue;
            }

            if (weapon.ik.attachmentBone.empty())
            {
                std::string error =
                    WeaponSlotNames[index];

                error +=
                    " attachment bone is empty.";

                validationErrors_.push_back(
                    std::move(error));
            }

            if (weapon.ik.enabled &&
                weapon.ik.rightHandBone.empty())
            {
                std::string error =
                    WeaponSlotNames[index];

                error +=
                    " right hand bone is empty.";

                validationErrors_.push_back(
                    std::move(error));
            }

            if (weapon.ik.enabled &&
                weapon.ik.leftHandBone.empty())
            {
                std::string error =
                    WeaponSlotNames[index];

                error +=
                    " left hand bone is empty.";

                validationErrors_.push_back(
                    std::move(error));
            }
        }

        const CharacterMeshSlot& head =
            character_.modules[
                ToIndex(CharacterModuleType::Head)];

        const CharacterMeshSlot& legs =
            character_.modules[
                ToIndex(CharacterModuleType::Legs)];

        const CharacterMeshSlot& shoes =
            character_.modules[
                ToIndex(CharacterModuleType::Shoes)];

        if (head.meshFile.empty())
        {
            validationWarnings_.emplace_back(
                "Head mesh is not selected.");
        }

        if (legs.meshFile.empty())
        {
            validationWarnings_.emplace_back(
                "Legs mesh is not selected.");
        }

        if (shoes.meshFile.empty())
        {
            validationWarnings_.emplace_back(
                "Shoes mesh is not selected.");
        }

        selectedSection_ =
            InspectorSection::Validation;

        if (validationErrors_.empty())
        {
            SetStatus(
                "Character validation completed.");
        }
        else
        {
            SetStatus(
                "Character validation failed.",
                true);
        }
    }

    bool CharacterEditor::SerializeCharacter(
        const std::filesystem::path& file) noexcept
    {
        std::ofstream stream(
            file,
            std::ios::binary |
                std::ios::trunc);

        if (!stream)
        {
            return false;
        }

        stream << "character_version=1\n";

        WritePath(
            stream,
            "body_skeleton",
            character_.bodySkeletonFile);

        for (std::size_t index = 0;
             index < character_.modules.size();
             ++index)
        {
            const CharacterMeshSlot& slot =
                character_.modules[index];

            const std::string prefix =
                "module." +
                std::to_string(index);

            WritePath(
                stream,
                (prefix + ".mesh").c_str(),
                slot.meshFile);

            stream
                << prefix
                << ".visible="
                << (slot.visible ? 1 : 0)
                << '\n';
        }

        for (std::size_t index = 0;
             index < character_.armor.size();
             ++index)
        {
            const CharacterArmorSlot& slot =
                character_.armor[index];

            const std::string prefix =
                "armor." +
                std::to_string(index);

            WritePath(
                stream,
                (prefix + ".mesh").c_str(),
                slot.meshFile);

            stream
                << prefix
                << ".visible="
                << (slot.visible ? 1 : 0)
                << '\n';

            stream
                << prefix
                << ".bone="
                << std::quoted(
                    slot.attachmentBone)
                << '\n';

            WriteTransform(
                stream,
                prefix.c_str(),
                slot.localTransform);
        }

        for (std::size_t index = 0;
             index < character_.weapons.size();
             ++index)
        {
            const CharacterWeapon& weapon =
                character_.weapons[index];

            const std::string prefix =
                "weapon." +
                std::to_string(index);

            WritePath(
                stream,
                (prefix + ".mesh").c_str(),
                weapon.meshFile);

            stream
                << prefix
                << ".visible="
                << (weapon.visible ? 1 : 0)
                << '\n';

            stream
                << prefix
                << ".ik_enabled="
                << (weapon.ik.enabled ? 1 : 0)
                << '\n';

            stream
                << prefix
                << ".attachment_bone="
                << std::quoted(
                    weapon.ik.attachmentBone)
                << '\n';

            stream
                << prefix
                << ".right_hand_bone="
                << std::quoted(
                    weapon.ik.rightHandBone)
                << '\n';

            stream
                << prefix
                << ".left_hand_bone="
                << std::quoted(
                    weapon.ik.leftHandBone)
                << '\n';

            const std::string weaponTransformPrefix =
                prefix +
                ".weapon_transform";

            const std::string rightHandPrefix =
                prefix +
                ".right_hand";

            const std::string leftHandPrefix =
                prefix +
                ".left_hand";

            WriteTransform(
                stream,
                weaponTransformPrefix.c_str(),
                weapon.ik.weaponTransform);

            WriteTransform(
                stream,
                rightHandPrefix.c_str(),
                weapon.ik.rightHandTransform);

            WriteTransform(
                stream,
                leftHandPrefix.c_str(),
                weapon.ik.leftHandTransform);
        }

        return stream.good();
    }

    bool CharacterEditor::DeserializeCharacter(
        const std::filesystem::path& file) noexcept
    {
        std::ifstream stream(
            file,
            std::ios::binary);

        if (!stream)
        {
            return false;
        }

        CharacterDefinition loadedCharacter;

        std::string line;

        while (std::getline(stream, line))
        {
            std::string key;
            std::string value;

            if (!ReadQuotedValue(
                    line,
                    key,
                    value))
            {
                continue;
            }

            if (key == "body_skeleton")
            {
                loadedCharacter.bodySkeletonFile =
                    value;

                continue;
            }

            for (std::size_t index = 0;
                 index <
                 loadedCharacter.modules.size();
                 ++index)
            {
                const std::string prefix =
                    "module." +
                    std::to_string(index);

                if (key == prefix + ".mesh")
                {
                    loadedCharacter
                        .modules[index]
                        .meshFile = value;
                }
                else if (
                    key ==
                    prefix + ".visible")
                {
                    loadedCharacter
                        .modules[index]
                        .visible =
                        value == "1";
                }
            }

            for (std::size_t index = 0;
                 index <
                 loadedCharacter.armor.size();
                 ++index)
            {
                const std::string prefix =
                    "armor." +
                    std::to_string(index);

                CharacterArmorSlot& armor =
                    loadedCharacter.armor[index];

                if (key == prefix + ".mesh")
                {
                    armor.meshFile = value;
                }
                else if (
                    key ==
                    prefix + ".visible")
                {
                    armor.visible =
                        value == "1";
                }
                else if (
                    key ==
                    prefix + ".bone")
                {
                    armor.attachmentBone =
                        value;
                }
                else
                {
                    const TransformParseResult result =
                        ParseTransformField(
                            key,
                            prefix,
                            value,
                            armor.localTransform);

                    if (result ==
                        TransformParseResult::Invalid)
                    {
                        return false;
                    }
                }
            }

            for (std::size_t index = 0;
                 index <
                 loadedCharacter.weapons.size();
                 ++index)
            {
                const std::string prefix =
                    "weapon." +
                    std::to_string(index);

                CharacterWeapon& weapon =
                    loadedCharacter.weapons[index];

                if (key == prefix + ".mesh")
                {
                    weapon.meshFile =
                        value;
                }
                else if (
                    key ==
                    prefix + ".visible")
                {
                    weapon.visible =
                        value == "1";
                }
                else if (
                    key ==
                    prefix + ".ik_enabled")
                {
                    weapon.ik.enabled =
                        value == "1";
                }
                else if (
                    key ==
                    prefix + ".attachment_bone")
                {
                    weapon.ik.attachmentBone =
                        value;
                }
                else if (
                    key ==
                    prefix + ".right_hand_bone")
                {
                    weapon.ik.rightHandBone =
                        value;
                }
                else if (
                    key ==
                    prefix + ".left_hand_bone")
                {
                    weapon.ik.leftHandBone =
                        value;
                }
                else
                {
                    const std::string weaponTransformPrefix =
                        prefix +
                        ".weapon_transform";

                    const TransformParseResult weaponResult =
                        ParseTransformField(
                            key,
                            weaponTransformPrefix,
                            value,
                            weapon.ik.weaponTransform);

                    if (weaponResult ==
                        TransformParseResult::Invalid)
                    {
                        return false;
                    }

                    if (weaponResult ==
                        TransformParseResult::Parsed)
                    {
                        continue;
                    }

                    const std::string rightHandPrefix =
                        prefix +
                        ".right_hand";

                    const TransformParseResult rightHandResult =
                        ParseTransformField(
                            key,
                            rightHandPrefix,
                            value,
                            weapon.ik.rightHandTransform);

                    if (rightHandResult ==
                        TransformParseResult::Invalid)
                    {
                        return false;
                    }

                    if (rightHandResult ==
                        TransformParseResult::Parsed)
                    {
                        continue;
                    }

                    const std::string leftHandPrefix =
                        prefix +
                        ".left_hand";

                    const TransformParseResult leftHandResult =
                        ParseTransformField(
                            key,
                            leftHandPrefix,
                            value,
                            weapon.ik.leftHandTransform);

                    if (leftHandResult ==
                        TransformParseResult::Invalid)
                    {
                        return false;
                    }
                }
            }
        }

        if (!stream.eof() &&
            stream.fail())
        {
            return false;
        }

        character_ =
            std::move(loadedCharacter);

        character_.sourceFile =
            file;

        previewDirty_ = true;

        return true;
    }

    bool CharacterEditor::IsDirty() const noexcept
    {
        return dirty_;
    }

    void CharacterEditor::MarkDirty() noexcept
    {
        dirty_ = true;
        previewDirty_ = true;
    }

    void CharacterEditor::SetStatus(
        std::string message,
        const bool error) noexcept
    {
        status_ = std::move(message);
        statusIsError_ = error;
    }
}