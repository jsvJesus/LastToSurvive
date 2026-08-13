#include "Editor/LevelEditor/Viewport/CameraController.h"

#include <Windows.h>

#include <algorithm>
#include <cmath>

namespace lts::editor
{
    namespace
    {
        constexpr float MinimumPitch = -1.55334306F;
        constexpr float MaximumPitch = 1.55334306F;
        constexpr float MinimumMoveSpeed = 1.0F;
        constexpr float MaximumMoveSpeed = 250.0F;
        constexpr float FastMoveMultiplier = 4.0F;

        [[nodiscard]]
        bool IsKeyDown(const int virtualKey) noexcept
        {
            return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
        }

        [[nodiscard]]
        HWND ToWindowHandle(
            const engine::platform::NativeWindowHandle handle) noexcept
        {
            return reinterpret_cast<HWND>(handle.Value());
        }

        [[nodiscard]]
        DirectX::XMVECTOR BuildForwardVector(
            const float yaw,
            const float pitch) noexcept
        {
            const float cosPitch = std::cos(pitch);

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

            if (!GetClientRect(window, &clientRectangle))
            {
                return false;
            }

            POINT center{};
            center.x = (clientRectangle.right - clientRectangle.left) / 2;
            center.y = (clientRectangle.bottom - clientRectangle.top) / 2;

            if (!ClientToScreen(window, &center))
            {
                return false;
            }

            outCenter = center;
            return true;
        }
    }

    CameraController::CameraController() noexcept = default;

    CameraController::~CameraController() noexcept
    {
        EndMouseLook();
    }

    void CameraController::SetViewportWindow(
        const engine::platform::NativeWindowHandle window) noexcept
    {
        if (viewportWindow_.Value() == window.Value())
        {
            return;
        }

        EndMouseLook();
        viewportWindow_ = window;
    }

    void CameraController::Reset() noexcept
    {
        EndMouseLook();

        position_ = {18.0F, 14.0F, -18.0F};
        yawRadians_ = -0.785398163F;
        pitchRadians_ = -0.488692191F;
        moveSpeed_ = 10.0F;
    }

    void CameraController::FocusOn(
        const DirectX::XMFLOAT3& target,
        const float distance) noexcept
    {
        if (
            !std::isfinite(target.x) ||
            !std::isfinite(target.y) ||
            !std::isfinite(target.z) ||
            !std::isfinite(distance) ||
            distance <= 0.0F)
        {
            return;
        }

        EndMouseLook();

        const DirectX::XMVECTOR forward =
            BuildForwardVector(yawRadians_, pitchRadians_);

        const DirectX::XMVECTOR targetVector =
            DirectX::XMLoadFloat3(&target);

        const DirectX::XMVECTOR position =
            DirectX::XMVectorSubtract(
                targetVector,
                DirectX::XMVectorScale(
                    forward,
                    std::clamp(distance, 1.0F, 500.0F)));

        DirectX::XMStoreFloat3(&position_, position);
    }

    void CameraController::Update(
        const double deltaSeconds,
        const float wheelSteps) noexcept
    {
        if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0)
        {
            return;
        }

        if (std::isfinite(wheelSteps) && wheelSteps != 0.0F)
        {
            moveSpeed_ *= std::pow(1.20F, wheelSteps);
            moveSpeed_ = std::clamp(
                moveSpeed_,
                MinimumMoveSpeed,
                MaximumMoveSpeed);
        }

        const HWND viewportWindow = ToWindowHandle(viewportWindow_);

        if (viewportWindow == nullptr || !IsWindow(viewportWindow))
        {
            EndMouseLook();
            return;
        }

        const HWND rootWindow = GetAncestor(viewportWindow, GA_ROOT);

        const bool editorIsForeground =
            rootWindow != nullptr &&
            GetForegroundWindow() == rootWindow;

        const bool viewportHasFocus =
            GetFocus() == viewportWindow;

        const bool rightMouseDown =
            IsKeyDown(VK_RBUTTON);

