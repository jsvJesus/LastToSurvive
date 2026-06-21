#include "r3dPCH.h"
#include "r3d.h"
#include "r3dNetwork.h"
#include "shellapi.h"
#include "resource.h"

#include "Particle.h"

#pragma warning (disable: 4244)
#pragma warning (disable: 4305)
#pragma warning (disable: 4101)

#include "cvar.h"
#include "fmod/soundsys.h"

#include "GameCommon.h"
#include "GameLevel.h"

#include "ObjectsCode/world/EnvmapProbes.h"
#include "ObjectsCode/world/DecalChief.h"
#include "ObjectsCode/world/MaterialTypes.h"
#include "ObjectsCode/world/WaterPlane.h"

#include "ObjectsCode/Nature/wind.h"

#include "../SF/CmdProcessor/CmdProcessor.h"
#include "../SF/Console/Config.h"
#include "../SF/Console/EngineConsole.h"
#include "../SF/Version.h"

#include "Rendering/Deffered/CommonPostFX.h"

#include "Menus\m_AppSelect.h"
#include "Menus\m_Main.h"

#include "UI\m_LoadingScreen.h"
#include "DiscordPresence.h"

#include "UI/HUDCameraEffects.h"

#include "Editors/ObjectManipulator3d.h"
#include "Editors/LevelEditor_Collections.h"

#include "RENDERING\Deffered\VisibilityGrid.h"
#include "rendering\Deffered\D3DMiscFunctions.h"
#include "rendering\Probes\ProbeMaster.h"

#include "RENDERING\DX11\RenderDX11.h"
#include "RENDERING\DX11\RenderDX11World.h"

#include "ObjectsCode/weapons/ClientWeaponArmory.h"

#include "CkHttpRequest.h"
#include "CkHttp.h"
#include "CkHttpResponse.h"

#include "DamageLib.h"
#include "MeshPropertyLib.h"

#include "JobChief.h"
#include "r3dBackgroundTaskDispatcher.h"
#include "Rendering/Deffered/RenderDeferredPointLightsOptimized.h"

#include "LangMngr.h"

#include "HWInfo.h"
#include "SteamHelper.h"

#include "ObjectsCode/Nature/GrassLib.h"

#include "ObjectsCode/Gameplay/obj_Zombie.h"

#include "ObjectsCode/WEAPONS/FlashbangVisualController.h"
#include "../../Eternity/Source/r3dEternityWebBrowser.h"
#include "Editors/CollectionsManager.h"

#include "r3dDeviceQueue.h"

#include "GameCode\UserRewards.h"
#include "GameCode\UserSettings.h"

#include "../RmlUI/RmlUISystem.h"
#include "../RmlUI/RmlRuntime.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>

#include <dwmapi.h>

#include "ui/FrontendShared.h"
#pragma comment(lib, "dwmapi.lib")

extern bool g_bEditMode;
extern bool g_bStartedAsParticleEditor;
extern bool g_bExit;

#include "Gameplay_Params.h"
	const CGamePlayParams* GPP = new CGamePlayParams();

CD3DFont* 	Font_Label;
CD3DFont* 	Font_Editor;

extern void 	PlayEditor();

extern char	LevelEditName[64];
extern char initialCameraSpotName[64];
extern HANDLE	r3d_CurrentProcess;
extern void r3dFreeGOBMeshes();
extern void AI_Player_FreeStuff();
#if APEX_ENABLED
void DestroyApexUserRenderer();
#endif

void r3dInitShaders();

void SaveSettingsCallback( int oldI, float oldF )
{
	void writeGameOptionsFile() ;
	writeGameOptionsFile() ;
}

void CursorModeCallback( int oldI, float oldF )
{
	if( !oldI )
	{
		r3dMouse::Show( true ) ;
	}
}

extern void RegisterMsgProc(
	bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam)
);

extern void UnregisterMsgProc(
	bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam)
);

static bool g_StudioResizePending = false;
static int g_StudioPendingWidth = 0;
static int g_StudioPendingHeight = 0;
static r3dDX11Renderer* g_DX11Renderer = nullptr;
static bool g_StudioCmdLineDX11Boot = false;
static bool g_StudioCmdLineDX11World = false;

static Rml::Context* g_DX11SmokeRmlContext = nullptr;
static Rml::ElementDocument* g_DX11SmokeRmlDocument = nullptr;
static bool g_DX11SmokeRmlReady = false;

static bool IsDX11BootActive()
{
	return g_DX11Renderer && g_DX11Renderer->IsInitialized();
}

bool StudioDX11WorldHybridEnabled()
{
	return g_StudioCmdLineDX11World;
}

static void ShutdownDX11SmokeRml()
{
	RmlRuntime& Runtime = RmlRuntime::Get();

	if (g_DX11SmokeRmlContext)
	{
		Runtime.ClearActiveContext(g_DX11SmokeRmlContext);

		if (g_DX11SmokeRmlDocument)
		{
			g_DX11SmokeRmlContext->UnloadDocument(g_DX11SmokeRmlDocument);
			g_DX11SmokeRmlDocument = nullptr;
		}

		Runtime.DestroyContext(g_DX11SmokeRmlContext);
	}

	g_DX11SmokeRmlReady = false;
}

static bool InitDX11SmokeRml()
{
	if (g_DX11SmokeRmlReady)
		return true;

	if (!g_DX11Renderer || !g_DX11Renderer->IsInitialized())
	{
		r3dOutToLog("[DX11][RmlUI] Smoke RML init failed: DX11 renderer is not ready\n");
		return false;
	}

	RmlRuntime& Runtime = RmlRuntime::Get();

	g_DX11SmokeRmlContext = Runtime.CreateContext(
		"DX11Smoke",
		Rml::Vector2i(
			g_DX11Renderer->GetWidth(),
			g_DX11Renderer->GetHeight()
		)
	);

	if (!g_DX11SmokeRmlContext)
	{
		r3dOutToLog("[DX11][RmlUI] Smoke RML init failed: CreateContext failed\n");
		return false;
	}

	g_DX11SmokeRmlDocument =
		g_DX11SmokeRmlContext->LoadDocument("Rml/Studio/DX11Smoke.rml");

	if (!g_DX11SmokeRmlDocument)
	{
		r3dOutToLog("[DX11][RmlUI] Failed to load Data/Rml/Studio/DX11Smoke.rml\n");
		r3dOutToLog("[DX11][RmlUI] Trying fallback Data/Rml/Studio/AppSelect.rml\n");

		g_DX11SmokeRmlDocument =
			g_DX11SmokeRmlContext->LoadDocument("Rml/Studio/AppSelect.rml");
	}

	if (!g_DX11SmokeRmlDocument)
	{
		r3dOutToLog("[DX11][RmlUI] Smoke RML init failed: no document loaded\n");

		Runtime.DestroyContext(g_DX11SmokeRmlContext);
		g_DX11SmokeRmlContext = nullptr;

		return false;
	}

	g_DX11SmokeRmlDocument->Show();

	Runtime.SetActiveContext(g_DX11SmokeRmlContext);

	g_DX11SmokeRmlReady = true;

	r3dOutToLog(
		"[DX11][RmlUI] Smoke document loaded. context=%s size=%dx%d\n",
		g_DX11SmokeRmlContext->GetName().c_str(),
		g_DX11Renderer->GetWidth(),
		g_DX11Renderer->GetHeight()
	);

	return true;
}

void StudioDX11WorldHybridInit()
{
	if (!g_StudioCmdLineDX11World || g_DX11Renderer)
		return;

	g_DX11Renderer = new r3dDX11Renderer;
	if (!g_DX11Renderer->Init(
			win::hWnd,
			r_width->GetInt(),
			r_height->GetInt(),
			r_fullscreen->GetBool(),
#ifndef FINAL_BUILD
			true
#else
			false
#endif
			,
			false
		))
	{
		SAFE_DELETE(g_DX11Renderer);
		r3dOutToLog("[DX11][World] Hybrid renderer init failed\n");
		return;
	}

	r3dOutToLog("[DX11][World] Hybrid renderer initialized\n");
}

void StudioDX11WorldHybridShutdown()
{
	if (!g_StudioCmdLineDX11World || !g_DX11Renderer)
		return;

	g_DX11Renderer->Shutdown();
	SAFE_DELETE(g_DX11Renderer);
	r3dOutToLog("[DX11][World] Hybrid renderer shutdown\n");
}

static const char* DX11CheckState(bool pass, bool warn)
{
	if (pass)
		return "PASS";

	if (warn)
		return "WARN";

	return "FAIL";
}

