# DX9 to DX11 File Matrix

Static audit for the parallel DX11 world renderer migration.

Build/run status is intentionally not recorded here because this pass was
performed without launching a build or the game/editor.

## Migration Rules

- Keep DX9 renderer files in place.
- Keep DX9 as the fallback path.
- Keep UI, Scaleform, and RmlUI on DX9 until the DX11 world path is complete.
- Add DX11 code in explicit `DX11` paths or files.
- Gate DX11 world rendering with compile-time flags and the `-dx11world`
  runtime switch.
- Do not make DX11 own the window, backbuffer, UI, or `Present` in the early
  world-renderer stages.

## Current Gates

| Area | Current file | Current state |
|---|---|---|
| Compile flags | `src/Eternity/Include/r3dRendererConfig.h` | Defines `LTS_STUDIO_DX9`, `LTS_STUDIO_DX11`, and `LTS_STUDIO_DX11_WORLD` defaults. |
| Studio x64 flags | `src/EclipseStudio/GameEditor.vcxproj` | `Release\|x64` defines `LTS_STUDIO_DX11=1` and `LTS_STUDIO_DX11_WORLD=1`. |
| Runtime world switch | `src/EclipseStudio/Sources/RENDERING/World/WorldRenderer.h` | Uses `-dx11world` or `/dx11world` to select DX11 world. |
| Runtime preview switch | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Uses `-dx11preview` or `/dx11preview`. |
| Runtime smoke/debug switch | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Uses `-dx11smoke` or `/dx11smoke`. |
| Runtime atlas refresh switch | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Uses `-dx11terrainatlasrefresh` or `/dx11terrainatlasrefresh` for the expensive forced Terrain2 atlas RT refresh diagnostic path. Normal preview uses a one-volume-per-frame auto refresh when visible atlas signatures change. |
| Fallback decision | `src/EclipseStudio/Sources/RENDERING/World/WorldRenderer.h` | `WorldRender_TryRenderDX11()` returns false on unavailable DX11 so DX9 continues. |
| Preview status overlay | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | `-dx11preview` draws the preview panel even when the DX11 texture is not valid, with a short status string. |
| DX11 shutdown | `src/EclipseStudio/Sources/Game.cpp` | `DestroyGame()` calls `WorldRender_Shutdown()` before world/material resources are destroyed. |
| Debug readback gating | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Preview readback is gated by `-dx11preview`; smoke readback texture is created only for `-dx11smoke`. |
| Runtime flag caching | `src/EclipseStudio/Sources/RENDERING/World/WorldRenderer.h`, `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | DX11 command-line switches are cached after first query. |
| Frame sync | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Normal DX11 world frame no longer calls `ID3D11DeviceContext::Flush()`; shutdown still flushes after `ClearState()`. |
| Frame target failure logs | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Frame target creation failures are logged once and reset after a successful target recreate. |
| Frame stage failure logs | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Internal DX11 world stage failures unbind targets, log the failed stage once, disable the DX11 world path until shutdown/reinit, and fall back to DX9 world. |
| Terrain patches | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11_Terrain.hpp`, `src/EclipseStudio/Sources/RENDERING/DX11/DrawWorldDX11.hpp` | DX11 terrain currently writes color and depth in the GBuffer pass using visible Terrain2 atlas tiles through the terrain patch cache, with the patch-grid path still available as fallback. The separate terrain depth prepass is kept out of the preview path to avoid double terrain draw cost. |
| Terrain textures | `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp`, `bin/Data/Shaders/DX11_P1/Nature/dx11_terrain.hls` | DX11 terrain bridges Terrain2 layer/mask/atlas textures, tracks visible atlas signatures, refreshes changed atlas volumes with a small per-frame budget, keeps forced full refresh behind `-dx11terrainatlasrefresh`, and uses wrap sampling for tiled layers plus clamp sampling for terrain color, masks, and atlas textures. |

## Core Renderer