        const bool controlDown =
            IsKeyDown(VK_CONTROL) ||
            IsKeyDown(VK_LCONTROL) ||
            IsKeyDown(VK_RCONTROL);

        if (
            !editorIsForeground ||
            !viewportHasFocus ||
            !rightMouseDown ||
            controlDown)
        {
            EndMouseLook();
            return;
        }

        if (!mouseLookActive_)
        {
            POINT currentCursor{};

            if (GetCursorPos(&currentCursor))
            {
                lastCursorX_ = static_cast<std::int32_t>(currentCursor.x);
                lastCursorY_ = static_cast<std::int32_t>(currentCursor.y);
            }

            SetCapture(viewportWindow);
            mouseLookActive_ = true;
        }
        else
        {
            POINT cursorPosition{};

            if (GetCursorPos(&cursorPosition))
            {
                const LONG deltaX = cursorPosition.x - lastCursorX_;
                const LONG deltaY = cursorPosition.y - lastCursorY_;

                lastCursorX_ = static_cast<std::int32_t>(cursorPosition.x);
                lastCursorY_ = static_cast<std::int32_t>(cursorPosition.y);

                yawRadians_ +=
                    static_cast<float>(deltaX) *
                    lookSensitivity_;

                pitchRadians_ -=
                    static_cast<float>(deltaY) *
                    lookSensitivity_;

                pitchRadians_ = std::clamp(
                    pitchRadians_,
                    MinimumPitch,
                    MaximumPitch);
            }

        }

        const float safeDeltaSeconds =
            static_cast<float>(std::min(deltaSeconds, 0.1));

