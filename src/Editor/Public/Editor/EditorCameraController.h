#pragma once

#include <Platform/Window.h>

#include <DirectXMath.h>

#include <cstdint>

namespace lts::editor
{
    class EditorCameraController final
    {
    public:
        EditorCameraController() noexcept;

        ~EditorCameraController() noexcept;

        EditorCameraController(
            const EditorCameraController&) = delete;

        EditorCameraController& operator=(
            const EditorCameraController&) = delete;

        void SetViewportWindow(
            engine::platform::NativeWindowHandle window) noexcept;

        void Reset() noexcept;

        void Update(
            double deltaSeconds,
            float wheelSteps) noexcept;

        [[nodiscard]]
        bool BuildViewProjection(
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight,
            DirectX::XMFLOAT4X4& outViewProjection) const noexcept;

        [[nodiscard]]
        float GetMoveSpeed() const noexcept;

    private:
        void EndMouseLook() noexcept;

        engine::platform::NativeWindowHandle
            viewportWindow_;

        DirectX::XMFLOAT3 position_
        {
            18.0F,
            14.0F,
            -18.0F
        };

        float yawRadians_ = -0.785398163F;
        float pitchRadians_ = -0.488692191F;

        float moveSpeed_ = 10.0F;
        float lookSensitivity_ = 0.0025F;

        std::int32_t restoreCursorX_ = 0;
        std::int32_t restoreCursorY_ = 0;

        bool mouseLookActive_ = false;
    };
}