#include "Graphics/Viewport.h"

#include <cmath>

namespace engine::graphics
{
    bool Viewport::IsValid() const noexcept
    {
        return std::isfinite(x) &&
            std::isfinite(y) &&
            std::isfinite(width) &&
            std::isfinite(height) &&
            std::isfinite(minDepth) &&
            std::isfinite(maxDepth) &&
            width > 0.0F &&
            height > 0.0F &&
            minDepth >= 0.0F &&
            maxDepth <= 1.0F &&
            minDepth <= maxDepth;
    }

    bool ScissorRect::IsValid() const noexcept
    {
        return right > left && bottom > top;
    }

    std::uint32_t ScissorRect::Width() const noexcept
    {
        if (!IsValid())
        {
            return 0;
        }

        return static_cast<std::uint32_t>(right - left);
    }

    std::uint32_t ScissorRect::Height() const noexcept
    {
        if (!IsValid())
        {
            return 0;
        }

        return static_cast<std::uint32_t>(bottom - top);
    }
}