static void LogDX11WorldValidation(
	const r3dDX11WorldRenderStats& S,
	bool bWorldRendered,
	bool bLightingRendered
)
{
	r3dOutToLog("[DX11][Check] ================= World parity check =================\n");

	// 1. Static mesh виден в depth.
	{
		const bool pass =
			bWorldRendered &&
			S.DepthStaticMeshes > 0 &&
			S.DepthDrawnMeshes > 0 &&
			S.DepthSkippedFailed == 0;

		const bool warn =
			bWorldRendered &&
			S.DepthStaticMeshes > 0 &&
			S.DepthDrawnMeshes > 0 &&
			S.DepthSkippedFailed > 0;

		r3dOutToLog(
			"[DX11][Check][%s] 01 Static mesh depth: depth_static=%u depth_drawn=%u depth_failed=%u\n",
			DX11CheckState(pass, warn),
			S.DepthStaticMeshes,
			S.DepthDrawnMeshes,
			S.DepthSkippedFailed
		);
	}

	// 2. Skinned zombie/player виден в depth.
	{
		const bool pass =
			bWorldRendered &&
			S.DepthSkinnedMeshes > 0 &&
			S.DepthDrawnMeshes > 0;

		const bool warn =
			bWorldRendered &&
			S.DepthSkinnedMeshes == 0 &&
			S.ShadowSkinnedMeshes > 0;

		r3dOutToLog(
			"[DX11][Check][%s] 02 Skinned depth: depth_skin=%u shadow_skin=%u depth_drawn=%u\n",
			DX11CheckState(pass, warn),
			S.DepthSkinnedMeshes,
			S.ShadowSkinnedMeshes,
			S.DepthDrawnMeshes
		);
	}

	// 3. Alpha-test mesh режется правильно.
	{
		const bool pass =
			bWorldRendered &&
			S.DepthAlphaTestedMeshes > 0;

		const bool warn =
			bWorldRendered &&
			S.DepthAlphaTestedMeshes == 0 &&
			(S.ShadowAlphaTested > 0 || S.TransparentShadowAlphaTested > 0);

		r3dOutToLog(
			"[DX11][Check][%s] 03 Alpha-test depth: depth_alpha=%u shadow_alpha=%u transparent_shadow_alpha=%u\n",
			DX11CheckState(pass, warn),
			S.DepthAlphaTestedMeshes,
			S.ShadowAlphaTested,
			S.TransparentShadowAlphaTested
		);
	}

	// 4. First-person weapon не ломает depth range.
	{
		const bool pass =
			bWorldRendered &&
			S.DepthFirstPersonMeshes > 0;

		const bool warn =
			bWorldRendered &&
			S.DepthFirstPersonMeshes == 0;

		r3dOutToLog(
			"[DX11][Check][%s] 04 First-person depth range: depth_fp=%u depth_failed=%u\n",
			DX11CheckState(pass, warn),
			S.DepthFirstPersonMeshes,
			S.DepthSkippedFailed
		);
	}

	// 5. Static/skinned shadows совпадают визуально.
	{
		const bool pass =
			S.ShadowSlicesRendered > 0 &&
			S.ShadowDrawnMeshes > 0 &&
			S.ShadowStaticMeshes > 0 &&
			S.ShadowSkinnedMeshes > 0 &&
			S.ShadowSkippedFailed == 0;

		const bool warn =
			S.ShadowSlicesRendered > 0 &&
			S.ShadowDrawnMeshes > 0 &&
			S.ShadowStaticMeshes > 0 &&
			S.ShadowSkippedFailed <= 2;

		r3dOutToLog(
			"[DX11][Check][%s] 05 Static/skinned shadows: slices=%u shadow_drawn=%u static=%u skinned=%u failed=%u\n",
			DX11CheckState(pass, warn),
			S.ShadowSlicesRendered,
			S.ShadowDrawnMeshes,
			S.ShadowStaticMeshes,
			S.ShadowSkinnedMeshes,
			S.ShadowSkippedFailed
		);
	}

	// 6. Alpha-tested shadows есть у заборов/листвы/решёток.
	{
		const bool pass =
			S.ShadowSlicesRendered > 0 &&
			(S.ShadowAlphaTested > 0 || S.TransparentShadowAlphaTested > 0) &&
			(S.ShadowDrawnMeshes > 0 || S.TransparentShadowDrawnMeshes > 0);

		const bool warn =
			S.ShadowSlicesRendered > 0 &&
			S.ShadowAlphaTested == 0 &&
			S.TransparentShadowAlphaTested == 0;

		r3dOutToLog(
			"[DX11][Check][%s] 06 Alpha-tested shadows: shadow_alpha=%u tshadow_alpha=%u shadow_drawn=%u tshadow_drawn=%u\n",
			DX11CheckState(pass, warn),
			S.ShadowAlphaTested,
			S.TransparentShadowAlphaTested,
			S.ShadowDrawnMeshes,
			S.TransparentShadowDrawnMeshes
		);
	}

	// 7. Directional/point/spot lights работают.
	{
		const bool pass =
			bLightingRendered &&
			S.LightingPasses > 0 &&
			S.LightingDirectionalLights > 0 &&
			S.LightingPointLights > 0 &&
			S.LightingSpotLights > 0;

		const bool warn =
			bLightingRendered &&
			S.LightingPasses > 0 &&
			S.LightingDirectionalLights > 0;

		r3dOutToLog(
			"[DX11][Check][%s] 07 Lights: passes=%u dir=%u point=%u spot=%u failed=%u\n",
			DX11CheckState(pass, warn),
			S.LightingPasses,
			S.LightingDirectionalLights,
			S.LightingPointLights,
			S.LightingSpotLights,
			S.LightingSkippedFailed
		);
	}

	// 8. Spec/gloss похож на DX9.
	// Автоматом можно проверить только то, что spec/gloss decode включён.
	// Визуальное совпадение с DX9 проверяется скриншотом.
	{
		const bool pass =
			bLightingRendered &&
			S.LightingSpecGlossDecoded > 0 &&
			S.LightingGBufferDecoded > 0;

		const bool warn =
			bLightingRendered &&
			S.LightingSpecGlossDecoded == 0;

		r3dOutToLog(
			"[DX11][Check][%s] 08 Spec/gloss decode: gdecode=%u specgloss=%u -- visual DX9 parity still needs screenshot check\n",
			DX11CheckState(pass, warn),
			S.LightingGBufferDecoded,
			S.LightingSpecGlossDecoded
		);
	}

	// 9. Terrain пишет GBuffer.
	{
		const bool pass =
			S.TerrainGBufferDraws > 0 &&
			S.TerrainSkippedFailed == 0;

		const bool warn =
			S.TerrainGBufferDraws > 0 &&
			S.TerrainSkippedFailed > 0;

		r3dOutToLog(
			"[DX11][Check][%s] 09 Terrain GBuffer: terrain_g=%u tris=%u layers=%u detail=%u failed=%u\n",
			DX11CheckState(pass, warn),
			S.TerrainGBufferDraws,
			S.TerrainGBufferTriangles,
			S.TerrainSplatLayers,
			S.TerrainDetailLayers,
			S.TerrainSkippedFailed
		);
	}

	// 10. Terrain получает shadows и lighting.
	{
		const bool pass =
			S.TerrainGBufferDraws > 0 &&
			S.TerrainShadowDraws > 0 &&
			bLightingRendered &&
			S.LightingPasses > 0 &&
			S.LightingShadowed > 0;

		const bool warn =
			S.TerrainGBufferDraws > 0 &&
			bLightingRendered &&
			S.LightingPasses > 0;

		r3dOutToLog(
			"[DX11][Check][%s] 10 Terrain shadow/lighting: terrain_g=%u terrain_s=%u lighting=%u lshadow=%u\n",
			DX11CheckState(pass, warn),
			S.TerrainGBufferDraws,
			S.TerrainShadowDraws,
			S.LightingPasses,
			S.LightingShadowed
		);
	}

	r3dOutToLog("[DX11][Check] =====================================================\n");
}

void StudioDX11WorldHybridTick()
{
	if (!g_StudioCmdLineDX11World || !g_DX11Renderer || !g_DX11Renderer->IsInitialized())
		return;

	static DWORD LastWorldStatsLog = 0;
	static DWORD LastWorldValidationLog = 0;

	g_DX11Renderer->BeginFrame(
		0.010f,
		0.012f,
		0.011f,
		1.0f
	);

	r3dDX11WorldRenderStats WorldStats;
	r3dDX11ResetWorldRenderStats(WorldStats);

	const bool bWorldRendered =
		g_DX11Renderer->RenderWorldGBuffer(
			gCam,
			&WorldStats
		);

	const bool bLightingRendered =
		g_DX11Renderer->RenderWorldLighting(
			gCam,
			&WorldStats
		);

	g_DX11Renderer->EndFrame(false, nullptr);

	const DWORD Now = GetTickCount();

	if (Now - LastWorldStatsLog >= 1000)
	{
		r3dOutToLog(
			"[DX11][World] ok=%d world_ok=%d light_ok=%d total=%u mesh=%u "
			"depth_total=%u depth_mesh=%u depth_static=%u depth_skin=%u depth_alpha=%u depth_fp=%u depth_drawn=%u depth_unsupported=%u depth_failed=%u "
			"gbuffer_drawn=%u unsupported=%u failed=%u "
			"shadow=%u smesh=%u sstatic=%u sskin=%u sdraw=%u salpha=%u sslices=%u sunsupported=%u sfailed=%u "
			"tshadow=%u tsmesh=%u tsstatic=%u tsskin=%u tsdraw=%u tsalpha=%u tcase=%u tsunsupported=%u tsfailed=%u "
			"lighting=%u dir=%u point=%u spot=%u lshadow=%u gdecode=%u specgloss=%u fog=%u ambient=%u probe=%u lfailed=%u "
			"terrain_g=%u terrain_gtris=%u terrain_d=%u terrain_dtris=%u terrain_s=%u terrain_stris=%u terrain_layers=%u terrain_detail=%u terrain_failed=%u "
			"\n",

			(bWorldRendered && bLightingRendered) ? 1 : 0,
			bWorldRendered ? 1 : 0,
			bLightingRendered ? 1 : 0,

			WorldStats.TotalRenderables,
			WorldStats.MeshRenderables,

			WorldStats.DepthTotalRenderables,
			WorldStats.DepthMeshRenderables,
			WorldStats.DepthStaticMeshes,
			WorldStats.DepthSkinnedMeshes,
			WorldStats.DepthAlphaTestedMeshes,
			WorldStats.DepthFirstPersonMeshes,
			WorldStats.DepthDrawnMeshes,
			WorldStats.DepthSkippedUnsupported,
			WorldStats.DepthSkippedFailed,

			WorldStats.DrawnMeshes,
			WorldStats.SkippedUnsupported,
			WorldStats.SkippedFailed,

			WorldStats.ShadowRenderables,
			WorldStats.ShadowMeshRenderables,
			WorldStats.ShadowStaticMeshes,
			WorldStats.ShadowSkinnedMeshes,
			WorldStats.ShadowDrawnMeshes,
			WorldStats.ShadowAlphaTested,
			WorldStats.ShadowSlicesRendered,
			WorldStats.ShadowSkippedUnsupported,
			WorldStats.ShadowSkippedFailed,

			WorldStats.TransparentShadowRenderables,
			WorldStats.TransparentShadowMeshRenderables,
			WorldStats.TransparentShadowStaticMeshes,
			WorldStats.TransparentShadowSkinnedMeshes,
			WorldStats.TransparentShadowDrawnMeshes,
			WorldStats.TransparentShadowAlphaTested,
			WorldStats.TransparentShadowCasesRendered,
			WorldStats.TransparentShadowSkippedUnsupported,
			WorldStats.TransparentShadowSkippedFailed,

			WorldStats.LightingPasses,
			WorldStats.LightingDirectionalLights,
			WorldStats.LightingPointLights,
			WorldStats.LightingSpotLights,
			WorldStats.LightingShadowed,
			WorldStats.LightingGBufferDecoded,
			WorldStats.LightingSpecGlossDecoded,
			WorldStats.LightingFogApplied,
			WorldStats.LightingAmbientApplied,
			WorldStats.LightingProbeApplied,
			WorldStats.LightingSkippedFailed,

			WorldStats.TerrainGBufferDraws,
			WorldStats.TerrainGBufferTriangles,
			WorldStats.TerrainDepthDraws,
			WorldStats.TerrainDepthTriangles,
			WorldStats.TerrainShadowDraws,
			WorldStats.TerrainShadowTriangles,
			WorldStats.TerrainSplatLayers,
			WorldStats.TerrainDetailLayers,
			WorldStats.TerrainSkippedFailed
		);

		LastWorldStatsLog = Now;
	}

	if (Now - LastWorldValidationLog >= 5000)
	{
		LogDX11WorldValidation(
			WorldStats,
			bWorldRendered,
			bLightingRendered
		);

		LastWorldValidationLog = Now;
	}
}

