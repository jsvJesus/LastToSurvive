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

    bool IsInitialized() const;

private:
    void RenderCharacterToTarget();
    void FinishPreparedFrame();

private:
    obj_Player* Player = nullptr;

    r3dCamera PreviousCamera;

    bool bInitialized = false;
    bool bFramePrepared = false;
};