        const DirectX::XMVECTOR worldUp =
            DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);

        const DirectX::XMVECTOR forward =
            BuildForwardVector(yawRadians_, pitchRadians_);

        const DirectX::XMVECTOR right =
            DirectX::XMVector3Normalize(
                DirectX::XMVector3Cross(worldUp, forward));

        DirectX::XMVECTOR movement =
            DirectX::XMVectorZero();

        if (IsKeyDown('W'))
        {
            movement = DirectX::XMVectorAdd(movement, forward);
        }

        if (IsKeyDown('S'))
        {
            movement = DirectX::XMVectorSubtract(movement, forward);
        }

        if (IsKeyDown('D'))
        {
            movement = DirectX::XMVectorAdd(movement, right);
        }

        if (IsKeyDown('A'))
        {
            movement = DirectX::XMVectorSubtract(movement, right);
        }

        if (IsKeyDown('E'))
        {
            movement = DirectX::XMVectorAdd(movement, worldUp);
        }

        if (IsKeyDown('Q'))
        {
            movement = DirectX::XMVectorSubtract(movement, worldUp);
        }

        const float movementLengthSquared =
            DirectX::XMVectorGetX(
                DirectX::XMVector3LengthSq(movement));

        if (movementLengthSquared <= 0.000001F)
        {
            return;
        }

        movement = DirectX::XMVector3Normalize(movement);

        float currentSpeed = moveSpeed_;

        if (
            IsKeyDown(VK_SHIFT) ||
            IsKeyDown(VK_LSHIFT) ||
            IsKeyDown(VK_RSHIFT))
        {
            currentSpeed *= FastMoveMultiplier;
        }

        DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(&position_);

        position = DirectX::XMVectorMultiplyAdd(
            movement,
            DirectX::XMVectorReplicate(
                currentSpeed *
                safeDeltaSeconds),
            position);

        DirectX::XMStoreFloat3(&position_, position);
    }

    bool CameraController::BuildViewProjection(
        const std::uint32_t viewportWidth,
        const std::uint32_t viewportHeight,
        DirectX::XMFLOAT4X4& outViewProjection) const noexcept
    {
        if (viewportWidth == 0U || viewportHeight == 0U)
        {
            return false;
        }

        const float aspectRatio =
            static_cast<float>(viewportWidth) /
            static_cast<float>(viewportHeight);

        if (!std::isfinite(aspectRatio) || aspectRatio <= 0.0F)
        {
            return false;
        }

        const DirectX::XMVECTOR position =
            DirectX::XMLoadFloat3(&position_);

        const DirectX::XMVECTOR forward =
            BuildForwardVector(yawRadians_, pitchRadians_);

        const DirectX::XMVECTOR up =
            DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);

        const DirectX::XMMATRIX view =
            DirectX::XMMatrixLookToLH(
                position,
                forward,
                up);

        /*
         * Reverse-Z:
         * физическая near plane = 0.1,
         * физическая far plane = 2000.
         *
         * DirectXMath создаёт reverse-Z projection,
         * когда NearZ передаётся больше FarZ.
        */
        constexpr float PhysicalNearPlane =
            0.1F;

        constexpr float PhysicalFarPlane =
            2000.0F;

        const DirectX::XMMATRIX projection =
            DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XMConvertToRadians(
                    60.0F),
                aspectRatio,
                PhysicalFarPlane,
                PhysicalNearPlane);

        const DirectX::XMMATRIX viewProjection =
            DirectX::XMMatrixMultiply(
                view,
                projection);

        DirectX::XMStoreFloat4x4(
            &outViewProjection,
            viewProjection);

        return true;
    }

    bool CameraController::BuildPickRay(
        const std::uint32_t mouseX,
        const std::uint32_t mouseY,
        const std::uint32_t viewportWidth,
        const std::uint32_t viewportHeight,
        EditorPickRay& outRay) const noexcept
    {
        if (
            viewportWidth == 0U ||
            viewportHeight == 0U ||
            mouseX >= viewportWidth ||
            mouseY >= viewportHeight)
        {
            return false;
        }

        const float width = static_cast<float>(viewportWidth);
        const float height = static_cast<float>(viewportHeight);

        const float normalizedX =
            ((static_cast<float>(mouseX) + 0.5F) / width) *
            2.0F - 1.0F;

        const float normalizedY =
            1.0F -
            ((static_cast<float>(mouseY) + 0.5F) / height) *
            2.0F;

        const float aspectRatio = width / height;

        constexpr float FieldOfViewDegrees = 60.0F;

        const float tangentHalfFieldOfView =
            std::tan(
                DirectX::XMConvertToRadians(
                    FieldOfViewDegrees) *
                0.5F);

        const float viewX =
            normalizedX *
            aspectRatio *
            tangentHalfFieldOfView;

        const float viewY =
            normalizedY *
            tangentHalfFieldOfView;

        const DirectX::XMVECTOR worldUp =
            DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);

        const DirectX::XMVECTOR forward =
            BuildForwardVector(yawRadians_, pitchRadians_);

        const DirectX::XMVECTOR right =
            DirectX::XMVector3Normalize(
                DirectX::XMVector3Cross(worldUp, forward));

        const DirectX::XMVECTOR cameraUp =
            DirectX::XMVector3Normalize(
                DirectX::XMVector3Cross(forward, right));

        DirectX::XMVECTOR direction =
            DirectX::XMVectorAdd(
                forward,
                DirectX::XMVectorAdd(
                    DirectX::XMVectorScale(right, viewX),
                    DirectX::XMVectorScale(cameraUp, viewY)));

        direction = DirectX::XMVector3Normalize(direction);

        outRay.origin = position_;

        DirectX::XMStoreFloat3(
            &outRay.direction,
            direction);

        return true;
    }

    float CameraController::GetMoveSpeed() const noexcept
    {
        return moveSpeed_;
    }

    DirectX::XMFLOAT3 CameraController::GetPosition() const noexcept
    {
        return position_;
    }

    void CameraController::EndMouseLook() noexcept
    {
        if (!mouseLookActive_)
        {
            return;
        }

        const HWND viewportWindow =
            ToWindowHandle(viewportWindow_);

        if (
            viewportWindow != nullptr &&
            GetCapture() == viewportWindow)
        {
            ReleaseCapture();
        }

        mouseLookActive_ = false;
    }
}