static bool StudioWindowResizeMsgProc(
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	if (Message != WM_SIZE)
		return false;

	if (WParam == SIZE_MINIMIZED)
		return false;

	const int Width =
		static_cast<int>(LOWORD(LParam));

	const int Height =
		static_cast<int>(HIWORD(LParam));

	if (Width <= 0 || Height <= 0)
		return false;

	g_StudioPendingWidth = Width;
	g_StudioPendingHeight = Height;
	g_StudioResizePending = true;

	return false;
}

bool ProcessStudioPendingResize(
	RmlUISystem* ActiveRmlUI
)
{
	if (!g_StudioResizePending)
		return false;

	const int Width = g_StudioPendingWidth;
	const int Height = g_StudioPendingHeight;

	g_StudioResizePending = false;

	if (IsDX11BootActive())
	{
		if (Width <= 0 || Height <= 0)
			return false;

		r3dOutToLog(
			"[Studio][DX11] Resize backbuffer: %dx%d\n",
			Width,
			Height
		);

		const bool bResized =
			g_DX11Renderer->Resize(
				Width,
				Height
			);

		if (bResized)
		{
			r_width->SetInt(Width);
			r_height->SetInt(Height);

			InvalidateRect(
				win::hWnd,
				nullptr,
				FALSE
			);
		}

		return bResized;
	}

	if (!r3dRenderer)
		return false;

	if (!r3dRenderer->pd3ddev)
		return false;

	if (!r3dRenderer->d3dpp.Windowed)
		return false;

	if (Width <= 0 || Height <= 0)
		return false;

	const int CurrentWidth =
		static_cast<int>(
			r3dRenderer->d3dpp.BackBufferWidth
		);

	const int CurrentHeight =
		static_cast<int>(
			r3dRenderer->d3dpp.BackBufferHeight
		);

	if (
		CurrentWidth == Width &&
		CurrentHeight == Height
	)
	{
		return true;
	}

	r3dOutToLog(
		"[Studio] Resize backbuffer: %dx%d -> %dx%d\n",
		CurrentWidth,
		CurrentHeight,
		Width,
		Height
	);

	if (RmlRuntime::Get().IsInitialized())
	{
		RmlRuntime::Get().OnDeviceReset(
			Width,
			Height
		);
	}
	else if (
		ActiveRmlUI &&
		ActiveRmlUI->IsInitialized()
	)
	{
		ActiveRmlUI->OnDeviceReset();
	}

	const D3DPRESENT_PARAMETERS OldParameters =
		r3dRenderer->d3dpp;

	r3dRenderer->d3dpp.BackBufferWidth =
		static_cast<UINT>(Width);

	r3dRenderer->d3dpp.BackBufferHeight =
		static_cast<UINT>(Height);

	r3dRenderer->d3dpp.hDeviceWindow =
		win::hWnd;

	if (!r3dRenderer->Reset())
	{
		r3dOutToLog(
			"[Studio] Resize failed, restoring old backbuffer\n"
		);

		r3dRenderer->d3dpp = OldParameters;
		r3dRenderer->Reset();

		if (RmlRuntime::Get().IsInitialized())
		{
			RmlRuntime::Get().OnDeviceReset(
				static_cast<int>(
					r3dRenderer->d3dpp.BackBufferWidth
				),
				static_cast<int>(
					r3dRenderer->d3dpp.BackBufferHeight
				)
			);
		}
		else if (
			ActiveRmlUI &&
			ActiveRmlUI->IsInitialized()
		)
		{
			ActiveRmlUI->OnDeviceReset();
		}

		return false;
	}

	r3dRenderer->UpdateDimmensions();
	r3dRenderer->ResetViewport();

	r_width->SetInt(Width);
	r_height->SetInt(Height);

	Desktop().SetViewSize(
		r3dRenderer->ScreenW,
		r3dRenderer->ScreenH
	);

	if (
		ActiveRmlUI &&
		ActiveRmlUI->IsInitialized()
	)
	{
		ActiveRmlUI->OnDeviceReset();
	}

	InvalidateRect(
		win::hWnd,
		nullptr,
		FALSE
	);

	r3dOutToLog(
		"[Studio] Backbuffer resized successfully\n"
	);

	return true;
}

static void EnableStudioWindowResize(HWND WindowHandle)
{
	if (!WindowHandle)
		return;

	LONG_PTR WindowStyle = GetWindowLongPtr(
		WindowHandle,
		GWL_STYLE
	);

	WindowStyle |= WS_THICKFRAME;
	WindowStyle |= WS_MAXIMIZEBOX;

	SetWindowLongPtr(
		WindowHandle,
		GWL_STYLE,
		WindowStyle
	);

	SetWindowPos(
		WindowHandle,
		nullptr,
		0,
		0,
		0,
		0,
		SWP_NOMOVE |
		SWP_NOSIZE |
		SWP_NOZORDER |
		SWP_NOACTIVATE |
		SWP_FRAMECHANGED
	);
}

static void ApplyStudioDarkTitleBar(HWND WindowHandle)
{
	if (!WindowHandle)
		return;

	// Атрибуты DWM.
	const DWORD ImmersiveDarkModeAttribute = 20;
	const DWORD LegacyImmersiveDarkModeAttribute = 19;
	const DWORD BorderColorAttribute = 34;
	const DWORD CaptionColorAttribute = 35;
	const DWORD TextColorAttribute = 36;

	const BOOL EnableDarkMode = TRUE;

	HRESULT Result = DwmSetWindowAttribute(
		WindowHandle,
		ImmersiveDarkModeAttribute,
		&EnableDarkMode,
		sizeof(EnableDarkMode)
	);

	// Fallback для некоторых старых сборок Windows 10.
	if (FAILED(Result))
	{
		DwmSetWindowAttribute(
			WindowHandle,
			LegacyImmersiveDarkModeAttribute,
			&EnableDarkMode,
			sizeof(EnableDarkMode)
		);
	}

	const COLORREF CaptionColor = RGB(0, 0, 0);
	const COLORREF TextColor = RGB(255, 255, 255);
	const COLORREF BorderColor = RGB(18, 18, 18);

	DwmSetWindowAttribute(
		WindowHandle,
		CaptionColorAttribute,
		&CaptionColor,
		sizeof(CaptionColor)
	);

	DwmSetWindowAttribute(
		WindowHandle,
		TextColorAttribute,
		&TextColor,
		sizeof(TextColor)
	);

	DwmSetWindowAttribute(
		WindowHandle,
		BorderColorAttribute,
		&BorderColor,
		sizeof(BorderColor)
	);

	SetWindowPos(
		WindowHandle,
		nullptr,
		0,
		0,
		0,
		0,
		SWP_NOMOVE |
		SWP_NOSIZE |
		SWP_NOZORDER |
		SWP_NOACTIVATE |
		SWP_FRAMECHANGED
	);
}

static void ExecuteDX11SmokeLoop()
{
	r3dOutToLog(
		"[DX11] Entering experimental smoke loop. Close the window to exit. world=%d\n",
		g_StudioCmdLineDX11World ? 1 : 0
	);

	const bool bRmlReady = InitDX11SmokeRml();

	if (!bRmlReady)
	{
		r3dOutToLog(
			"[DX11][RmlUI] RML smoke screen is not ready. DX11 will continue with clear screen only.\n"
		);
	}

	DWORD LastWorldStatsLog = 0;

	while (!g_bExit)
	{
		MSG Message;
		while (PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE))
		{
			if (Message.message == WM_QUIT)
			{
				g_bExit = true;
				break;
			}

			if (g_DX11SmokeRmlContext)
			{
				LRESULT RmlResult = 0;

				RmlRuntime::Get().ProcessWin32Message(
					g_DX11SmokeRmlContext,
					win::hWnd,
					Message.message,
					Message.wParam,
					Message.lParam,
					&RmlResult
				);
			}

			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		if (g_bExit)
			break;

		if (win::ProcessSuspended())
		{
			Sleep(10);
			continue;
		}

		ProcessStudioPendingResize(nullptr);

		if (g_DX11SmokeRmlContext)
		{
			g_DX11SmokeRmlContext->SetDimensions(
				Rml::Vector2i(
					g_DX11Renderer->GetWidth(),
					g_DX11Renderer->GetHeight()
				)
			);

			RmlRuntime::Get().SetActiveContext(
				g_DX11SmokeRmlContext
			);

			g_DX11SmokeRmlContext->Update();
		}

		g_DX11Renderer->BeginFrame(
			0.010f,
			0.012f,
			0.011f,
			1.0f
		);

		if (g_StudioCmdLineDX11World)
		{
			r3dDX11WorldRenderStats WorldStats;
			r3dDX11ResetWorldRenderStats(WorldStats);

			const bool bWorldRendered =
				g_DX11Renderer->RenderWorldGBuffer(
					gCam,
					&WorldStats
				);

			const DWORD Now = GetTickCount();

			if (Now - LastWorldStatsLog >= 1000)
			{
				r3dOutToLog(
					"[DX11][World] ok=%d total=%u mesh=%u depth=%u drawn=%u unsupported=%u failed=%u shadow=%u smesh=%u sdraw=%u salpha=%u sslices=%u sunsupported=%u sfailed=%u\n",
					bWorldRendered ? 1 : 0,
					WorldStats.TotalRenderables,
					WorldStats.MeshRenderables,
					WorldStats.DepthDrawnMeshes,
					WorldStats.DrawnMeshes,
					WorldStats.SkippedUnsupported,
					WorldStats.SkippedFailed,
					WorldStats.ShadowRenderables,
					WorldStats.ShadowMeshRenderables,
					WorldStats.ShadowDrawnMeshes,
					WorldStats.ShadowAlphaTested,
					WorldStats.ShadowSlicesRendered,
					WorldStats.ShadowSkippedUnsupported,
					WorldStats.ShadowSkippedFailed
				);

				LastWorldStatsLog = Now;
			}
		}

		g_DX11Renderer->EndFrame(
			r_vsync_enabled->GetBool(),
			g_DX11SmokeRmlContext
		);

		Sleep(1);
	}

	ShutdownDX11SmokeRml();

	r3dOutToLog("[DX11] Leaving experimental smoke loop\n");
}

