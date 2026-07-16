#include "Editor/EditorTransformController.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <cmath>

namespace lts::editor
{
    namespace
    {
        [[nodiscard]]
        HWND ToWindowHandle(
            const engine::platform::NativeWindowHandle handle) noexcept
        {
            return reinterpret_cast<HWND>(handle.Value());
        }

        [[nodiscard]]
        bool IsKeyDown(const int virtualKey) noexcept
        {
            return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        }

        [[nodiscard]]
        bool HasViewportInputFocus(const HWND viewportWindow) noexcept
        {
            if (
                viewportWindow == nullptr ||
                !IsWindow(viewportWindow))
            {
                return false;
            }

            const HWND rootWindow = GetAncestor(
                viewportWindow,
                GA_ROOT);

            return
                rootWindow != nullptr &&
                GetForegroundWindow() == rootWindow &&
                GetFocus() == viewportWindow;
        }
    }

    void EditorTransformController::SetViewportWindow(
        const engine::platform::NativeWindowHandle window) noexcept
    {
        viewportWindow_ = window;
    }

    bool EditorTransformController::Update(
        EditorSceneDocument& document,
        const double deltaSeconds) noexcept
    {
        if (
            !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0)
        {
            return false;
        }

        if (document.GetSelectedEntity() == nullptr)
        {
            return false;
        }

        const HWND viewportWindow =
            ToWindowHandle(viewportWindow_);

        if (!HasViewportInputFocus(viewportWindow))
        {
            return false;
        }

        float movementX = 0.0F;
        float movementY = 0.0F;
        float movementZ = 0.0F;

        if (IsKeyDown(VK_LEFT))
        {
            movementX -= 1.0F;
        }

        if (IsKeyDown(VK_RIGHT))
        {
            movementX += 1.0F;
        }

        if (IsKeyDown(VK_UP))
        {
            movementZ += 1.0F;
        }

        if (IsKeyDown(VK_DOWN))
        {
            movementZ -= 1.0F;
        }

        if (IsKeyDown(VK_PRIOR))
        {
            movementY += 1.0F;
        }

        if (IsKeyDown(VK_NEXT))
        {
            movementY -= 1.0F;
        }

        const float movementLengthSquared =
            movementX * movementX +
            movementY * movementY +
            movementZ * movementZ;

        if (movementLengthSquared <= 0.000001F)
        {
            return false;
        }

        const float movementLength =
            std::sqrt(movementLengthSquared);

        movementX /= movementLength;
        movementY /= movementLength;
        movementZ /= movementLength;

        const bool fastMovement =
            IsKeyDown(VK_SHIFT) ||
            IsKeyDown(VK_LSHIFT) ||
            IsKeyDown(VK_RSHIFT);

        const float movementSpeed =
            fastMovement
                ? 12.0F
                : 4.0F;

        const float safeDeltaSeconds =
            static_cast<float>(
                deltaSeconds > 0.1
                    ? 0.1
                    : deltaSeconds);

        const float distance =
            movementSpeed *
            safeDeltaSeconds;

        return document.TranslateSelectedEntity(
            movementX * distance,
            movementY * distance,
            movementZ * distance);
    }
}