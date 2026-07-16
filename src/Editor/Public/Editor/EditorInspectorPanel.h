#pragma once

#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorSceneDocument.h"

#include <Platform/Window.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace lts::editor
{
    class EditorInspectorPanel final
    {
    public:
        EditorInspectorPanel() noexcept = default;
        ~EditorInspectorPanel() noexcept;

        EditorInspectorPanel(const EditorInspectorPanel&) = delete;
        EditorInspectorPanel& operator=(const EditorInspectorPanel&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::platform::NativeWindowHandle mainWindow) noexcept;

        void Shutdown() noexcept;

        [[nodiscard]]
        bool Update(
            EditorSceneDocument& document,
            EditorCommandHistory& history) noexcept;

        void Refresh(
            const EditorSceneDocument& document) noexcept;

    private:
        void UpdateLayout() noexcept;
        void LayoutControls() noexcept;

        void SetVisible(bool visible) noexcept;

        void RefreshEntity(
            const EditorSceneEntity* entity) noexcept;

        [[nodiscard]]
        bool IsEditFocused() const noexcept;

        [[nodiscard]]
        bool TryReadTransform(
            EditorTransform& transform) const noexcept;

        void WriteTransform(
            const EditorTransform& transform) noexcept;

        void FinishEditing(
            EditorSceneDocument& document,
            EditorCommandHistory& history) noexcept;

        void DestroyControls() noexcept;

        void* mainWindow_ = nullptr;
        void* anchorWindow_ = nullptr;
        void* panelWindow_ = nullptr;

        void* nameValue_ = nullptr;
        void* typeValue_ = nullptr;

        std::array<void*, 9U> transformEdits_{};
        std::array<void*, 3U> groupLabels_{};
        std::array<void*, 3U> axisLabels_{};

        void* font_ = nullptr;

        EditorEntityId displayedEntityId_ = 0U;
        EditorTransform displayedTransform_;
        EditorSceneSnapshot editBefore_;

        bool editing_ = false;
        bool initialized_ = false;
    };
}