void InitRender(int bUseSet = 0)
{
	r_out_of_vmem_encountered->SetChangeCallback( &SaveSettingsCallback ) ;

	if (!bUseSet)
	{
		r_width->SetInt( 1024 );
		r_height->SetInt( 768 );
		r_bpp->SetInt( 32 );
		r_fullscreen->SetBool( false ); 
	}

	int Flags = 0;

	if ( !r_fullscreen->GetBool() ) 
		Flags |= R3DSetMode_Windowed;

	MoveWindow(win::hWnd, 0, 0, r_width->GetInt(), r_height->GetInt(), 0);

	if (r_dx11_boot->GetBool())
	{
		r3dOutToLog(
			"[DX11] Experimental boot requested: %dx%d fullscreen=%d\n",
			r_width->GetInt(),
			r_height->GetInt(),
			r_fullscreen->GetBool() ? 1 : 0
		);

		g_DX11Renderer = new r3dDX11Renderer;

		if (!g_DX11Renderer->Init(
			win::hWnd,
			r_width->GetInt(),
			r_height->GetInt(),
			r_fullscreen->GetBool(),
#ifndef FINAL_BUILD
			true
#else
			false
#endif
		))
		{
			SAFE_DELETE(g_DX11Renderer);
			r3dError("Failed to init DX11 renderer!\n");
		}

		EnableStudioWindowResize(win::hWnd);
		ApplyStudioDarkTitleBar(win::hWnd);

		ShowWindow(win::hWnd, TRUE);
		UpdateWindow(win::hWnd);
		return;
	}

	r3dRenderer = new r3dRenderLayer;

	r3dRenderer->Init(win::hWnd, NULL);

	if( 
#if 0
		( r_local_vmem_size->GetInt() && r_local_vmem_size->GetInt() <= 256 * 1024 * 1024 && !r_ini_read->GetInt() )
			||
#endif
		r_out_of_vmem_encountered->GetInt()
		) 
	{
		r3dOutToLog( "Setting low memory requirement options because we have only %d memory\n", r_local_vmem_size->GetInt() / 1024 / 1024 ) ;

		r_out_of_vmem_encountered->SetInt( 0 ) ;

		r_width->SetInt( 800 ) ;
		r_height->SetInt( 600 ) ;

		r_fullscreen->SetInt( 0 ) ;

		r_texture_quality->SetInt( 1 ) ;

		void applyGraphicOptionsSoft( uint32_t ) ;
		applyGraphicOptionsSoft( 1 << 1 );

		void writeGameOptionsFile();
		writeGameOptionsFile();
	}

	r3dOutToLog("Setting mode:  %dx%dx%d Flags=%d\n", r_width->GetInt(), r_height->GetInt(), r_bpp->GetInt(), Flags);

	r3dRenderer->InitStereo() ;

	if( !r3dRenderer->SetMode( r_width->GetInt(), r_height->GetInt(), r_bpp->GetInt(), Flags, 0 /*R3D_PATH_DX9*/) )
	{
		bool failed = true ;
		if( ! ( Flags & R3DSetMode_Windowed ) )
		{
			r3dOutToLog("SetMode failed, trying to set windowed flag and trying again\n");
			Flags |= R3DSetMode_Windowed;
			if( r3dRenderer->SetMode( r_width->GetInt(), r_height->GetInt(), r_bpp->GetInt(), Flags, 0 /*R3D_PATH_DX9*/) )
			{
				failed = false ;
			}
		}

		if( failed )
		{
			r3dError("Failed to init D3D Device!\n");
			r3dRenderer->Close();
			exit( 0 );
		}
	}

	EnableStudioWindowResize(win::hWnd);
	ApplyStudioDarkTitleBar(win::hWnd);

	ShowWindow(win::hWnd, TRUE);
	UpdateWindow(win::hWnd);

	r3dInitShaders();

	r3dInitMaterials();

	r3d_assert(g_pJobChief == 0);
	g_pJobChief = new JobChief();
	g_pJobChief->Init();

	g_pBackgroundTaskDispatcher = new r3dBackgroundTaskDispatcher();
	g_pBackgroundTaskDispatcher->Init() ;

	g_EnvmapProbes.Init();
	r3d_assert(g_pDecalChief == 0);
	g_pDecalChief = new DecalChief();
	g_pDecalChief->Init();
	// should follow g_DecalChief
	r3d_assert(g_pMaterialTypes == 0);
	g_pMaterialTypes = new MaterialTypes();
	g_pMaterialTypes->Load();

#if R3D_ALLOW_LIGHT_PROBES
	g_pProbeMaster = new ProbeMaster ;
	g_pProbeMaster->Init() ;
#endif

	r3dRenderer->StartRender();
	r3dRenderer->EndRender( true );

	r3dUtilInit();
	InitOcclusionQuerySystem();

	gFlashbangVisualController.Init();

#ifndef FINAL_BUILD
	InitObjCategories();
#endif

	if ( d_mouse_window_lock->GetBool() )
	{
		Mouse->MoveTo((int)r3dRenderer->ScreenW2, (int)r3dRenderer->ScreenH2);
	}
	Mouse->SetCapture();

	{
		r3dIntegrityGuardian ig ;

		Font_Label = new CD3DFont( ig, "Tahoma", 12, D3DFONT_BOLD | D3DFONT_FILTERED | D3DFONT_SKIPGLYPH );
		Font_Label->CreateSystemFont();
	}

	{
		r3dIntegrityGuardian ig ;

		Font_Editor = new CD3DFont(ig, "Verdana", 10, D3DFONT_BOLD | D3DFONT_FILTERED | D3DFONT_SKIPGLYPH);
		Font_Editor->CreateSystemFont();
	}

#if ENABLE_WEB_BROWSER
	g_pBrowserManager = new EternityWebBrowser();
#endif
}


void CloseRender()
{
	if (g_DX11Renderer)
	{
		g_DX11Renderer->Shutdown();
		SAFE_DELETE(g_DX11Renderer);
	}

	if (!r3dRenderer)
		return;

	ReleaseCheatScreenshot();

	delete Font_Label;
	delete Font_Editor; 

#if R3D_ALLOW_LIGHT_PROBES
	if( g_pProbeMaster )
		g_pProbeMaster->Close() ;

	SAFE_DELETE( g_pProbeMaster ) ;
#endif

#if ENABLE_WEB_BROWSER
	SAFE_DELETE(g_pBrowserManager);
#endif

#ifndef FINAL_BUILD
	CloseCategories();
#endif

	CloseOcclusionQuerySystem();
	r3dUtilClose();

	SAFE_DELETE(g_pMaterialTypes);
	g_pDecalChief->Close();
	SAFE_DELETE(g_pDecalChief);
	g_EnvmapProbes.Close();

	gFlashbangVisualController.Destroy();

	g_pJobChief->Close();
	SAFE_DELETE(g_pJobChief);

	g_pBackgroundTaskDispatcher->Close();
	SAFE_DELETE(g_pBackgroundTaskDispatcher);

#if APEX_ENABLED
	DestroyApexUserRenderer();
#endif

	r3dCloseMaterials();

	r3dRenderer->Close(); 

	SAFE_DELETE(r3dRenderer); 
}


#ifdef FINAL_BUILD
const char * g_szApplicationName = "LTS - Last To Survive";
#else
const char * g_szApplicationName = "LTS - Last To Survive";
#endif 

int32_t	g_nProjectVersionMajor = 1;
int32_t	g_nProjectVersionMinor = 0;

static const char* GetApplicationWindowTitle()
{
	static char Title[128];
	sprintf(Title, "%s v%d.%d", g_szApplicationName, g_nProjectVersionMajor, g_nProjectVersionMinor);
	return Title;
}

extern	char	Login_PassedUser[256];
extern	char	Login_PassedPwd[256];
extern	char	Login_PassedAuth[256];
extern	char	Login_GNA_userid[256];
extern	char	Login_GNA_appkey[256];
extern	char	Login_GNA_token[256];

static	char*	gSurveyOutLink = NULL;

PCHAR* CommandLineToArgvA(PCHAR CmdLine, int* _argc)
{
	PCHAR* argv;
	PCHAR  _argv;
	ULONG   len;
	ULONG   argc;
	CHAR   a;
	ULONG   i, j;

	BOOLEAN  in_QM;
	BOOLEAN  in_TEXT;
	BOOLEAN  in_SPACE;

	len = strlen(CmdLine);
	i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

	argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
		i + (len+2)*sizeof(CHAR));

	_argv = (PCHAR)(((PUCHAR)argv)+i);

	argc = 0;
	argv[argc] = _argv;
	in_QM = FALSE;
	in_TEXT = FALSE;
	in_SPACE = TRUE;
	i = 0;
	j = 0;

	while( a = CmdLine[i] ) {
		if(in_QM) {
			if(a == '\"') {
				in_QM = FALSE;
			} else {
				_argv[j] = a;
				j++;
			}
		} else {
			switch(a) {
		case '\"':
			in_QM = TRUE;
			in_TEXT = TRUE;
			if(in_SPACE) {
				argv[argc] = _argv+j;
				argc++;
			}
			in_SPACE = FALSE;
			break;
		case ' ':
		case '\t':
		case '\n':
		case '\r':
			if(in_TEXT) {
				_argv[j] = '\0';
				j++;
			}
			in_TEXT = FALSE;
			in_SPACE = TRUE;
			break;
		default:
			in_TEXT = TRUE;
			if(in_SPACE) {
				argv[argc] = _argv+j;
				argc++;
			}
			_argv[j] = a;
			j++;
			in_SPACE = FALSE;
			break;
			}
		}
		i++;
	}
	_argv[j] = '\0';
	argv[argc] = NULL;

	(*_argc) = argc;
	return argv;
}

CHWInfo g_HardwareInfo;

// This function called by engine before main app window created, before any IO initialized. 
void game::PreInit()
{
	u_srand(GetTickCount());

	g_HardwareInfo.Grab();

	win::hWinIcon = ::LoadIcon(win::hInstance, MAKEINTRESOURCE(IDI_WARZ));
	win::szWinName = GetApplicationWindowTitle();

#ifdef FINAL_BUILD
	win::hWinIcon = ::LoadIcon(win::hInstance, MAKEINTRESOURCE(IDI_WARZ));
	if(strstr(__r3dCmdLine, "-WOUpdatedOk") == NULL && strstr(__r3dCmdLine, "-gna") == NULL)
	{
		MessageBox(NULL, "Please run WarZ launcher.", g_szApplicationName, MB_OK);
		ExitProcess(0);
	}
#endif	

#ifdef _DEBUG
	r3dOutToLog("cmd: %s\n", __r3dCmdLine);
#endif	
	
	// parse command line
	int argc = 0;
	char** argv = CommandLineToArgvA(__r3dCmdLine, &argc);
	for(int i=0; i<argc; i++) 
	{
		if(strcmp(argv[i], "-login") == 0 && (i + 1) < argc)
		{
			r3dscpy(Login_PassedUser, argv[++i]);
			continue;
		}
		if(strcmp(argv[i], "-pwd") == 0 && (i + 1) < argc)
		{
			r3dscpy(Login_PassedPwd, argv[++i]);
			continue;
		}
		if(strcmp(argv[i], "-WOLogin") == 0 && (i + 1) < argc)
		{
			r3dscpy(Login_PassedAuth, argv[++i]);
			continue;
		}
		
		if(strcmp(argv[i], "-steam") == 0)
		{
			gSteam.IS_ENABLED = true;
			continue;
		}

		if(strcmp(argv[i], "-dx11") == 0)
		{
			g_StudioCmdLineDX11Boot = true;
			continue;
		}

		if(strcmp(argv[i], "-dx11world") == 0)
		{
			g_StudioCmdLineDX11World = true;
			continue;
		}

		if(strcmp(argv[i], "-survey") == 0 && (i + 1) < argc)
		{
			gSurveyOutLink = argv[++i];
			continue;
		}

#ifndef FINAL_BUILD
#if !DISABLE_PROFILER
		if (strcmp(argv[i], "-profile") == 0)
		{
			if (i + 1 >= argc)
				r3dError("Incorrect syntax for '-profile' option. Use '-profile level_name'");
			strcpy_s(LevelEditName, _countof(LevelEditName), argv[i + 1]);
			//	Set scheduled profile time to -1 to adjust it automatically after level will be loaded
			gScheduledProfileTime = -1.0f;
		}

		if (strcmp(argv[i], "-gprofile") == 0)
		{
			if (i + 1 >= argc)
				r3dError("Incorrect syntax for '-gprofile' option. Use '-gprofile level_name'");

			strcpy_s(LevelEditName, _countof(LevelEditName), argv[i + 1]);

			gProfileD3DFromCommandLine = true;
			gScheduledProfileTime = -1.0f;
		}
#endif

		if (strcmp(argv[i], "-camera") == 0)
		{
			if (i + 1 >= argc)
				r3dError("Incorrect syntax for '-camera' option. Use '-camera camera_spot_name'");

			strcpy_s(initialCameraSpotName, _countof(initialCameraSpotName), argv[i + 1]);
		}
#endif
	}
}


