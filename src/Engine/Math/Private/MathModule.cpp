#include "Math/Scalar.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"

#include <type_traits>

static_assert(
    sizeof(engine::math::Vector2) ==
    sizeof(float) * 2U
);

static_assert(
    sizeof(engine::math::Vector3) ==
    sizeof(float) * 3U
);

static_assert(
    alignof(engine::math::Vector2) ==
    alignof(float)
);

static_assert(
    alignof(engine::math::Vector3) ==
    alignof(float)
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Vector2
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Vector3
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Vector2
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Vector3
    >
);

namespace engine::math::detail
{
    void AnchorMathModule() noexcept
    {
    }
}