#include "Editor/LevelEditor/Play/PlayInEditorController.h"

#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"

#include <Core/Log.h>

#include <Windows.h>

#include <algorithm>
#include <cmath>

namespace lts::editor
{
    namespace
    {
        constexpr float MinimumPitchRadians =
            -1.13446401F;

        constexpr float MaximumPitchRadians =
            0.261799388F;

        constexpr float MouseSensitivity =
            0.0025F;

        constexpr float CameraDistance =
            4.5F;

        constexpr float CameraTargetHeight =
            0.55F;

        constexpr float CameraGroundClearance =
            0.25F;

        constexpr float MaximumStepHeight =
            0.65F;

        [[nodiscard]]
        bool IsKeyDown(
            const int virtualKey) noexcept
        {
            return
                (GetAsyncKeyState(
                    virtualKey) &
                    0x8000) != 0;
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
            const float yawRadians,
            const float pitchRadians) noexcept
        {
            const float cosinePitch =
                std::cos(
                    pitchRadians);

            return DirectX::XMVector3Normalize(
                DirectX::XMVectorSet(
                    std::sin(
                        yawRadians) *
                        cosinePitch,

                    std::sin(
                        pitchRadians),

                    std::cos(
                        yawRadians) *
                        cosinePitch,

                    0.0F));
        }

        [[nodiscard]]
        float MoveTowards(
            const float current,
            const float target,
            const float maximumDelta) noexcept
        {
            if (current < target)
            {
                return
                    (std::min)(
                        current +
                            maximumDelta,
                        target);
            }

            if (current > target)
            {
                return
                    (std::max)(
                        current -
                            maximumDelta,
                        target);
            }

            return target;
        }

        [[nodiscard]]
        float MoveAngleDegrees(
            const float current,
            const float target,
            const float maximumDelta) noexcept
        {
            const float difference =
                std::remainder(
                    target -
                        current,
                    360.0F);

            return std::remainder(
                current +
                    std::clamp(
                        difference,
                        -maximumDelta,
                        maximumDelta),
                360.0F);
        }

        [[nodiscard]]
        float ResolveGroundHeight(
            const SceneDocument& document,
            const TerrainRenderer&
                terrainRenderer,
            const float worldX,
            const float worldZ) noexcept
        {
            float terrainHeight = 0.0F;

            if (terrainRenderer.
                    TryGetSurfaceHeight(
                        document,
                        worldX,
                        worldZ,
                        terrainHeight))
            {
                return terrainHeight;
            }

            /*
             * Уровень без Terrain использует
             * резервную плоскость Y = 0.
             */
            return 0.0F;
        }
    }

    PlayInEditorController::
        PlayInEditorController() noexcept =
            default;

    PlayInEditorController::
        ~PlayInEditorController() noexcept
    {
        ReleaseCursor();
    }

