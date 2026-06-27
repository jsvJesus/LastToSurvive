#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/World/WorldDX11.h"
#include "rendering/World/WorldDX11Device.h"

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
    (void)Desc;

#if LTS_STUDIO_DX11_WORLD
    if (!WorldDX11_IsAvailable())
    {
        OutputDebugStringA(
            "[WorldDX11] Render skipped: device is not available\n"
        );

        return false;
    }

    OutputDebugStringA(
        "[WorldDX11] Render entry reached. Real world rendering is not implemented yet.\n"
    );
#endif

    // false = caller falls back to old DX9 RenderDeferredScene1().
    return false;
}

#endif // LTS_STUDIO_DX11