#pragma once

#include <Platform/Window.h>

#include <DirectXMath.h>

#include <cstdint>

namespace lts::editor
{
    struct EditorPickRay final
    {
        DirectX::XMFLOAT3 origin
        {
            0.0F,
            0.0F,
            0.0F
        };

        DirectX::XMFLOAT3 direction
        {
            0.0F,
            0.0F,
            1.0F
        };
    };

    class EditorCameraController final
    {
    public:
        EditorCameraController() noexcept;
        ~EditorCameraController() noexcept;

        EditorCameraController(const EditorCameraController&) = delete;
        EditorCameraController& operator=(const EditorCameraController&) = delete;

        void SetViewportWindow(
            engine::platform::NativeWindowHandle window) noexcept;

        void Reset() noexcept;

        void FocusOn(
            const DirectX::XMFLOAT3& target,
            float distance) noexcept;

        void Update(
            double deltaSeconds,
            float wheelSteps) noexcept;

        [[nodiscard]]
        bool BuildViewProjection(
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight,
            DirectX::XMFLOAT4X4& outViewProjection) const noexcept;

        [[nodiscard]]
        bool BuildPickRay(
            std::uint32_t mouseX,
            std::uint32_t mouseY,
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight,
            EditorPickRay& outRay) const noexcept;

        [[nodiscard]]
        float GetMoveSpeed() const noexcept;

    private:
        void EndMouseLook() noexcept;

        engine::platform::NativeWindowHandle viewportWindow_;

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

        std::int32_t lastCursorX_ = 0;
        std::int32_t lastCursorY_ = 0;

        bool mouseLookActive_ = false;
    };
}
