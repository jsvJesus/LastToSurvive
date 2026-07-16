#include "Editor/EditorShell.h"

#include <Windows.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <memory>

namespace lts::editor
{
    namespace
    {
        constexpr wchar_t ShellPropertyName[] =
            L"LTS.EditorShell.Instance";

        constexpr int IdModeLevel = 1001;
        constexpr int IdModeCharacter = 1002;
        constexpr int IdModeIcon = 1003;
        constexpr int IdModePhysics = 1004;
        constexpr int IdModeParticles = 1005;
        constexpr int IdModePlay = 1006;

        constexpr int IdHierarchyTitle = 1101;
        constexpr int IdHierarchyList = 1102;

        constexpr int IdViewportTitle = 1201;
        constexpr int IdViewportWindow = 1202;

        constexpr int IdInspectorTitle = 1301;
        constexpr int IdInspectorList = 1302;

        constexpr int IdAssetTitle = 1401;
        constexpr int IdAssetList = 1402;

        constexpr int IdConsoleTitle = 1501;
        constexpr int IdConsole = 1502;

        constexpr int IdStatusBar = 1601;

        constexpr int IdMenuNewLevel = 2001;
        constexpr int IdMenuOpen = 2002;
        constexpr int IdMenuSave = 2003;
        constexpr int IdMenuSaveAs = 2004;
        constexpr int IdMenuExit = 2005;

        constexpr int IdMenuUndo = 2101;
        constexpr int IdMenuRedo = 2102;
        constexpr int IdMenuDelete = 2103;
        constexpr int IdMenuDuplicate = 2104;

        constexpr int IdMenuResetLayout = 2201;
        constexpr int IdMenuAbout = 2301;

        struct ModeButtonDescription final
        {
            int controlId = 0;
            EditorMode mode = EditorMode::Level;
            const wchar_t* text = L"";
        };

        constexpr std::array<ModeButtonDescription, 6U>
            ModeButtons
        {
            ModeButtonDescription
            {
                IdModeLevel,
                EditorMode::Level,
                L"LEVEL"
            },
            ModeButtonDescription
            {
                IdModeCharacter,
                EditorMode::Character,
                L"CHARACTER"
            },
            ModeButtonDescription
            {
                IdModeIcon,
                EditorMode::Icon,
                L"ICON"
            },
            ModeButtonDescription
            {
                IdModePhysics,
                EditorMode::Physics,
                L"PHYSICS"
            },
            ModeButtonDescription
            {
                IdModeParticles,
                EditorMode::Particles,
                L"PARTICLES"
            },
            ModeButtonDescription
            {
                IdModePlay,
                EditorMode::Play,
                L"PLAY"
            }
        };

        [[nodiscard]]
        const wchar_t* GetModeName(
            const EditorMode mode) noexcept
        {
            switch (mode)
            {
                case EditorMode::Level:
                    return L"Level";

                case EditorMode::Character:
                    return L"Character";

                case EditorMode::Icon:
                    return L"Icon";

                case EditorMode::Physics:
                    return L"Physics";

                case EditorMode::Particles:
                    return L"Particles";

                case EditorMode::Play:
                    return L"Play";

                default:
                    return L"Unknown";
            }
        }

        [[nodiscard]]
        const wchar_t* GetEntityKindName(
            const EditorEntityKind kind) noexcept
        {
            switch (kind)
            {
                case EditorEntityKind::Environment:
                    return L"Environment";
                case EditorEntityKind::DirectionalLight:
                    return L"Directional Light";
                case EditorEntityKind::SpawnPoint:
                    return L"Spawn Point";
                case EditorEntityKind::Anomaly:
                    return L"Anomaly";
                case EditorEntityKind::LootContainer:
                    return L"Loot Container";
                case EditorEntityKind::Empty:
                default:
                    return L"Empty";
            }
        }

        void DestroyControl(
            HWND& control) noexcept
        {
            if (control != nullptr)
            {
                DestroyWindow(control);
                control = nullptr;
            }
        }
    }

    class EditorShell::Impl final
    {
    public:
        Impl() noexcept = default;

        ~Impl() noexcept
        {
            Shutdown();
        }

        [[nodiscard]]
        bool Initialize(
            const engine::platform::NativeWindowHandle
                mainWindow) noexcept
        {
            if (initialized_)
            {
                return true;
            }

            if (!mainWindow.IsValid())
            {
                return false;
            }

            parentWindow_ =
                reinterpret_cast<HWND>(
                    mainWindow.Value());

            if (
                parentWindow_ == nullptr ||
                !IsWindow(parentWindow_)
            )
            {
                parentWindow_ = nullptr;
                return false;
            }

            instance_ =
                GetModuleHandleW(nullptr);

            if (instance_ == nullptr)
            {
                parentWindow_ = nullptr;
                return false;
            }

            if (!CreateBrushes())
            {
                Shutdown();
                return false;
            }

            if (!RegisterViewportClass())
            {
                Shutdown();
                return false;
            }

            if (!CreateMainMenu())
            {
                Shutdown();
                return false;
            }

            if (!CreateControls())
            {
                Shutdown();
                return false;
            }

            if (!InstallWindowSubclass())
            {
                Shutdown();
                return false;
            }

            LayoutFromParent();
            UpdateStatusText();

            initialized_ = true;
            return true;
        }

        void Shutdown() noexcept
        {
            RestoreWindowSubclass();

            DestroyControls();

            if (
                parentWindow_ != nullptr &&
                IsWindow(parentWindow_)
            )
            {
                SetMenu(
                    parentWindow_,
                    nullptr);

                DrawMenuBar(
                    parentWindow_);
            }

            if (mainMenu_ != nullptr)
            {
                DestroyMenu(mainMenu_);
                mainMenu_ = nullptr;
            }

            if (
                viewportClassAtom_ != 0 &&
                instance_ != nullptr &&
                viewportClassName_[0] != L'\0'
            )
            {
                UnregisterClassW(
                    viewportClassName_.data(),
                    instance_);

                viewportClassAtom_ = 0;
            }

            DeleteBrush(backgroundBrush_);
            DeleteBrush(panelBrush_);
            DeleteBrush(inputBrush_);
            DeleteBrush(accentBrush_);
            DeleteBrush(borderBrush_);

            instance_ = nullptr;
            parentWindow_ = nullptr;

            viewportClassName_.fill(L'\0');

            activeMode_ = EditorMode::Level;
            modeChanged_ = false;
            initialized_ = false;
            viewportWheelSteps_ = 0.0F;
            pendingViewportClick_ = {};
            viewportClickPending_ = false;
            pendingHierarchySelection_ = InvalidEditorEntityIndex;
            hierarchySelectionChanged_ = false;
        }

