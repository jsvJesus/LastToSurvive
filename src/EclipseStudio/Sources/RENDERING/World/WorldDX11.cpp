#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/World/WorldDX11.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>

// Пока не линкуем d3d11.lib/dxgi.lib, потому что LTS_STUDIO_DX11 = 0.
// Когда включим LTS_STUDIO_DX11 = 1, тогда отдельно добавим libs в проект.

bool StudioWorldDX11_IsAvailable()
{
#if LTS_STUDIO_DX11_WORLD
    return true;
#else
    return false;
#endif
}

bool StudioWorldDX11_RenderWorld(const StudioWorldDX11FrameDesc& Desc)
{
    (void)Desc;

#if LTS_STUDIO_DX11_WORLD
    OutputDebugStringA(
        "[StudioWorldDX11] DX11 world renderer entry reached, "
        "but real DX11 world rendering is not implemented yet.\n"
    );
#endif

    // false = caller must fallback to old DX9 RenderDeferredScene1().
    return false;
}

#endif // LTS_STUDIO_DX11