#pragma once

#include "Graphics/PipelineState.h"
#include "Graphics/ResourceHandle.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace engine::graphics
{
    enum class TextureFilter : std::uint8_t
    {
        Point = 0,
        Linear,
        Anisotropic,
        ComparisonPoint,
        ComparisonLinear
    };

    enum class TextureAddressMode : std::uint8_t
    {
        Wrap = 0,
        Mirror,
        Clamp,
        Border
    };

    struct SamplerDesc final
    {
        TextureFilter filter = TextureFilter::Linear;
        TextureAddressMode addressU = TextureAddressMode::Clamp;
        TextureAddressMode addressV = TextureAddressMode::Clamp;
        TextureAddressMode addressW = TextureAddressMode::Clamp;
        float mipLodBias = 0.0F;
        std::uint32_t maximumAnisotropy = 1U;
        ComparisonFunction comparisonFunction = ComparisonFunction::Always;
        std::array<float, 4U> borderColor{};
        float minimumLod = 0.0F;
        float maximumLod = (std::numeric_limits<float>::max)();
        const char* debugName = nullptr;

        [[nodiscard]] bool IsValid() const noexcept
        {
            const bool validFilter =
                filter >= TextureFilter::Point &&
                filter <= TextureFilter::ComparisonLinear;
            const auto validAddress = [](const TextureAddressMode value) noexcept
            {
                return value >= TextureAddressMode::Wrap &&
                    value <= TextureAddressMode::Border;
            };
            const bool comparisonFilter =
                filter == TextureFilter::ComparisonPoint ||
                filter == TextureFilter::ComparisonLinear;
            const bool validComparison =
                comparisonFunction >= ComparisonFunction::Never &&
                comparisonFunction <= ComparisonFunction::Always &&
                (comparisonFilter ||
                    comparisonFunction == ComparisonFunction::Always);
            const bool anisotropyValid =
                maximumAnisotropy >= 1U && maximumAnisotropy <= 16U &&
                (filter == TextureFilter::Anisotropic ||
                    maximumAnisotropy == 1U);
            bool finiteBorder = true;
            for (const float value : borderColor)
                finiteBorder = finiteBorder && std::isfinite(value);
            return validFilter && validAddress(addressU) &&
                validAddress(addressV) && validAddress(addressW) &&
                std::isfinite(mipLodBias) &&
                anisotropyValid && validComparison && finiteBorder &&
                std::isfinite(minimumLod) && std::isfinite(maximumLod) &&
                minimumLod <= maximumLod;
        }
    };
}
