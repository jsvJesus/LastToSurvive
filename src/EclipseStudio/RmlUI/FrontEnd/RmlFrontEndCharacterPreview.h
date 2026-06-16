#pragma once

#include "r3d.h"

struct wiCharDataFull;

class obj_Player;

class RmlFrontEndCharacterPreview final
{
public:
    RmlFrontEndCharacterPreview();
    ~RmlFrontEndCharacterPreview();

    bool Initialize(
        const wiCharDataFull& Character
    );

    void Shutdown();

    void SetCharacter(
        const wiCharDataFull& Character
    );

    void PrepareFrame();
    void RenderFrame();

    void Rotate(
        float DeltaPixelsX,
        float DeltaPixelsY
    );

    void Move(
        float DeltaPixelsX,
        float DeltaPixelsY
    );

    void Zoom(
        float WheelSteps
    );

    void ResetView();

    bool IsInitialized() const;

private:
    bool CreatePortraitTarget();
    void ReleasePortraitTarget();

    void ApplyFullBodyCamera();
    void ApplyPortraitCamera();

    void RenderCharacterToTarget(
        bool bPortrait
    );

    void FinishPreparedFrame();

private:
    obj_Player* Player = nullptr;

    r3dCamera PreviousCamera;

    float ViewYawDegrees = 0.0f;
    float ViewDistanceScale = 1.0f;

    float ViewHorizontalOffset = 0.0f;
    float ViewVerticalOffset = 0.0f;

    bool bInitialized = false;
    bool bFramePrepared = false;
};