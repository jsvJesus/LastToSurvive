#include "Editor/EditorCameraController.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>

namespace lts::editor
{
    namespace
    {
        constexpr float MinimumPitch =
            -1.55334306F;

        constexpr float MaximumPitch =
            1.55334306F;

        constexpr float MinimumMoveSpeed =
            1.0F;

        constexpr float MaximumMoveSpeed =
            250.0F;

        constexpr float FastMoveMultiplier =
            4.0F;

        [[nodiscard]]
        bool IsKeyDown(
            const int virtualKey) noexcept
        {
            return
                (GetAsyncKeyState(
                    virtualKey) & 0x8000) != 0;
        }

        [[nodiscard]]
        HWND ToWindowHandle(
            const engine::platform::
                NativeWindowHandle handle) noexcept
        {
            return reinterpret_cast<HWND>(
                handle.Value());
        }

        [[nodiscard]]
        DirectX::XMVECTOR BuildForwardVector(
            const float yaw,
            const float pitch) noexcept
        {
            const float cosPitch =
                std::cos(pitch);

            return DirectX::XMVector3Normalize(
                DirectX::XMVectorSet(
                    std::sin(yaw) * cosPitch,
                    std::sin(pitch),
                    std::cos(yaw) * cosPitch,
                    0.0F));
        }

        [[nodiscard]]
        bool GetViewportCenterOnScreen(
            const HWND window,
            POINT& outCenter) noexcept
        {
            RECT clientRectangle{};

            if (!GetClientRect(
                    window,
                    &clientRectangle))
            {
                return false;
            }

            POINT center{};

            center.x =
                (clientRectangle.right -
                    clientRectangle.left) / 2;

            center.y =
                (clientRectangle.bottom -
                    clientRectangle.top) / 2;

            if (!ClientToScreen(
                    window,
                    &center))
            {
                return false;
            }

            outCenter = center;
            return true;
        }
    }

    EditorCameraController::
        EditorCameraController() noexcept =
            default;

    EditorCameraController::
        ~EditorCameraController() noexcept
    {
        EndMouseLook();
    }

    void EditorCameraController::
        SetViewportWindow(
            const engine::platform::
                NativeWindowHandle window) noexcept
    {
        if (
            viewportWindow_.Value() ==
            window.Value()
        )
        {
            return;
        }

        EndMouseLook();
        viewportWindow_ = window;
    }

    void EditorCameraController::Reset() noexcept
    {
        EndMouseLook();

        position_ =
        {
            18.0F,
            14.0F,
            -18.0F
        };

        yawRadians_ =
            -0.785398163F;

        pitchRadians_ =
            -0.488692191F;

        moveSpeed_ = 10.0F;
    }

    void EditorCameraController::Update(
        const double deltaSeconds,
        const float wheelSteps) noexcept
    {
        if (
            !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0
        )
        {
            return;
        }

        if (
            std::isfinite(wheelSteps) &&
            wheelSteps != 0.0F
        )
        {
            moveSpeed_ *=
                std::pow(
                    1.20F,
                    wheelSteps);

            moveSpeed_ =
                std::clamp(
                    moveSpeed_,
                    MinimumMoveSpeed,
                    MaximumMoveSpeed);
        }

        const HWND viewportWindow =
            ToWindowHandle(
                viewportWindow_);

        if (
            viewportWindow == nullptr ||
            !IsWindow(viewportWindow)
        )
        {
            EndMouseLook();
            return;
        }

        const HWND rootWindow =
            GetAncestor(
                viewportWindow,
                GA_ROOT);

        const bool editorIsForeground =
            rootWindow != nullptr &&
            GetForegroundWindow() ==
                rootWindow;

        const bool viewportHasFocus =
            GetFocus() ==
                viewportWindow;

        const bool rightMouseDown =
            IsKeyDown(
                VK_RBUTTON);

        if (
            !editorIsForeground ||
            !viewportHasFocus ||
            !rightMouseDown
        )
        {
            EndMouseLook();
            return;
        }

        POINT viewportCenter{};

        if (
            !GetViewportCenterOnScreen(
                viewportWindow,
                viewportCenter)
        )
        {
            EndMouseLook();
            return;
        }

        if (!mouseLookActive_)
        {
            POINT currentCursor{};

            if (GetCursorPos(
                    &currentCursor))
            {
                restoreCursorX_ =
                    static_cast<std::int32_t>(
                        currentCursor.x);

                restoreCursorY_ =
                    static_cast<std::int32_t>(
                        currentCursor.y);
            }

            SetCapture(
                viewportWindow);

            SetCursorPos(
                viewportCenter.x,
                viewportCenter.y);

            mouseLookActive_ = true;
        }
        else
        {
            POINT cursorPosition{};

            if (GetCursorPos(
                    &cursorPosition))
            {
                const LONG deltaX =
                    cursorPosition.x -
                    viewportCenter.x;

                const LONG deltaY =
                    cursorPosition.y -
                    viewportCenter.y;

                yawRadians_ +=
                    static_cast<float>(
                        deltaX) *
                    lookSensitivity_;

                pitchRadians_ -=
                    static_cast<float>(
                        deltaY) *
                    lookSensitivity_;

                pitchRadians_ =
                    std::clamp(
                        pitchRadians_,
                        MinimumPitch,
                        MaximumPitch);
            }

            SetCursorPos(
                viewportCenter.x,
                viewportCenter.y);
        }

        const float safeDeltaSeconds =
            static_cast<float>(
                std::min(
                    deltaSeconds,
                    0.1));

        const DirectX::XMVECTOR worldUp =
            DirectX::XMVectorSet(
                0.0F,
                1.0F,
                0.0F,
                0.0F);

        const DirectX::XMVECTOR forward =
            BuildForwardVector(
                yawRadians_,
                pitchRadians_);

        const DirectX::XMVECTOR right =
            DirectX::XMVector3Normalize(
                DirectX::XMVector3Cross(
                    worldUp,
                    forward));

        DirectX::XMVECTOR movement =
            DirectX::XMVectorZero();

        if (IsKeyDown('W'))
        {
            movement =
                DirectX::XMVectorAdd(
                    movement,
                    forward);
        }

        if (IsKeyDown('S'))
        {
            movement =
                DirectX::XMVectorSubtract(
                    movement,
                    forward);
        }

        if (IsKeyDown('D'))
        {
            movement =
                DirectX::XMVectorAdd(
                    movement,
                    right);
        }

        if (IsKeyDown('A'))
        {
            movement =
                DirectX::XMVectorSubtract(
                    movement,
                    right);
        }

        if (IsKeyDown('E'))
        {
            movement =
                DirectX::XMVectorAdd(
                    movement,
                    worldUp);
        }

        if (IsKeyDown('Q'))
        {
            movement =
                DirectX::XMVectorSubtract(
                    movement,
                    worldUp);
        }

        const float movementLengthSquared =
            DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(
                    movement));

