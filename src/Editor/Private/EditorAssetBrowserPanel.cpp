#include "Editor/EditorAssetBrowserPanel.h"

#include <Assets/AssetResult.h>
#include <Assets/LegacyMeshImporter.h>

#include <Windows.h>
#include <commctrl.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

namespace lts::editor
{
    namespace
    {
        constexpr wchar_t AssetBrowserPropertyName[] =
            L"LTS.Editor.AssetBrowser.Instance";

        constexpr int AssetBrowserAnchorId = 1402;

        constexpr int IdAssetPanel = 5100;
        constexpr int IdAssetFilter = 5101;
        constexpr int IdAssetRefresh = 5102;
        constexpr int IdAssetList = 5103;

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
        std::wstring Lowercase(
            std::wstring value)
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
        bool IsSupportedExtension(
            const std::filesystem::path& path)
        {
            const std::wstring extension =
                Lowercase(
                    path.extension().wstring());

            return
                extension == L".sco" ||
                extension == L".scb" ||
                extension == L".ltsmesh";
        }

        [[nodiscard]]
        HWND CreateChild(
            const HWND parent,
            const DWORD extendedStyle,
            const wchar_t* const className,
            const wchar_t* const text,
            const DWORD style,
            const int controlId) noexcept
        {
            return CreateWindowExW(
                extendedStyle,
                className,
                text,
                WS_CHILD |
                WS_VISIBLE |
                style,
                0,
                0,
                1,
                1,
                parent,
                reinterpret_cast<HMENU>(
                    static_cast<INT_PTR>(
                        controlId)),
                GetModuleHandleW(nullptr),
                nullptr);
        }

        void DestroyWindowSafe(
            void*& value) noexcept
        {
            const HWND window =
                ToWindow(value);

            if (window != nullptr)
            {
                DestroyWindow(window);
                value = nullptr;
            }
        }

        void ApplyFont(
            const HWND control,
            const HFONT font) noexcept
        {
            if (
                control != nullptr &&
                font != nullptr)
            {
                SendMessageW(
                    control,
                    WM_SETFONT,
                    reinterpret_cast<WPARAM>(
                        font),
                    TRUE);
            }
        }

        [[nodiscard]]
        std::wstring ReadWindowText(
            const HWND window)
        {
            if (window == nullptr)
            {
                return {};
            }

            const int length =
                GetWindowTextLengthW(window);

            if (length <= 0)
            {
                return {};
            }

            std::wstring text(
                static_cast<std::size_t>(
                    length) +
                    1U,
                L'\0');

            const int copied =
                GetWindowTextW(
                    window,
                    text.data(),
                    length + 1);

            if (copied <= 0)
            {
                return {};
            }

            text.resize(
                static_cast<std::size_t>(
                    copied));

            return text;
        }
    }

    EditorAssetBrowserPanel::
        ~EditorAssetBrowserPanel() noexcept
    {
        Shutdown();
    }

    bool EditorAssetBrowserPanel::Initialize(
        const engine::platform::
            NativeWindowHandle mainWindow) noexcept
    {
        if (initialized_)
        {
            return true;
        }

        if (!mainWindow.IsValid())
        {
            return false;
        }

        const HWND root =
            reinterpret_cast<HWND>(
                mainWindow.Value());

        if (
            root == nullptr ||
            !IsWindow(root))
        {
            return false;
        }

        const HWND anchor =
            GetDlgItem(
                root,
                AssetBrowserAnchorId);

        if (anchor == nullptr)
        {
            return false;
        }

        mainWindow_ = root;
        anchorWindow_ = anchor;

        std::error_code filesystemError;

        dataRoot_ =
            std::filesystem::current_path(
                filesystemError);

        if (filesystemError)
        {
            dataRoot_ = L".";
        }

        dataRoot_ /= L"Data";

        if (
            !CreateControls() ||
            !InstallWindowSubclass())
        {
            Shutdown();
            return false;
        }

        ShowWindow(
            anchor,
            SW_HIDE);

        initialized_ = true;
        scanRequested_ = true;

        Update();
        return true;
    }

