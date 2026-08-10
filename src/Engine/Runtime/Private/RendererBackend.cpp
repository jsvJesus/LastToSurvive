#include "Runtime/RendererBackend.h"

namespace engine::runtime
{
    const char* ToString(
        const RendererBackend backend) noexcept
    {
        switch (backend)
        {
        case RendererBackend::None:
            return "none";

        case RendererBackend::D3D11:
            return "d3d11";

        default:
            return "unknown";
        }
    }
}
