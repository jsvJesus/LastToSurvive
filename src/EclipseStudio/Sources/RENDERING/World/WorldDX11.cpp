#include "r3dPCH.h"
#include "r3d.h"

#include "rendering/World/WorldDX11.h"

#if LTS_STUDIO_DX11

#include <D3D11.h>
#include <DXGI.h>

bool WorldDX11_IsAvailable()
{
#if LTS_STUDIO_DX11_WORLD
    return true;
#else
    return false;
#endif
}

bool WorldDX11_Render(const WorldDX11FrameDesc& Desc)
{
    (void)Desc;

#if LTS_STUDIO_DX11_WORLD
    OutputDebugStringA(
        "[WorldDX11] DX11 world renderer entry reached, "
        "but real DX11 world rendering is not implemented yet.\n"
    );
#endif

    // false = caller must fallback to old DX9 RenderDeferredScene1().
    return false;
}

#endif // LTS_STUDIO_DX11