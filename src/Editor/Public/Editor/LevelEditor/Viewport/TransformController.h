#pragma once

#include "Editor/LevelEditor/Viewport/CameraController.h"
#include "Editor/Commands/CommandHistory.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Viewport/TransformTypes.h"

#include <Platform/Window.h>

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <string>

namespace lts::editor
{
    class StaticMeshRenderer;
    struct EditorInteractionResult final
    {
        bool documentChanged = false;
        bool selectionChanged = false;
        bool hierarchyChanged = false;
        bool statusChanged = false;
    };

    class TransformController final
    {
    public:
        TransformController() noexcept = default;
        ~TransformController() noexcept;

        TransformController(const TransformController&) = delete;
        TransformController& operator=(const TransformController&) = delete;

        void SetViewportWindow(
            engine::platform::NativeWindowHandle window) noexcept;

        void SetViewportRegion(
            std::int32_t x,
            std::int32_t y,
            std::uint32_t width,
            std::uint32_t height) noexcept;

        [[nodiscard]]
        EditorInteractionResult Update(
            SceneDocument& document,
            CommandHistory& history,
            CameraController& camera,
            engine::platform::WindowSize viewportSize,
            const ViewportClick* viewportClick,
            const StaticMeshRenderer* meshRenderer = nullptr) noexcept;

        [[nodiscard]]
        const EditorTransformVisualState&
            GetVisualState() const noexcept;

        [[nodiscard]]
        std::wstring BuildStatusText() const;

        void SetOperation(EditorTransformOperation operation) noexcept;

        void SetKeyboardShortcutsEnabled(bool enabled) noexcept;
        [[nodiscard]] bool AreKeyboardShortcutsEnabled() const noexcept;

        void SetEditorRoadPickingEnabled(bool enabled) noexcept;
        [[nodiscard]] bool IsEditorRoadPickingEnabled() const noexcept;

        void SetEditorWaterPlanePickingEnabled(
            bool enabled) noexcept;

        [[nodiscard]]
        bool IsEditorWaterPlanePickingEnabled() const noexcept;

        void ToggleSpace() noexcept;
        void SetSnappingEnabled(bool enabled) noexcept;
        [[nodiscard]] bool IsSnappingEnabled() const noexcept;
        void SetSnapSteps(float move, float rotate, float scale) noexcept;
        [[nodiscard]] std::array<float, 3U> GetSnapSteps() const noexcept;

    private:
        [[nodiscard]]
        bool IsViewportFocused() const noexcept;

        [[nodiscard]]
        bool WasPressed(int virtualKey) noexcept;

        [[nodiscard]]
        bool BuildCurrentPickRay(
            CameraController& camera,
            engine::platform::WindowSize viewportSize,
            EditorPickRay& ray) const noexcept;

        [[nodiscard]]
        bool TryBeginDrag(
            SceneDocument& document,
            const EditorPickRay& ray) noexcept;

        [[nodiscard]]
        bool UpdateDrag(
            SceneDocument& document,
            CommandHistory& history,
            CameraController& camera,
            engine::platform::WindowSize viewportSize) noexcept;

        void EndDrag(
            SceneDocument& document,
            CommandHistory& history,
            bool cancel) noexcept;

        void UpdateHotAxis(
            const SceneDocument& document,
            const EditorPickRay& ray) noexcept;

        [[nodiscard]]
        EditorTransformAxis PickAxis(
            const EditorSceneEntity& entity,
            const EditorPickRay& ray,
            float& parameter) const noexcept;

        [[nodiscard]]
        DirectX::XMFLOAT3 GetAxisVector(
            const EditorTransform& transform,
            EditorTransformAxis axis) const noexcept;

        engine::platform::NativeWindowHandle viewportWindow_;
        std::int32_t viewportX_ = 0;
        std::int32_t viewportY_ = 0;

        EditorTransformVisualState visualState_;

        std::array<bool, 256U> previousKeyDown_{};

        EditorSceneSnapshot dragBefore_;
        EditorTransform dragStartTransform_;

        DirectX::XMFLOAT3 dragOrigin_{};
        DirectX::XMFLOAT3 dragAxis_{};
        DirectX::XMFLOAT3 dragPlaneNormal_{};
        DirectX::XMFLOAT3 dragStartVector_{};

        float dragStartParameter_ = 0.0F;

        bool dragChanged_ = false;
        bool keyboardShortcutsEnabled_ = true;
        bool editorRoadPickingEnabled_ = true;
        bool editorWaterPlanePickingEnabled_ = true;
        bool snappingEnabled_ = false;
        float moveSnap_ = 0.5F;
        float rotateSnap_ = 15.0F;
        float scaleSnap_ = 0.1F;
    };
}