    bool PlayInEditorController::Start(
        SceneDocument& document,
        TerrainRenderer& terrainRenderer,
        const engine::platform::
            NativeWindowHandle window,
        const float viewportX,
        const float viewportY,
        const float viewportWidth,
        const float viewportHeight) noexcept
    {
        if (playing_)
        {
            return true;
        }

        EditorEntityId playerEntityId = 0U;

        const auto isPlayableCharacter =
            [](
                const EditorSceneEntity&
                    entity) noexcept
            {
                return
                    entity.skeletalMesh.
                        has_value() &&
                    entity.characterController.
                        has_value() &&
                    entity.characterController->
                        playerControlled;
            };

        const EditorSceneEntity* const selected =
            document.GetSelectedEntity();

        if (
            selected != nullptr &&
            isPlayableCharacter(
                *selected))
        {
            playerEntityId =
                selected->id;
        }
        else
        {
            for (
                const EditorSceneEntity& entity :
                document.GetEntities())
            {
                if (isPlayableCharacter(
                        entity))
                {
                    playerEntityId =
                        entity.id;

                    break;
                }
            }
        }

        if (playerEntityId == 0U)
        {
            engine::core::GetLogger().Write(
                engine::core::LogLevel::Warning,
                "LTS.Editor.Play",
                "Play In Editor needs a "
                "player-controlled Character.");

            const HWND owner =
                ToWindowHandle(
                    window);

            if (
                owner != nullptr &&
                IsWindow(owner))
            {
                MessageBoxW(
                    owner,
                    L"Add a Character actor with "
                    L"Player Controlled enabled.",
                    L"Play In Editor",
                    MB_OK |
                        MB_ICONWARNING);
            }

            return false;
        }

        snapshot_ =
            document.CreateSnapshot();

        playerEntityId_ =
            playerEntityId;

        EditorSceneEntity* const player =
            document.FindEntityMutable(
                playerEntityId_);

        if (
            player == nullptr ||
            !player->skeletalMesh.
                has_value() ||
            !player->characterController.
                has_value())
        {
            snapshot_ = {};
            playerEntityId_ = 0U;

            return false;
        }

        originalIdleAnimation_ =
            player->skeletalMesh->
                idleAnimation;

        const EditorSceneEntity* playerStart =
            nullptr;

        for (
            const EditorSceneEntity& entity :
            document.GetEntities())
        {
            if (!entity.spawnPoint.has_value())
            {
                continue;
            }

            if (
                entity.spawnPoint->
                    spawnTag.empty() ||
                entity.spawnPoint->
                    spawnTag == L"Player")
            {
                playerStart = &entity;

                break;
            }

            if (playerStart == nullptr)
            {
                playerStart = &entity;
            }
        }

        if (playerStart != nullptr)
        {
            player->transform.position =
                playerStart->
                    transform.position;

            player->transform.
                rotationDegrees[1] =
                    playerStart->
                        transform.
                        rotationDegrees[1];
        }

        const auto& controller =
            *player->characterController;

        const float halfHeight =
            (std::max)(
                controller.capsuleHeight *
                    0.5F,
                controller.capsuleRadius);

        const float groundHeight =
            ResolveGroundHeight(
                document,
                terrainRenderer,
                player->transform.
                    position[0],
                player->transform.
                    position[2]);

        player->transform.position[1] =
            groundHeight +
            halfHeight;

        cameraYawRadians_ =
            DirectX::XMConvertToRadians(
                player->transform.
                    rotationDegrees[1]);

        cameraPitchRadians_ =
            -0.261799388F;

        velocityX_ = 0.0F;
        velocityZ_ = 0.0F;
        verticalVelocity_ = 0.0F;

        grounded_ = true;
        spaceWasDown_ = false;
        escapeWasDown_ = false;

        const DirectX::XMVECTOR target =
            DirectX::XMVectorSet(
                player->transform.position[0],
                player->transform.position[1] +
                    CameraTargetHeight,
                player->transform.position[2],
                1.0F);

        const DirectX::XMVECTOR forward =
            BuildForwardVector(
                cameraYawRadians_,
                cameraPitchRadians_);

        DirectX::XMStoreFloat3(
            &cameraPosition_,
            DirectX::XMVectorSubtract(
                target,
                DirectX::XMVectorScale(
                    forward,
                    CameraDistance)));

        /*
         * Убираем editor selection tint и gizmo на время PIE.
         * Snapshot восстановит выделение после Stop.
         */
        document.ClearSelection();

        window_ = window;
        playing_ = true;

        CaptureCursor(
            viewportX,
            viewportY,
            viewportWidth,
            viewportHeight);

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.Play",
            "Play In Editor started.");