    void EditorAssetBrowserPanel::Shutdown() noexcept
    {
        RestoreWindowSubclass();
        DestroyControls();

        const HWND anchor =
            ToWindow(anchorWindow_);

        if (
            anchor != nullptr &&
            IsWindow(anchor))
        {
            ShowWindow(
                anchor,
                SW_SHOW);
        }

        assets_.clear();
        pendingAssetPath_.clear();
        dataRoot_.clear();

        mainWindow_ = nullptr;
        anchorWindow_ = nullptr;
        font_ = nullptr;

        requestedAssetIndex_ =
            static_cast<std::size_t>(-1);

        scanRequested_ = false;
        activationRequested_ = false;
        pendingAssetReady_ = false;

        initialized_ = false;
    }

    void EditorAssetBrowserPanel::Update() noexcept
    {
        if (!initialized_)
        {
            return;
        }

        UpdateLayout();

        if (scanRequested_)
        {
            scanRequested_ = false;
            ScanAssets();
        }

        if (activationRequested_)
        {
            activationRequested_ = false;
            ProcessRequestedAsset();
        }
    }

    bool EditorAssetBrowserPanel::
        ConsumeActivatedAsset(
            std::filesystem::path& assetPath)
    {
        if (!pendingAssetReady_)
        {
            return false;
        }

        assetPath =
            std::move(pendingAssetPath_);

        pendingAssetPath_.clear();
        pendingAssetReady_ = false;

        return true;
    }

    bool EditorAssetBrowserPanel::
        CreateControls() noexcept
    {
        const HWND root =
            ToWindow(mainWindow_);

        const HWND panel =
            CreateChild(
                root,
                WS_EX_CLIENTEDGE,
                L"STATIC",
                L"",
                SS_LEFT |
                WS_CLIPCHILDREN,
                IdAssetPanel);

        if (panel == nullptr)
        {
            return false;
        }

        panelWindow_ = panel;

        filterEdit_ =
            CreateChild(
                panel,
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                ES_LEFT |
                ES_AUTOHSCROLL |
                WS_TABSTOP,
                IdAssetFilter);

        refreshButton_ =
            CreateChild(
                panel,
                0U,
                L"BUTTON",
                L"Refresh",
                BS_PUSHBUTTON |
                WS_TABSTOP,
                IdAssetRefresh);

        assetList_ =
            CreateChild(
                panel,
                WS_EX_CLIENTEDGE,
                L"LISTBOX",
                L"",
                WS_VSCROLL |
                LBS_NOTIFY |
                LBS_NOINTEGRALHEIGHT,
                IdAssetList);

        if (
            ToWindow(filterEdit_) == nullptr ||
            ToWindow(refreshButton_) == nullptr ||
            ToWindow(assetList_) == nullptr)
        {
            return false;
        }

        SendMessageW(
            ToWindow(filterEdit_),
            EM_SETCUEBANNER,
            TRUE,
            reinterpret_cast<LPARAM>(
                L"Filter meshes..."));

        font_ =
            GetStockObject(
                DEFAULT_GUI_FONT);

        const HFONT font =
            static_cast<HFONT>(
                font_);

        ApplyFont(panel, font);

        ApplyFont(
            ToWindow(filterEdit_),
            font);

        ApplyFont(
            ToWindow(refreshButton_),
            font);

        ApplyFont(
            ToWindow(assetList_),
            font);

        return true;
    }

    bool EditorAssetBrowserPanel::
        InstallWindowSubclass() noexcept
    {
        const HWND root =
            ToWindow(mainWindow_);

        if (
            root == nullptr ||
            !SetPropW(
                root,
                AssetBrowserPropertyName,
                this))
        {
            return false;
        }

        SetLastError(ERROR_SUCCESS);

        const LONG_PTR previous =
            SetWindowLongPtrW(
                root,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(
                    &EditorAssetBrowserPanel::
                        WindowProcedure));

        if (
            previous == 0 &&
            GetLastError() != ERROR_SUCCESS)
        {
            RemovePropW(
                root,
                AssetBrowserPropertyName);

            return false;
        }

        previousWindowProcedure_ =
            reinterpret_cast<void*>(
                previous);

        subclassInstalled_ = true;
        return true;
    }

