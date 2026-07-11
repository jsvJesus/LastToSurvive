#include "Core/Version.h"

namespace engine::core
{
    Version GetEngineVersion() noexcept
    {
        return Version
        {
            0,
            1,
            0,
            0
        };
    }
}