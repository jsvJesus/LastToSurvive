#pragma once

#include "Runtime/EngineMode.h"
#include "Runtime/RendererBackend.h"

#include <string>

namespace engine::runtime
{
    struct EngineConfig final
    {
        std::string applicationName =
            "LastToSurvive";

        EngineMode mode =
            EngineMode::Studio;

        RendererBackend rendererBackend =
            RendererBackend::D3D9;

        bool enableValidation = true;

        bool enableMainThreadChecks = true;
    };
}