    void EditorAssetBrowserPanel::
        RestoreWindowSubclass() noexcept
    {
        if (!subclassInstalled_)
        {
            return;
        }

        const HWND root =
            ToWindow(mainWindow_);

        if (
            root != nullptr &&
            IsWindow(root))
        {
            const LONG_PTR current =
                GetWindowLongPtrW(
                    root,
                    GWLP_WNDPROC);

            if (
                current ==
                reinterpret_cast<LONG_PTR>(
                    &EditorAssetBrowserPanel::
                        WindowProcedure))
            {
                SetWindowLongPtrW(
                    root,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(
                        ToWindowProcedure(
                            previousWindowProcedure_)));
            }

            RemovePropW(
                root,
                AssetBrowserPropertyName);
        }

        previousWindowProcedure_ = nullptr;
        subclassInstalled_ = false;
    }

    void EditorAssetBrowserPanel::
        DestroyControls() noexcept
    {
        DestroyWindowSafe(assetList_);
        DestroyWindowSafe(refreshButton_);
        DestroyWindowSafe(filterEdit_);
        DestroyWindowSafe(panelWindow_);
    }

    void EditorAssetBrowserPanel::
        UpdateLayout() noexcept
    {
        const HWND root =
            ToWindow(mainWindow_);

        const HWND anchor =
            ToWindow(anchorWindow_);

        const HWND panel =
            ToWindow(panelWindow_);

        if (
            root == nullptr ||
            anchor == nullptr ||
            panel == nullptr)
        {
            return;
        }

        RECT rectangle{};

        if (!GetWindowRect(
                anchor,
                &rectangle))
        {
            return;
        }

        POINT points[2]
        {
            {
                rectangle.left,
                rectangle.top
            },
            {
                rectangle.right,
                rectangle.bottom
            }
        };

        MapWindowPoints(
            nullptr,
            root,
            points,
            2U);

        const int width =
            (std::max)(
        static_cast<int>(
            points[1].x -
            points[0].x),
        1);

        const int height =
            (std::max)(
                static_cast<int>(
                    points[1].y -
                    points[0].y),
                1);

        MoveWindow(
            panel,
            points[0].x,
            points[0].y,
            width,
            height,
            TRUE);

        constexpr int Margin = 4;
        constexpr int ToolbarHeight = 26;
        constexpr int RefreshWidth = 72;

        MoveWindow(
            ToWindow(filterEdit_),
            Margin,
            Margin,
            std::max(
                width -
                    RefreshWidth -
                    Margin * 3,
                1),
            ToolbarHeight,
            TRUE);

        MoveWindow(
            ToWindow(refreshButton_),
            std::max(
                width -
                    RefreshWidth -
                    Margin,
                Margin),
            Margin,
            RefreshWidth,
            ToolbarHeight,
            TRUE);

        MoveWindow(
            ToWindow(assetList_),
            Margin,
            ToolbarHeight +
                Margin * 2,
            std::max(
                width -
                    Margin * 2,
                1),
            std::max(
                height -
                    ToolbarHeight -
                    Margin * 3,
                1),
            TRUE);
    }