static void InitSounds()
{
	snd_InitSoundSystem();
	snd_LoadSoundEffects("Data\\Sounds", "Sounds.fev");
}

void ReloadMesh(const char* fname);


#ifndef FINAL_BUILD
//--------------------------------------------------------------------------------------------------------
void CallbackFileChange( const char * szFileName )
{
	char buffer[ MAX_PATH ];
	GetCurrentDirectory( sizeof( buffer ), buffer );
	uint32_t dwSize = strlen( buffer );
	if ( 0 != strncmp( buffer, szFileName, dwSize ) )
		return;

	const char * szName = szFileName + dwSize + 1;

	FixedString s( szName );
	FixedString sExt = s.GetExt();
	strlwr(sExt.str());
	if ( sExt == ".dds" || sExt == ".tga" || sExt == ".bmp" )
	{
		r3dRenderer->ReloadTextureData( szName );
		return;
	}	

	if ( sExt == ".hls")
	{
		r3dRenderer->ReloadShaderByFileName(szName);
		return;
	}

	if ( sExt == ".sco")
	{
		ReloadMesh( szName );
		return;
	}

	if ( sExt == ".anm")
	{
		extern r3dAnimPool* g_CharactersAnimationsPool;
		g_CharactersAnimationsPool->Reload(szName);
		return;
	}

	if( g_pMaterialTypes->CheckNeedReload( szName ) )
	{
		if( !g_pMaterialTypes->Load() )
		{
			MessageBox( NULL, "Error reloading material types! Please, check your XML syntax!", "Error", MB_ICONEXCLAMATION );
		}
	}
}
#endif

#define INI_FILE "gameSettings.ini"
#define INPUT_MAP_FILE "inputMap.xml"

void applyGraphicOptionsSoft( uint32_t settingsFlags )
{
	extern float __WorldRenderBias;

	switch( r_anisotropy_quality->GetInt() )
	{
	case 1:
		r_anisotropy->SetInt( 1 );
		//__WorldRenderBias = -1.45f;
		break;
	case 2:
		//__WorldRenderBias = -0.45f;
		r_anisotropy->SetInt( 2 );
		break;
	case 3:
		r_anisotropy->SetInt( 4 );
		//__WorldRenderBias = 0.f;
		break;
	case 4:
		r_anisotropy->SetInt( 8 );
		//__WorldRenderBias = 0.f;
		break;
	}

	switch( r_ssao_quality->GetInt() )
	{
	case 1:
		r_ssao->SetInt( 0 );
		break;
	case 2:
		r_ssao->SetInt( 1 );
		r_ssao_method->SetInt( 1 );
		r_half_scale_ssao->SetInt( 1 );
		r_ssao_blur_w_normals->SetInt( 0 );
		break;
	case 3:
		r_ssao->SetInt( 1 );
		r_ssao_method->SetInt( 2 );
		r_half_scale_ssao->SetInt( 1 );
		r_ssao_blur_w_normals->SetInt( 0 );
		break;
	case 4:
		r_ssao->SetInt( 1 );
		r_ssao_method->SetInt( 1 );
		r_half_scale_ssao->SetInt( 0 );
		r_ssao_blur_w_normals->SetInt( 0 );
		break;
	case 5:
		r_ssao->SetInt( 1 );
		r_ssao_method->SetInt( 2 );
		r_half_scale_ssao->SetInt( 0 );
		r_ssao_blur_w_normals->SetInt( 1 );
		break;
	};

	switch( r_antialiasing_quality->GetInt() )
	{
	case 1:
		r_fxaa->SetInt( 0 );
		break;
	case 2:
		r_fxaa->SetInt( 1 );
		break;
	case 3:
		r_fxaa->SetInt( 1 );
		break;
	case 4:
		r_fxaa->SetInt( 1 );
		break;
	}

	switch( r_postprocess_quality->GetInt() )
	{
	case 1:
		r_dof			->SetInt( 0 );
		r_film_grain	->SetInt( 0 );
		r_bloom			->SetInt( 0 );
		r_glow			->SetInt( 0 );
		r_sun_rays		->SetInt( 0 );
		break;
	case 2:
		r_dof			->SetInt( 1 );
		r_film_grain	->SetInt( 0 );
		r_bloom			->SetInt( 1 );
		r_glow			->SetInt( 1 );
		r_sun_rays		->SetInt( 0 );
		break;
	case 3:
		r_dof			->SetInt( 1 );
		r_film_grain	->SetInt( 1 );
		r_bloom			->SetInt( 1 );
		r_glow			->SetInt( 1 );
		r_sun_rays		->SetInt( 1 );
		break;
	}

	if( settingsFlags & FrontEndShared::SC_SHADOWS_QUALITY )
	{

		const int MAX_DIR_TEX_SIZE = r_max_texture_dim->GetInt() ? R3D_MIN( r_max_texture_dim->GetInt(), 2048 ) : 2048 ;

		switch( r_shadows_quality->GetInt() )
		{
		case 1:
			r_transp_shadows->SetInt( 0 ) ;
			r_terra_shadows->SetInt( 0 );
			r_shadow_blur->SetInt( 0 );
			r_dir_sm_size->SetInt( MAX_DIR_TEX_SIZE );
			r_shared_sm_size->SetInt( 1024 );
			r_shared_sm_cube_size->SetInt( 512 );
			r_active_shadow_slices->SetInt( NumShadowSlices - 2 );
			r3d_assert( r_active_shadow_slices->GetInt() ) ;
			r_shadows->SetInt( 1 );
			r_dd_pointlight_shadows->SetInt( 0 );
			ShadowSplitDistancesOpaque = &ShadowSplitDistancesOpaqueLow[0];
			break;

		case 2:
			r_transp_shadows->SetInt( 0 ) ;
			r_terra_shadows->SetInt( 1 );
			r_shadow_blur->SetInt( 0 );
			r_dir_sm_size->SetInt( MAX_DIR_TEX_SIZE );
			r_shared_sm_size->SetInt( 1024 );
			r_shared_sm_cube_size->SetInt( 512 );
			r_active_shadow_slices->SetInt( NumShadowSlices -1);
			r_shadows->SetInt( 1 );
			r_dd_pointlight_shadows->SetInt( 0 );
			ShadowSplitDistancesOpaque = &ShadowSplitDistancesOpaqueMed[0];
			break;

		case 3:
			r_terra_shadows->SetInt( 1 );
			r_shadow_blur->SetInt( 0 );
			r_dir_sm_size->SetInt( MAX_DIR_TEX_SIZE );
			r_shared_sm_size->SetInt( 1024 );
			r_shared_sm_cube_size->SetInt( 512 );
			r_active_shadow_slices->SetInt( NumShadowSlices );
			r_shadows->SetInt( 1 );
			r_dd_pointlight_shadows->SetInt( 1 );
			ShadowSplitDistancesOpaque = &ShadowSplitDistancesOpaqueHigh[0];
			break;

		case 4:
			r_terra_shadows->SetInt( 1 );
			r_shadow_blur->SetInt( 1 );
			r_dir_sm_size->SetInt( MAX_DIR_TEX_SIZE );
			r_shared_sm_size->SetInt( 1024 );
			r_shared_sm_cube_size->SetInt( 1024 );
			r_active_shadow_slices->SetInt( NumShadowSlices );
			r_shadows->SetInt( 1 );
			r_dd_pointlight_shadows->SetInt( 1 );
			ShadowSplitDistancesOpaque = &ShadowSplitDistancesOpaqueHigh[0];
			break;		
		}
	}

	if( r_force_shared_sm_size->GetInt() )
	{
		r_shared_sm_size->SetInt( r_force_shared_sm_size->GetInt() );
	}

	if( settingsFlags & FrontEndShared::SC_PARTICLES_QUALITY )
	{
		switch( r_particles_quality->GetInt() )
		{
		case 1:
			r_distort->SetInt( 0 );
			r_half_res_particles->SetInt( 1 ) ;
			r_particle_shadows->SetInt( 0 ) ;
			break ;

		case 2:
			r_distort->SetInt( 1 );
			r_half_res_particles->SetInt( 1 ) ;
			r_particle_shadows->SetInt( 1 ) ;
			break ;

		case 3:
			r_distort->SetInt( 1 );
			r_half_res_particles->SetInt( 1 ) ;
			r_particle_shadows->SetInt( 1 ) ;
			break ;

		case 4:
			r_distort->SetInt( 1 );
			r_half_res_particles->SetInt( 0 ) ;
			r_particle_shadows->SetInt( 1 ) ;
			break ;
		};
	}

	if( settingsFlags & FrontEndShared::SC_DECORATIONS_QUALITY )
	{
		switch( r_decoration_quality->GetInt() )
		{
		case 1:
			r_grass_view_coef->SetFloat( 0.5f );
			r_grass_draw->SetBool( 1 );
			r_grass_skip_step->SetInt( 1 ) ;
			break ;

		case 2:
			// view coef now stays the same, but density of grass
			// gets less 
			r_grass_view_coef->SetFloat( 1.0f );
			r_grass_draw->SetBool( 1 );
			r_grass_skip_step->SetInt( 1 ) ;
			break ;

		case 3:
			r_grass_view_coef->SetFloat( 1.0f );
			r_grass_draw->SetBool( 1 );
			r_grass_skip_step->SetInt( 0 ) ;
			break ;
		};
	}

	if( !r_half_scale_ssao->GetInt() && r_ssao->GetInt() && r_grass_draw->GetInt() )
	{
		r_split_grass_render->SetInt( 1 ) ;
	}
	else
	{
		r_split_grass_render->SetInt( 0 ) ;
	}

	void SyncLightingAndSSAO();
	SyncLightingAndSSAO();
}

