#include "Editor/LevelEditor/Play/PlayInEditorController.h"

#include "Editor/LevelEditor/Terrain/TerrainRenderer.h"
#include "Editor/LevelEditor/Rendering/ModularCharacterRenderer.h"

#include <Core/Log.h>

#include <Windows.h>

#include <algorithm>
#include <cmath>

namespace lts::editor
{
    namespace
    {
        constexpr float MinimumPitchRadians =
            -1.04719755F;

        constexpr float MaximumPitchRadians =
            0.872664626F;

        constexpr float MouseSensitivity =
            0.0025F;

        constexpr float ThirdPersonCameraDistance =
            3.35F;

        constexpr float ThirdPersonShoulderOffset =
            0.58F;

        constexpr float ThirdPersonTargetUpOffset =
            0.06F;

        constexpr float StandingFallbackHeadHeight =
            1.66F;

        constexpr float CrouchedFallbackHeadHeight =
            1.18F;

        constexpr float FirstPersonForwardOffset =
            0.06F;

        constexpr float FirstPersonUpOffset =
            0.02F;

        constexpr float CameraHeadFollowSpeed =
            11.0F;

        constexpr float CameraHeadVerticalDeadZone =
            0.012F;

        constexpr float MaximumLookPitchDegrees =
            50.0F;

        constexpr float CrouchedSpeedMultiplier =
            0.55F;

        constexpr float CameraGroundClearance =
            0.25F;

        constexpr float MaximumStepHeight =
            0.65F;

        constexpr float
            CharacterVisualYawOffsetDegrees =
                180.0F;

