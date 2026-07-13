#include "Graphics/CommandContext.h"

#include <cmath>

namespace engine::graphics
{
    bool ClearColor::IsValid() const noexcept
    {
        return
            std::isfinite(red) &&
            std::isfinite(green) &&
            std::isfinite(blue) &&
            std::isfinite(alpha);
    }
}