        void Resize(
            const std::uint32_t width,
            const std::uint32_t height) noexcept
        {
            Layout(
                static_cast<int>(width),
                static_cast<int>(height));
        }

        [[nodiscard]]
        engine::platform::NativeWindowHandle
            GetViewportWindowHandle() const noexcept
        {
            return engine::platform::
                NativeWindowHandle::FromValue(
                    reinterpret_cast<std::uintptr_t>(
                        viewportWindow_));
        }

        [[nodiscard]]
        engine::platform::WindowSize
            GetViewportSize() const noexcept
        {
            engine::platform::WindowSize result;

            if (
                viewportWindow_ == nullptr ||
                !IsWindow(viewportWindow_)
            )
            {
                return result;
            }

            RECT rectangle{};

            if (!GetClientRect(
                    viewportWindow_,
                    &rectangle))
            {
                return result;
            }

            const LONG width =
                rectangle.right -
                rectangle.left;

            const LONG height =
                rectangle.bottom -
                rectangle.top;

            if (width > 0)
            {
                result.width =
                    static_cast<std::uint32_t>(
                        width);
            }

            if (height > 0)
            {
                result.height =
                    static_cast<std::uint32_t>(
                        height);
            }

            return result;
        }

        [[nodiscard]]
        EditorMode GetActiveMode() const noexcept
        {
            return activeMode_;
        }

        [[nodiscard]]
        bool ConsumeModeChanged(
            EditorMode& mode) noexcept
        {
            if (!modeChanged_)
            {
                return false;
            }

            mode = activeMode_;
            modeChanged_ = false;

            return true;
        }

        [[nodiscard]]
        float ConsumeViewportWheelSteps() noexcept
        {
            const float result =
                viewportWheelSteps_;

            viewportWheelSteps_ = 0.0F;

            return result;
        }

        [[nodiscard]]
        bool ConsumeViewportClick(ViewportClick& click) noexcept
        {
            if (!viewportClickPending_)
            {
                return false;
            }

            click = pendingViewportClick_;
            viewportClickPending_ = false;

            return true;
        }

        void SelectHierarchyEntity(
            const std::size_t entityIndex) noexcept
        {
            if (hierarchyList_ == nullptr)
            {
                return;
            }

            if (entityIndex == InvalidEditorEntityIndex)
            {
                SendMessageW(
                    hierarchyList_,
                    LB_SETCURSEL,
                    static_cast<WPARAM>(-1),
                    0);
            }
            else
            {
                SendMessageW(
                    hierarchyList_,
                    LB_SETCURSEL,
                    static_cast<WPARAM>(entityIndex),
                    0);
            }

            pendingHierarchySelection_ =
                InvalidEditorEntityIndex;

            hierarchySelectionChanged_ = false;
        }

        void RefreshScene(
            const EditorSceneDocument& document) noexcept
        {
            if (hierarchyList_ == nullptr)
            {
                return;
            }

            SendMessageW(
                hierarchyList_,
                LB_RESETCONTENT,
                0,
                0);

            const auto& entities =
                document.GetEntities();

            for (
                std::size_t index = 0U;
                index < entities.size();
                ++index)
            {
                const LRESULT itemIndex =
                    SendMessageW(
                        hierarchyList_,
                        LB_ADDSTRING,
                        0,
                        reinterpret_cast<LPARAM>(
                            entities[index].name.c_str()));

                if (
                    itemIndex == LB_ERR ||
                    itemIndex == LB_ERRSPACE)
                {
                    continue;
                }

                SendMessageW(
                    hierarchyList_,
                    LB_SETITEMDATA,
                    static_cast<WPARAM>(itemIndex),
                    static_cast<LPARAM>(index));
            }

            const std::size_t selectedIndex =
                document.GetSelectedIndex();

            if (selectedIndex < entities.size())
            {
                SendMessageW(
                    hierarchyList_,
                    LB_SETCURSEL,
                    static_cast<WPARAM>(selectedIndex),
                    0);
            }

            ShowEntityDetails(
                document.GetSelectedEntity());
        }

        [[nodiscard]]
        bool ConsumeHierarchySelection(
            std::size_t& entityIndex) noexcept
        {
            if (!hierarchySelectionChanged_)
            {
                return false;
            }

            entityIndex = pendingHierarchySelection_;

            pendingHierarchySelection_ =
                InvalidEditorEntityIndex;

            hierarchySelectionChanged_ = false;

            return entityIndex !=
                InvalidEditorEntityIndex;
        }

        void ShowEntityDetails(
            const EditorSceneEntity* const entity) noexcept
        {
            if (inspectorList_ == nullptr)
            {
                return;
            }

            SendMessageW(
                inspectorList_,
                LB_RESETCONTENT,
                0,
                0);

            if (entity == nullptr)
            {
                AddListItem(
                    inspectorList_,
                    L"No object selected");

                return;
            }

            std::array<wchar_t, 256U> line{};

            swprintf_s(
                line.data(),
                line.size(),
                L"Name: %ls",
                entity->name.c_str());

            AddListItem(
                inspectorList_,
                line.data());

            swprintf_s(
                line.data(),
                line.size(),
                L"Type: %ls",
                GetEntityKindName(entity->kind));

            AddListItem(
                inspectorList_,
                line.data());

            AddListItem(
                inspectorList_,
                L"");

            AddListItem(
                inspectorList_,
                L"TRANSFORM");

            swprintf_s(
                line.data(),
                line.size(),
                L"Location  X %.2f  Y %.2f  Z %.2f",
                entity->transform.position[0],
                entity->transform.position[1],
                entity->transform.position[2]);

            AddListItem(
                inspectorList_,
                line.data());

            swprintf_s(
                line.data(),
                line.size(),
                L"Rotation  X %.2f  Y %.2f  Z %.2f",
                entity->transform.rotationDegrees[0],
                entity->transform.rotationDegrees[1],
                entity->transform.rotationDegrees[2]);

            AddListItem(
                inspectorList_,
                line.data());

            swprintf_s(
                line.data(),
                line.size(),
                L"Scale     X %.2f  Y %.2f  Z %.2f",
                entity->transform.scale[0],
                entity->transform.scale[1],
                entity->transform.scale[2]);

            AddListItem(
                inspectorList_,
                line.data());
        }

