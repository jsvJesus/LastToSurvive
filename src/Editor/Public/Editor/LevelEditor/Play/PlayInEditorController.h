#pragma once

#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include <Scene/CharacterAnimationStateMachine.h>

#include <Platform/Window.h>

#include <DirectXMath.h>

#include <cstdint>
#include <string>

namespace lts::editor
{
    class TerrainRenderer;

    class PlayInEditorController final
    {
    public:
        PlayInEditorController() noexcept;
        ~PlayInEditorController() noexcept;

        PlayInEditorController(
            const PlayInEditorController&) = delete;

        PlayInEditorController& operator=(
            const PlayInEditorController&) = delete;

        [[nodiscard]]
        bool Start(
            SceneDocument& document,
            TerrainRenderer& terrainRenderer,
            engine::platform::NativeWindowHandle window,
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight) noexcept;

        void Stop(
            SceneDocument& document) noexcept;

        void Update(
            double deltaSeconds,
            SceneDocument& document,
            TerrainRenderer& terrainRenderer,
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight) noexcept;

        [[nodiscard]]
        bool BuildViewProjection(
            std::uint32_t viewportWidth,
            std::uint32_t viewportHeight,
            DirectX::XMFLOAT4X4&
                viewProjection) const noexcept;

        [[nodiscard]]
        DirectX::XMFLOAT3
            GetCameraPosition() const noexcept;

        [[nodiscard]]
        bool IsPlaying() const noexcept;

    private:
        void CaptureCursor(
            float viewportX,
            float viewportY,
            float viewportWidth,
            float viewportHeight) noexcept;

        void ReleaseCursor() noexcept;

        engine::scene::
            CharacterAnimationStateMachine
                animationStateMachine_;

        engine::platform::NativeWindowHandle window_;

        EditorSceneSnapshot snapshot_;
        EditorEntityId playerEntityId_ = 0U;

        DirectX::XMFLOAT3 cameraPosition_
        {
            0.0F,
            2.0F,
            -4.0F
        };

        /*
         * Игровое направление ног и корпуса.
         *
         * Оно не содержит визуальный разворот
         * импортированной модели на 180 градусов.
         */
        float bodyYawDegrees_ = 0.0F;

        float cameraYawRadians_ = 0.0F;
        float cameraPitchRadians_ =
            -0.261799388F;

        float velocityX_ = 0.0F;
        float velocityZ_ = 0.0F;
        float verticalVelocity_ = 0.0F;

        std::int32_t restoreCursorX_ = 0;
        std::int32_t restoreCursorY_ = 0;

        bool playing_ = false;
        bool grounded_ = false;
        bool spaceWasDown_ = false;
        bool escapeWasDown_ = false;
        bool cursorCaptured_ = false;
    };
}