    void EditorAssetBrowserPanel::
        ScanAssets() noexcept
    {
        assets_.clear();

        try
        {
            std::error_code filesystemError;

            if (!std::filesystem::is_directory(
                    dataRoot_,
                    filesystemError) ||
                filesystemError)
            {
                RebuildVisibleList();
                return;
            }

            const auto options =
                std::filesystem::
                    directory_options::
                        skip_permission_denied;

            std::filesystem::
                recursive_directory_iterator iterator(
                    dataRoot_,
                    options,
                    filesystemError);

            const std::filesystem::
                recursive_directory_iterator end;

            while (iterator != end)
            {
                const std::filesystem::
                    directory_entry entry =
                        *iterator;

                iterator.increment(
                    filesystemError);

                if (filesystemError)
                {
                    filesystemError.clear();
                }

                std::error_code entryError;

                if (
                    !entry.is_regular_file(
                        entryError) ||
                    entryError ||
                    !IsSupportedExtension(
                        entry.path()))
                {
                    continue;
                }

                std::error_code relativeError;

                const std::filesystem::path relative =
                    std::filesystem::relative(
                        entry.path(),
                        dataRoot_,
                        relativeError);

                AssetEntry asset;

                asset.sourcePath =
                    entry.path();

                const std::wstring extension =
                    Lowercase(
                        entry.path().
                            extension().
                            wstring());

                if (extension == L".sco")
                {
                    asset.displayName =
                        L"[SCO] ";
                }
                else if (extension == L".scb")
                {
                    asset.displayName =
                        L"[SCB] ";
                }
                else
                {
                    asset.displayName =
                        L"[LTS] ";
                }

                asset.displayName +=
                    relativeError
                        ? entry.path().
                            filename().
                            wstring()
                        : relative.
                            generic_wstring();

                assets_.push_back(
                    std::move(asset));
            }

            std::sort(
                assets_.begin(),
                assets_.end(),
                [](const AssetEntry& left,
                   const AssetEntry& right)
                {
                    return
                        Lowercase(
                            left.displayName) <
                        Lowercase(
                            right.displayName);
                });
        }
        catch (...)
        {
            assets_.clear();
        }

        RebuildVisibleList();
    }

    void EditorAssetBrowserPanel::
        RebuildVisibleList() noexcept
    {
        const HWND list =
            ToWindow(assetList_);

        if (list == nullptr)
        {
            return;
        }

        SendMessageW(
            list,
            LB_RESETCONTENT,
            0U,
            0);

        try
        {
            const std::wstring filter =
                Lowercase(
                    ReadWindowText(
                        ToWindow(
                            filterEdit_)));

            for (
                std::size_t index = 0U;
                index < assets_.size();
                ++index)
            {
                const std::wstring searchable =
                    Lowercase(
                        assets_[index].
                            displayName);

                if (
                    !filter.empty() &&
                    searchable.find(filter) ==
                        std::wstring::npos)
                {
                    continue;
                }

                const LRESULT item =
                    SendMessageW(
                        list,
                        LB_ADDSTRING,
                        0U,
                        reinterpret_cast<LPARAM>(
                            assets_[index].
                                displayName.
                                c_str()));

                if (
                    item == LB_ERR ||
                    item == LB_ERRSPACE)
                {
                    continue;
                }

                SendMessageW(
                    list,
                    LB_SETITEMDATA,
                    static_cast<WPARAM>(
                        item),
                    static_cast<LPARAM>(
                        index));
            }
        }
        catch (...)
        {
            SendMessageW(
                list,
                LB_RESETCONTENT,
                0U,
                0);
        }
    }

    void EditorAssetBrowserPanel::
        QueueSelectedAsset() noexcept
    {
        const HWND list =
            ToWindow(assetList_);

        if (list == nullptr)
        {
            return;
        }

        const LRESULT selectedItem =
            SendMessageW(
                list,
                LB_GETCURSEL,
                0U,
                0);

        if (selectedItem == LB_ERR)
        {
            return;
        }

        const LRESULT itemData =
            SendMessageW(
                list,
                LB_GETITEMDATA,
                static_cast<WPARAM>(
                    selectedItem),
                0);

        if (itemData == LB_ERR)
        {
            return;
        }

        requestedAssetIndex_ =
            static_cast<std::size_t>(
                itemData);

        activationRequested_ = true;
    }

