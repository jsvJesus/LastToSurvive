#pragma once

#include "r3dRendererConfig.h"

#include <windows.h>
#include <string.h>
#include <ctype.h>

enum EStudioWorldRendererBackend
{
	STUDIO_WORLD_RENDER_DX9 = 0,
	STUDIO_WORLD_RENDER_DX11 = 1
};

static inline bool StudioWorldRenderer_IsSwitchBoundary(char Ch)
{
	return
		Ch == 0 ||
		isspace(static_cast<unsigned char>(Ch)) ||
		Ch == '"' ||
		Ch == '\'';
}

static inline bool StudioWorldRenderer_CommandLineHasSwitch(const char* SwitchName)
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
			StudioWorldRenderer_IsSwitchBoundary(*(It - 1));

		if (!bStartBoundary)
			continue;

		if (_strnicmp(It, SwitchName, SwitchLen) != 0)
			continue;

		if (!StudioWorldRenderer_IsSwitchBoundary(It[SwitchLen]))
			continue;

		return true;
	}

	return false;
}

static inline bool StudioWorldRenderer_WantsDX11World()
{
	return
		StudioWorldRenderer_CommandLineHasSwitch("-dx11world") ||
		StudioWorldRenderer_CommandLineHasSwitch("/dx11world");
}

static inline bool StudioWorldRenderer_IsDX11WorldCompiled()
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	return true;
#else
	return false;
#endif
}

static inline EStudioWorldRendererBackend StudioWorldRenderer_GetBackend()
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	if (StudioWorldRenderer_WantsDX11World())
		return STUDIO_WORLD_RENDER_DX11;
#endif

	return STUDIO_WORLD_RENDER_DX9;
}

static inline bool StudioWorldRenderer_IsDX11WorldActive()
{
	return StudioWorldRenderer_GetBackend() == STUDIO_WORLD_RENDER_DX11;
}

static inline const char* StudioWorldRenderer_GetBackendName()
{
	return
		StudioWorldRenderer_GetBackend() == STUDIO_WORLD_RENDER_DX11
		? "DX11_WORLD"
		: "DX9_WORLD";
}

static inline void StudioWorldRenderer_LogSelectedBackendOnce()
{
	static bool bLogged = false;

	if (bLogged)
		return;

	bLogged = true;

	if (
		StudioWorldRenderer_WantsDX11World() &&
		!StudioWorldRenderer_IsDX11WorldCompiled()
	)
	{
		OutputDebugStringA(
			"[StudioWorldRenderer] -dx11world requested, "
			"but LTS_STUDIO_DX11_WORLD is disabled. Using DX9 world.\n"
		);
	}

	if (StudioWorldRenderer_GetBackend() == STUDIO_WORLD_RENDER_DX11)
	{
		OutputDebugStringA(
			"[StudioWorldRenderer] Selected backend: DX11 world\n"
		);
	}
	else
	{
		OutputDebugStringA(
			"[StudioWorldRenderer] Selected backend: DX9 world\n"
		);
	}
}

// Temporary stub.
// Later this will call real DX11 world renderer.
// For now it always returns false, so caller falls back to DX9.
static inline bool StudioWorldRenderer_RenderDX11WorldStub()
{
#if LTS_STUDIO_DX11 && LTS_STUDIO_DX11_WORLD
	OutputDebugStringA(
		"[StudioWorldRenderer] DX11 world path selected, "
		"but DX11 world renderer is not implemented yet. Falling back to DX9.\n"
	);
#endif

	return false;
}