        return true;
    }

    void PlayInEditorController::Stop(
        SceneDocument& document) noexcept
    {
        if (!playing_)
        {
            return;
        }

        ReleaseCursor();

        document.RestoreSnapshot(
            snapshot_,
            false);

        snapshot_ = {};
        playerEntityId_ = 0U;

        originalIdleAnimation_.clear();

        velocityX_ = 0.0F;
        velocityZ_ = 0.0F;
        verticalVelocity_ = 0.0F;

        grounded_ = false;
        spaceWasDown_ = false;
        escapeWasDown_ = false;
        playing_ = false;

        engine::core::GetLogger().Write(
            engine::core::LogLevel::Information,
            "LTS.Editor.Play",
            "Play In Editor stopped. "
            "The editor scene was restored.");
    }

    void PlayInEditorController::Update(
        const double deltaSeconds,
        SceneDocument& document,
        TerrainRenderer& terrainRenderer,
        const float viewportX,
        const float viewportY,
        const float viewportWidth,
        const float viewportHeight) noexcept
    {
        if (!playing_)
        {
            return;
        }

        const bool escapeDown =
            IsKeyDown(
                VK_ESCAPE);

        if (
            escapeDown &&
            !escapeWasDown_)
        {
            escapeWasDown_ = true;

            Stop(document);

            return;
        }

        escapeWasDown_ =
            escapeDown;

        if (
            !std::isfinite(
                deltaSeconds) ||
            deltaSeconds <= 0.0)
        {
            return;
        }

        if (
            viewportWidth <= 1.0F ||
            viewportHeight <= 1.0F)
        {
            ReleaseCursor();

            return;
        }

        CaptureCursor(
            viewportX,
            viewportY,
            viewportWidth,
            viewportHeight);

        if (!cursorCaptured_)
        {
            return;
        }

        EditorSceneEntity* const player =
            document.FindEntityMutable(
                playerEntityId_);

        if (
            player == nullptr ||
            !player->skeletalMesh.
                has_value() ||
            !player->characterController.
                has_value())
        {
            Stop(document);

            return;
        }

        const HWND window =
            ToWindowHandle(
                window_);

        POINT center
        {
            static_cast<LONG>(
                std::lround(
                    viewportX +
                    viewportWidth *
                        0.5F)),

            static_cast<LONG>(
                std::lround(
                    viewportY +
                    viewportHeight *
                        0.5F))
        };

        if (
            window == nullptr ||
            !ClientToScreen(
                window,
                &center))
        {
            ReleaseCursor();

            return;
        }

        POINT cursor{};

        if (GetCursorPos(&cursor))
        {
            const LONG deltaX =
                cursor.x -
                center.x;

            const LONG deltaY =
                cursor.y -
                center.y;

            cameraYawRadians_ +=
                static_cast<float>(
                    deltaX) *
                MouseSensitivity;

            cameraPitchRadians_ -=
                static_cast<float>(
                    deltaY) *
                MouseSensitivity;

            cameraYawRadians_ =
                std::remainder(
                    cameraYawRadians_,
                    6.28318530718F);

            cameraPitchRadians_ =
                std::clamp(
                    cameraPitchRadians_,
                    MinimumPitchRadians,
                    MaximumPitchRadians);
        }

        SetCursorPos(
            center.x,
            center.y);

        const float safeDeltaSeconds =
            static_cast<float>(
                (std::min)(
                    deltaSeconds,
                    0.05));

        const auto& controller =
            *player->characterController;

        const float inputX =
            (
                IsKeyDown('D')
                    ? 1.0F
                    : 0.0F
            ) -
            (
                IsKeyDown('A')
                    ? 1.0F
                    : 0.0F
            );

        const float inputZ =
            (
                IsKeyDown('W')
                    ? 1.0F
                    : 0.0F
            ) -
            (
                IsKeyDown('S')
                    ? 1.0F
                    : 0.0F
            );

        const float inputLengthSquared =
            inputX *
                inputX +
            inputZ *
                inputZ;

        float directionX = 0.0F;
        float directionZ = 0.0F;

        if (inputLengthSquared > 0.000001F)
        {
            const float inverseLength =
                1.0F /
                std::sqrt(
                    inputLengthSquared);

            const float normalizedX =
                inputX *
                inverseLength;

            const float normalizedZ =
                inputZ *
                inverseLength;

            const float forwardX =
                std::sin(
                    cameraYawRadians_);

            const float forwardZ =
                std::cos(
                    cameraYawRadians_);

            const float rightX =
                forwardZ;

            const float rightZ =
                -forwardX;

            directionX =
                rightX *
                    normalizedX +
                forwardX *
                    normalizedZ;

            directionZ =
                rightZ *
                    normalizedX +
                forwardZ *
                    normalizedZ;

            const float worldLength =
                std::sqrt(
                    directionX *
                        directionX +
                    directionZ *
                        directionZ);

            if (worldLength > 0.000001F)
            {
                directionX /=
                    worldLength;

                directionZ /=
                    worldLength;
            }
        }

        const bool moving =
            inputLengthSquared >
                0.000001F;

        const bool running =
            moving &&
            (
                IsKeyDown(
                    VK_SHIFT) ||
                IsKeyDown(
                    VK_LSHIFT) ||
                IsKeyDown(
                    VK_RSHIFT)
            );

        const float targetSpeed =
            moving
                ? (
                    running
                        ? controller.runSpeed
                        : controller.walkSpeed
                )
                : 0.0F;

        const float targetVelocityX =
            directionX *
            targetSpeed;

        const float targetVelocityZ =
            directionZ *
            targetSpeed;

        const float velocityRate =
            moving
                ? controller.acceleration
                : controller.deceleration;

        const float maximumVelocityDelta =
            (std::max)(
                velocityRate,
                0.0F) *
            safeDeltaSeconds;

        velocityX_ =
            MoveTowards(
                velocityX_,
                targetVelocityX,
                maximumVelocityDelta);

        velocityZ_ =
            MoveTowards(
                velocityZ_,
                targetVelocityZ,
                maximumVelocityDelta);

        float nextX =
            player->transform.position[0] +
            velocityX_ *
                safeDeltaSeconds;

        float nextZ =
            player->transform.position[2] +
            velocityZ_ *
                safeDeltaSeconds;

        const float halfHeight =
            (std::max)(
                controller.capsuleHeight *
                    0.5F,
                controller.capsuleRadius);

        float groundHeight =
            ResolveGroundHeight(
                document,
                terrainRenderer,
                nextX,
                nextZ);

        float standingHeight =
            groundHeight +
            halfHeight;

        if (
            grounded_ &&
            standingHeight -
                player->transform.position[1] >
                MaximumStepHeight)
        {
            nextX =
                player->transform.position[0];

            nextZ =
                player->transform.position[2];

            velocityX_ = 0.0F;
            velocityZ_ = 0.0F;

            groundHeight =
                ResolveGroundHeight(
                    document,
                    terrainRenderer,
                    nextX,
                    nextZ);

            standingHeight =
                groundHeight +
                halfHeight;
        }

        const bool spaceDown =
            IsKeyDown(
                VK_SPACE);

        const bool jumpPressed =
            spaceDown &&
            !spaceWasDown_;

        spaceWasDown_ =
            spaceDown;

        if (
            jumpPressed &&
            grounded_)
        {
            verticalVelocity_ =
                controller.jumpVelocity;

            grounded_ = false;
        }

        float nextY =
            player->transform.position[1];

        if (
            grounded_ &&
            nextY -
                standingHeight >
                MaximumStepHeight)
        {
            grounded_ = false;
        }

        if (grounded_)
        {
            nextY =
                standingHeight;
        }
        else
        {
            verticalVelocity_ -=
                (std::max)(
                    controller.gravity,
                    0.0F) *
                safeDeltaSeconds;

            nextY +=
                verticalVelocity_ *
                safeDeltaSeconds;
        }

        if (nextY <= standingHeight)
        {
            nextY =
                standingHeight;

            verticalVelocity_ = 0.0F;
            grounded_ = true;
        }

        player->transform.position =
        {
            nextX,
            nextY,
            nextZ
        };

        if (moving)
        {
            const float targetYawDegrees =
                DirectX::XMConvertToDegrees(
                    std::atan2(
                        directionX,
                        directionZ));

            player->transform.
                rotationDegrees[1] =
                    MoveAngleDegrees(
                        player->transform.
                            rotationDegrees[1],
                        targetYawDegrees,
                        (std::max)(
                            controller.
                                rotationSpeedDegrees,
                            0.0F) *
                        safeDeltaSeconds);
        }

        UpdateAnimation(
            *player,
            moving,
            running);

        const DirectX::XMVECTOR target =
            DirectX::XMVectorSet(
                nextX,
                nextY +
                    CameraTargetHeight,
                nextZ,
                1.0F);

        const DirectX::XMVECTOR cameraForward =
            BuildForwardVector(
                cameraYawRadians_,
                cameraPitchRadians_);

        DirectX::XMVECTOR desiredCamera =
            DirectX::XMVectorSubtract(
                target,
                DirectX::XMVectorScale(
                    cameraForward,
                    CameraDistance));

        DirectX::XMFLOAT3
            desiredCameraPosition{};

        DirectX::XMStoreFloat3(
            &desiredCameraPosition,
            desiredCamera);

        const float cameraGround =
            ResolveGroundHeight(
                document,
                terrainRenderer,
                desiredCameraPosition.x,
                desiredCameraPosition.z);

        desiredCameraPosition.y =
            (std::max)(
                desiredCameraPosition.y,
                cameraGround +
                    CameraGroundClearance);

        desiredCamera =
            DirectX::XMLoadFloat3(
                &desiredCameraPosition);

        const float cameraBlend =
            1.0F -
            std::exp(
                -12.0F *
                safeDeltaSeconds);

        DirectX::XMStoreFloat3(
            &cameraPosition_,
            DirectX::XMVectorLerp(
                DirectX::XMLoadFloat3(
                    &cameraPosition_),
                desiredCamera,
                cameraBlend));
    }

    bool PlayInEditorController::
        BuildViewProjection(
            const std::uint32_t viewportWidth,
            const std::uint32_t viewportHeight,
            DirectX::XMFLOAT4X4&
                viewProjection) const noexcept
    {
        if (
            !playing_ ||
            viewportWidth == 0U ||
            viewportHeight == 0U)
        {
            return false;
        }

        const float aspectRatio =
            static_cast<float>(
                viewportWidth) /
            static_cast<float>(
                viewportHeight);

        if (
            !std::isfinite(
                aspectRatio) ||
            aspectRatio <= 0.0F)
        {
            return false;
        }

        const DirectX::XMMATRIX view =
            DirectX::XMMatrixLookToLH(
                DirectX::XMLoadFloat3(
                    &cameraPosition_),

                BuildForwardVector(
                    cameraYawRadians_,
                    cameraPitchRadians_),

                DirectX::XMVectorSet(
                    0.0F,
                    1.0F,
                    0.0F,
                    0.0F));

        const DirectX::XMMATRIX projection =
            DirectX::XMMatrixPerspectiveFovLH(
                DirectX::XMConvertToRadians(
                    60.0F),

                aspectRatio,
                0.1F,
                2000.0F);

        DirectX::XMStoreFloat4x4(
            &viewProjection,
            DirectX::XMMatrixMultiply(
                view,
                projection));

        return true;
    }

    DirectX::XMFLOAT3
        PlayInEditorController::
            GetCameraPosition() const noexcept
    {
        return cameraPosition_;
    }

    bool PlayInEditorController::
        IsPlaying() const noexcept
    {
        return playing_;
    }

    void PlayInEditorController::CaptureCursor(
        const float viewportX,
        const float viewportY,
        const float viewportWidth,
        const float viewportHeight) noexcept
    {
        if (
            !playing_ ||
            viewportWidth <= 1.0F ||
            viewportHeight <= 1.0F)
        {
            return;
        }

        const HWND window =
            ToWindowHandle(
                window_);

        if (
            window == nullptr ||
            !IsWindow(window))
        {
            return;
        }

        const HWND root =
            GetAncestor(
                window,
                GA_ROOT);

        if (
            root == nullptr ||
            GetForegroundWindow() != root)
        {
            ReleaseCursor();

            return;
        }

        POINT topLeft
        {
            static_cast<LONG>(
                std::lround(
                    viewportX)),

            static_cast<LONG>(
                std::lround(
                    viewportY))
        };

        POINT bottomRight
        {
            static_cast<LONG>(
                std::lround(
                    viewportX +
                    viewportWidth)),

            static_cast<LONG>(
                std::lround(
                    viewportY +
                    viewportHeight))
        };

        POINT center
        {
            static_cast<LONG>(
                std::lround(
                    viewportX +
                    viewportWidth *
                        0.5F)),

            static_cast<LONG>(
                std::lround(
                    viewportY +
                    viewportHeight *
                        0.5F))
        };

        if (
            !ClientToScreen(
                window,
                &topLeft) ||
            !ClientToScreen(
                window,
                &bottomRight) ||
            !ClientToScreen(
                window,
                &center))
        {
            return;
        }

        if (!cursorCaptured_)
        {
            POINT previous{};

            if (GetCursorPos(&previous))
            {
                restoreCursorX_ =
                    static_cast<
                        std::int32_t>(
                            previous.x);

                restoreCursorY_ =
                    static_cast<
                        std::int32_t>(
                            previous.y);
            }

            SetFocus(window);
            SetCapture(window);

            while (ShowCursor(FALSE) >= 0)
            {
            }

            cursorCaptured_ = true;
        }

        const RECT clipRectangle
        {
            topLeft.x,
            topLeft.y,
            bottomRight.x,
            bottomRight.y
        };

        ClipCursor(
            &clipRectangle);

        SetCursorPos(
            center.x,
            center.y);
    }

    void PlayInEditorController::
        ReleaseCursor() noexcept
    {
        if (!cursorCaptured_)
        {
            return;
        }

        ClipCursor(nullptr);
        ReleaseCapture();

        while (ShowCursor(TRUE) < 0)
        {
        }

        SetCursorPos(
            restoreCursorX_,
            restoreCursorY_);

        cursorCaptured_ = false;
    }

    void PlayInEditorController::UpdateAnimation(
        EditorSceneEntity& player,
        const bool moving,
        const bool running) noexcept
    {
        if (!player.skeletalMesh.has_value())
        {
            return;
        }

        auto& skeletalMesh =
            *player.skeletalMesh;

        const std::wstring* desiredAnimation =
            &originalIdleAnimation_;

        if (
            !grounded_ &&
            !skeletalMesh.
                jumpAnimation.empty())
        {
            desiredAnimation =
                &skeletalMesh.
                    jumpAnimation;
        }
        else if (
            running &&
            !skeletalMesh.
                runAnimation.empty())
        {
            desiredAnimation =
                &skeletalMesh.
                    runAnimation;
        }
        else if (
            moving &&
            !skeletalMesh.
                walkAnimation.empty())
        {
            desiredAnimation =
                &skeletalMesh.
                    walkAnimation;
        }

        if (
            skeletalMesh.idleAnimation !=
                *desiredAnimation)
        {
            /*
             * Текущий renderer проигрывает путь
             * idleAnimation. В PIE временно подставляем
             * locomotion-анимацию. Snapshot восстановит
             * исходный путь после Stop.
             */
            skeletalMesh.idleAnimation =
                *desiredAnimation;
        }
    }
}