    void EditorAssetBrowserPanel::
        ProcessRequestedAsset() noexcept
    {
        if (
            requestedAssetIndex_ >=
            assets_.size())
        {
            requestedAssetIndex_ =
                static_cast<std::size_t>(-1);

            return;
        }

        std::filesystem::path runtimePath;
        std::wstring error;

        if (!PrepareRuntimeAsset(
                assets_[requestedAssetIndex_],
                runtimePath,
                error))
        {
            if (error.empty())
            {
                error =
                    L"Failed to prepare the selected mesh.";
            }

            MessageBoxW(
                ToWindow(mainWindow_),
                error.c_str(),
                L"Mesh Import",
                MB_OK |
                MB_ICONERROR);

            requestedAssetIndex_ =
                static_cast<std::size_t>(-1);

            return;
        }

        pendingAssetPath_ =
            std::move(runtimePath);

        pendingAssetReady_ = true;

        requestedAssetIndex_ =
            static_cast<std::size_t>(-1);

        scanRequested_ = true;
    }

    bool EditorAssetBrowserPanel::
        PrepareRuntimeAsset(
            const AssetEntry& entry,
            std::filesystem::path& runtimePath,
            std::wstring& error) const
    {
        error.clear();

        const std::wstring extension =
            Lowercase(
                entry.sourcePath.
                    extension().
                    wstring());

        std::filesystem::path finalPath =
            entry.sourcePath;

        if (
            extension == L".sco" ||
            extension == L".scb")
        {
            std::error_code relativeError;

            std::filesystem::path relative =
                std::filesystem::relative(
                    entry.sourcePath,
                    dataRoot_,
                    relativeError);

            if (relativeError)
            {
                relative =
                    entry.sourcePath.
                        filename();
            }

            finalPath =
                dataRoot_ /
                L"Imported" /
                L"Meshes" /
                relative;

            finalPath.replace_extension(
                L".ltsmesh");

            bool conversionRequired = true;

            std::error_code filesystemError;

            if (
                std::filesystem::is_regular_file(
                    finalPath,
                    filesystemError) &&
                !filesystemError)
            {
                const auto sourceTime =
                    std::filesystem::
                        last_write_time(
                            entry.sourcePath,
                            filesystemError);

                if (!filesystemError)
                {
                    const auto destinationTime =
                        std::filesystem::
                            last_write_time(
                                finalPath,
                                filesystemError);

                    if (
                        !filesystemError &&
                        destinationTime >= sourceTime)
                    {
                        conversionRequired = false;
                    }
                }
            }

            if (conversionRequired)
            {
                const engine::assets::AssetResult result =
                    engine::assets::
                        LegacyMeshImporter::Import(
                            entry.sourcePath,
                            finalPath,
                            error);

                if (engine::assets::Failed(result))
                {
                    return false;
                }
            }
        }
        else if (extension != L".ltsmesh")
        {
            error =
                L"Unsupported mesh extension.";

            return false;
        }

        std::error_code currentPathError;

        const std::filesystem::path gameRoot =
            std::filesystem::current_path(
                currentPathError);

        if (currentPathError)
        {
            runtimePath =
                finalPath.lexically_normal();

            return true;
        }

        std::error_code relativeError;

        runtimePath =
            std::filesystem::relative(
                finalPath,
                gameRoot,
                relativeError);

        if (relativeError)
        {
            runtimePath = finalPath;
        }

        runtimePath =
            runtimePath.lexically_normal();

        return true;
    }

    LRESULT CALLBACK
        EditorAssetBrowserPanel::WindowProcedure(
            const HWND window,
            const UINT message,
            const WPARAM wParam,
            const LPARAM lParam) noexcept
    {
        auto* const self =
            static_cast<
                EditorAssetBrowserPanel*>(
                    GetPropW(
                        window,
                        AssetBrowserPropertyName));

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
            const int controlId =
                LOWORD(wParam);

            const int notification =
                HIWORD(wParam);

            if (
                controlId == IdAssetFilter &&
                notification == EN_CHANGE)
            {
                self->RebuildVisibleList();
                return 0;
            }

            if (
                controlId == IdAssetRefresh &&
                notification == BN_CLICKED)
            {
                self->scanRequested_ = true;
                return 0;
            }

            if (
                controlId == IdAssetList &&
                notification == LBN_DBLCLK)
            {
                self->QueueSelectedAsset();
                return 0;
            }
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