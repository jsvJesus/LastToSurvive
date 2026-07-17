#include "Editor/EditorAssetBrowserPanel.h"

#include <Assets/AssetResult.h>
#include <Assets/LegacyMeshImporter.h>

#include <Windows.h>
#include <commctrl.h>

#include <algorithm>
#include <array>
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

        constexpr std::size_t InvalidAssetIndex =
            static_cast<std::size_t>(-1);

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
        bool IsLegacyMeshExtension(
            const std::filesystem::path& path)
        {
            const std::wstring extension =
                Lowercase(
                    path.extension().wstring());

            return
                extension == L".sco" ||
                extension == L".scb";
        }

        [[nodiscard]]
        bool IsGameMeshExtension(
            const std::filesystem::path& path)
        {
            return
                Lowercase(
                    path.extension().wstring()) ==
                L".ltsmesh";
        }

        [[nodiscard]]
        bool IsProjectRoot(
            const std::filesystem::path& path) noexcept
        {
            try
            {
                std::error_code filesystemError;

                const bool hasGame =
                    std::filesystem::is_directory(
                        path / L"game",
                        filesystemError);

                if (
                    filesystemError ||
                    !hasGame)
                {
                    return false;
                }

                filesystemError.clear();

                const bool hasBin =
                    std::filesystem::is_directory(
                        path / L"bin",
                        filesystemError);

                return
                    !filesystemError &&
                    hasBin;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        bool FindProjectRoot(
            std::filesystem::path startPath,
            std::filesystem::path& projectRoot) noexcept
        {
            try
            {
                if (startPath.empty())
                {
                    return false;
                }

                std::error_code filesystemError;

                startPath =
                    std::filesystem::absolute(
                        startPath,
                        filesystemError);

                if (filesystemError)
                {
                    return false;
                }

                if (std::filesystem::is_regular_file(
                        startPath,
                        filesystemError))
                {
                    if (filesystemError)
                    {
                        return false;
                    }

                    startPath =
                        startPath.parent_path();
                }

                startPath =
                    startPath.lexically_normal();

                std::filesystem::path current =
                    startPath;

                for (
                    std::size_t depth = 0U;
                    depth < 12U;
                    ++depth)
                {
                    if (IsProjectRoot(current))
                    {
                        projectRoot = current;
                        return true;
                    }

                    if (
                        Lowercase(
                            current.filename().
                                wstring()) ==
                            L"game" &&
                        IsProjectRoot(
                            current.parent_path()))
                    {
                        projectRoot =
                            current.parent_path();

                        return true;
                    }

                    const std::filesystem::path parent =
                        current.parent_path();

                    if (
                        parent.empty() ||
                        parent == current)
                    {
                        break;
                    }

                    current = parent;
                }

                return false;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        std::filesystem::path
            GetExecutableDirectory() noexcept
        {
            try
            {
                std::array<wchar_t, 32768U>
                    executablePath{};

                const DWORD length =
                    GetModuleFileNameW(
                        nullptr,
                        executablePath.data(),
                        static_cast<DWORD>(
                            executablePath.size()));

                if (
                    length == 0U ||
                    length >=
                        executablePath.size())
                {
                    return {};
                }

                return
                    std::filesystem::path(
                        std::wstring(
                            executablePath.data(),
                            length)).
                        parent_path();
            }
            catch (...)
            {
                return {};
            }
        }

        [[nodiscard]]
        bool ContainsParentTraversal(
            const std::filesystem::path& path)
        {
            for (
                const std::filesystem::path& segment :
                path)
            {
                if (segment == L"..")
                {
                    return true;
                }
            }

            return false;
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

        [[nodiscard]]
        const wchar_t* GetEntryPrefix(
            const EditorAssetBrowserPanel::
                AssetEntryKind kind) noexcept
        {
            switch (kind)
            {
                case EditorAssetBrowserPanel::
                    AssetEntryKind::LegacySco:
                    return L"[LEGACY SCO] ";

                case EditorAssetBrowserPanel::
                    AssetEntryKind::LegacyScb:
                    return L"[LEGACY SCB] ";

                case EditorAssetBrowserPanel::
                    AssetEntryKind::GameMesh:
                default:
                    return L"[GAME MESH] ";
            }
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

        std::wstring error;

        if (
            !ResolveProjectPaths(error) ||
            !EnsureOutputDirectories(error))
        {
            if (error.empty())
            {
                error =
                    L"Failed to initialize asset workspace paths.";
            }

            MessageBoxW(
                root,
                error.c_str(),
                L"Asset Browser",
                MB_OK |
                MB_ICONERROR);

            Shutdown();
            return false;
        }

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

        projectRoot_.clear();
        gameRoot_.clear();

        legacyObjectsRoot_.clear();
        meshesRoot_.clear();
        materialsRoot_.clear();
        texturesRoot_.clear();

        mainWindow_ = nullptr;
        anchorWindow_ = nullptr;
        font_ = nullptr;

        requestedAssetIndex_ =
            InvalidAssetIndex;

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
        ResolveProjectPaths(
            std::wstring& error) noexcept
    {
        error.clear();

        try
        {
            std::filesystem::path foundRoot;

            std::error_code currentPathError;

            const std::filesystem::path currentPath =
                std::filesystem::current_path(
                    currentPathError);

            if (
                !currentPathError &&
                FindProjectRoot(
                    currentPath,
                    foundRoot))
            {
                projectRoot_ =
                    foundRoot.lexically_normal();
            }
            else
            {
                const std::filesystem::path
                    executableDirectory =
                        GetExecutableDirectory();

                if (
                    executableDirectory.empty() ||
                    !FindProjectRoot(
                        executableDirectory,
                        foundRoot))
                {
                    error =
                        L"Project root was not found. "
                        L"The editor expects folders \"game\" and "
                        L"\"bin\" under one common project directory.";

                    return false;
                }

                projectRoot_ =
                    foundRoot.lexically_normal();
            }

            gameRoot_ =
                projectRoot_ /
                L"game";

            legacyObjectsRoot_ =
                projectRoot_ /
                L"bin" /
                L"Data" /
                L"ObjectsDepot";

            meshesRoot_ =
                gameRoot_ /
                L"Data" /
                L"Meshes";

            materialsRoot_ =
                gameRoot_ /
                L"Data" /
                L"Materials";

            texturesRoot_ =
                gameRoot_ /
                L"Data" /
                L"Textures";

            std::error_code filesystemError;

            if (!std::filesystem::is_directory(
                    gameRoot_,
                    filesystemError) ||
                filesystemError)
            {
                error =
                    L"The game directory does not exist:\n";

                error +=
                    gameRoot_.wstring();

                return false;
            }

            filesystemError.clear();

            if (!std::filesystem::is_directory(
                    legacyObjectsRoot_,
                    filesystemError) ||
                filesystemError)
            {
                error =
                    L"The legacy ObjectsDepot directory does not exist:\n";

                error +=
                    legacyObjectsRoot_.wstring();

                return false;
            }

            return true;
        }
        catch (...)
        {
            error =
                L"Unexpected failure while resolving project paths.";

            return false;
        }
    }

    bool EditorAssetBrowserPanel::
        EnsureOutputDirectories(
            std::wstring& error) noexcept
    {
        error.clear();

        try
        {
            const std::array<
                std::filesystem::path,
                3U> directories
            {
                meshesRoot_,
                materialsRoot_,
                texturesRoot_
            };

            for (
                const std::filesystem::path& directory :
                directories)
            {
                std::error_code filesystemError;

                std::filesystem::create_directories(
                    directory,
                    filesystemError);

                if (filesystemError)
                {
                    error =
                        L"Failed to create asset output directory:\n";

                    error +=
                        directory.wstring();

                    return false;
                }
            }

            return true;
        }
        catch (...)
        {
            error =
                L"Unexpected failure while creating asset directories.";

            return false;
        }
    }

    bool EditorAssetBrowserPanel::
    CreateControls() noexcept
    {
        const HWND root =
            ToWindow(mainWindow_);

        if (root == nullptr)
        {
            return false;
        }

        const HWND panel =
            CreateChild(
                root,
                WS_EX_CLIENTEDGE,
                L"STATIC",
                L"",
                SS_LEFT |
                WS_CLIPCHILDREN |
                WS_CLIPSIBLINGS,
                IdAssetPanel);

        if (panel == nullptr)
        {
            return false;
        }

        panelWindow_ = panel;

        /*
         * Элементы управления создаются дочерними
         * элементами главного окна, а не panel.
         *
         * Поэтому WM_COMMAND, EN_CHANGE, BN_CLICKED
         * и LBN_DBLCLK приходят в WindowProcedure
         * главного окна.
         */
        filterEdit_ =
            CreateChild(
                root,
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"",
                ES_LEFT |
                ES_AUTOHSCROLL |
                WS_TABSTOP |
                WS_CLIPSIBLINGS,
                IdAssetFilter);

        refreshButton_ =
            CreateChild(
                root,
                0U,
                L"BUTTON",
                L"Refresh",
                BS_PUSHBUTTON |
                WS_TABSTOP |
                WS_CLIPSIBLINGS,
                IdAssetRefresh);

        assetList_ =
            CreateChild(
                root,
                WS_EX_CLIENTEDGE,
                L"LISTBOX",
                L"",
                WS_VSCROLL |
                WS_HSCROLL |
                LBS_NOTIFY |
                LBS_NOINTEGRALHEIGHT |
                WS_TABSTOP |
                WS_CLIPSIBLINGS,
                IdAssetList);

        if (
            ToWindow(filterEdit_) == nullptr ||
            ToWindow(refreshButton_) == nullptr ||
            ToWindow(assetList_) == nullptr)
        {
            DestroyWindowSafe(assetList_);
            DestroyWindowSafe(refreshButton_);
            DestroyWindowSafe(filterEdit_);
            DestroyWindowSafe(panelWindow_);

            return false;
        }

        SendMessageW(
            ToWindow(filterEdit_),
            EM_SETCUEBANNER,
            TRUE,
            reinterpret_cast<LPARAM>(
                L"Filter legacy and game meshes..."));

        font_ =
            GetStockObject(
                DEFAULT_GUI_FONT);

        const HFONT font =
            static_cast<HFONT>(
                font_);

        ApplyFont(
            panel,
            font);

        ApplyFont(
            ToWindow(filterEdit_),
            font);

        ApplyFont(
            ToWindow(refreshButton_),
            font);

        ApplyFont(
            ToWindow(assetList_),
            font);

        /*
         * Гарантируем, что фон находится позади
         * фильтра, кнопки и списка.
         */
        SetWindowPos(
            panel,
            HWND_BOTTOM,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOACTIVATE);

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

        const HWND filter =
            ToWindow(filterEdit_);

        const HWND refresh =
            ToWindow(refreshButton_);

        const HWND list =
            ToWindow(assetList_);

        if (
            root == nullptr ||
            anchor == nullptr ||
            panel == nullptr ||
            filter == nullptr ||
            refresh == nullptr ||
            list == nullptr)
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

        const int originX =
            static_cast<int>(
                points[0].x);

        const int originY =
            static_cast<int>(
                points[0].y);

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

        constexpr int Margin = 4;
        constexpr int ToolbarHeight = 26;
        constexpr int RefreshWidth = 72;

        MoveWindow(
            panel,
            originX,
            originY,
            width,
            height,
            TRUE);

        MoveWindow(
            filter,
            originX +
                Margin,
            originY +
                Margin,
            (std::max)(
                width -
                    RefreshWidth -
                    Margin * 3,
                1),
            ToolbarHeight,
            TRUE);

        MoveWindow(
            refresh,
            originX +
                (std::max)(
                    width -
                        RefreshWidth -
                        Margin,
                    Margin),
            originY +
                Margin,
            RefreshWidth,
            ToolbarHeight,
            TRUE);

        MoveWindow(
            list,
            originX +
                Margin,
            originY +
                ToolbarHeight +
                Margin * 2,
            (std::max)(
                width -
                    Margin * 2,
                1),
            (std::max)(
                height -
                    ToolbarHeight -
                    Margin * 3,
                1),
            TRUE);

        SetWindowPos(
            panel,
            HWND_BOTTOM,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOACTIVATE);

        SetWindowPos(
            filter,
            HWND_TOP,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOACTIVATE);

        SetWindowPos(
            refresh,
            HWND_TOP,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOACTIVATE);

        SetWindowPos(
            list,
            HWND_TOP,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOACTIVATE);
    }

    void EditorAssetBrowserPanel::
        ScanAssets() noexcept
    {
        assets_.clear();

        ScanLegacyObjects();
        ScanGameMeshes();

        std::sort(
            assets_.begin(),
            assets_.end(),
            [](const AssetEntry& left,
               const AssetEntry& right)
            {
                if (left.kind != right.kind)
                {
                    return
                        static_cast<std::uint8_t>(
                            left.kind) <
                        static_cast<std::uint8_t>(
                            right.kind);
                }

                return
                    Lowercase(
                        left.displayName) <
                    Lowercase(
                        right.displayName);
            });

        RebuildVisibleList();
    }

    void EditorAssetBrowserPanel::
        ScanLegacyObjects() noexcept
    {
        try
        {
            std::error_code filesystemError;

            if (!std::filesystem::is_directory(
                    legacyObjectsRoot_,
                    filesystemError) ||
                filesystemError)
            {
                return;
            }

            const auto options =
                std::filesystem::
                    directory_options::
                        skip_permission_denied;

            std::filesystem::
                recursive_directory_iterator iterator(
                    legacyObjectsRoot_,
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
                    !IsLegacyMeshExtension(
                        entry.path()))
                {
                    continue;
                }

                std::error_code relativeError;

                const std::filesystem::path relative =
                    std::filesystem::relative(
                        entry.path(),
                        legacyObjectsRoot_,
                        relativeError);

                if (
                    relativeError ||
                    relative.empty() ||
                    ContainsParentTraversal(relative))
                {
                    continue;
                }

                AssetEntry asset;

                asset.sourcePath =
                    entry.path();

                const std::wstring extension =
                    Lowercase(
                        entry.path().
                            extension().
                            wstring());

                asset.kind =
                    extension == L".sco"
                        ? AssetEntryKind::LegacySco
                        : AssetEntryKind::LegacyScb;

                asset.displayName =
                    GetEntryPrefix(asset.kind);

                asset.displayName +=
                    relative.generic_wstring();

                assets_.push_back(
                    std::move(asset));
            }
        }
        catch (...)
        {
            // Повреждённый или недоступный файл
            // не должен ломать весь Asset Browser.
        }
    }

    void EditorAssetBrowserPanel::
        ScanGameMeshes() noexcept
    {
        try
        {
            std::error_code filesystemError;

            if (!std::filesystem::is_directory(
                    meshesRoot_,
                    filesystemError) ||
                filesystemError)
            {
                return;
            }

            const auto options =
                std::filesystem::
                    directory_options::
                        skip_permission_denied;

            std::filesystem::
                recursive_directory_iterator iterator(
                    meshesRoot_,
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
                    !IsGameMeshExtension(
                        entry.path()))
                {
                    continue;
                }

                std::error_code relativeError;

                const std::filesystem::path relative =
                    std::filesystem::relative(
                        entry.path(),
                        meshesRoot_,
                        relativeError);

                if (
                    relativeError ||
                    relative.empty() ||
                    ContainsParentTraversal(relative))
                {
                    continue;
                }

                AssetEntry asset;

                asset.sourcePath =
                    entry.path();

                asset.kind =
                    AssetEntryKind::GameMesh;

                asset.displayName =
                    GetEntryPrefix(asset.kind);

                asset.displayName +=
                    relative.generic_wstring();

                assets_.push_back(
                    std::move(asset));
            }
        }
        catch (...)
        {
            // Повреждённый или недоступный файл
            // не должен ломать весь Asset Browser.
        }
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

            int widestTextLength = 0;

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

                widestTextLength =
                    (std::max)(
                        widestTextLength,
                        static_cast<int>(
                            assets_[index].
                                displayName.
                                size()) *
                            8);
            }

            SendMessageW(
                list,
                LB_SETHORIZONTALEXTENT,
                static_cast<WPARAM>(
                    widestTextLength),
                0);
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
                InvalidAssetIndex;

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
                InvalidAssetIndex;

            return;
        }

        pendingAssetPath_ =
            std::move(runtimePath);

        pendingAssetReady_ = true;

        requestedAssetIndex_ =
            InvalidAssetIndex;

        scanRequested_ = true;
    }

    bool EditorAssetBrowserPanel::
        PrepareRuntimeAsset(
            const AssetEntry& entry,
            std::filesystem::path& runtimePath,
            std::wstring& error) const
    {
        error.clear();
        runtimePath.clear();

        try
        {
            std::filesystem::path finalPath;

            if (
                entry.kind ==
                    AssetEntryKind::LegacySco ||
                entry.kind ==
                    AssetEntryKind::LegacyScb)
            {
                std::error_code relativeError;

                std::filesystem::path relative =
                    std::filesystem::relative(
                        entry.sourcePath,
                        legacyObjectsRoot_,
                        relativeError);

                if (
                    relativeError ||
                    relative.empty() ||
                    ContainsParentTraversal(relative))
                {
                    error =
                        L"Legacy mesh is outside ObjectsDepot.";

                    return false;
                }

                finalPath =
                    meshesRoot_ /
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
                            destinationTime >=
                                sourceTime)
                        {
                            conversionRequired =
                                false;
                        }
                    }
                }

                if (conversionRequired)
                {
                    const engine::assets::
                        AssetResult result =
                            engine::assets::
                                LegacyMeshImporter::
                                    Import(
                                        entry.sourcePath,
                                        finalPath,
                                        error);

                    if (
                        engine::assets::Failed(
                            result))
                    {
                        return false;
                    }
                }
            }
            else
            {
                finalPath =
                    entry.sourcePath;
            }

            std::error_code fileError;

            if (!std::filesystem::is_regular_file(
                    finalPath,
                    fileError) ||
                fileError)
            {
                error =
                    L"The converted mesh file does not exist:\n";

                error +=
                    finalPath.wstring();

                return false;
            }

            std::error_code relativeError;

            runtimePath =
                std::filesystem::relative(
                    finalPath,
                    gameRoot_,
                    relativeError);

            if (
                relativeError ||
                runtimePath.empty() ||
                ContainsParentTraversal(
                    runtimePath))
            {
                error =
                    L"The mesh path cannot be converted "
                    L"to a game-relative path.";

                return false;
            }

            runtimePath =
                runtimePath.lexically_normal();

            return true;
        }
        catch (...)
        {
            error =
                L"Unexpected failure while preparing the mesh.";

            runtimePath.clear();
            return false;
        }
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