        [[nodiscard]]
        bool IsKeyDown(
            const int virtualKey) noexcept
        {
            return
                (
                    GetAsyncKeyState(
                        virtualKey) &
                    0x8000
                ) != 0;
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
        DirectX::XMVECTOR BuildRightVector(
            const float yawRadians) noexcept
        {
            return DirectX::XMVectorSet(
                std::cos(yawRadians),
                0.0F,
                -std::sin(yawRadians),
                0.0F);
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
        float NormalizeAngleDegrees(
            const float angleDegrees) noexcept
        {
            if (!std::isfinite(
                    angleDegrees))
            {
                return 0.0F;
            }

            return std::remainder(
                angleDegrees,
                360.0F);
        }

        [[nodiscard]]
        float AngleDifferenceDegrees(
            const float targetDegrees,
            const float currentDegrees) noexcept
        {
            return NormalizeAngleDegrees(
                targetDegrees -
                    currentDegrees);
        }

        [[nodiscard]]
        float MoveAngleDegrees(
            const float currentDegrees,
            const float targetDegrees,
            const float maximumDeltaDegrees) noexcept
        {
            const float differenceDegrees =
                AngleDifferenceDegrees(
                    targetDegrees,
                    currentDegrees);

            return NormalizeAngleDegrees(
                currentDegrees +
                    std::clamp(
                        differenceDegrees,
                        -maximumDeltaDegrees,
                        maximumDeltaDegrees));
        }

        [[nodiscard]]
        engine::scene::CharacterMovementDirection
            ResolveMovementDirection(
                const float inputX,
                const float inputZ) noexcept
        {
            const int horizontal =
                inputX > 0.25F
                    ? 1
                    : (
                        inputX < -0.25F
                            ? -1
                            : 0
                    );

            const int vertical =
                inputZ > 0.25F
                    ? 1
                    : (
                        inputZ < -0.25F
                            ? -1
                            : 0
                    );

            if (vertical > 0)
            {
                if (horizontal < 0)
                {
                    return
                        engine::scene::
                            CharacterMovementDirection::
                                ForwardLeft;
                }

                if (horizontal > 0)
                {
                    return
                        engine::scene::
                            CharacterMovementDirection::
                                ForwardRight;
                }

                return
                    engine::scene::
                        CharacterMovementDirection::
                            Forward;
            }

            if (vertical < 0)
            {
                if (horizontal < 0)
                {
                    return
                        engine::scene::
                            CharacterMovementDirection::
                                BackwardLeft;
                }

                if (horizontal > 0)
                {
                    return
                        engine::scene::
                            CharacterMovementDirection::
                                BackwardRight;
                }

                return
                    engine::scene::
                        CharacterMovementDirection::
                            Backward;
            }

            if (horizontal < 0)
            {
                return
                    engine::scene::
                        CharacterMovementDirection::
                            Left;
            }

            if (horizontal > 0)
            {
                return
                    engine::scene::
                        CharacterMovementDirection::
                            Right;
            }

            return
                engine::scene::
                    CharacterMovementDirection::
                        None;
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

        if (!player->characterAnimation.
                 has_value())
        {
            player->characterAnimation.emplace();
        }

        if (
            !player->characterAnimation->
                profileLoaded)
        {
            std::wstring profileError;

            static_cast<void>(
                document.
                    ReloadCharacterAnimationProfile(
                        playerEntityId_,
                        profileError));
        }

        animationStateMachine_.Reset(
            *player->characterAnimation);

        const EditorSceneEntity* playerStart =
            nullptr;

        for (
            const EditorSceneEntity& entity :
            document.GetEntities())
        {
            if (!entity.spawnPoint.
                    has_value())
            {
                continue;
            }

            if (
                entity.spawnPoint->
                    spawnTag.empty() ||
                entity.spawnPoint->
                    spawnTag == L"Player")
            {
                playerStart =
                    &entity;

                break;
            }

            if (playerStart == nullptr)
            {
                playerStart =
                    &entity;
            }
        }

        float gameplayYawDegrees =
            player->transform.
                rotationDegrees[1];

        if (playerStart != nullptr)
        {
            player->transform.position =
                playerStart->
                    transform.position;

            gameplayYawDegrees =
                playerStart->
                    transform.
                    rotationDegrees[1];
        }

        bodyYawDegrees_ =
            NormalizeAngleDegrees(
                gameplayYawDegrees);

        player->transform.
            rotationDegrees[1] =
                NormalizeAngleDegrees(
                    bodyYawDegrees_ +
                        CharacterVisualYawOffsetDegrees);

        const float groundHeight =
            ResolveGroundHeight(
                document,
                terrainRenderer,
                player->transform.
                    position[0],
                player->transform.
                    position[2]);

        /*
         * Transform персонажа хранит позицию ступней.
         * Capsule height не прибавляется к render transform.
         */
        player->transform.position[1] =
            groundHeight;

        cameraYawRadians_ =
            DirectX::XMConvertToRadians(
                bodyYawDegrees_);

        cameraPitchRadians_ =
            -0.087266463F;

        viewMode_ =
            engine::scene::
                CharacterViewMode::
                    ThirdPerson;

        stance_ =
            engine::scene::
                CharacterStance::
                    Standing;

        auto& animationRuntime =
            player->characterAnimation->
                runtime;

        animationRuntime.viewMode =
            viewMode_;

        animationRuntime.stance =
            stance_;

        animationRuntime.actorYawDegrees =
            bodyYawDegrees_;

        animationRuntime.lowerBodyYawDegrees =
            bodyYawDegrees_;

        animationRuntime.turnTargetYawDegrees =
            bodyYawDegrees_;

        animationRuntime.
            upperBodyYawOffsetDegrees =
                0.0F;

        animationRuntime.
            upperBodyPitchOffsetDegrees =
                0.0F;

        animationRuntime.turnInPlaceActive =
            false;

        velocityX_ = 0.0F;
        velocityZ_ = 0.0F;
        verticalVelocity_ = 0.0F;

        grounded_ = true;

        spaceWasDown_ = false;
        escapeWasDown_ = false;

        viewToggleWasDown_ = false;
        crouchToggleWasDown_ = false;

        primaryActionWasDown_ = false;
        reloadWasDown_ = false;

        turnDirection_ = 0;

        cameraAnchor_ =
        {
            player->transform.position[0],
            player->transform.position[1] +
                StandingFallbackHeadHeight,
            player->transform.position[2]
        };

        cameraAnchorValid_ = true;

        const DirectX::XMVECTOR forward =
            BuildForwardVector(
                cameraYawRadians_,
                cameraPitchRadians_);

        const DirectX::XMVECTOR right =
            BuildRightVector(
                cameraYawRadians_);

        const DirectX::XMVECTOR target =
            DirectX::XMVectorAdd(
                DirectX::XMLoadFloat3(
                    &cameraAnchor_),
                DirectX::XMVectorSet(
                    0.0F,
                    ThirdPersonTargetUpOffset,
                    0.0F,
                    0.0F));

        DirectX::XMVECTOR initialCamera =
            DirectX::XMVectorAdd(
                target,
                DirectX::XMVectorScale(
                    right,
                    ThirdPersonShoulderOffset));

        initialCamera =
            DirectX::XMVectorSubtract(
                initialCamera,
                DirectX::XMVectorScale(
                    forward,
                    ThirdPersonCameraDistance));

        DirectX::XMStoreFloat3(
            &cameraPosition_,
            initialCamera);

        document.ClearSelection();

        window_ =
            window;

        playing_ =
            true;

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

        velocityX_ = 0.0F;
        velocityZ_ = 0.0F;
        verticalVelocity_ = 0.0F;

        bodyYawDegrees_ = 0.0F;
        turnDirection_ = 0;

        cameraAnchor_ =
        {
            0.0F,
            1.65F,
            0.0F
        };

        cameraAnchorValid_ = false;

        viewMode_ =
            engine::scene::
                CharacterViewMode::
                    ThirdPerson;

        stance_ =
            engine::scene::
                CharacterStance::
                    Standing;

        grounded_ = false;

        spaceWasDown_ = false;
        escapeWasDown_ = false;

        viewToggleWasDown_ = false;
        crouchToggleWasDown_ = false;

        primaryActionWasDown_ = false;
        reloadWasDown_ = false;

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
        const ModularCharacterRenderer&
            characterRenderer,
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
            escapeWasDown_ =
                true;

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

        if (!player->characterAnimation.
                has_value())
        {
            player->characterAnimation.emplace();
        }

        const HWND window =
            ToWindowHandle(
                window_);

        if (
            window == nullptr ||
            !IsWindow(window))
        {
            ReleaseCursor();

            return;
        }

        const POINT center
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

        POINT cursor{};

        if (GetCursorPos(
                &cursor))
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

        const bool viewToggleDown =
            IsKeyDown('C');

        const bool viewTogglePressed =
            viewToggleDown &&
            !viewToggleWasDown_;

        viewToggleWasDown_ =
            viewToggleDown;

        if (viewTogglePressed)
        {
            viewMode_ =
                viewMode_ ==
                    engine::scene::
                        CharacterViewMode::
                            ThirdPerson
                    ? engine::scene::
                        CharacterViewMode::
                            FirstPerson
                    : engine::scene::
                        CharacterViewMode::
                            ThirdPerson;
        }

        const bool crouchToggleDown =
            IsKeyDown(
                VK_CONTROL) ||
            IsKeyDown(
                VK_LCONTROL) ||
            IsKeyDown(
                VK_RCONTROL);

        const bool crouchTogglePressed =
            crouchToggleDown &&
            !crouchToggleWasDown_;

        crouchToggleWasDown_ =
            crouchToggleDown;

        if (
            crouchTogglePressed &&
            grounded_)
        {
            stance_ =
                stance_ ==
                    engine::scene::
                        CharacterStance::
                            Standing
                    ? engine::scene::
                        CharacterStance::
                            Crouched
                    : engine::scene::
                        CharacterStance::
                            Standing;
        }

        const bool aiming =
            IsKeyDown(
                VK_RBUTTON);

        const bool primaryActionDown =
            IsKeyDown(
                VK_LBUTTON);

        const bool primaryActionPressed =
            primaryActionDown &&
            !primaryActionWasDown_;

        primaryActionWasDown_ =
            primaryActionDown;

        const bool reloadDown =
            IsKeyDown('R');

        const bool reloadPressed =
            reloadDown &&
            !reloadWasDown_;

        reloadWasDown_ =
            reloadDown;

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

        const bool moving =
            inputLengthSquared >
                0.000001F;

        float directionX = 0.0F;
        float directionZ = 0.0F;

        if (moving)
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

            if (worldLength >
                    0.000001F)
            {
                directionX /=
                    worldLength;

                directionZ /=
                    worldLength;
            }
        }

        const bool crouched =
            stance_ ==
                engine::scene::
                    CharacterStance::
                        Crouched;

        const bool sprinting =
            moving &&
            !crouched &&
            (
                IsKeyDown(
                    VK_SHIFT) ||
                IsKeyDown(
                    VK_LSHIFT) ||
                IsKeyDown(
                    VK_RSHIFT)
            );

        float targetSpeed =
            moving
                ? (
                    sprinting
                        ? controller.runSpeed
                        : controller.walkSpeed
                )
                : 0.0F;

        if (crouched)
        {
            targetSpeed *=
                CrouchedSpeedMultiplier;
        }

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

        /*
         * Transform Y — уровень ступней.
         */
        float standingHeight =
            groundHeight;

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
                groundHeight;
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
            grounded_ &&
            stance_ ==
                engine::scene::
                    CharacterStance::
                        Standing)
        {
            verticalVelocity_ =
                controller.jumpVelocity;

            grounded_ =
                false;
        }

        float nextY =
            player->transform.position[1];

        if (
            grounded_ &&
            nextY -
                standingHeight >
                MaximumStepHeight)
        {
            grounded_ =
                false;
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

            verticalVelocity_ =
                0.0F;

            grounded_ =
                true;
        }

        player->transform.position =
        {
            nextX,
            nextY,
            nextZ
        };

        auto& characterAnimation =
            *player->characterAnimation;

        auto& animationRuntime =
            characterAnimation.runtime;

        const auto& animationTuning =
            characterAnimation.animationSet.tuning;

        const float cameraYawDegrees =
            NormalizeAngleDegrees(
                DirectX::XMConvertToDegrees(
                    cameraYawRadians_));

        const float cameraPitchDegrees =
            DirectX::XMConvertToDegrees(
                cameraPitchRadians_);

        const bool firstPersonView =
            viewMode_ ==
                engine::scene::
                    CharacterViewMode::FirstPerson;

        const float turnEnterDegrees =
            (std::max)(
                std::fabs(
                    animationTuning.
                        turnInPlaceEnterDegrees),
                1.0F);

        const float turnExitDegrees =
            std::clamp(
                std::fabs(
                    animationTuning.
                        turnInPlaceExitDegrees),
                0.0F,
                turnEnterDegrees);

        const float turnSpeedDegrees =
            (std::max)(
                animationTuning.
                    turnInPlaceSpeedDegrees,
                1.0F);

        if (moving)
        {
            bodyYawDegrees_ =
                MoveAngleDegrees(
                    bodyYawDegrees_,
                    cameraYawDegrees,
                    (std::max)(
                        controller.
                            rotationSpeedDegrees,
                        0.0F) *
                    safeDeltaSeconds);

            animationRuntime.turnInPlaceActive =
                false;

            animationRuntime.turnTargetYawDegrees =
                bodyYawDegrees_;

            turnDirection_ = 0;
        }
        else if (!grounded_)
        {
            animationRuntime.turnInPlaceActive =
                false;

            animationRuntime.turnTargetYawDegrees =
                bodyYawDegrees_;

            turnDirection_ = 0;
        }
        else if (firstPersonView)
        {
            /*
             * Для локального FPS отдельный turn-track
             * не нужен: корпус следует за камерой.
             */
            bodyYawDegrees_ =
                MoveAngleDegrees(
                    bodyYawDegrees_,
                    cameraYawDegrees,
                    turnSpeedDegrees *
                        safeDeltaSeconds);

            animationRuntime.turnInPlaceActive =
                false;

            animationRuntime.turnTargetYawDegrees =
                bodyYawDegrees_;

            turnDirection_ = 0;
        }
        else
        {
            const float cameraBodyDifference =
                AngleDifferenceDegrees(
                    cameraYawDegrees,
                    bodyYawDegrees_);

            if (
                !animationRuntime.turnInPlaceActive &&
                std::fabs(cameraBodyDifference) >=
                    turnEnterDegrees)
            {
                animationRuntime.turnInPlaceActive =
                    true;

                /*
                 * Target фиксируется в момент запуска,
                 * иначе ноги никогда не догоняют мышь.
                 */
                animationRuntime.turnTargetYawDegrees =
                    cameraYawDegrees;

                turnDirection_ =
                    cameraBodyDifference >= 0.0F
                        ? 1
                        : -1;
            }

            if (animationRuntime.turnInPlaceActive)
            {
                const float remainingDifference =
                    AngleDifferenceDegrees(
                        animationRuntime.
                            turnTargetYawDegrees,
                        bodyYawDegrees_);

                if (
                    std::fabs(remainingDifference) <=
                        turnExitDegrees)
                {
                    bodyYawDegrees_ =
                        animationRuntime.
                            turnTargetYawDegrees;

                    animationRuntime.turnInPlaceActive =
                        false;

                    turnDirection_ = 0;
                }
                else
                {
                    bodyYawDegrees_ =
                        MoveAngleDegrees(
                            bodyYawDegrees_,
                            animationRuntime.
                                turnTargetYawDegrees,
                            turnSpeedDegrees *
                                safeDeltaSeconds);
                }
            }
            else
            {
                animationRuntime.turnTargetYawDegrees =
                    bodyYawDegrees_;

                turnDirection_ = 0;
            }
        }

        bodyYawDegrees_ =
            NormalizeAngleDegrees(
                bodyYawDegrees_);

        player->transform.
            rotationDegrees[1] =
                NormalizeAngleDegrees(
                    bodyYawDegrees_ +
                        CharacterVisualYawOffsetDegrees);

        animationRuntime.actorYawDegrees =
            bodyYawDegrees_;

        animationRuntime.lowerBodyYawDegrees =
            bodyYawDegrees_;

        const float maximumLookYaw =
            (std::max)(
                std::fabs(
                    animationTuning.
                        maximumUpperBodyYawDegrees),
                0.0F);

        animationRuntime.
            upperBodyYawOffsetDegrees =
                std::clamp(
                    AngleDifferenceDegrees(
                        cameraYawDegrees,
                        bodyYawDegrees_),
                    -maximumLookYaw,
                    maximumLookYaw);

        animationRuntime.
            upperBodyPitchOffsetDegrees =
                std::clamp(
                    cameraPitchDegrees,
                    -MaximumLookPitchDegrees,
                    MaximumLookPitchDegrees);

        const float horizontalSpeed =
            std::sqrt(
                velocityX_ *
                    velocityX_ +
                velocityZ_ *
                    velocityZ_);

        engine::scene::
            CharacterLocomotionState
                locomotionState =
                    engine::scene::
                        CharacterLocomotionState::
                            Idle;

        if (!grounded_)
        {
            locomotionState =
                engine::scene::
                    CharacterLocomotionState::
                        JumpLoop;
        }
        else if (
            animationRuntime.turnInPlaceActive &&
            turnDirection_ != 0)
        {
            locomotionState =
                turnDirection_ < 0
                    ? engine::scene::
                        CharacterLocomotionState::
                            TurnInPlaceLeft
                    : engine::scene::
                        CharacterLocomotionState::
                            TurnInPlaceRight;
        }
        else if (
            moving &&
            !crouched &&
            sprinting)
        {
            locomotionState =
                engine::scene::
                    CharacterLocomotionState::
                        Sprint;
        }
        else if (
            moving &&
            !crouched &&
            aiming)
        {
            locomotionState =
                engine::scene::
                    CharacterLocomotionState::
                        Walk;
        }
        else if (moving)
        {
            locomotionState =
                engine::scene::
                    CharacterLocomotionState::
                        Run;
        }

        engine::scene::
            CharacterAnimationStateInput
                animationInput;

        animationInput.deltaSeconds =
            static_cast<double>(
                safeDeltaSeconds);

        animationInput.viewMode =
            viewMode_;

        animationInput.stance =
            stance_;

        animationInput.movementDirection =
            ResolveMovementDirection(
                inputX,
                inputZ);

        animationInput.locomotionState =
            locomotionState;

        animationInput.movementSpeed =
            horizontalSpeed;

        animationInput.grounded =
            grounded_;

        animationInput.aiming =
            aiming;

        animationInput.upperBodyState =
            aiming
                ? engine::scene::
                    CharacterUpperBodyState::
                        Aiming
                : engine::scene::
                    CharacterUpperBodyState::
                        Relaxed;

        if (reloadPressed)
        {
            animationInput.actionRequest =
                engine::scene::
                    CharacterActionState::
                        Reload;

            animationInput.restartAction =
                true;
        }
        else if (primaryActionPressed)
        {
            animationInput.actionRequest =
                engine::scene::
                    CharacterActionState::
                        Primary;

            animationInput.restartAction =
                true;
        }

        animationStateMachine_.Update(
            animationInput,
            characterAnimation);

        const bool crouchedCamera =
            stance_ ==
                engine::scene::
                    CharacterStance::Crouched;

        float resolvedHeadY =
            nextY +
            (
                crouchedCamera
                    ? CrouchedFallbackHeadHeight
                    : StandingFallbackHeadHeight
            );

        DirectX::XMFLOAT3 animatedHeadPosition;

        if (
            characterRenderer.TryGetBoneWorldPosition(
                playerEntityId_,
                "Bip01_Head",
                animatedHeadPosition))
        {
            const float minimumHeadHeight =
                crouchedCamera
                    ? nextY + 0.75F
                    : nextY + 1.20F;

            const float maximumHeadHeight =
                crouchedCamera
                    ? nextY + 1.60F
                    : nextY + 2.15F;

            resolvedHeadY =
                std::clamp(
                    animatedHeadPosition.y,
                    minimumHeadHeight,
                    maximumHeadHeight);
        }

        const DirectX::XMFLOAT3 targetAnchor
        {
            nextX,
            resolvedHeadY,
            nextZ
        };

        if (!cameraAnchorValid_)
        {
            cameraAnchor_ = targetAnchor;
            cameraAnchorValid_ = true;
        }
        else
        {
            /*
             * Горизонталь следует за controller без lag.
             * Только высота Bip01_Head фильтруется,
             * поэтому шаги не трясут TPS-камеру.
             */
            cameraAnchor_.x = targetAnchor.x;
            cameraAnchor_.z = targetAnchor.z;

            const float verticalDifference =
                targetAnchor.y -
                cameraAnchor_.y;

            if (
                std::fabs(verticalDifference) >
                    CameraHeadVerticalDeadZone)
            {
                const float anchorBlend =
                    1.0F -
                    std::exp(
                        -CameraHeadFollowSpeed *
                        safeDeltaSeconds);

                cameraAnchor_.y +=
                    verticalDifference *
                    anchorBlend;
            }
        }

        const DirectX::XMVECTOR cameraForward =
            BuildForwardVector(
                cameraYawRadians_,
                cameraPitchRadians_);

        const DirectX::XMVECTOR cameraRight =
            BuildRightVector(
                cameraYawRadians_);

        if (firstPersonView)
        {
            DirectX::XMVECTOR firstPersonCamera =
                DirectX::XMLoadFloat3(
                    &cameraAnchor_);

            firstPersonCamera =
                DirectX::XMVectorAdd(
                    firstPersonCamera,
                    DirectX::XMVectorScale(
                        cameraForward,
                        FirstPersonForwardOffset));

            firstPersonCamera =
                DirectX::XMVectorAdd(
                    firstPersonCamera,
                    DirectX::XMVectorSet(
                        0.0F,
                        FirstPersonUpOffset,
                        0.0F,
                        0.0F));

            DirectX::XMStoreFloat3(
                &cameraPosition_,
                firstPersonCamera);
        }
        else
        {
            DirectX::XMVECTOR cameraTarget =
                DirectX::XMVectorAdd(
                    DirectX::XMLoadFloat3(
                        &cameraAnchor_),
                    DirectX::XMVectorSet(
                        0.0F,
                        ThirdPersonTargetUpOffset,
                        0.0F,
                        0.0F));

            DirectX::XMVECTOR desiredCamera =
                DirectX::XMVectorAdd(
                    cameraTarget,
                    DirectX::XMVectorScale(
                        cameraRight,
                        ThirdPersonShoulderOffset));

            desiredCamera =
                DirectX::XMVectorSubtract(
                    desiredCamera,
                    DirectX::XMVectorScale(
                        cameraForward,
                        ThirdPersonCameraDistance));

            DirectX::XMFLOAT3 desiredCameraPosition;

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

            cameraPosition_ =
                desiredCameraPosition;
        }
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

        const POINT topLeft
        {
            static_cast<LONG>(
                std::lround(
                    viewportX)),

            static_cast<LONG>(
                std::lround(
                    viewportY))
        };

        const POINT bottomRight
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

        const POINT center
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

        const RECT clipRectangle
        {
            topLeft.x,
            topLeft.y,
            bottomRight.x,
            bottomRight.y
        };

        if (!cursorCaptured_)
        {
            POINT previous{};

            if (GetCursorPos(
                    &previous))
            {
                restoreCursorX_ =
                    static_cast<std::int32_t>(
                        previous.x);

                restoreCursorY_ =
                    static_cast<std::int32_t>(
                        previous.y);
            }

            SetFocus(window);
            SetCapture(window);

            while (
                ShowCursor(FALSE) >= 0)
            {
            }

            cursorCaptured_ =
                true;

            ClipCursor(
                &clipRectangle);

            SetCursorPos(
                center.x,
                center.y);

            return;
        }

        ClipCursor(
            &clipRectangle);
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

        while (
            ShowCursor(TRUE) < 0)
        {
        }

        SetCursorPos(
            restoreCursorX_,
            restoreCursorY_);

        cursorCaptured_ =
            false;
    }
}