void applyGraphicsOptions( uint32_t settingsFlags )
{

	applyGraphicOptionsSoft( settingsFlags );

	struct PushPopBackGroundLoading
	{
		PushPopBackGroundLoading()
		{
			prevVal = g_async_loading->GetInt() ;
			r3dSetAsyncLoading( 0 ) ;
		}

		~PushPopBackGroundLoading()
		{
			r3dSetAsyncLoading( prevVal ) ;
		}

		int prevVal ;
	} pushPopBackGroundLoading ; (void)pushPopBackGroundLoading ;

	if( settingsFlags & FrontEndShared::SC_WATER_QUALITY )
	{
		for( GameObject *obj = GameWorld().GetFirstObject(); obj; obj = GameWorld().GetNextObject(obj) )
		{
			if( obj->Class->Name == "obj_Lake" )
			{
				// recreates it with new dimmensions
				static_cast<WaterBase*>( static_cast<obj_Lake*>(obj) )->CreateWaterBuffers();
			}
			else
			if( obj->Class->Name == "obj_WaterPlane" )
			{
				// recreates it with new dimmensions
				static_cast<WaterBase*>( static_cast<obj_WaterPlane*>(obj) )->CreateWaterBuffers();
			}
		}

		WaterBase::UpdateRefractionBuffer( true );
	}

	if( settingsFlags & FrontEndShared::SC_PARTICLES_QUALITY )
	{
		if( g_pDecalChief )
		{
			g_pDecalChief->UpdateTexturesForQualitySettings();
		}
	}

	if( settingsFlags & FrontEndShared::SC_DECORATIONS_QUALITY )
	{
		if( g_pGrassLib )
		{
			g_pGrassLib->Unload() ;
		}

		if( g_pGrassLib && ( r_decoration_quality->GetInt() > 0 || g_bEditMode ) )
		{
			g_pGrassLib->Load() ;
		}
	}

	if( settingsFlags & FrontEndShared::SC_TEXTURE_QUALITY )
	{
		void r3dParticleSystemReloadCachedDataTextures();
		r3dParticleSystemReloadCachedDataTextures();
		r3dMaterialLibrary::ReloadMaterialTextures();
		r3dGameLevel::Environment.ReloadTextures();

		if( g_pGrassLib )
			g_pGrassLib->ReloadTextures();

		if( g_pDecalChief )
			g_pDecalChief->ReloadTextures();

		if( Terrain )
		{
			Terrain->ReloadTextures();
		}

		for( GameObject *obj = GameWorld().GetFirstObject(); obj; obj = GameWorld().GetNextObject(obj) )
		{
			if( obj->Class->Name == "obj_Lake" )
			{
				// recreates it with new dimmensions
				static_cast<WaterBase*>( static_cast<obj_Lake*>(obj) )->ReloadTextures();
			}
			else
			if( obj->Class->Name == "obj_WaterPlane" )
			{
				// recreates it with new dimmensions
				static_cast<WaterBase*>( static_cast<obj_WaterPlane*>(obj) )->ReloadTextures();
			}
		}
	}

	if( settingsFlags & FrontEndShared::SC_TERRAIN_QUALITY )
	{
		if( Terrain1 )
		{
			// only do this if ql changes to 1 or from 1
			if( r_terrain_quality->GetInt() == 1 && Terrain1->LastQLInit != 1 
					||
				r_terrain_quality->GetInt() != 1 && Terrain1->LastQLInit <= 1 
				)
			{
				if( !g_bEditMode )
				{
					Terrain1->PrepareForSettingsUpdateInGame();
				}

				Terrain1->RecreateVertexBuffer();
				Terrain1->UpdateAllVertexData();
				Terrain1->RecreateIndexData();

				if( !g_bEditMode )
				{
					Terrain1->ReleaseSettingsUpdateData();
				}
			}
		}

#ifndef FINAL_BUILD
		if( Terrain2 )
		{
			Terrain2->UpdateQualitySettings() ;
		}
#endif
	}

	if( settingsFlags & FrontEndShared::SC_SHADOWS_QUALITY )
	{
		ResetShadowCache();
		UpdateHWSchadowScheme();
		CurRenderPipeline->DestroyShadowResources();
		CurRenderPipeline->CreateShadowResources();
	}

	if (settingsFlags & FrontEndShared::SC_LIGHTING_QUALITY)
	{
		CurRenderPipeline->DestroyAuxResource();
		CurRenderPipeline->CreateAuxResource();

		void SyncLightingAndSSAO();
		SyncLightingAndSSAO();
	}

	UpdateMLAA();
}

static void DoExecIni( const char* Path )
{
	ExecVarIni( Path );
	// always override game settings ini with "local" ini
	ExecVarIni( "local.ini" );

}

bool CreateFullIniPath( char* dest, bool old )
{
	bool res = old ? CreateWorkPath(dest) : CreateConfigPath(dest);
	if(res)
		strcat( dest, INI_FILE );
	return res;
}

bool CreateFullMappingPath( char* dest, bool old )
{
	bool res = old ? CreateWorkPath(dest) : CreateConfigPath(dest);
	if(res)
		strcat( dest, INPUT_MAP_FILE );
	return res;
}

void OnFoundIniFile( const char* FullIniPath )
{
	r3dOutToLog( "readGameOptionsFile: found INI at %s\n", FullIniPath );

	DoExecIni( FullIniPath );

	// check if user has changed monitor and his new monitor doesn't support his resolution (by checking his desktop res)
	int deskW, deskH ;
	r3dGetDesktopDimmensions( &deskW, &deskH );
	if(r_width->GetInt() > deskW || r_height->GetInt() > deskH)
	{
		r3dOutToLog("Desktop resolution is smaller than in settings. Resetting to desktop resolution\n");
		r_width->SetInt( deskW );
		r_height->SetInt( deskH );
	}



}

void readGameOptionsFile()
{
	r_ini_read->SetBool( true );

#ifdef FINAL_BUILD
	// before reading ini to allow geeks to override it
	r_limit_fps->SetInt( 60 );
#endif

	// try local first
	if( !r3d_access( "./" INI_FILE, 4 ) )
	{
		r3dOutToLog( "readGameOptionsFile: found %s in local folder\n", INI_FILE );
		DoExecIni( "./" INI_FILE );
		g_locl_settings->SetBool( true );
	}
	else
	{
		g_locl_settings->SetBool( false );

		char FullIniPath[ MAX_PATH * 2 ];

		bool createdPath = CreateFullIniPath( FullIniPath, false );

		if( createdPath && r3d_access( FullIniPath, 4 ) == 0 )
		{			
			OnFoundIniFile( FullIniPath );
		}
		else
		{
			// true using old folder (appdata)
			createdPath = CreateFullIniPath( FullIniPath, true ) ;

			if( createdPath && r3d_access( FullIniPath, 4 ) == 0 )
			{
				OnFoundIniFile( FullIniPath );
			}
			else
			{
				r_ini_read->SetBool( false );

				if( !createdPath )
				{
					r3dOutToLog( "readGameOptionsFile: Error: couldn't get local app path! Using defaults!\n" );
				}
				else
				{
					r3dOutToLog( "readGameOptionsFile: couldn't open both %s and %s! Using defaults.\n", INI_FILE, FullIniPath );
				}

				int deskW, deskH ;
				r3dGetDesktopDimmensions( &deskW, &deskH );

				r_width->SetInt( deskW );
				r_height->SetInt( deskH );

				r3dOutToLog( "Selected resolution from desktop dimensions: %dx%d\n", r_width->GetInt(), r_height->GetInt() );

				SetDefaultSettings( r3dGetDeviceStrength() );
			}
		}
	}
	r_fullscreen_load->SetInt(r_fullscreen->GetInt());
/*#ifdef FINAL_BUILD
	r_force_aspect->SetFloat( 16.f / 9.f );
#endif*/

	switch( r_overall_quality->GetInt() )
	{
	case 1:
		SetDefaultSettings( S_WEAK );
		break;

	case 2:
		SetDefaultSettings( S_MEDIUM );
		break;

	case 3:
		SetDefaultSettings( S_STRONG );
		break;

	case 4:
		SetDefaultSettings( S_ULTRA );
		break;
	}

	applyGraphicOptionsSoft( FrontEndShared::SC_ALL );

	if(r_server_region->GetInt()==-1) // locate our region
	{
		int our_region = 0; // 0-us, 1-eu
		LCID userLCID = GetUserDefaultLCID();
		switch(userLCID)
		{ // from http://support.microsoft.com/kb/193080
		case 1025: //		Arabic (Saudi Arabia)                
		case 2049: //		Arabic (Iraq)                        
		case 3073: //		Arabic (Egypt)                       
		case 4097: //		Arabic (Libya)                       
		case 5121: //		Arabic (Algeria)                     
		case 6145: //		Arabic (Morocco)                     
		case 7169: //		Arabic (Tunisia)                     
		case 8193: //		Arabic (Oman)                        
		case 9217: //		Arabic (Yemen)                       
		case 10241: //		Arabic (Syria)                      
		case 11265: //		Arabic (Jordan)                     
		case 12289: //		Arabic (Lebanon)                    
		case 13313: //		Arabic (Kuwait)                     
		case 14337: //		Arabic (U.A.E.)                     
		case 15361: //		Arabic (Bahrain)                    
		case 16385: //		Arabic (Qatar)                      
		case 1026: //		Bulgarian                            
		case 1029: //		Czech                                
		case 1030: //		Danish                               
		case 1031: //		German (Standard)                    
		case 2055: //		German (Swiss)                       
		case 3079: //		German (Austrian)                    
		case 4103: //		German (Luxembourg)                  
		case 5127: //		German (Liechtenstein)               
		case 1032: //		Greek                                
		case 2057: //		English (United Kingdom)             
		case 6153: //		English (Ireland)                    
		case 1035: //		Finnish                              
		case 1036: //		French (Standard)                    
		case 2060: //		French (Belgian)                     
		case 4108: //		French (Swiss)                       
		case 5132: //		French (Luxembourg)                  
		case 1037: //		Hebrew                               
		case 1038: //		Hungarian                            
		case 1039: //		Icelandic                            
		case 1040: //		Italian (Standard)                   
		case 2064: //		Italian (Swiss)                      
		case 1043: //		Dutch (Standard)                     
		case 2067: //		Dutch (Belgian)                      
		case 1044: //		Norwegian (Bokmal)                   
		case 2068: //		Norwegian (Nynorsk)                  
		case 1045: //		Polish                               
		case 2070: //		Portuguese (Portugal)                
		case 1048: //		Romanian                             
		case 1049: //		Russian                              
		case 1050: //		Croatian                             
		case 2074: //		Serbian (Latin)                      
		case 3098: //		Serbian (Cyrillic)                   
		case 1051: //		Slovak                               
		case 1052: //		Albanian                             
		case 1053: //		Swedish                              
		case 2077: //		Swedish (Finland)                    
		case 1055: //		Turkish                              
		case 1058: //		Ukrainian                            
		case 1059: //		Belarusian                           
		case 1060: //		Slovenian                            
		case 1061: //		Estonian                             
		case 1062: //		Latvian                              
		case 1063: //		Lithuanian                           
		case 1078: //		Afrikaans                            
		case 1080: //		Faeroese  
			our_region = 1;
		default:
			our_region = 0;
		}
		r_server_region->SetInt(our_region);
	}

	if(g_user_language->GetString()[0]=='\0') // locate our language
	{
		LCID userLCID = GetUserDefaultLCID();
		switch(userLCID)
		{ // from http://support.microsoft.com/kb/193080
		case 1025: //		Arabic (Saudi Arabia)                
		case 2049: //		Arabic (Iraq)                        
		case 3073: //		Arabic (Egypt)                       
		case 4097: //		Arabic (Libya)                       
		case 5121: //		Arabic (Algeria)                     
		case 6145: //		Arabic (Morocco)                     
		case 7169: //		Arabic (Tunisia)                     
		case 8193: //		Arabic (Oman)                        
		case 9217: //		Arabic (Yemen)                       
		case 10241: //		Arabic (Syria)                      
		case 11265: //		Arabic (Jordan)                     
		case 12289: //		Arabic (Lebanon)                    
		case 13313: //		Arabic (Kuwait)                     
		case 14337: //		Arabic (U.A.E.)                     
		case 15361: //		Arabic (Bahrain)                    
		case 16385: //		Arabic (Qatar) 
		case 1026: //		Bulgarian                            
		case 1029: //		Czech                                
		case 1030: //		Danish                               
			g_user_language->SetString("english");
			break;
		case 1031: //		German (Standard)                    
		case 2055: //		German (Swiss)                       
		case 3079: //		German (Austrian)                    
		case 4103: //		German (Luxembourg)                  
		case 5127: //		German (Liechtenstein)               
			g_user_language->SetString("german");
			break;
		case 1032: //		Greek                                
		case 2057: //		English (United Kingdom)             
		case 6153: //		English (Ireland)                    
		case 1035: //		Finnish  
			g_user_language->SetString("english");
			break;
		case 1036: //		French (Standard)                    
		case 2060: //		French (Belgian)                     
		case 4108: //		French (Swiss)                       
		case 5132: //		French (Luxembourg)                  
			g_user_language->SetString("french");
			break;
		case 1037: //		Hebrew                               
		case 1038: //		Hungarian                            
		case 1039: //		Icelandic    
			g_user_language->SetString("english");
			break;
		case 1040: //		Italian (Standard)                   
		case 2064: //		Italian (Swiss)                      
			g_user_language->SetString("italian");
			break;
		case 1043: //		Dutch (Standard)                     
		case 2067: //		Dutch (Belgian)                      
		case 1044: //		Norwegian (Bokmal)                   
		case 2068: //		Norwegian (Nynorsk)                  
		case 1045: //		Polish                               
			g_user_language->SetString("english");
			break;
		case 2070: //		Portuguese (Portugal)                
			g_user_language->SetString("spanish");
			break;
		case 1048: //		Romanian                             
			g_user_language->SetString("english");
			break;
		case 1049: //		Russian                              
			g_user_language->SetString("russian");
			break;
		case 1050: //		Croatian                             
		case 2074: //		Serbian (Latin)                      
		case 3098: //		Serbian (Cyrillic)                   
		case 1051: //		Slovak                               
		case 1052: //		Albanian                             
		case 1053: //		Swedish                              
		case 2077: //		Swedish (Finland)                    
		case 1055: //		Turkish                              
			g_user_language->SetString("english");
			break;
		case 1058: //		Ukrainian                            
		case 1059: //		Belarusian                           
			g_user_language->SetString("russian");
			break;
		case 1060: //		Slovenian                            
		case 1061: //		Estonian                             
		case 1062: //		Latvian                              
		case 1063: //		Lithuanian                           
		case 1078: //		Afrikaans                            
		case 1080: //		Faeroese  
			g_user_language->SetString("english");
			break;
		default:
			g_user_language->SetString("english");
			break;
		}
	}
}

