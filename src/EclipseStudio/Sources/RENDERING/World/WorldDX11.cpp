#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRendererConfig.h"
#include "rendering/World/WorldDX11.h"

#if LTS_STUDIO_DX11

#include "rendering/DX11/RenderDX11.h"

namespace
{
	bool gWorldDX11InitFailed = false;
	bool gWorldDX11InitFailedLogged = false;
	bool gWorldDX11UnavailableLogged = false;

	void WorldDX11_LogText(const char* Text)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}

	void WorldDX11_LogInitFailedOnce()
	{
		if (gWorldDX11InitFailedLogged)
			return;

		gWorldDX11InitFailedLogged = true;

		WorldDX11_LogText(
			"[DX11][World] Init failed. DX9 world fallback remains active.\n"
		);
	}

	void WorldDX11_LogUnavailableOnce()
	{
		if (gWorldDX11UnavailableLogged)
			return;

		gWorldDX11UnavailableLogged = true;

		WorldDX11_LogText(
			"[DX11][World] Render skipped: RenderDX11 is not available.\n"
		);
	}
}

bool WorldDX11_Init()
{
	if (gWorldDX11InitFailed)
		return false;

	if (RenderDX11_Init())
		return true;

	gWorldDX11InitFailed = true;
	WorldDX11_LogInitFailedOnce();

	return false;
}

void WorldDX11_Shutdown()
{
	if (RenderDX11_IsReady())
		RenderDX11_Shutdown();

	gWorldDX11InitFailed = false;
	gWorldDX11InitFailedLogged = false;
	gWorldDX11UnavailableLogged = false;
}

bool WorldDX11_IsAvailable()
{
#if LTS_STUDIO_DX11_WORLD
	if (gWorldDX11InitFailed)
		return false;

	return
		RenderDX11_IsReady() ||
		WorldDX11_Init();
#else
	return false;
#endif
}

bool WorldDX11_Render(
	const WorldDX11FrameDesc& Desc
)
{
#if LTS_STUDIO_DX11_WORLD
	if (!WorldDX11_IsAvailable())
	{
		WorldDX11_LogUnavailableOnce();
		return false;
	}

	return RenderDX11_RenderWorld(Desc);
#else
	(void)Desc;
	return false;
#endif
}

void WorldDX11_DrawDebugPreviewDX9()
{
#if LTS_STUDIO_DX11_WORLD
	RenderDX11_DrawDebugPreviewDX9();
#endif
}

#endif // LTS_STUDIO_DX11
