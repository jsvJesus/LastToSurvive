#pragma once
#include "Graphics/Viewport.h"
#include "Math/Matrix4.h"
#include "Math/Vector3.h"
#include <cstdint>
namespace engine::renderer
{
    enum class MaterialDebugMode : std::uint8_t
    {
        Lit=0,
        BaseColor,
        WorldNormal,
        TangentNormal,
        Roughness,
        Specular,
        Emissive,
        Count
    };
    struct RenderView final
    {
        engine::math::Matrix4 view;
        engine::math::Matrix4 projection;
        engine::math::Matrix4 viewProjection;
        engine::math::Vector3 cameraPosition;
        engine::graphics::Viewport viewport;
        engine::math::Vector3 lightDirection{-0.45F,0.75F,-0.55F};
        engine::math::Vector3 lightColor{1.0F,1.0F,1.0F};
        float lightIntensity=1.0F;
        engine::math::Vector3 ambientColor{0.22F,0.22F,0.22F};
        float elapsedTime=0.0F;
        MaterialDebugMode debugMode=MaterialDebugMode::Lit;
        [[nodiscard]] bool IsValid() const noexcept;
    };
}