| DX9 file | DX11 file/status | Notes |
|---|---|---|
| `src/Eternity/Source/r3d.cpp` | Not split into `src/Eternity/DX11/Source/r3dDX11.cpp` yet | DX9 core remains the owner. |
| `src/Eternity/Source/r3dRender.CPP` | World DX11 implementation currently lives in `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Dedicated Eternity DX11 renderer core is still pending. |
| `src/Eternity/Include/r3dRender.h` | Not split into `src/Eternity/DX11/Include/r3dRenderDX11.h` yet | Shared renderer interface is still DX9-centered. |
| `src/Eternity/Source/r3dBuffer.cpp` | Not split into `src/Eternity/DX11/Source/r3dBufferDX11.cpp` yet | DX11 buffer helpers currently exist inside `RenderDX11.cpp`. |
| `src/Eternity/Include/r3dbuffer.h` | Not split into `src/Eternity/DX11/Include/r3dBufferDX11.h` yet | Dedicated DX11 buffer wrapper is pending. |
| `src/Eternity/Source/r3dTex.cpp` | Not split into `src/Eternity/DX11/Source/r3dTexDX11.cpp` yet | DX11 texture bridge code currently lives in `RenderDX11.cpp`. |
| `src/Eternity/Include/r3dTex.h` | Not split into `src/Eternity/DX11/Include/r3dTexDX11.h` yet | Dedicated DX11 texture wrapper is pending. |
| `src/Eternity/Source/r3dCubeMap.cpp` | Not split into `src/Eternity/DX11/Source/r3dCubeMapDX11.cpp` yet | Pending. |
| `src/Eternity/Include/r3dCubeMap.h` | Not split into `src/Eternity/DX11/Include/r3dCubeMapDX11.h` yet | Pending. |
| `src/Eternity/Source/r3dD3DCache.cpp` | Not split into `src/Eternity/DX11/Source/r3dD3DCacheDX11.cpp` yet | Pending. |
| `src/Eternity/Include/r3dD3DCache.h` | Not split into `src/Eternity/DX11/Include/r3dD3DCacheDX11.h` yet | Pending. |

## World Renderer Bridge

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/EclipseStudio/Sources/RENDERING/DX9/RenderDX9.cpp` | Existing | DX9 renderer remains available. |
| `src/EclipseStudio/Sources/RENDERING/DX9/DrawWorld.hpp` | Existing | DX9 world draw remains available. |
| `src/EclipseStudio/Sources/RENDERING/World/WorldRenderer.h` | Existing bridge | Chooses DX9 or DX11 world backend. |
| `src/EclipseStudio/Sources/RENDERING/World/WorldDX11.h` | Existing DX11 facade | Provides stubbed false/no-op functions when DX11 is disabled. |
| `src/EclipseStudio/Sources/RENDERING/World/WorldDX11.cpp` | Existing DX11 facade | Calls `RenderDX11_*` behind compile-time guards. |
| `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.h` | Existing DX11 interface | Declares init/shutdown/render/preview functions. |
| `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11.cpp` | Existing DX11 implementation | Contains device, states, targets, shaders, preview, terrain path, and fallback return. |
| `src/EclipseStudio/Sources/RENDERING/DX11/DrawWorldDX11.hpp` | Existing frame skeleton | Contains begin/depth/GBuffer/lighting/transparent/post/end hooks. |
| `src/EclipseStudio/Sources/RENDERING/DX11/RenderDX11_Terrain.hpp` | Existing terrain helper | Used by the DX11 render path. |

## Deferred, GBuffer, and Post Effects

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/EclipseStudio/Sources/RENDERING/Deffered/RenderDeffered.cpp` | Calls `WorldRender_TryRenderDX11()` when enabled | DX9 deferred path remains fallback. |
| `src/EclipseStudio/Sources/RENDERING/Deffered/RenderDefferedScene.hpp` | Calls `WorldRender_TryRenderDX11()` when enabled | DX9 scene path remains fallback. |
| `src/EclipseStudio/Sources/RENDERING/Deffered/D3DMiscFunctions.h` | No dedicated `D3DMiscFunctionsDX11.cpp` found | Pending separate DX11 split. |
| `src/EclipseStudio/Sources/RENDERING/Deffered/PostFXChief.cpp` | No dedicated `PostFXChiefDX11.cpp` found | Pending separate DX11 split. |
| `src/EclipseStudio/Sources/RENDERING/Deffered/PostFX.h` | No dedicated `PostEffectsDX11.hpp` found | Pending separate DX11 split. |
| `src/EclipseStudio/Sources/RENDERING/Deffered/FogEffects.hpp` | No dedicated `FogEffectsDX11.hpp` found | Pending separate DX11 split. |

## Terrain, Nature, Sky, and Clouds

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/GameEngine/TrueNature/Terrain.cpp` | No `src/GameEngine/DX11/TrueNature/TerrainDX11.cpp` found | Pending split. |
| `src/GameEngine/TrueNature/ITerrain.cpp` | No `src/GameEngine/DX11/TrueNature/ITerrainDX11.cpp` found | Pending split. |
| `src/GameEngine/TrueNature2/Terrain2.cpp` | DX11 access is bridged by existing `RenderDX11.cpp` terrain code | Dedicated `Terrain2DX11.cpp` not found. |
| `src/GameEngine/TrueNature/SkyDome.cpp` | No `src/GameEngine/DX11/TrueNature/SkyDomeDX11.cpp` found | Pending. |
| `src/GameEngine/TrueNature/CloudPlane/Cloud.cpp` | No `src/GameEngine/DX11/TrueNature/CloudPlane/CloudDX11.cpp` found | Pending. |
| `src/GameEngine/TrueNature/CloudPlane/CloudGrid.cpp` | No `CloudGridDX11.cpp` found | Pending. |
| `src/GameEngine/TrueNature/CloudPlane/CloudPlane.cpp` | No `CloudPlaneDX11.cpp` found | Pending. |
| `src/GameEngine/TrueNature/CloudPlane/Shaders.cpp` | No `ShadersDX11.cpp` found | Pending. |

