#include "Core/Log.h"

namespace engine::core
{
    Logger& GetLogger() noexcept
    {
        static Logger logger;
        return logger;
    }
}