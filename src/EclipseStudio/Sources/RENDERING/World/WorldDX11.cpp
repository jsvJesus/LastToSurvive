#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/World/WorldDX11.h"
#include "rendering/World/WorldDX11Device.h"
#include "rendering/World/WorldDX11Resources.h"

#if LTS_STUDIO_DX11

namespace
{
	bool GWorldDX11Initialized = false;
}

bool WorldDX11_Init()
{
	if (GWorldDX11Initialized)
		return true;

	if (!WorldDX11Device_Init())
	{
		OutputDebugStringA(
			"[WorldDX11] Device initialization failed\n"
		);

		return false;
	}

	GWorldDX11Initialized = true;

	OutputDebugStringA(
		"[WorldDX11] Initialized\n"
	);

	return true;
}

void WorldDX11_Shutdown()
{
	if (!GWorldDX11Initialized)
		return;

	WorldDX11Resources_Shutdown();

	GWorldDX11Initialized = false;

	WorldDX11Device_Shutdown();

	OutputDebugStringA(
		"[WorldDX11] Shutdown\n"
	);
}

bool WorldDX11_IsAvailable()
{
#if LTS_STUDIO_DX11_WORLD
	return GWorldDX11Initialized || WorldDX11_Init();
#else
	return false;
#endif
}

bool WorldDX11_Render(const WorldDX11FrameDesc& Desc)
{
#if LTS_STUDIO_DX11_WORLD
	if (!WorldDX11_IsAvailable())
	{
		OutputDebugStringA(
			"[WorldDX11] Render skipped: device is not available\n"
		);

		return false;
	}

	WorldDX11ResourcesDesc ResourcesDesc = {};
	ResourcesDesc.Width = Desc.Width;
	ResourcesDesc.Height = Desc.Height;
	ResourcesDesc.ColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	ResourcesDesc.DepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	if (!WorldDX11Resources_Resize(ResourcesDesc))
	{
		OutputDebugStringA(
			"[WorldDX11] Render skipped: resources resize failed\n"
		);

		return false;
	}

	WorldDX11Resources_Bind();

	// Test clear. This proves that the DX11 world path can create,
	// bind and clear its own render targets.
	// We still return false, so old DX9 world renders normally after this.
	WorldDX11Resources_Clear(
		0.02f,
		0.04f,
		0.06f,
		1.0f
	);

	OutputDebugStringA(
		"[WorldDX11] Clear test completed. Falling back to DX9 world.\n"
	);
#endif

	// false = caller falls back to old DX9 RenderDeferredScene1().
	return false;
}

#endif // LTS_STUDIO_DX11