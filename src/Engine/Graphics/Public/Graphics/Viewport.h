#pragma once

#include <cstdint>

namespace engine::graphics
{
    struct Viewport final
    {
        float x = 0.0F;
        float y = 0.0F;
        float width = 0.0F;
        float height = 0.0F;
        float minDepth = 0.0F;
        float maxDepth = 1.0F;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    struct ScissorRect final
    {
        std::int32_t left = 0;
        std::int32_t top = 0;
        std::int32_t right = 0;
        std::int32_t bottom = 0;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] std::uint32_t Width() const noexcept;

        [[nodiscard]] std::uint32_t Height() const noexcept;
    };
}
