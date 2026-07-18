#pragma once

#include "Editor/EditorCameraController.h"
#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorSceneDocument.h"
#include "Editor/EditorShell.h"
#include "Editor/EditorTransformTypes.h"

#include <Platform/Window.h>

#include <DirectXMath.h>

#include <array>
#include <cstdint>
#include <string>

namespace lts::editor
{
    struct EditorInteractionResult final
    {
        bool documentChanged = false;
        bool selectionChanged = false;
        bool hierarchyChanged = false;
        bool statusChanged = false;
    };

    class EditorTransformController final
    {
    public:
        EditorTransformController() noexcept = default;
        ~EditorTransformController() noexcept;

        EditorTransformController(const EditorTransformController&) = delete;
        EditorTransformController& operator=(const EditorTransformController&) = delete;

        void SetViewportWindow(
            engine::platform::NativeWindowHandle window) noexcept;

        [[nodiscard]]
        EditorInteractionResult Update(
            EditorSceneDocument& document,
            EditorCommandHistory& history,
            EditorCameraController& camera,
            engine::platform::WindowSize viewportSize,
            const ViewportClick* viewportClick) noexcept;

        [[nodiscard]]
        const EditorTransformVisualState&
            GetVisualState() const noexcept;

        [[nodiscard]]
        std::wstring BuildStatusText() const;

        void SetOperation(EditorTransformOperation operation) noexcept;
        void ToggleSpace() noexcept;

    private:
        [[nodiscard]]
        bool IsViewportFocused() const noexcept;

        [[nodiscard]]
        bool WasPressed(int virtualKey) noexcept;

        [[nodiscard]]
        bool BuildCurrentPickRay(
            EditorCameraController& camera,
            engine::platform::WindowSize viewportSize,
            EditorPickRay& ray) const noexcept;

        [[nodiscard]]
        bool TryBeginDrag(
            EditorSceneDocument& document,
            const EditorPickRay& ray) noexcept;

        [[nodiscard]]
        bool UpdateDrag(
            EditorSceneDocument& document,
            EditorCommandHistory& history,
            EditorCameraController& camera,
            engine::platform::WindowSize viewportSize) noexcept;

        void EndDrag(
            EditorSceneDocument& document,
            EditorCommandHistory& history,
            bool cancel) noexcept;

        void UpdateHotAxis(
            const EditorSceneDocument& document,
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
    };
}