        if (movementLengthSquared <= 0.000001F)
        {
            return;
        }

        movement =
            DirectX::XMVector3Normalize(
                movement);

        float currentSpeed =
            moveSpeed_;

        if (
            IsKeyDown(VK_SHIFT) ||
            IsKeyDown(VK_LSHIFT) ||
            IsKeyDown(VK_RSHIFT)
        )
        {
            currentSpeed *=
                FastMoveMultiplier;
        }

        DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(
                &position_);

        position =
            DirectX::XMVectorMultiplyAdd(
                movement,
                DirectX::XMVectorReplicate(
                    currentSpeed *
                    safeDeltaSeconds),
                position);

        DirectX::XMStoreFloat3(
            &position_,
            position);
    }

    bool EditorCameraController::
        BuildViewProjection(
            const std::uint32_t viewportWidth,
            const std::uint32_t viewportHeight,
            DirectX::XMFLOAT4X4&
                outViewProjection) const noexcept
    {
        if (
            viewportWidth == 0U ||
            viewportHeight == 0U
        )
        {
            return false;
        }

        const float aspectRatio =
            static_cast<float>(
                viewportWidth) /
            static_cast<float>(
                viewportHeight);

        if (
            !std::isfinite(aspectRatio) ||
            aspectRatio <= 0.0F
        )
        {
            return false;
        }

        const DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(
                &position_);

        const DirectX::XMVECTOR forward =
            BuildForwardVector(
                yawRadians_,
                pitchRadians_);

        const DirectX::XMVECTOR up =
            DirectX::XMVectorSet(
                0.0F,
                1.0F,
                0.0F,
                0.0F);

        const DirectX::XMMATRIX view =
            DirectX::XMMatrixLookToLH(
                position,
                forward,
                up);

        const DirectX::XMMATRIX projection =
            DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XMConvertToRadians(
                    60.0F),
                aspectRatio,
                0.1F,
                2000.0F);

        const DirectX::XMMATRIX
            viewProjection =
                DirectX::XMMatrixMultiply(
                    view,
                    projection);

        DirectX::XMStoreFloat4x4(
            &outViewProjection,
            viewProjection);

        return true;
    }

    float EditorCameraController::
        GetMoveSpeed() const noexcept
    {
        return moveSpeed_;
    }

    void EditorCameraController::
        EndMouseLook() noexcept
    {
        if (!mouseLookActive_)
        {
            return;
        }

        const HWND viewportWindow =
            ToWindowHandle(
                viewportWindow_);

        if (
            viewportWindow != nullptr &&
            GetCapture() == viewportWindow
        )
        {
            ReleaseCapture();
        }

        SetCursorPos(
            restoreCursorX_,
            restoreCursorY_);

        mouseLookActive_ = false;
    }
}