        void SetStatusText(
            const std::wstring_view text) noexcept
        {
            if (statusBar_ == nullptr)
            {
                return;
            }

            std::array<wchar_t, 512U> buffer{};

            const std::size_t characterCount =
                std::min(
                    text.size(),
                    buffer.size() - 1U);

            if (characterCount > 0U)
            {
                std::wmemcpy(
                    buffer.data(),
                    text.data(),
                    characterCount);
            }

            buffer[characterCount] = L'\0';

            SetWindowTextW(
                statusBar_,
                buffer.data());
        }

    private:
        [[nodiscard]]
        bool CreateBrushes() noexcept
        {
            backgroundBrush_ =
                CreateSolidBrush(
                    RGB(15, 19, 22));

            panelBrush_ =
                CreateSolidBrush(
                    RGB(28, 34, 38));

            inputBrush_ =
                CreateSolidBrush(
                    RGB(20, 25, 29));

            accentBrush_ =
                CreateSolidBrush(
                    RGB(177, 76, 24));

            borderBrush_ =
                CreateSolidBrush(
                    RGB(74, 84, 90));

            return
                backgroundBrush_ != nullptr &&
                panelBrush_ != nullptr &&
                inputBrush_ != nullptr &&
                accentBrush_ != nullptr &&
                borderBrush_ != nullptr;
        }

        static void DeleteBrush(HBRUSH& brush) noexcept
        {
            if (brush == nullptr)
            {
                return;
            }

            ::DeleteObject(
                static_cast<HGDIOBJ>(brush));

            brush = nullptr;
        }

        [[nodiscard]]
        bool RegisterViewportClass() noexcept
        {
            const int written =
                swprintf_s(
                    viewportClassName_.data(),
                    viewportClassName_.size(),
                    L"LTS.Editor.Viewport.%lu",
                    GetCurrentProcessId());

            if (written <= 0)
            {
                viewportClassName_.fill(L'\0');
                return false;
            }

            WNDCLASSEXW windowClass{};

            windowClass.cbSize =
                sizeof(WNDCLASSEXW);

            windowClass.style =
                CS_OWNDC |
                CS_HREDRAW |
                CS_VREDRAW;

            windowClass.lpfnWndProc =
                &Impl::ViewportWindowProcedure;

            windowClass.hInstance =
                instance_;

            windowClass.hCursor =
                LoadCursorW(
                    nullptr,
                    IDC_CROSS);

            windowClass.hbrBackground =
                nullptr;

            windowClass.lpszClassName =
                viewportClassName_.data();

            viewportClassAtom_ =
                RegisterClassExW(
                    &windowClass);

            return viewportClassAtom_ != 0;
        }

        [[nodiscard]]
        bool CreateMainMenu() noexcept
        {
            mainMenu_ = CreateMenu();

            HMENU fileMenu =
                CreatePopupMenu();

            HMENU editMenu =
                CreatePopupMenu();

            HMENU viewMenu =
                CreatePopupMenu();

            HMENU helpMenu =
                CreatePopupMenu();

            if (
                mainMenu_ == nullptr ||
                fileMenu == nullptr ||
                editMenu == nullptr ||
                viewMenu == nullptr ||
                helpMenu == nullptr
            )
            {
                if (fileMenu != nullptr)
                {
                    DestroyMenu(fileMenu);
                }

                if (editMenu != nullptr)
                {
                    DestroyMenu(editMenu);
                }

                if (viewMenu != nullptr)
                {
                    DestroyMenu(viewMenu);
                }

                if (helpMenu != nullptr)
                {
                    DestroyMenu(helpMenu);
                }

                if (mainMenu_ != nullptr)
                {
                    DestroyMenu(mainMenu_);
                    mainMenu_ = nullptr;
                }

                return false;
            }

            AppendMenuW(
                fileMenu,
                MF_STRING,
                IdMenuNewLevel,
                L"New Level");

            AppendMenuW(
                fileMenu,
                MF_STRING,
                IdMenuOpen,
                L"Open...");

            AppendMenuW(
                fileMenu,
                MF_SEPARATOR,
                0,
                nullptr);

            AppendMenuW(
                fileMenu,
                MF_STRING,
                IdMenuSave,
                L"Save");

            AppendMenuW(
                fileMenu,
                MF_STRING,
                IdMenuSaveAs,
                L"Save As...");

            AppendMenuW(
                fileMenu,
                MF_SEPARATOR,
                0,
                nullptr);

            AppendMenuW(
                fileMenu,
                MF_STRING,
                IdMenuExit,
                L"Exit");

            AppendMenuW(
                editMenu,
                MF_STRING,
                IdMenuUndo,
                L"Undo");

            AppendMenuW(
                editMenu,
                MF_STRING,
                IdMenuRedo,
                L"Redo");

            AppendMenuW(
                editMenu,
                MF_SEPARATOR,
                0,
                nullptr);

            AppendMenuW(
                editMenu,
                MF_STRING,
                IdMenuDelete,
                L"Delete");

            AppendMenuW(
                editMenu,
                MF_STRING,
                IdMenuDuplicate,
                L"Duplicate");

            AppendMenuW(
                viewMenu,
                MF_STRING,
                IdMenuResetLayout,
                L"Reset Layout");

            AppendMenuW(
                helpMenu,
                MF_STRING,
                IdMenuAbout,
                L"About LTS Editor");

            AppendMenuW(
                mainMenu_,
                MF_POPUP,
                reinterpret_cast<UINT_PTR>(
                    fileMenu),
                L"File");

            AppendMenuW(
                mainMenu_,
                MF_POPUP,
                reinterpret_cast<UINT_PTR>(
                    editMenu),
                L"Edit");

            AppendMenuW(
                mainMenu_,
                MF_POPUP,
                reinterpret_cast<UINT_PTR>(
                    viewMenu),
                L"View");

            AppendMenuW(
                mainMenu_,
                MF_POPUP,
                reinterpret_cast<UINT_PTR>(
                    helpMenu),
                L"Help");

            if (!SetMenu(
                    parentWindow_,
                    mainMenu_))
            {
                DestroyMenu(mainMenu_);
                mainMenu_ = nullptr;

                return false;
            }

            DrawMenuBar(
                parentWindow_);

            return true;
        }