## World Objects

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/EclipseStudio/Sources/ObjectsCode/WORLD/water.cpp` | No `src/EclipseStudio/Sources/DX11/ObjectsCode/WORLD/waterDX11.cpp` found | Pending. |
| `src/EclipseStudio/Sources/ObjectsCode/WORLD/obj_Road.cpp` | No `obj_RoadDX11.cpp` found | Pending. |
| `src/EclipseStudio/Sources/ObjectsCode/WORLD/Lamp.cpp` | No `LampDX11.cpp` found | Pending. |
| `src/EclipseStudio/Sources/ObjectsCode/WORLD/DecalChief.cpp` | No `DecalChiefDX11.cpp` found | Pending. |
| `src/EclipseStudio/Sources/ObjectsCode/Nature/GrassEditorPlanes.cpp` | No `GrassEditorPlanesDX11.cpp` found | Pending. |

## Meshes, Objects, and Particles

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/GameEngine/gameobjects/obj_Mesh.cpp` | No `src/GameEngine/DX11/gameobjects/obj_MeshDX11.cpp` found | Pending. |
| `src/GameEngine/gameobjects/sceneBox.cpp` | No `sceneBoxDX11.cpp` found | Pending. |
| `src/Eternity/Source/Particle.cpp` | No `src/Eternity/DX11/Source/ParticleDX11.cpp` found | Pending. |
| `src/Eternity/Source/r3dObj.cpp` | No `r3dObjDX11.cpp` found in `src/Eternity/DX11` | Pending or not present in this checkout. |

## Shaders, Materials, and Fonts

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/Eternity/Source/VShader.cpp` | No `src/Eternity/DX11/Source/VShaderDX11.cpp` found | Pending. |
| `src/Eternity/Include/VShader.h` | No `VShaderDX11.h` found | Pending. |
| `src/Eternity/Source/PShader.cpp` | No `PShaderDX11.cpp` found | Pending. |
| `src/Eternity/Include/pShader.h` | No `PShaderDX11.h` found | Pending. |
| `src/Eternity/Source/r3dMat.cpp` | No `r3dMatDX11.cpp` found | Pending. |
| `src/Eternity/Include/r3dMat.h` | No `r3dMatDX11.h` found | Pending. |
| `src/Eternity/Source/d3dFont.cpp` | No `d3dFontDX11.cpp` found | UI/font migration should remain late-stage. |
| `src/Eternity/Include/d3dFont.h` | No `d3dFontDX11.h` found | UI/font migration should remain late-stage. |

## UI and RmlUI

| DX9/current file | DX11 file/status | Notes |
|---|---|---|
| `src/EclipseStudio/RmlUI/RmlRenderDX9.cpp` | No `RmlRenderDX11.cpp` found | Correct for current roadmap stage. |
| `src/EclipseStudio/RmlUI/RmlRenderDX9.h` | No `RmlRenderDX11.h` found | Keep DX9 RmlUI fallback. |
| `src/EclipseStudio/RmlUI/RmlRuntime.cpp` | DX9 device based | UI migration is late-stage only. |

## Shader Assets

| Expected DX11 shader area | Current status |
|---|---|
| `bin/Data/Shaders/DX11_P1` | Referenced by existing DX11 roadmap/code. |
| Terrain shader | Existing code expects DX11 terrain shader assets. |
| Water/grass/tree/character/particle/post shaders | Not audited in detail in this pass. |

## High-Value DX9 Dependency Clusters

The static search shows the heaviest remaining DX9/D3DX dependency clusters in:

- `src/Eternity/Source/*` and `src/Eternity/Include/*` renderer primitives.
- `src/GameEngine/TrueNature2/Terrain2.cpp` terrain resources, D3DX texture conversion, and render states.
- `src/EclipseStudio/Sources/RENDERING/Deffered/*` deferred/post effects.
- `src/EclipseStudio/RmlUI/*` UI backend, intentionally DX9 for now.
- `src\EclipseStudio\Sources\ObjectsCode\WORLD\*` water, roads, decals, lights.
- `src\EclipseStudio\Sources\ObjectsCode\Nature\*` grass and vegetation.
- `src\GameEngine\gameobjects\*` meshes and scene objects.
- `src\EclipseStudio\Sources\ObjectsCode\WEAPONS\*` weapon/attachment matrix and effect paths.

## Immediate Next Work

1. Keep `RenderDX11.cpp` as the active DX11 world implementation for now.
2. Avoid moving code into `src/Eternity/DX11` until a small wrapper can be
   extracted without changing behavior.
3. Split the next migration by subsystem:
   - terrain helpers out of the monolithic `RenderDX11.cpp`;
   - texture bridge helpers;
   - buffer/constant-buffer helpers;
   - state/shader helpers.
4. Keep the current DX9 fallback return from `RenderDX11_RenderWorld()` until
   the world composite stage is ready.
5. Keep GPU readback out of the normal `-dx11world` path; use `-dx11preview`
   and `-dx11smoke` for CPU readback diagnostics.
6. Keep frame-level synchronization explicit and rare; avoid per-frame
   `Flush()` unless a later diagnostic path deliberately requires it.
7. Keep failure exits centralized so fallback to DX9 stays predictable.

## Verification Not Run

- No `Studio.exe x64 Release` build was launched.
- No editor/game runtime was launched.
- No Alt+Tab, resize, fallback, or preview visual checks were run.
