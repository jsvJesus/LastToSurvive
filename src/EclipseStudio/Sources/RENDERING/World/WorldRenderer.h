#pragma once

#include "r3d.h"
#include "r3dRendererConfig.h"
#include "rendering/World/WorldDX11.h"

#include <windows.h>
#include <string.h>
#include <ctype.h>

enum EWorldRenderBackend
{
	WORLD_RENDER_BACKEND_DX9 = 0,
	WORLD_RENDER_BACKEND_DX11 = 1
};

static inline void WorldRender_LogText(const char* Text)
{
	if (!Text)
		return;

	OutputDebugStringA(Text);
	r3dOutToLog("%s", Text);
}

static inline bool WorldRender_IsSwitchBoundary(char Ch)
{
	return
		Ch == 0 ||
		isspace(static_cast<unsigned char>(Ch)) ||
		Ch == '"' ||
		Ch == '\'';
}

static inline bool WorldRender_CommandLineHasSwitch(const char* SwitchName)
{
	if (!SwitchName || !SwitchName[0])
		return false;

	const char* CmdLine = GetCommandLineA();

	if (!CmdLine || !CmdLine[0])
		return false;

	const size_t SwitchLen = strlen(SwitchName);

	for (const char* It = CmdLine; *It; ++It)
	{
		const bool bStartBoundary =
			It == CmdLine ||
			WorldRender_IsSwitchBoundary(*(It - 1));

		if (!bStartBoundary)
			continue;

		if (_strnicmp(It, SwitchName, SwitchLen) != 0)
			continue;

		if (!WorldRender_IsSwitchBoundary(It[SwitchLen]))
			continue;

		return true;
	}

	return false;
}

static inline bool WorldRender_WantsDX11()
{
	return
		WorldRender_CommandLineHasSwitch("-dx11world") ||
		WorldRender_CommandLineHasSwitch("/dx11world");
}

static inline bool WorldRender_IsDX11Compiled()
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	return true;
#else
	return false;
#endif
}

static inline EWorldRenderBackend WorldRender_GetBackend()
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	if (WorldRender_WantsDX11())
		return WORLD_RENDER_BACKEND_DX11;
#endif

	return WORLD_RENDER_BACKEND_DX9;
}

static inline bool WorldRender_IsDX11Active()
{
	return WorldRender_GetBackend() == WORLD_RENDER_BACKEND_DX11;
}

static inline const char* WorldRender_GetBackendName()
{
	return
		WorldRender_GetBackend() == WORLD_RENDER_BACKEND_DX11
		? "DX11_WORLD"
		: "DX9_WORLD";
}

static inline void WorldRender_LogSelectedBackendOnce()
{
	static bool bLogged = false;

	if (bLogged)
		return;

	bLogged = true;

	if (
		WorldRender_WantsDX11() &&
		!WorldRender_IsDX11Compiled()
	)
	{
		WorldRender_LogText(
			"[WorldRenderer] -dx11world requested, "
			"but LTS_STUDIO_DX11_WORLD is disabled. Using DX9 world.\n"
		);
	}

	if (WorldRender_GetBackend() == WORLD_RENDER_BACKEND_DX11)
	{
		WorldRender_LogText(
			"[WorldRenderer] Selected backend: DX11 world\n"
		);
	}
	else
	{
		WorldRender_LogText(
			"[WorldRenderer] Selected backend: DX9 world\n"
		);
	}
}

static inline bool WorldRender_TryRenderDX11()
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	if (!WorldDX11_IsAvailable())
	{
		WorldRender_LogText(
			"[WorldRenderer] DX11 world requested, "
			"but WorldDX11 is not available. Falling back to DX9.\n"
		);

		return false;
	}

	WorldDX11FrameDesc Desc = {};
	Desc.Width = r3dRenderer ? r3dRenderer->ScreenW : 1;
	Desc.Height = r3dRenderer ? r3dRenderer->ScreenH : 1;
	Desc.NearClip = r3dRenderer ? r3dRenderer->NearClip : 0.1f;
	Desc.FarClip = r3dRenderer ? r3dRenderer->FarClip : 10000.0f;

	return WorldDX11_Render(Desc);
#else
	return false;
#endif
}

static inline void WorldRender_Shutdown()
{
#if LTS_STUDIO_DX11
	WorldDX11_Shutdown();
#endif
}
