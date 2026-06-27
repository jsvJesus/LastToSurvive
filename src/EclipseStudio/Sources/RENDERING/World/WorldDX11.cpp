#include "r3dPCH.h"
#include "r3d.h"

#include "r3dRendererConfig.h"
#include "rendering/World/WorldDX11.h"

#if LTS_STUDIO_DX11

#include "rendering/DX11/RenderDX11.h"

namespace
{
	void WorldDX11_LogText(const char* Text)
	{
		if (!Text)
			return;

		OutputDebugStringA(Text);
		r3dOutToLog("%s", Text);
	}
}

bool WorldDX11_Init()
{
	return RenderDX11_Init();
}

void WorldDX11_Shutdown()
{
	RenderDX11_Shutdown();
}

bool WorldDX11_IsAvailable()
{
#if LTS_STUDIO_DX11_WORLD
	return
		RenderDX11_IsReady() ||
		RenderDX11_Init();
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
		WorldDX11_LogText(
			"[WorldDX11] Render skipped: RenderDX11 is not available\n"
		);

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
