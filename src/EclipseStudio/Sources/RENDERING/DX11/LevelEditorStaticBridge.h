#pragma once

#include "r3dRendererConfig.h"

#if LTS_STUDIO_DX11

#include <d3d11.h>

class GameObject;
struct WorldDX11FrameDesc;

bool LevelEditorStaticBridge_IsRequested();
bool LevelEditorStaticBridge_IsReady();
bool LevelEditorStaticBridge_IsActive();

bool LevelEditorStaticBridge_Initialize();
void LevelEditorStaticBridge_Shutdown();

bool LevelEditorStaticBridge_ShouldReplaceObject(
    const GameObject* object);

bool LevelEditorStaticBridge_Render(
    const WorldDX11FrameDesc& frame,
    ID3D11RenderTargetView* colorTarget,
    ID3D11DepthStencilView* depthTarget,
    const D3D11_VIEWPORT& viewport);

#endif