        [[nodiscard]]
        HWND CreateControl(
            const DWORD extendedStyle,
            const wchar_t* className,
            const wchar_t* text,
            const DWORD style,
            const int controlId) const noexcept
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
                parentWindow_,
                reinterpret_cast<HMENU>(
                    static_cast<INT_PTR>(
                        controlId)),
                instance_,
                nullptr);
        }

        [[nodiscard]]
        bool CreateControls() noexcept
        {
            for (
                std::size_t index = 0U;
                index < ModeButtons.size();
                ++index
            )
            {
                modeButtons_[index] =
                    CreateControl(
                        0,
                        L"BUTTON",
                        ModeButtons[index].text,
                        BS_OWNERDRAW |
                        WS_TABSTOP,
                        ModeButtons[index].controlId);

                if (modeButtons_[index] == nullptr)
                {
                    return false;
                }
            }

            EnableWindow(
                modeButtons_.back(),
                FALSE);

            hierarchyTitle_ =
                CreateControl(
                    0,
                    L"STATIC",
                    L"HIERARCHY",
                    SS_LEFT,
                    IdHierarchyTitle);

            hierarchyList_ =
                CreateControl(
                    WS_EX_CLIENTEDGE,
                    L"LISTBOX",
                    L"",
                    WS_VSCROLL |
                    LBS_NOTIFY |
                    LBS_NOINTEGRALHEIGHT,
                    IdHierarchyList);

            viewportTitle_ =
                CreateControl(
                    0,
                    L"STATIC",
                    L"VIEWPORT",
                    SS_LEFT,
                    IdViewportTitle);

            viewportWindow_ =
                CreateWindowExW(
                    WS_EX_CLIENTEDGE,
                    viewportClassName_.data(),
                    L"",
                    WS_CHILD |
                    WS_VISIBLE |
                    WS_CLIPSIBLINGS |
                    WS_CLIPCHILDREN,
                    0,
                    0,
                    1,
                    1,
                    parentWindow_,
                    reinterpret_cast<HMENU>(
                        static_cast<INT_PTR>(
                            IdViewportWindow)),
                    instance_,
                    this);

            inspectorTitle_ =
                CreateControl(
                    0,
                    L"STATIC",
                    L"INSPECTOR",
                    SS_LEFT,
                    IdInspectorTitle);

            inspectorList_ =
                CreateControl(
                    WS_EX_CLIENTEDGE,
                    L"LISTBOX",
                    L"",
                    WS_VSCROLL |
                    LBS_NOINTEGRALHEIGHT,
                    IdInspectorList);

            assetTitle_ =
                CreateControl(
                    0,
                    L"STATIC",
                    L"ASSET BROWSER",
                    SS_LEFT,
                    IdAssetTitle);

            assetList_ =
                CreateControl(
                    WS_EX_CLIENTEDGE,
                    L"LISTBOX",
                    L"",
                    WS_VSCROLL |
                    LBS_NOTIFY |
                    LBS_NOINTEGRALHEIGHT,
                    IdAssetList);

            consoleTitle_ =
                CreateControl(
                    0,
                    L"STATIC",
                    L"CONSOLE",
                    SS_LEFT,
                    IdConsoleTitle);

            consoleEdit_ =
                CreateControl(
                    WS_EX_CLIENTEDGE,
                    L"EDIT",
                    L"[LTS.Editor] Editor shell initialized.\r\n",
                    ES_LEFT |
                    ES_MULTILINE |
                    ES_AUTOVSCROLL |
                    ES_READONLY |
                    WS_VSCROLL,
                    IdConsole);

            statusBar_ =
                CreateControl(
                    0,
                    L"STATIC",
                    L"Ready",
                    SS_LEFT |
                    SS_CENTERIMAGE,
                    IdStatusBar);

            if (
                hierarchyTitle_ == nullptr ||
                hierarchyList_ == nullptr ||
                viewportTitle_ == nullptr ||
                viewportWindow_ == nullptr ||
                inspectorTitle_ == nullptr ||
                inspectorList_ == nullptr ||
                assetTitle_ == nullptr ||
                assetList_ == nullptr ||
                consoleTitle_ == nullptr ||
                consoleEdit_ == nullptr ||
                statusBar_ == nullptr
            )
            {
                return false;
            }

            font_ =
                reinterpret_cast<HFONT>(
                    GetStockObject(
                        DEFAULT_GUI_FONT));

            ApplyFontToControls();
            PopulatePlaceholderContent();

            return true;
        }

        void ApplyFont(
            const HWND control) const noexcept
        {
            if (
                control != nullptr &&
                font_ != nullptr
            )
            {
                SendMessageW(
                    control,
                    WM_SETFONT,
                    reinterpret_cast<WPARAM>(
                        font_),
                    TRUE);
            }
        }

        void ApplyFontToControls() const noexcept
        {
            for (const HWND button : modeButtons_)
            {
                ApplyFont(button);
            }

            ApplyFont(hierarchyTitle_);
            ApplyFont(hierarchyList_);

            ApplyFont(viewportTitle_);

            ApplyFont(inspectorTitle_);
            ApplyFont(inspectorList_);

            ApplyFont(assetTitle_);
            ApplyFont(assetList_);

            ApplyFont(consoleTitle_);
            ApplyFont(consoleEdit_);

            ApplyFont(statusBar_);
        }

        static void AddListItem(
            const HWND listBox,
            const wchar_t* text) noexcept
        {
            if (listBox != nullptr)
            {
                SendMessageW(
                    listBox,
                    LB_ADDSTRING,
                    0,
                    reinterpret_cast<LPARAM>(
                        text));
            }
        }

        void PopulatePlaceholderContent() const noexcept
        {
            AddListItem(
                hierarchyList_,
                L"Loading level...");

            AddListItem(
                inspectorList_,
                L"No object selected");

            AddListItem(
                assetList_,
                L"Assets/");

            AddListItem(
                assetList_,
                L"  Models/");

            AddListItem(
                assetList_,
                L"  Materials/");

            AddListItem(
                assetList_,
                L"  Textures/");

            AddListItem(
                assetList_,
                L"  Levels/");
        }

        void DestroyControls() noexcept
        {
            for (HWND& button : modeButtons_)
            {
                DestroyControl(button);
            }

            DestroyControl(hierarchyTitle_);
            DestroyControl(hierarchyList_);

            DestroyControl(viewportTitle_);
            DestroyControl(viewportWindow_);

            DestroyControl(inspectorTitle_);
            DestroyControl(inspectorList_);

            DestroyControl(assetTitle_);
            DestroyControl(assetList_);

            DestroyControl(consoleTitle_);
            DestroyControl(consoleEdit_);

            DestroyControl(statusBar_);

            font_ = nullptr;
        }

        [[nodiscard]]
        bool InstallWindowSubclass() noexcept
        {
            if (!SetPropW(
                    parentWindow_,
                    ShellPropertyName,
                    this))
            {
                return false;
            }

            SetLastError(ERROR_SUCCESS);

            const LONG_PTR previousProcedure =
                SetWindowLongPtrW(
                    parentWindow_,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(
                        &Impl::WindowProcedure));

            if (
                previousProcedure == 0 &&
                GetLastError() != ERROR_SUCCESS
            )
            {
                RemovePropW(
                    parentWindow_,
                    ShellPropertyName);

                return false;
            }

            originalWindowProcedure_ =
                reinterpret_cast<WNDPROC>(
                    previousProcedure);

            subclassInstalled_ = true;
            return true;
        }

        void RestoreWindowSubclass() noexcept
        {
            if (
                subclassInstalled_ &&
                parentWindow_ != nullptr &&
                IsWindow(parentWindow_)
            )
            {
                SetWindowLongPtrW(
                    parentWindow_,
                    GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(
                        originalWindowProcedure_));

                RemovePropW(
                    parentWindow_,
                    ShellPropertyName);
            }

            originalWindowProcedure_ = nullptr;
            subclassInstalled_ = false;
        }

        void LayoutFromParent() noexcept
        {
            if (parentWindow_ == nullptr)
            {
                return;
            }

            RECT clientRectangle{};

            if (!GetClientRect(
                    parentWindow_,
                    &clientRectangle))
            {
                return;
            }

            Layout(
                clientRectangle.right -
                    clientRectangle.left,
                clientRectangle.bottom -
                    clientRectangle.top);
        }

        static void MoveControl(
            const HWND control,
            const int x,
            const int y,
            const int width,
            const int height) noexcept
        {
            if (control == nullptr)
            {
                return;
            }

            MoveWindow(
                control,
                x,
                y,
                std::max(width, 1),
                std::max(height, 1),
                TRUE);
        }

        void Layout(
            const int requestedWidth,
            const int requestedHeight) noexcept
        {
            const int width =
                std::max(
                    requestedWidth,
                    1);

            const int height =
                std::max(
                    requestedHeight,
                    1);

            constexpr int gap = 4;
            constexpr int toolbarHeight = 42;
            constexpr int statusHeight = 24;
            constexpr int titleHeight = 22;

            int leftPanelWidth =
                std::clamp(
                    width / 6,
                    170,
                    250);

            int rightPanelWidth =
                std::clamp(
                    width / 5,
                    220,
                    320);

            if (
                leftPanelWidth +
                rightPanelWidth +
                240 >
                width
            )
            {
                leftPanelWidth =
                    std::max(
                        120,
                        width / 5);

                rightPanelWidth =
                    std::max(
                        140,
                        width / 4);
            }

            const int contentTop =
                toolbarHeight +
                gap;

            const int contentBottom =
                std::max(
                    contentTop + 1,
                    height -
                    statusHeight -
                    gap);

            const int availableHeight =
                std::max(
                    1,
                    contentBottom -
                    contentTop);

            int bottomPanelHeight =
                std::clamp(
                    availableHeight / 4,
                    140,
                    240);

            if (availableHeight < 320)
            {
                bottomPanelHeight =
                    std::max(
                        70,
                        availableHeight / 3);
            }

            const int centerLeft =
                leftPanelWidth +
                gap;

            const int centerRight =
                std::max(
                    centerLeft + 1,
                    width -
                    rightPanelWidth -
                    gap);

            const int centerWidth =
                std::max(
                    1,
                    centerRight -
                    centerLeft);

            const int bottomTop =
                std::max(
                    contentTop +
                    titleHeight +
                    1,
                    contentBottom -
                    bottomPanelHeight);

            const int viewportTop =
                contentTop +
                titleHeight;

            const int viewportHeight =
                std::max(
                    1,
                    bottomTop -
                    gap -
                    viewportTop);

            const int modeButtonWidth = 110;
            const int modeButtonHeight = 30;

            for (
                std::size_t index = 0U;
                index < modeButtons_.size();
                ++index
            )
            {
                MoveControl(
                    modeButtons_[index],
                    gap +
                    static_cast<int>(index) *
                    (modeButtonWidth + gap),
                    6,
                    modeButtonWidth,
                    modeButtonHeight);
            }

            MoveControl(
                hierarchyTitle_,
                gap,
                contentTop,
                leftPanelWidth - gap,
                titleHeight);

            MoveControl(
                hierarchyList_,
                gap,
                contentTop + titleHeight,
                leftPanelWidth - gap,
                availableHeight - titleHeight);

            MoveControl(
                viewportTitle_,
                centerLeft,
                contentTop,
                centerWidth,
                titleHeight);

            MoveControl(
                viewportWindow_,
                centerLeft,
                viewportTop,
                centerWidth,
                viewportHeight);

            MoveControl(
                inspectorTitle_,
                width - rightPanelWidth,
                contentTop,
                rightPanelWidth - gap,
                titleHeight);

            MoveControl(
                inspectorList_,
                width - rightPanelWidth,
                contentTop + titleHeight,
                rightPanelWidth - gap,
                availableHeight - titleHeight);

            const int browserWidth =
                std::max(
                    1,
                    centerWidth / 2 -
                    gap / 2);

            const int consoleLeft =
                centerLeft +
                browserWidth +
                gap;

            const int consoleWidth =
                std::max(
                    1,
                    centerRight -
                    consoleLeft);

            MoveControl(
                assetTitle_,
                centerLeft,
                bottomTop,
                browserWidth,
                titleHeight);

            MoveControl(
                assetList_,
                centerLeft,
                bottomTop + titleHeight,
                browserWidth,
                bottomPanelHeight - titleHeight);

            MoveControl(
                consoleTitle_,
                consoleLeft,
                bottomTop,
                consoleWidth,
                titleHeight);

            MoveControl(
                consoleEdit_,
                consoleLeft,
                bottomTop + titleHeight,
                consoleWidth,
                bottomPanelHeight - titleHeight);

            MoveControl(
                statusBar_,
                0,
                height - statusHeight,
                width,
                statusHeight);

            InvalidateRect(
                parentWindow_,
                nullptr,
                FALSE);
        }

        void SetActiveMode(
            const EditorMode mode) noexcept
        {
            if (mode == EditorMode::Play)
            {
                return;
            }

            if (activeMode_ != mode)
            {
                activeMode_ = mode;
                modeChanged_ = true;

                LogConsole(
                    GetModeName(mode));

                UpdateStatusText();
            }

            for (const HWND button : modeButtons_)
            {
                if (button != nullptr)
                {
                    InvalidateRect(
                        button,
                        nullptr,
                        TRUE);
                }
            }
        }

        void UpdateStatusText() noexcept
        {
            std::array<wchar_t, 256U> buffer{};

            const int written =
                swprintf_s(
                    buffer.data(),
                    buffer.size(),
                    L"Ready | Mode: %ls | Renderer: DX11",
                    GetModeName(activeMode_));

            if (written > 0)
            {
                SetWindowTextW(
                    statusBar_,
                    buffer.data());
            }
        }

        void LogConsole(
            const wchar_t* text) const noexcept
        {
            if (
                consoleEdit_ == nullptr ||
                text == nullptr
            )
            {
                return;
            }

            const int textLength =
                GetWindowTextLengthW(
                    consoleEdit_);

            SendMessageW(
                consoleEdit_,
                EM_SETSEL,
                static_cast<WPARAM>(
                    textLength),
                static_cast<LPARAM>(
                    textLength));

            SendMessageW(
                consoleEdit_,
                EM_REPLACESEL,
                FALSE,
                reinterpret_cast<LPARAM>(
                    L"[LTS.Editor] "));

            SendMessageW(
                consoleEdit_,
                EM_REPLACESEL,
                FALSE,
                reinterpret_cast<LPARAM>(
                    text));

            SendMessageW(
                consoleEdit_,
                EM_REPLACESEL,
                FALSE,
                reinterpret_cast<LPARAM>(
                    L"\r\n"));
        }

        void HandleHierarchySelectionChanged() noexcept
        {
            const LRESULT selectedItem =
                SendMessageW(
                    hierarchyList_,
                    LB_GETCURSEL,
                    0,
                    0);

            if (selectedItem == LB_ERR)
            {
                pendingHierarchySelection_ =
                    InvalidEditorEntityIndex;

                hierarchySelectionChanged_ = true;
                return;
            }

            const LRESULT itemData =
                SendMessageW(
                    hierarchyList_,
                    LB_GETITEMDATA,
                    static_cast<WPARAM>(selectedItem),
                    0);

            if (itemData == LB_ERR)
            {
                return;
            }

            pendingHierarchySelection_ =
                static_cast<std::size_t>(itemData);

            hierarchySelectionChanged_ = true;
        }

        [[nodiscard]]
        bool HandleCommand(
            const int commandId) noexcept
        {
            switch (commandId)
            {
                case IdModeLevel:
                    SetActiveMode(
                        EditorMode::Level);

                    return true;

                case IdModeCharacter:
                    SetActiveMode(
                        EditorMode::Character);

                    return true;

                case IdModeIcon:
                    SetActiveMode(
                        EditorMode::Icon);

                    return true;

                case IdModePhysics:
                    SetActiveMode(
                        EditorMode::Physics);

                    return true;

                case IdModeParticles:
                    SetActiveMode(
                        EditorMode::Particles);

                    return true;

                case IdModePlay:
                    return true;

                case IdMenuNewLevel:
                    SetActiveMode(
                        EditorMode::Level);

                    LogConsole(
                        L"New Level command.");

                    return true;

                case IdMenuOpen:
                    LogConsole(
                        L"Open command is not implemented yet.");

                    return true;

                case IdMenuSave:
                    LogConsole(
                        L"Save command is not implemented yet.");

                    return true;

                case IdMenuSaveAs:
                    LogConsole(
                        L"Save As command is not implemented yet.");

                    return true;

                case IdMenuUndo:
                    LogConsole(
                        L"Undo stack is not implemented yet.");

                    return true;

                case IdMenuRedo:
                    LogConsole(
                        L"Redo stack is not implemented yet.");

                    return true;

                case IdMenuDelete:
                    LogConsole(
                        L"No selected object to delete.");

                    return true;

                case IdMenuDuplicate:
                    LogConsole(
                        L"No selected object to duplicate.");

                    return true;

                case IdMenuResetLayout:
                    LayoutFromParent();

                    LogConsole(
                        L"Editor layout reset.");

                    return true;

                case IdMenuAbout:
                    MessageBoxW(
                        parentWindow_,
                        L"LastToSurvive Editor\n"
                        L"New Engine / DX11",
                        L"About",
                        MB_OK |
                        MB_ICONINFORMATION);

                    return true;

                case IdMenuExit:
                    PostMessageW(
                        parentWindow_,
                        WM_CLOSE,
                        0,
                        0);

                    return true;

                default:
                    return false;
            }
        }

        [[nodiscard]]
        bool IsActiveModeButton(
            const int controlId) const noexcept
        {
            for (
                const ModeButtonDescription&
                    description : ModeButtons
            )
            {
                if (
                    description.controlId ==
                    controlId
                )
                {
                    return
                        description.mode ==
                        activeMode_;
                }
            }

            return false;
        }

        [[nodiscard]]
        const wchar_t* GetModeButtonText(
            const int controlId) const noexcept
        {
            for (
                const ModeButtonDescription&
                    description : ModeButtons
            )
            {
                if (
                    description.controlId ==
                    controlId
                )
                {
                    return description.text;
                }
            }

            return L"";
        }

        [[nodiscard]]
        bool DrawOwnerButton(
            const DRAWITEMSTRUCT& item) const noexcept
        {
            if (item.CtlType != ODT_BUTTON)
            {
                return false;
            }

            const int controlId =
                static_cast<int>(
                    item.CtlID);

            bool knownButton = false;

            for (
                const ModeButtonDescription&
                    description : ModeButtons
            )
            {
                if (
                    description.controlId ==
                    controlId
                )
                {
                    knownButton = true;
                    break;
                }
            }

            if (!knownButton)
            {
                return false;
            }

            HBRUSH fillBrush =
                IsActiveModeButton(controlId)
                    ? accentBrush_
                    : panelBrush_;

            if (
                (item.itemState &
                    ODS_SELECTED) != 0
            )
            {
                fillBrush =
                    inputBrush_;
            }

            FillRect(
                item.hDC,
                &item.rcItem,
                fillBrush);

            FrameRect(
                item.hDC,
                &item.rcItem,
                IsActiveModeButton(controlId)
                    ? accentBrush_
                    : borderBrush_);

            SetBkMode(
                item.hDC,
                TRANSPARENT);

            SetTextColor(
                item.hDC,
                IsWindowEnabled(item.hwndItem)
                    ? RGB(235, 239, 241)
                    : RGB(105, 112, 116));

            RECT textRectangle =
                item.rcItem;

            DrawTextW(
                item.hDC,
                GetModeButtonText(controlId),
                -1,
                &textRectangle,
                DT_CENTER |
                DT_VCENTER |
                DT_SINGLELINE);

            if (
                (item.itemState &
                    ODS_FOCUS) != 0
            )
            {
                RECT focusRectangle =
                    item.rcItem;

                InflateRect(
                    &focusRectangle,
                    -3,
                    -3);

                DrawFocusRect(
                    item.hDC,
                    &focusRectangle);
            }

            return true;
        }

        static LRESULT CALLBACK ViewportWindowProcedure(
            const HWND window,
            const UINT message,
            const WPARAM wParam,
            const LPARAM lParam) noexcept
        {
            if (message == WM_NCCREATE)
            {
                const auto* const createData =
                    reinterpret_cast<const CREATESTRUCTW*>(
                        lParam);

                if (createData != nullptr)
                {
                    auto* const self =
                        static_cast<Impl*>(
                            createData->lpCreateParams);

                    SetWindowLongPtrW(
                        window,
                        GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(self));
                }
            }

            auto* const self =
                reinterpret_cast<Impl*>(
                    GetWindowLongPtrW(
                        window,
                        GWLP_USERDATA));

            switch (message)
            {
                case WM_LBUTTONDOWN:
                {
                    SetFocus(window);

                    if (self != nullptr)
                    {
                        const int mouseX =
                            GET_X_LPARAM(lParam);

                        const int mouseY =
                            GET_Y_LPARAM(lParam);

                        self->pendingViewportClick_.x =
                            mouseX >= 0
                                ? static_cast<std::uint32_t>(mouseX)
                                : 0U;

                        self->pendingViewportClick_.y =
                            mouseY >= 0
                                ? static_cast<std::uint32_t>(mouseY)
                                : 0U;

                        self->viewportClickPending_ = true;
                    }

                    return 0;
                }

                case WM_MBUTTONDOWN:
                case WM_RBUTTONDOWN:
                {
                    SetFocus(window);
                    return 0;
                }

                case WM_MOUSEWHEEL:
                {
                    if (self != nullptr)
                    {
                        const float wheelDelta =
                            static_cast<float>(
                                GET_WHEEL_DELTA_WPARAM(wParam));

                        self->viewportWheelSteps_ +=
                            wheelDelta /
                            static_cast<float>(WHEEL_DELTA);
                    }

                    return 0;
                }

                case WM_ERASEBKGND:
                    return 1;

                case WM_PAINT:
                {
                    PAINTSTRUCT paint{};

                    BeginPaint(
                        window,
                        &paint);

                    EndPaint(
                        window,
                        &paint);

                    return 0;
                }

                case WM_NCDESTROY:
                {
                    SetWindowLongPtrW(
                        window,
                        GWLP_USERDATA,
                        0);

                    break;
                }

                default:
                    break;
            }

            return DefWindowProcW(
                window,
                message,
                wParam,
                lParam);
        }

        static LRESULT CALLBACK
            WindowProcedure(
                const HWND window,
                const UINT message,
                const WPARAM wParam,
                const LPARAM lParam) noexcept
        {
            auto* self =
                static_cast<Impl*>(
                    GetPropW(
                        window,
                        ShellPropertyName));

            if (
                self == nullptr ||
                self->originalWindowProcedure_ ==
                    nullptr
            )
            {
                return DefWindowProcW(
                    window,
                    message,
                    wParam,
                    lParam);
            }

            switch (message)
            {
                case WM_COMMAND:
                {
                    const int commandId =
                        LOWORD(wParam);

                    const int notificationCode =
                        HIWORD(wParam);

                    if (
                        commandId == IdHierarchyList &&
                        notificationCode == LBN_SELCHANGE)
                    {
                        self->HandleHierarchySelectionChanged();
                        return 0;
                    }

                    if (
                        self->HandleCommand(
                            commandId)
                    )
                    {
                        return 0;
                    }

                    break;
                }

                case WM_SIZE:
                    self->LayoutFromParent();
                    break;

                case WM_DRAWITEM:
                {
                    const auto* item =
                        reinterpret_cast<
                            const DRAWITEMSTRUCT*>(
                                lParam);

                    if (
                        item != nullptr &&
                        self->DrawOwnerButton(
                            *item)
                    )
                    {
                        return TRUE;
                    }

                    break;
                }

                case WM_CTLCOLORSTATIC:
                {
                    const HDC deviceContext =
                        reinterpret_cast<HDC>(
                            wParam);

                    SetTextColor(
                        deviceContext,
                        RGB(220, 226, 229));

                    SetBkColor(
                        deviceContext,
                        RGB(28, 34, 38));

                    return reinterpret_cast<LRESULT>(
                        self->panelBrush_);
                }

                case WM_CTLCOLORLISTBOX:
                case WM_CTLCOLOREDIT:
                {
                    const HDC deviceContext =
                        reinterpret_cast<HDC>(
                            wParam);

                    SetTextColor(
                        deviceContext,
                        RGB(220, 226, 229));

                    SetBkColor(
                        deviceContext,
                        RGB(20, 25, 29));

                    return reinterpret_cast<LRESULT>(
                        self->inputBrush_);
                }

                case WM_ERASEBKGND:
                    return 1;

                case WM_PAINT:
                {
                    PAINTSTRUCT paint{};

                    HDC deviceContext =
                        BeginPaint(
                            window,
                            &paint);

                    if (deviceContext != nullptr)
                    {
                        FillRect(
                            deviceContext,
                            &paint.rcPaint,
                            self->backgroundBrush_);
                    }

                    EndPaint(
                        window,
                        &paint);

                    return 0;
                }

                default:
                    break;
            }

            return CallWindowProcW(
                self->originalWindowProcedure_,
                window,
                message,
                wParam,
                lParam);
        }

        HWND parentWindow_ = nullptr;
        HINSTANCE instance_ = nullptr;

        WNDPROC originalWindowProcedure_ = nullptr;

        HMENU mainMenu_ = nullptr;

        ATOM viewportClassAtom_ = 0;

        std::array<wchar_t, 96U>
            viewportClassName_{};

        std::array<HWND, 6U>
            modeButtons_{};

        HWND hierarchyTitle_ = nullptr;
        HWND hierarchyList_ = nullptr;

        HWND viewportTitle_ = nullptr;
        HWND viewportWindow_ = nullptr;

        HWND inspectorTitle_ = nullptr;
        HWND inspectorList_ = nullptr;

        HWND assetTitle_ = nullptr;
        HWND assetList_ = nullptr;

        HWND consoleTitle_ = nullptr;
        HWND consoleEdit_ = nullptr;

        HWND statusBar_ = nullptr;

        HFONT font_ = nullptr;

        HBRUSH backgroundBrush_ = nullptr;
        HBRUSH panelBrush_ = nullptr;
        HBRUSH inputBrush_ = nullptr;
        HBRUSH accentBrush_ = nullptr;
        HBRUSH borderBrush_ = nullptr;

        EditorMode activeMode_ =
            EditorMode::Level;

        float viewportWheelSteps_ = 0.0F;
        ViewportClick pendingViewportClick_;
        bool viewportClickPending_ = false;

        std::size_t pendingHierarchySelection_ =
            InvalidEditorEntityIndex;

        bool hierarchySelectionChanged_ = false;
        bool modeChanged_ = false;
        bool subclassInstalled_ = false;
        bool initialized_ = false;
    };

    EditorShell::EditorShell()
        : impl_(
            std::make_unique<Impl>())
    {
    }

    EditorShell::~EditorShell() noexcept =
        default;

    bool EditorShell::Initialize(
        const engine::platform::
            NativeWindowHandle mainWindow) noexcept
    {
        return impl_ != nullptr &&
            impl_->Initialize(
                mainWindow);
    }

    void EditorShell::Shutdown() noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->Shutdown();
        }
    }

    void EditorShell::Resize(
        const std::uint32_t width,
        const std::uint32_t height) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->Resize(
                width,
                height);
        }
    }

    engine::platform::NativeWindowHandle
        EditorShell::
            GetViewportWindowHandle() const noexcept
    {
        if (impl_ == nullptr)
        {
            return {};
        }

        return impl_->
            GetViewportWindowHandle();
    }

    engine::platform::WindowSize
        EditorShell::
            GetViewportSize() const noexcept
    {
        if (impl_ == nullptr)
        {
            return {};
        }

        return impl_->
            GetViewportSize();
    }

    EditorMode EditorShell::
        GetActiveMode() const noexcept
    {
        if (impl_ == nullptr)
        {
            return EditorMode::Level;
        }

        return impl_->GetActiveMode();
    }

    bool EditorShell::ConsumeModeChanged(
        EditorMode& mode) noexcept
    {
        return impl_ != nullptr &&
            impl_->ConsumeModeChanged(
                mode);
    }

    float EditorShell::
    ConsumeViewportWheelSteps() noexcept
    {
        if (impl_ == nullptr)
        {
            return 0.0F;
        }

        return impl_->
            ConsumeViewportWheelSteps();
    }

    bool EditorShell::ConsumeViewportClick(
    ViewportClick& click) noexcept
    {
        return impl_ != nullptr &&
            impl_->ConsumeViewportClick(click);
    }

    void EditorShell::RefreshScene(
        const EditorSceneDocument& document) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->RefreshScene(document);
        }
    }

    bool EditorShell::ConsumeHierarchySelection(
        std::size_t& entityIndex) noexcept
    {
        return
            impl_ != nullptr &&
            impl_->ConsumeHierarchySelection(
                entityIndex);
    }

    void EditorShell::SelectHierarchyEntity(
    const std::size_t entityIndex) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->SelectHierarchyEntity(entityIndex);
        }
    }

    void EditorShell::ShowEntityDetails(
        const EditorSceneEntity* const entity) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->ShowEntityDetails(entity);
        }
    }

    void EditorShell::SetStatusText(
        const std::wstring_view text) noexcept
    {
        if (impl_ != nullptr)
        {
            impl_->SetStatusText(
                text);
        }
    }
}