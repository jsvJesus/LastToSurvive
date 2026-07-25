#include "Editor/LevelEditor/Documents/LevelDocument.h"
#include "Editor/LevelEditor/Documents/LevelSerializer.h"

#include <Core/Log.h>

#include <Windows.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace lts::editor
{
    namespace
    {
        using Microsoft::WRL::ComPtr;

        constexpr wchar_t
            LevelDocumentPropertyName[] =
                L"LTS.Editor.LevelDocument.Instance";

        constexpr int IdMenuNewLevel = 2001;
        constexpr int IdMenuOpen = 2002;
        constexpr int IdMenuSave = 2003;
        constexpr int IdMenuSaveAs = 2004;
        constexpr int IdMenuExit = 2005;

        [[nodiscard]]
        HWND ToWindow(
            const engine::platform::
                NativeWindowHandle handle) noexcept
        {
            return reinterpret_cast<HWND>(
                handle.Value());
        }

        [[nodiscard]]
        HWND ToWindow(
            const void* const value) noexcept
        {
            return static_cast<HWND>(
                const_cast<void*>(value));
        }

        [[nodiscard]]
        WNDPROC ToWindowProcedure(
            const void* const value) noexcept
        {
            return reinterpret_cast<WNDPROC>(
                const_cast<void*>(value));
        }

        [[nodiscard]]
        bool IsKeyDown(
            const int virtualKey) noexcept
        {
            return
                virtualKey >= 0 &&
                virtualKey < 256 &&
                (GetAsyncKeyState(
                    virtualKey) & 0x8000) != 0;
        }

        [[nodiscard]]
        std::wstring GenerateGuid()
        {
            GUID guid{};

            if (FAILED(
                    CoCreateGuid(&guid)))
            {
                return
                    L"{00000000-0000-0000-0000-000000000000}";
            }

            wchar_t buffer[64]{};

            if (
                StringFromGUID2(
                    guid,
                    buffer,
                    static_cast<int>(
                        std::size(buffer))) <= 0)
            {
                return
                    L"{00000000-0000-0000-0000-000000000000}";
            }

            return buffer;
        }

        [[nodiscard]]
        std::filesystem::path
            GetWorldsFolder()
        {
            std::error_code error;

            std::filesystem::path folder =
                std::filesystem::current_path(
                    error);

            if (error)
            {
                folder = L".";
            }

            folder /= L"Projects";
            folder /= L"Worlds";

            std::filesystem::create_directories(
                folder,
                error);

            return folder;
        }
    }

    LevelDocument::
        ~LevelDocument() noexcept
    {
        Shutdown();
    }

    bool LevelDocument::Initialize(
        const engine::platform::
            NativeWindowHandle mainWindow,
        const SceneDocument&
            sceneDocument) noexcept
    {
        if (initialized_)
        {
            return true;
        }

        const HWND window =
            ToWindow(mainWindow);

        if (
            window == nullptr ||
            !IsWindow(window))
        {
            return false;
        }

        mainWindow_ = window;

        const HRESULT comResult =
            CoInitializeEx(
                nullptr,
                COINIT_APARTMENTTHREADED |
                    COINIT_DISABLE_OLE1DDE);

        comNeedsUninitialize_ =
            SUCCEEDED(comResult);

        if (
            FAILED(comResult) &&
            comResult != RPC_E_CHANGED_MODE)
        {
            mainWindow_ = nullptr;
            return false;
        }

        ResetUntitledMetadata();

        if (!InstallWindowSubclass())
        {
            if (comNeedsUninitialize_)
            {
                CoUninitialize();
                comNeedsUninitialize_ = false;
            }

            mainWindow_ = nullptr;
            return false;
        }

        initialized_ = true;

        SynchronizeWindowTitle(
            sceneDocument);

        return true;
    }

    void LevelDocument::Shutdown() noexcept
    {
        RestoreWindowSubclass();

        pendingCommand_ =
            PendingCommand::None;

        previousKeyDown_.fill(false);

        currentPath_.clear();
        levelName_ = L"Untitled";
        levelGuid_.clear();
        lastWindowTitle_.clear();

        allowClose_ = false;
        initialized_ = false;
        mainWindow_ = nullptr;

        if (comNeedsUninitialize_)
        {
            CoUninitialize();
            comNeedsUninitialize_ = false;
        }
    }

    const std::filesystem::path&
    LevelDocument::GetCurrentPath() const noexcept
    {
        return currentPath_;
    }

    bool LevelDocument::SetWindowInterceptionEnabled(
        const bool enabled) noexcept
    {
        if (!initialized_)
        {
            return false;
        }

        if (enabled)
        {
            return subclassInstalled_ || InstallWindowSubclass();
        }

        RestoreWindowSubclass();
        return true;
    }

    void LevelDocument::RequestNewLevel() noexcept
    {
        QueueCommand(PendingCommand::NewLevel);
    }

    void LevelDocument::RequestOpenLevel() noexcept
    {
        QueueCommand(PendingCommand::OpenLevel);
    }

    void LevelDocument::RequestSaveLevel() noexcept
    {
        QueueCommand(PendingCommand::Save);
    }

    void LevelDocument::RequestSaveLevelAs() noexcept
    {
        QueueCommand(PendingCommand::SaveAs);
    }

    void LevelDocument::RequestCloseLevel() noexcept
    {
        QueueCommand(PendingCommand::Exit);
    }

    EditorLevelUpdateResult
        LevelDocument::Update(
            SceneDocument& sceneDocument,
            CommandHistory&
                commandHistory) noexcept
    {
        EditorLevelUpdateResult result;

        if (!initialized_)
        {
            return result;
        }

        PollShortcuts();

        const PendingCommand command =
            pendingCommand_;

        pendingCommand_ =
            PendingCommand::None;

        try
        {
            switch (command)
            {
                case PendingCommand::NewLevel:
                    if (
                        ConfirmSaveChanges(
                            sceneDocument) ==
                            OperationResult::Success &&
                        CreateNewLevel(
                            sceneDocument,
                            commandHistory) ==
                            OperationResult::Success)
                    {
                        result.sceneReplaced = true;
                    }
                    break;

                case PendingCommand::OpenLevel:
                    if (
                        ConfirmSaveChanges(
                            sceneDocument) ==
                            OperationResult::Success &&
                        OpenLevel(
                            sceneDocument,
                            commandHistory) ==
                            OperationResult::Success)
                    {
                        result.sceneReplaced = true;
                    }
                    break;

                case PendingCommand::Save:
                    result.documentSaved =
                        SaveLevel(
                            sceneDocument) ==
                        OperationResult::Success;
                    break;

                case PendingCommand::SaveAs:
                    result.documentSaved =
                        SaveLevelAs(
                            sceneDocument) ==
                        OperationResult::Success;
                    break;

                case PendingCommand::Exit:
                    if (
                        ConfirmSaveChanges(
                            sceneDocument) ==
                        OperationResult::Success)
                    {
                        allowClose_ = true;
                        result.closeApproved = true;

                        PostMessageW(
                            ToWindow(mainWindow_),
                            WM_CLOSE,
                            0,
                            0);
                    }
                    break;

                case PendingCommand::None:
                default:
                    break;
            }
        }
        catch (const std::exception& exception)
        {
            std::wstring message =
                L"Level operation failed: ";

            const std::string what =
                exception.what();

            message.append(
                what.begin(),
                what.end());

            ShowError(
                L"Level Document",
                message);
        }
        catch (...)
        {
            ShowError(
                L"Level Document",
                L"An unknown level document error occurred.");
        }

        SynchronizeWindowTitle(
            sceneDocument);

        return result;
    }

    void LevelDocument::
        SynchronizeWindowTitle(
            const SceneDocument&
                sceneDocument) noexcept
    {
        const HWND window =
            ToWindow(mainWindow_);

        if (
            window == nullptr ||
            !IsWindow(window))
        {
            return;
        }

        std::wstring title =
            L"LastToSurvive Editor — ";

        title += levelName_.empty()
            ? L"Untitled"
            : levelName_;

        if (sceneDocument.IsDirty())
        {
            title += L" *";
        }

        if (title == lastWindowTitle_)
        {
            return;
        }

        SetWindowTextW(
            window,
            title.c_str());

        lastWindowTitle_ =
            std::move(title);
    }

    bool LevelDocument::
        InstallWindowSubclass() noexcept
    {
        const HWND window =
            ToWindow(mainWindow_);

        if (
            window == nullptr ||
            !SetPropW(
                window,
                LevelDocumentPropertyName,
                this))
        {
            return false;
        }

        SetLastError(ERROR_SUCCESS);

        const LONG_PTR previous =
            SetWindowLongPtrW(
                window,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(
                    &LevelDocument::
                        WindowProcedure));

        if (
            previous == 0 &&
            GetLastError() != ERROR_SUCCESS)
        {
            RemovePropW(
                window,
                LevelDocumentPropertyName);

            return false;
        }

        previousWindowProcedure_ =
            reinterpret_cast<void*>(
                previous);

        subclassInstalled_ = true;
        return true;
    }

    void LevelDocument::
        RestoreWindowSubclass() noexcept
    {
        if (!subclassInstalled_)
        {
            return;
        }

        const HWND window =
            ToWindow(mainWindow_);

        if (
            window != nullptr &&
            IsWindow(window))
        {
            const LONG_PTR current =
                GetWindowLongPtrW(
                    window,
                    GWLP_WNDPROC);

            if (
                current ==
                reinterpret_cast<LONG_PTR>(
                    &LevelDocument::
                        WindowProcedure))
            {
                SetWindowLongPtrW(
                    window,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(
                        ToWindowProcedure(
                            previousWindowProcedure_)));
            }

            RemovePropW(
                window,
                LevelDocumentPropertyName);
        }

        previousWindowProcedure_ = nullptr;
        subclassInstalled_ = false;
    }

    void LevelDocument::QueueCommand(
        const PendingCommand command) noexcept
    {
        if (
            pendingCommand_ ==
                PendingCommand::None ||
            command == PendingCommand::Exit)
        {
            pendingCommand_ = command;
        }
    }

    bool LevelDocument::WasPressed(
        const int virtualKey) noexcept
    {
        if (
            virtualKey < 0 ||
            virtualKey >=
                static_cast<int>(
                    previousKeyDown_.size()))
        {
            return false;
        }

        const bool down =
            IsKeyDown(virtualKey);

        const std::size_t index =
            static_cast<std::size_t>(
                virtualKey);

        const bool pressed =
            down &&
            !previousKeyDown_[index];

        previousKeyDown_[index] = down;

        return pressed;
    }

    void LevelDocument::PollShortcuts() noexcept
    {
        const bool controlDown =
            IsKeyDown(VK_CONTROL) ||
            IsKeyDown(VK_LCONTROL) ||
            IsKeyDown(VK_RCONTROL);

        const bool shiftDown =
            IsKeyDown(VK_SHIFT) ||
            IsKeyDown(VK_LSHIFT) ||
            IsKeyDown(VK_RSHIFT);

        const bool newPressed =
            WasPressed('N');

        const bool openPressed =
            WasPressed('O');

        const bool savePressed =
            WasPressed('S');

        if (
            controlDown &&
            !shiftDown &&
            newPressed)
        {
            QueueCommand(
                PendingCommand::NewLevel);
        }

        if (
            controlDown &&
            !shiftDown &&
            openPressed)
        {
            QueueCommand(
                PendingCommand::OpenLevel);
        }

        if (
            controlDown &&
            savePressed)
        {
            QueueCommand(
                shiftDown
                    ? PendingCommand::SaveAs
                    : PendingCommand::Save);
        }
    }

    LevelDocument::OperationResult
        LevelDocument::
            ConfirmSaveChanges(
                SceneDocument&
                    sceneDocument) noexcept
    {
        if (!sceneDocument.IsDirty())
        {
            return OperationResult::Success;
        }

        std::wstring message =
            L"Save changes to \"";

        message += levelName_;
        message += L"\" before continuing?";

        const int answer =
            MessageBoxW(
                ToWindow(mainWindow_),
                message.c_str(),
                L"Unsaved Level",
                MB_YESNOCANCEL |
                    MB_ICONWARNING |
                    MB_DEFBUTTON1);

        if (answer == IDCANCEL)
        {
            return OperationResult::Cancelled;
        }

        if (answer == IDNO)
        {
            return OperationResult::Success;
        }

        return SaveLevel(
            sceneDocument);
    }

    LevelDocument::OperationResult
        LevelDocument::CreateNewLevel(
            SceneDocument& sceneDocument,
            CommandHistory&
                commandHistory) noexcept
    {
        sceneDocument.Clear();

        const EditorSceneSnapshot snapshot =
            sceneDocument.CreateSnapshot();

        sceneDocument.RestoreSnapshot(
            snapshot,
            true);

        commandHistory.Clear();
        ResetUntitledMetadata();

        return OperationResult::Success;
    }

    LevelDocument::OperationResult
        LevelDocument::OpenLevel(
            SceneDocument& sceneDocument,
            CommandHistory&
                commandHistory) noexcept
    {
        std::filesystem::path path;
        std::wstring error;
        bool cancelled = false;

        if (!ShowFileDialog(
                false,
                path,
                cancelled,
                error))
        {
            if (!cancelled)
            {
                ShowError(
                    L"Open Level",
                    error);
            }

            return cancelled
                ? OperationResult::Cancelled
                : OperationResult::Failed;
        }

        EditorLevelFileData data;

        if (!LevelSerializer::Load(
                path,
                data,
                error))
        {
            ShowError(
                L"Open Level",
                error);

            return OperationResult::Failed;
        }

        sceneDocument.RestoreSnapshot(
            data.snapshot,
            false);

        sceneDocument.MarkSaved();
        commandHistory.Clear();

        currentPath_ =
            std::filesystem::absolute(path);

        levelName_ =
            std::move(data.name);

        levelGuid_ =
            std::move(data.guid);

        if (levelName_.empty())
        {
            UpdateNameFromPath();
        }

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.Level",
            "Level opened.");

        return OperationResult::Success;
    }

    LevelDocument::OperationResult
        LevelDocument::SaveLevel(
            SceneDocument&
                sceneDocument) noexcept
    {
        if (currentPath_.empty())
        {
            return SaveLevelAs(
                sceneDocument);
        }

        EditorLevelFileData data;

        data.name = levelName_;
        data.guid = levelGuid_;

        data.snapshot =
            sceneDocument.CreateSnapshot();

        std::wstring error;

        if (!LevelSerializer::Save(
                currentPath_,
                data,
                error))
        {
            ShowError(
                L"Save Level",
                error);

            return OperationResult::Failed;
        }

        sceneDocument.MarkSaved();

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.Level",
            "Level saved.");

        return OperationResult::Success;
    }

    LevelDocument::OperationResult
        LevelDocument::SaveLevelAs(
            SceneDocument&
                sceneDocument) noexcept
    {
        std::filesystem::path path;
        std::wstring error;
        bool cancelled = false;

        if (!ShowFileDialog(
                true,
                path,
                cancelled,
                error))
        {
            if (!cancelled)
            {
                ShowError(
                    L"Save Level As",
                    error);
            }

            return cancelled
                ? OperationResult::Cancelled
                : OperationResult::Failed;
        }

        if (
            path.extension() !=
            L".ltslevel")
        {
            path.replace_extension(
                L".ltslevel");
        }

        const std::filesystem::path
            worldsFolder =
                GetWorldsFolder();

        if (
            path.parent_path() ==
            worldsFolder)
        {
            const std::wstring mapName =
                path.stem().wstring();

            path =
                worldsFolder /
                mapName /
                path.filename();
        }

        const std::filesystem::path oldPath =
            currentPath_;

        const std::wstring oldName =
            levelName_;

        currentPath_ =
            std::filesystem::absolute(path);

        UpdateNameFromPath();

        const OperationResult result =
            SaveLevel(sceneDocument);

        if (
            result !=
            OperationResult::Success)
        {
            currentPath_ = oldPath;
            levelName_ = oldName;
        }

        return result;
    }

    bool LevelDocument::ShowFileDialog(
        const bool saveDialog,
        std::filesystem::path& path,
        bool& cancelled,
        std::wstring& error) noexcept
    {
        cancelled = false;
        path.clear();
        error.clear();

        ComPtr<IFileDialog> dialog;

        HRESULT result =
            CoCreateInstance(
                saveDialog
                    ? CLSID_FileSaveDialog
                    : CLSID_FileOpenDialog,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_PPV_ARGS(
                    dialog.GetAddressOf()));

        if (FAILED(result))
        {
            error =
                L"Failed to create the Windows file dialog.";

            return false;
        }

        constexpr COMDLG_FILTERSPEC
            filters[]
        {
            {
                L"LastToSurvive Level (*.ltslevel)",
                L"*.ltslevel"
            },
            {
                L"All Files (*.*)",
                L"*.*"
            }
        };

        static_cast<void>(
            dialog->SetFileTypes(
                static_cast<UINT>(
                    std::size(filters)),
                filters));

        static_cast<void>(
            dialog->SetFileTypeIndex(1U));

        static_cast<void>(
            dialog->SetDefaultExtension(
                L"ltslevel"));

        DWORD options = 0U;

        if (SUCCEEDED(
                dialog->GetOptions(
                    &options)))
        {
            options |=
                FOS_FORCEFILESYSTEM |
                FOS_PATHMUSTEXIST |
                FOS_NOCHANGEDIR;

            if (saveDialog)
            {
                options |=
                    FOS_OVERWRITEPROMPT |
                    FOS_NOREADONLYRETURN;
            }
            else
            {
                options |=
                    FOS_FILEMUSTEXIST;
            }

            static_cast<void>(
                dialog->SetOptions(
                    options));
        }

        const std::filesystem::path
            defaultFolder =
                GetWorldsFolder();

        ComPtr<IShellItem> folderItem;

        if (SUCCEEDED(
                SHCreateItemFromParsingName(
                    defaultFolder.c_str(),
                    nullptr,
                    IID_PPV_ARGS(
                        folderItem.GetAddressOf()))))
        {
            static_cast<void>(
                dialog->SetDefaultFolder(
                    folderItem.Get()));

            static_cast<void>(
                dialog->SetFolder(
                    folderItem.Get()));
        }

        if (saveDialog)
        {
            std::wstring defaultName =
                levelName_.empty()
                    ? L"Untitled"
                    : levelName_;

            defaultName += L".ltslevel";

            static_cast<void>(
                dialog->SetFileName(
                    defaultName.c_str()));

            static_cast<void>(
                dialog->SetTitle(
                    L"Save LastToSurvive Level"));
        }
        else
        {
            static_cast<void>(
                dialog->SetTitle(
                    L"Open LastToSurvive Level"));
        }

        result =
            dialog->Show(
                ToWindow(mainWindow_));

        if (
            result ==
            HRESULT_FROM_WIN32(
                ERROR_CANCELLED))
        {
            cancelled = true;
            return false;
        }

        if (FAILED(result))
        {
            error =
                L"The Windows file dialog failed.";

            return false;
        }

        ComPtr<IShellItem>
            selectedItem;

        if (FAILED(
                dialog->GetResult(
                    selectedItem.GetAddressOf())))
        {
            error =
                L"Failed to read the selected file.";

            return false;
        }

        PWSTR selectedPath = nullptr;

        result =
            selectedItem->GetDisplayName(
                SIGDN_FILESYSPATH,
                &selectedPath);

        if (
            FAILED(result) ||
            selectedPath == nullptr)
        {
            error =
                L"The selected item is not a filesystem path.";

            CoTaskMemFree(selectedPath);
            return false;
        }

        path = selectedPath;

        CoTaskMemFree(selectedPath);
        return true;
    }

    void LevelDocument::
        ResetUntitledMetadata() noexcept
    {
        currentPath_.clear();
        levelName_ = L"Untitled";
        levelGuid_ = GenerateGuid();
        lastWindowTitle_.clear();
    }

    void LevelDocument::
        UpdateNameFromPath() noexcept
    {
        levelName_ =
            currentPath_.empty()
                ? L"Untitled"
                : currentPath_.
                    stem().
                    wstring();

        if (levelName_.empty())
        {
            levelName_ = L"Untitled";
        }
    }

    void LevelDocument::ShowError(
        const std::wstring_view title,
        const std::wstring_view message) const noexcept
    {
        const std::wstring safeTitle(title);
        const std::wstring safeMessage(message);

        MessageBoxW(
            ToWindow(mainWindow_),
            safeMessage.c_str(),
            safeTitle.c_str(),
            MB_OK |
                MB_ICONERROR);
    }

    LRESULT CALLBACK
        LevelDocument::WindowProcedure(
            const HWND window,
            const UINT message,
            const WPARAM wParam,
            const LPARAM lParam) noexcept
    {
        auto* const self =
            static_cast<LevelDocument*>(
                GetPropW(
                    window,
                    LevelDocumentPropertyName));

        if (
            self == nullptr ||
            self->previousWindowProcedure_ ==
                nullptr)
        {
            return DefWindowProcW(
                window,
                message,
                wParam,
                lParam);
        }

        if (message == WM_COMMAND)
        {
            switch (LOWORD(wParam))
            {
                case IdMenuNewLevel:
                    self->QueueCommand(
                        PendingCommand::NewLevel);
                    return 0;

                case IdMenuOpen:
                    self->QueueCommand(
                        PendingCommand::OpenLevel);
                    return 0;

                case IdMenuSave:
                    self->QueueCommand(
                        PendingCommand::Save);
                    return 0;

                case IdMenuSaveAs:
                    self->QueueCommand(
                        PendingCommand::SaveAs);
                    return 0;

                case IdMenuExit:
                    self->QueueCommand(
                        PendingCommand::Exit);
                    return 0;

                default:
                    break;
            }
        }

        if (message == WM_CLOSE)
        {
            if (self->allowClose_)
            {
                self->allowClose_ = false;

                return CallWindowProcW(
                    ToWindowProcedure(
                        self->
                            previousWindowProcedure_),
                    window,
                    message,
                    wParam,
                    lParam);
            }

            self->QueueCommand(
                PendingCommand::Exit);

            return 0;
        }

        return CallWindowProcW(
            ToWindowProcedure(
                self->
                    previousWindowProcedure_),
            window,
            message,
            wParam,
            lParam);
    }
}
