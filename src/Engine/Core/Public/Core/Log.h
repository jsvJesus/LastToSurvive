#pragma once

#include "Core/Logger.h"

namespace engine::core
{
    [[nodiscard]]
    Logger& GetLogger() noexcept;
}