void writeGameOptionsFile()
{
	char fullPath[ MAX_PATH * 2 ];

	bool saveToLocal = true ;

	if( !g_locl_settings->GetBool() )
	{
		if( CreateFullIniPath( fullPath, false ) )
		{
			r3dOutToLog( "writeGameOptionsFile: Saving settings to %s\n", fullPath );
			g_pCmdProc->SaveVars( fullPath );

			saveToLocal = false ;
		}
		else
		{
			r3dOutToLog( "writeGameOptionsFile: couldn't create path to %s\n", fullPath );
		}
	}

	if( saveToLocal )
	{
		r3dOutToLog( "writeGameOptionsFile: saving settings to local folder.\n" );
		g_pCmdProc->SaveVars( INI_FILE );
	}
}

void OnFoundInputMap( const char* FullIniPath )
{
	r3dOutToLog( "readInputMap: found file at %s\n", FullIniPath );
	InputMappingMngr->loadMapping(FullIniPath);
}

void readInputMap()
{
	char FullIniPath[ MAX_PATH * 2 ];
	bool createdPath = CreateFullMappingPath( FullIniPath, false );
	if( createdPath && r3d_access( FullIniPath, 4 ) == 0 )
	{
		OnFoundInputMap( FullIniPath );
	}
	else
	{
		createdPath = CreateFullMappingPath( FullIniPath, true );

		if( createdPath && r3d_access( FullIniPath, 4 ) == 0 )
		{
			OnFoundInputMap( FullIniPath );
		}
		else
		{
			if( !createdPath )
			{
				r3dOutToLog( "readInputMap: Error: couldn't get local app path! Using defaults!\n" );
			}
			else
			{
				r3dOutToLog( "readInputMap: couldn't open both %s and %s! Using defaults.\n", INPUT_MAP_FILE, FullIniPath );
			}
		}
	}
}

void writeInputMap()
{
	char fullPath[ MAX_PATH * 2 ];

	if( CreateFullMappingPath( fullPath, false ) )
	{
		r3dOutToLog( "writeInputMap: Saving settings to %s\n", fullPath );
		InputMappingMngr->saveMapping(fullPath);
	}
	else
	{
		r3dOutToLog( "writeInputMap: couldn't create path to %s\n", fullPath );
	}
}

// Called right after main application window is created and OS critical systems initialized
// Probably it's good place to start networking, etc
void game::Init()
{
	static const char* gameName = "Global\\WarZ_Game_001";

#ifdef FINAL_BUILD  
	HANDLE h;
	if((h = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, gameName)) != NULL)
	{
		r3dOutToLog("game is already running\n");
		CloseHandle(h);
		TerminateProcess(GetCurrentProcess(), 0);
		return;
	}
#endif

	// create named event to signalize that game is started
	// handle will be automatically closed on program termination
	static HANDLE g_gameEvt = CreateEvent(NULL, FALSE, FALSE, gameName);

	r3dOutToLog("ComputerID: 0x%I64x\n", g_HardwareInfo.uniqueId);
	r3dOutToLog("Game Version: %s\n", GetBuildVersionString());

	MEMORYSTATUSEX stat;
	stat.dwLength = sizeof(stat);
	GlobalMemoryStatusEx(&stat);
	r3dOutToLog("Available memory: %d MB\n", (DWORD)(stat.ullTotalPhys / 1024 / 1024));

	r3dFileManager_OpenArchive("wz");

	RegisterAllVars();
	
	if (g_StudioCmdLineDX11Boot && r_dx11_boot)
	{
		r_dx11_boot->SetBool(true);
	}
#ifndef FINAL_BUILD
	RegisterHUDCommands();
#endif

	readGameOptionsFile();
	g_num_game_executed2->SetInt(g_num_game_executed2->GetInt()+1);
	writeGameOptionsFile(); // to make sure that it always exists
	DiscordPresence_Init();
	DiscordPresence_SetMenu();

	// set language
	if(strcmp(g_user_language->GetString(), "english")==0)
		gLangMngr.Init(LANG_EN);
	else if(strcmp(g_user_language->GetString(), "french")==0)
		gLangMngr.Init(LANG_FR);
	else if(strcmp(g_user_language->GetString(), "german")==0)
		gLangMngr.Init(LANG_DE);
	else if(strcmp(g_user_language->GetString(), "italian")==0)
		gLangMngr.Init(LANG_IT);
	else if(strcmp(g_user_language->GetString(), "spanish")==0)
		gLangMngr.Init(LANG_SP);
	else if(strcmp(g_user_language->GetString(), "russian")==0)
		gLangMngr.Init(LANG_RU);
	else // default to english, should not happen
	{
		r3d_assert(false);
		gLangMngr.Init(LANG_EN);
	}

	InputMappingMngr = new r3dInputMappingMngr;

	readInputMap();
	writeInputMap(); // to make sure that it always exists

	gUserSettings.loadSettings();
	gUserSettings.saveSettings(); // to make sure that it always exists

	InitSounds();

#ifndef FINAL_BUILD
	char buffer[ MAX_PATH ];
	GetCurrentDirectory( sizeof( buffer ), buffer );
	FileTrackChanges( buffer, CallbackFileChange );
#endif	

	GameWorld_Create();
	ClientGameLogic::CreateInstance();
}


//
// Called after MainLoop returns 
//
void game::Shutdown()
{
	FileTrackShutdown();

	snd_CloseSoundSystem();
	DiscordPresence_Shutdown();

	gLangMngr.Destroy();

	SAFE_DELETE(InputMappingMngr);
	UnregisterAllVars();

	ClientGameLogic::DeleteInstance();
	GameWorld_Destroy();
}


void 	ExecuteModelViewer()
{
	int m_ret = 0;
}


void 	ExecuteDirector()
{
	int m_ret = 0;
}

void 	ExecuteFXCreator()
{
	int m_ret = 0;
}



#ifndef FINAL_BUILD
void UpdateDB(const char* api_addr, const char* out_xml);
#endif

#ifndef FINAL_BUILD
extern bool g_AppMainBackToAppSelect;
#endif

void ExecuteNetworkGame();
void ExecuteLevelEditor();
void ExecuteCharacterEditor();
void ExecuteParticleEditor();
void ExecutePhysicsEditor();
void ExecuteBackendTest();

extern int		_r3d_bTerminateOnZ;

