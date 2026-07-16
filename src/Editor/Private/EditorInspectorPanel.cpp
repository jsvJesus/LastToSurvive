#include "Editor/EditorInspectorPanel.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cwchar>
#include <cwctype>

namespace lts::editor
{
    namespace
    {
        constexpr int InspectorAnchorId = 1302;

        constexpr int IdNameValue = 4101;
        constexpr int IdTypeValue = 4102;

        constexpr std::array<int, 9U> TransformEditIds
        {
            4110,
            4111,
            4112,
            4120,
            4121,
            4122,
            4130,
            4131,
            4132
        };

        [[nodiscard]]
        HWND ToWindow(const void* const value) noexcept
        {
            return static_cast<HWND>(const_cast<void*>(value));
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

        [[nodiscard]]
        bool TransformEquals(
            const EditorTransform& left,
            const EditorTransform& right) noexcept
        {
            return
                left.position == right.position &&
                left.rotationDegrees == right.rotationDegrees &&
                left.scale == right.scale;
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
                    static_cast<INT_PTR>(controlId)),
                GetModuleHandleW(nullptr),
                nullptr);
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
                    reinterpret_cast<WPARAM>(font),
                    TRUE);
            }
        }

        void DestroyWindowSafe(void*& value) noexcept
        {
            const HWND window = ToWindow(value);

            if (window != nullptr)
            {
                DestroyWindow(window);
                value = nullptr;
            }
        }

        [[nodiscard]]
        bool TryParseFloat(
            const HWND edit,
            float& value) noexcept
        {
            std::array<wchar_t, 96U> text{};

            const int length = GetWindowTextW(
                edit,
                text.data(),
                static_cast<int>(text.size()));

            if (length <= 0)
            {
                return false;
            }

            wchar_t* end = nullptr;

            const float parsed = std::wcstof(
                text.data(),
                &end);

            if (end == text.data())
            {
                return false;
            }

            while (
                end != nullptr &&
                *end != L'\0' &&
                std::iswspace(*end) != 0)
            {
                ++end;
            }

            if (
                end == nullptr ||
                *end != L'\0' ||
                !std::isfinite(parsed))
            {
                return false;
            }

            value = parsed;
            return true;
        }

        void WriteFloat(
            const HWND edit,
            const float value) noexcept
        {
            std::array<wchar_t, 64U> text{};

            const int written = swprintf_s(
                text.data(),
                text.size(),
                L"%.3f",
                value);

            if (written > 0)
            {
                SetWindowTextW(
                    edit,
                    text.data());
            }
        }
    }

    EditorInspectorPanel::~EditorInspectorPanel() noexcept
    {
        Shutdown();
    }

    bool EditorInspectorPanel::Initialize(
        const engine::platform::NativeWindowHandle mainWindow) noexcept
    {
        if (initialized_)
        {
            return true;
        }

        if (!mainWindow.IsValid())
        {
            return false;
        }

        const HWND root = reinterpret_cast<HWND>(
            mainWindow.Value());

        if (
            root == nullptr ||
            !IsWindow(root))
        {
            return false;
        }

        const HWND anchor = GetDlgItem(
            root,
            InspectorAnchorId);

        if (anchor == nullptr)
        {
            return false;
        }

        const HWND panel = CreateChild(
            root,
            WS_EX_CLIENTEDGE,
            L"STATIC",
            L"",
            SS_LEFT |
                WS_CLIPCHILDREN,
            4199);

        if (panel == nullptr)
        {
            return false;
        }

        mainWindow_ = root;
        anchorWindow_ = anchor;
        panelWindow_ = panel;

        ShowWindow(anchor, SW_HIDE);

        font_ = GetStockObject(DEFAULT_GUI_FONT);

        const HWND nameLabel = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Name",
            SS_LEFT |
                SS_CENTERIMAGE,
            4201);

        const HWND typeLabel = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Type",
            SS_LEFT |
                SS_CENTERIMAGE,
            4202);

        nameValue_ = CreateChild(
            panel,
            0,
            L"STATIC",
            L"No object selected",
            SS_LEFT |
                SS_CENTERIMAGE,
            IdNameValue);

        typeValue_ = CreateChild(
            panel,
            0,
            L"STATIC",
            L"",
            SS_LEFT |
                SS_CENTERIMAGE,
            IdTypeValue);

        groupLabels_[0] = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Location",
            SS_LEFT |
                SS_CENTERIMAGE,
            4210);

        groupLabels_[1] = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Rotation",
            SS_LEFT |
                SS_CENTERIMAGE,
            4220);

        groupLabels_[2] = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Scale",
            SS_LEFT |
                SS_CENTERIMAGE,
            4230);

        axisLabels_[0] = CreateChild(
            panel,
            0,
            L"STATIC",
            L"X",
            SS_CENTER |
                SS_CENTERIMAGE,
            4240);

        axisLabels_[1] = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Y",
            SS_CENTER |
                SS_CENTERIMAGE,
            4241);

        axisLabels_[2] = CreateChild(
            panel,
            0,
            L"STATIC",
            L"Z",
            SS_CENTER |
                SS_CENTERIMAGE,
            4242);

        for (
            std::size_t index = 0U;
            index < transformEdits_.size();
            ++index)
        {
            transformEdits_[index] = CreateChild(
                panel,
                WS_EX_CLIENTEDGE,
                L"EDIT",
                L"0.000",
                ES_LEFT |
                    ES_AUTOHSCROLL |
                    WS_TABSTOP,
                TransformEditIds[index]);
        }

        if (
            nameLabel == nullptr ||
            typeLabel == nullptr ||
            ToWindow(nameValue_) == nullptr ||
            ToWindow(typeValue_) == nullptr)
        {
            Shutdown();
            return false;
        }

        for (const void* const control : groupLabels_)
        {
            if (ToWindow(control) == nullptr)
            {
                Shutdown();
                return false;
            }
        }

        for (const void* const control : axisLabels_)
        {
            if (ToWindow(control) == nullptr)
            {
                Shutdown();
                return false;
            }
        }

        for (const void* const control : transformEdits_)
        {
            if (ToWindow(control) == nullptr)
            {
                Shutdown();
                return false;
            }
        }

        ApplyFont(nameLabel, static_cast<HFONT>(font_));
        ApplyFont(typeLabel, static_cast<HFONT>(font_));
        ApplyFont(ToWindow(nameValue_), static_cast<HFONT>(font_));
        ApplyFont(ToWindow(typeValue_), static_cast<HFONT>(font_));

        for (const void* const control : groupLabels_)
        {
            ApplyFont(
                ToWindow(control),
                static_cast<HFONT>(font_));
        }

        for (const void* const control : axisLabels_)
        {
            ApplyFont(
                ToWindow(control),
                static_cast<HFONT>(font_));
        }

        for (const void* const control : transformEdits_)
        {
            ApplyFont(
                ToWindow(control),
                static_cast<HFONT>(font_));
        }

        UpdateLayout();
        LayoutControls();

        initialized_ = true;
        return true;
    }

    void EditorInspectorPanel::Shutdown() noexcept
    {
        if (editing_)
        {
            editing_ = false;
        }

        DestroyControls();

        const HWND anchor = ToWindow(anchorWindow_);

        if (anchor != nullptr)
        {
            ShowWindow(anchor, SW_SHOW);
        }

        mainWindow_ = nullptr;
        anchorWindow_ = nullptr;
        panelWindow_ = nullptr;
        font_ = nullptr;

        displayedEntityId_ = 0U;
        displayedTransform_ = {};
        editBefore_ = {};

        initialized_ = false;
    }

    bool EditorInspectorPanel::Update(
        EditorSceneDocument& document,
        EditorCommandHistory& history) noexcept
    {
        if (!initialized_)
        {
            return false;
        }

        UpdateLayout();
        LayoutControls();

        const EditorSceneEntity* selected =
            document.GetSelectedEntity();

        const EditorEntityId selectedId =
            selected != nullptr
                ? selected->id
                : 0U;

        if (selectedId != displayedEntityId_)
        {
            FinishEditing(document, history);
            RefreshEntity(selected);
        }

        const bool focused = IsEditFocused();
        bool changed = false;

        if (
            focused &&
            selected != nullptr)
        {
            if (!editing_)
            {
                editBefore_ = document.CreateSnapshot();
                editing_ = true;
            }

            EditorTransform transform;

            if (
                TryReadTransform(transform) &&
                document.SetSelectedTransform(transform))
            {
                displayedTransform_ = transform;
                changed = true;
            }
        }
        else
        {
            FinishEditing(document, history);

            selected = document.GetSelectedEntity();

            if (
                selected != nullptr &&
                !TransformEquals(
                    selected->transform,
                    displayedTransform_))
            {
                WriteTransform(selected->transform);
                displayedTransform_ = selected->transform;
            }
        }

        return changed;
    }

    void EditorInspectorPanel::Refresh(
        const EditorSceneDocument& document) noexcept
    {
        if (!initialized_)
        {
            return;
        }

        RefreshEntity(
            document.GetSelectedEntity());
    }

    void EditorInspectorPanel::UpdateLayout() noexcept
    {
        const HWND root = ToWindow(mainWindow_);
        const HWND anchor = ToWindow(anchorWindow_);
        const HWND panel = ToWindow(panelWindow_);

        if (
            root == nullptr ||
            anchor == nullptr ||
            panel == nullptr)
        {
            return;
        }

        RECT rectangle{};

        if (!GetWindowRect(anchor, &rectangle))
        {
            return;
        }

        POINT points[2]
        {
            {rectangle.left, rectangle.top},
            {rectangle.right, rectangle.bottom}
        };

        MapWindowPoints(
            nullptr,
            root,
            points,
            2U);

        const int width = static_cast<int>(
            std::max(points[1].x - points[0].x, 1L));

        const int height = static_cast<int>(
            std::max(points[1].y - points[0].y, 1L));

        MoveWindow(
            panel,
            static_cast<int>(points[0].x),
            static_cast<int>(points[0].y),
            width,
            height,
            TRUE);
    }

    void EditorInspectorPanel::LayoutControls() noexcept
    {
        const HWND panel = ToWindow(panelWindow_);

        if (panel == nullptr)
        {
            return;
        }

        RECT rectangle{};

        if (!GetClientRect(panel, &rectangle))
        {
            return;
        }

        const int width = static_cast<int>(
            std::max(
                rectangle.right - rectangle.left,
                1L));

        constexpr int margin = 8;
        constexpr int rowHeight = 24;
        constexpr int labelWidth = 66;
        constexpr int gap = 4;

        int y = margin;

        const HWND nameLabel = GetDlgItem(panel, 4201);
        const HWND typeLabel = GetDlgItem(panel, 4202);

        MoveWindow(
            nameLabel,
            margin,
            y,
            labelWidth,
            rowHeight,
            TRUE);

        MoveWindow(
            ToWindow(nameValue_),
            margin + labelWidth,
            y,
            std::max(width - margin * 2 - labelWidth, 1),
            rowHeight,
            TRUE);

        y += rowHeight + gap;

        MoveWindow(
            typeLabel,
            margin,
            y,
            labelWidth,
            rowHeight,
            TRUE);

        MoveWindow(
            ToWindow(typeValue_),
            margin + labelWidth,
            y,
            std::max(width - margin * 2 - labelWidth, 1),
            rowHeight,
            TRUE);

        y += rowHeight + margin;

        const int available = std::max(
            width - margin * 2 - labelWidth - gap * 2,
            3);

        const int editWidth = std::max(
            available / 3,
            1);

        for (std::size_t group = 0U; group < 3U; ++group)
        {
            MoveWindow(
                ToWindow(groupLabels_[group]),
                margin,
                y,
                labelWidth,
                rowHeight,
                TRUE);

            for (std::size_t axis = 0U; axis < 3U; ++axis)
            {
                const int x =
                    margin +
                    labelWidth +
                    static_cast<int>(axis) *
                        (editWidth + gap);

                MoveWindow(
                    ToWindow(axisLabels_[axis]),
                    x,
                    y,
                    editWidth,
                    16,
                    TRUE);

                MoveWindow(
                    ToWindow(transformEdits_[group * 3U + axis]),
                    x,
                    y + 16,
                    editWidth,
                    rowHeight,
                    TRUE);
            }

            y += rowHeight + 22;
        }
    }

    void EditorInspectorPanel::SetVisible(
        const bool visible) noexcept
    {
        const int command = visible
            ? SW_SHOW
            : SW_HIDE;

        for (void* const edit : transformEdits_)
        {
            ShowWindow(
                ToWindow(edit),
                command);
        }

        for (void* const label : groupLabels_)
        {
            ShowWindow(
                ToWindow(label),
                command);
        }

        for (void* const label : axisLabels_)
        {
            ShowWindow(
                ToWindow(label),
                command);
        }
    }

    void EditorInspectorPanel::RefreshEntity(
        const EditorSceneEntity* const entity) noexcept
    {
        if (entity == nullptr)
        {
            displayedEntityId_ = 0U;
            displayedTransform_ = {};

            SetWindowTextW(
                ToWindow(nameValue_),
                L"No object selected");

            SetWindowTextW(
                ToWindow(typeValue_),
                L"");

            SetVisible(false);
            return;
        }

        displayedEntityId_ = entity->id;
        displayedTransform_ = entity->transform;

        SetWindowTextW(
            ToWindow(nameValue_),
            entity->name.c_str());

        SetWindowTextW(
            ToWindow(typeValue_),
            GetEntityKindName(entity->kind));

        WriteTransform(entity->transform);
        SetVisible(true);
    }

    bool EditorInspectorPanel::IsEditFocused() const noexcept
    {
        const HWND focus = GetFocus();

        for (const void* const edit : transformEdits_)
        {
            if (focus == ToWindow(edit))
            {
                return true;
            }
        }

        return false;
    }

    bool EditorInspectorPanel::TryReadTransform(
        EditorTransform& transform) const noexcept
    {
        for (std::size_t axis = 0U; axis < 3U; ++axis)
        {
            if (!TryParseFloat(
                    ToWindow(transformEdits_[axis]),
                    transform.position[axis]))
            {
                return false;
            }

            if (!TryParseFloat(
                    ToWindow(transformEdits_[3U + axis]),
                    transform.rotationDegrees[axis]))
            {
                return false;
            }

            if (!TryParseFloat(
                    ToWindow(transformEdits_[6U + axis]),
                    transform.scale[axis]))
            {
                return false;
            }

            if (std::abs(transform.scale[axis]) < 0.001F)
            {
                return false;
            }
        }

        return true;
    }

    void EditorInspectorPanel::WriteTransform(
        const EditorTransform& transform) noexcept
    {
        for (std::size_t axis = 0U; axis < 3U; ++axis)
        {
            WriteFloat(
                ToWindow(transformEdits_[axis]),
                transform.position[axis]);

            WriteFloat(
                ToWindow(transformEdits_[3U + axis]),
                transform.rotationDegrees[axis]);

            WriteFloat(
                ToWindow(transformEdits_[6U + axis]),
                transform.scale[axis]);
        }
    }

    void EditorInspectorPanel::FinishEditing(
        EditorSceneDocument& document,
        EditorCommandHistory& history) noexcept
    {
        if (!editing_)
        {
            return;
        }

        static_cast<void>(
            history.Push(
                editBefore_,
                document.CreateSnapshot()));

        editing_ = false;
        editBefore_ = {};
    }

    void EditorInspectorPanel::DestroyControls() noexcept
    {
        for (void*& edit : transformEdits_)
        {
            DestroyWindowSafe(edit);
        }

        for (void*& label : groupLabels_)
        {
            DestroyWindowSafe(label);
        }

        for (void*& label : axisLabels_)
        {
            DestroyWindowSafe(label);
        }

        DestroyWindowSafe(nameValue_);
        DestroyWindowSafe(typeValue_);
        DestroyWindowSafe(panelWindow_);
    }
}