void game::MainLoop()
{
	// init steam we need to initialize this before the renderer for the overlay.
	{
		gSteam.InitSteam();
		if(gSteam.steamID) {
			gUserProfile.RegisterSteamCallbacks();
		}
	}
	

	if (g_StudioCmdLineDX11World && r_dx11_boot)
	{
		r_dx11_boot->SetBool(false);
	}

	InitRender(1);

	if (IsDX11BootActive())
	{
		ExecuteDX11SmokeLoop();

		if(gSteam.inited_) {
			gUserProfile.DeregisterSteamCallbacks();
			gSteam.Shutdown();
		}

		CloseRender();
		return;
	}

	StudioDX11WorldHybridInit();

	CurRenderPipeline = new r3dDefferedRenderer;
	CurRenderPipeline->Init();

	SetFocus(win::hWnd);

	r3dMenuInit();

	r3dParticleSystemInit();

	InitDesktopSystem();

	RegisterMsgProc(
		StudioWindowResizeMsgProc
	);

	InitPostFX();
	InitPointLightsRendererV2();

	g_pWind = new r3dWind ;

	g_pEngineConsole = new EngineConsole;
	g_pDefaultConsole = g_pEngineConsole;
	g_pEngineConsole->Init();
	g_pEngineConsole->SetCommandProcessor( g_pCmdProc );

	g_DamageLib = new DamageLib ;
	g_DamageLib->Load();

	g_MeshPropertyLib = new MeshPropertyLib;

#ifndef FINAL_BUILD
	g_Manipulator3d.Init();

	// show non-level specific art bugs
	r3dShowArtBugs() ;
#endif

	g_cursor_mode->SetChangeCallback( &CursorModeCallback ) ;

	_r3d_bTerminateOnZ = r_terminateOnZ->GetInt();

	// set dynamic matlib by default.
	// make it statis only in GAME mode
	r3dMaterialLibrary::IsDynamic = true;

	imgui_SetFixMouseCoords( false );


#if 0 // PAX_BUILD
	int m_ret = Menu_AppSelect::bStartGameSVN; 
#else
#ifdef FINAL_BUILD
	int m_ret = Menu_AppSelect::bStartGamePublic; // start game
#else
	int m_ret = 0;
	if ( *d_map_force_load->GetString() || LevelEditName[0]!='\0' )
		m_ret = Menu_AppSelect::bStartLevelEditor;
	else
		CALL_MENU(Menu_AppSelect);
#endif // __FINAL
#endif

#ifndef FINAL_BUILD
	// all choises is editors by default
	g_bEditMode = true;
	g_bStartedAsParticleEditor = false;
#endif

	void InitGrass();
	InitGrass();

	g_pHUDCameraEffects = new HUDCameraEffects ;

	r3d_assert(g_pWeaponArmory == NULL);
	g_pWeaponArmory = new ClientWeaponArmory();
	g_pWeaponArmory->Init();
	r3dShowArtBugs();

	// for editors, do not lock mouse. when we start game, in ExecuteNetworkGame we will set that var to true
	d_mouse_window_lock->SetBool(false);

#ifndef FINAL_BUILD
	AppSelectAgain:
#endif
	
	switch (m_ret)
	{
#ifndef FINAL_BUILD
	case	Menu_AppSelect::bUpdateDB:
		g_bEditMode = false;
		UpdateDB("26.163.92.76", "Data/Weapons/itemsDB.xml");
		MessageBox(0, "Successfully updated English DB!", "Result", MB_OK);
		break;
#endif

	case Menu_AppSelect::bStartGamePublic:
		DiscordPresence_SetGame("Game (Public Server)", r3dGameLevel::GetHomeDir());
		// override server settings if special key isn't set
		if(strstr(__r3dCmdLine, "-ffgrtvzdf") == NULL)
		{
			// hardcoded IP for now
			//g_serverip->SetString("127.0.0.1");
		}

		// override API settings
		//g_api_ip->SetString("26.163.92.76");
	case	Menu_AppSelect::bStartGameSVN:
		if(m_ret == Menu_AppSelect::bStartGameSVN)
			DiscordPresence_SetGame("Game (DEV Server)", r3dGameLevel::GetHomeDir());
		g_bEditMode = false;
		ExecuteNetworkGame();
		break;
#ifndef FINAL_BUILD
		
	case	Menu_AppSelect::bStartLevelEditor:
		DiscordPresence_SetEditor("Level Editor", LevelEditName);
		g_pVisibilityGrid = new VisibiltyGrid;

#ifndef FINAL_BUILD
		g_AppMainBackToAppSelect = false;
#endif

		ExecuteLevelEditor();

#ifndef FINAL_BUILD
		if (g_AppMainBackToAppSelect)
		{
			g_AppMainBackToAppSelect = false;

			SAFE_DELETE(g_pVisibilityGrid);

			LevelEditName[0] = 0;
			DiscordPresence_SetMenu();

			CALL_MENU(Menu_AppSelect);

			g_bEditMode = true;
			g_bStartedAsParticleEditor = false;

			goto AppSelectAgain;
		}
#endif

		break;

	case	Menu_AppSelect::bStartParticleEditor:
		DiscordPresence_SetEditor("Particle Editor", "WorkInProgress/Editor_Particles");
		g_bStartedAsParticleEditor = true;
		ExecuteParticleEditor();
		break;

	case Menu_AppSelect::bStartPhysicsEditor:
		DiscordPresence_SetEditor("Physics Editor", "WorkInProgress/Editor_Physics");
		ExecutePhysicsEditor();
		break;

	case Menu_AppSelect::bStartCharacterEditor:
		DiscordPresence_SetEditor("Character Editor", "WorkInProgress/Editor_Character");
		ExecuteCharacterEditor();
		break;
#endif
	};

#ifndef FINAL_BUILD
	if( g_bEditMode )
	{
		SAFE_DELETE( g_pVisibilityGrid );
	}
#endif
	
	// shutdown steam
	if(gSteam.inited_) {
		gUserProfile.DeregisterSteamCallbacks();
		gSteam.Shutdown();
	}

	SAFE_DELETE( g_pWind ) ;

	r3dMaterialLibrary::UnloadManaged();
	r3dMaterialLibrary::Reset();	
	MeshGlobalBuffer::unloadManaged();
	
	g_pWeaponArmory->Destroy();
	SAFE_DELETE(g_pWeaponArmory);

#ifndef FINAL_BUILD
	g_Manipulator3d.Close();
#endif

	r3dMenuClose();

	AI_Player_FreeStuff();

	SAFE_DELETE( g_pHUDCameraEffects ) ;
	
	SAFE_DELETE( g_GameRewards );

	void CloseGrass();
	CloseGrass();

	DestroyPointLightsRendererV2();

	ClosePostFX();

	SAFE_DELETE( g_DamageLib );
	SAFE_DELETE( g_MeshPropertyLib );
	
	r3dFreeGOBMeshes();

	obj_Zombie::FreePhysSkeletonCache();

	g_pEngineConsole->Release();
	SAFE_DELETE( g_pEngineConsole );

	g_pDefaultConsole = NULL;

	ReleaseDesktopSystem();

	UnregisterMsgProc(
		StudioWindowResizeMsgProc
	);

	DoneDrawCollections();

	r3dParticleSystemClose();
	CurRenderPipeline->Close();
	SAFE_DELETE(CurRenderPipeline);
	
	r3dMaterialLibrary::Destroy();

	StudioDX11WorldHybridShutdown();

	CloseRender();

	if(gSurveyOutLink)
		ShellExecute(NULL, "open", gSurveyOutLink, "", NULL, SW_SHOW);

	return;
}

#ifndef FINAL_BUILD
extern const char* g_ServerKey;
extern int		gDomainPort;
extern bool		gDomainUseSSL;

#include "CkByteData.h"
#include "CkBinData.h"
void UpdateDB(const char* api_addr, const char* out_xml)
{
	CkHttp http;

	// get items DB
	{
		CkHttpRequest req;
		req.put_HttpVerb("POST");
		req.put_Path("/APS/php/api_getItemsDB.php");
		req.AddParam("serverkey", "8B1E58D9-1D8A-4942-A2AB-B6809F0A4CDF");

		CkHttpResponse *resp = 0;
		resp = http.SynchronousRequest(api_addr, gDomainPort, gDomainUseSSL, req);
		if(!resp)
			r3dError("timeout getting items db");
			
		// we can't use getBosyStr() because it'll fuckup characters inside UTF-8 xml
		CkBinData responseBody;
		CkByteData bodyData;
		resp->GetBodyBd(responseBody);
		responseBody.GetBinary(bodyData);
		
		pugi::xml_document xmlFile;
		pugi::xml_parse_result parseResult = xmlFile.load_buffer_inplace(
			(void*)bodyData.getBytes(), 
			bodyData.getSize(), 
			pugi::parse_default, 
			pugi::encoding_utf8);
		if(!parseResult)
			r3dError("Failed to parse server weapon XML, error: %s", parseResult.description());

		xmlFile.save_file(
			out_xml, 
			PUGIXML_TEXT("\t"), 
			pugi::format_default, 
			pugi::encoding_utf8);
	}

	return;
}
#endif

void PrepareEditor(const char* levelName)
{
	r3dGameLevel::SetHomeDir(levelName);
	DiscordPresence_SetEditor("Editor", levelName);

	char Str[256];
	sprintf(Str, "%s\\Constants.var", r3dGameLevel::GetHomeDir());
	cvars_Read(Str);	
}

void 	ExecuteLevelEditor()
{
#ifndef FINAL_BUILD
	int m_ret = 0;

	//d_map_force_load->SetString("WorkInProgress\\ServerTest"); //@
	if (LevelEditName[0]=='\0')
	{
		if ( *d_map_force_load->GetString() )
		{
			r3dscpy( LevelEditName, d_map_force_load->GetString() ); 
		}
		else
		{
			CALL_MENU(Menu_Main);
			if(m_ret == 0) // exit
				return;
		}
	}

	PrepareEditor(LevelEditName);
	DiscordPresence_SetEditor("Level Editor", LevelEditName);
	PlayEditor();
#endif
}

extern int CurHUDID;

void ExecuteCharacterEditor()
{
	PrepareEditor("WorkInProgress/Editor_Character");
	DiscordPresence_SetEditor("Character Editor", "WorkInProgress/Editor_Character");
	CurHUDID = 5;
	PlayEditor();
}

void 	ExecuteParticleEditor()
{
	PrepareEditor("WorkInProgress/Editor_Particles");
	DiscordPresence_SetEditor("Particle Editor", "WorkInProgress/Editor_Particles");
	CurHUDID = 3;
	PlayEditor();
}

void ExecutePhysicsEditor()
{
	PrepareEditor("WorkInProgress/Editor_Physics");
	DiscordPresence_SetEditor("Physics Editor", "WorkInProgress/Editor_Physics");
	CurHUDID = 4;
	PlayEditor();
}

