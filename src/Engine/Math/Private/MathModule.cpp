#include "Math/Color.h"
#include "Math/Scalar.h"

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

#include "Math/Matrix4.h"
#include "Math/Quaternion.h"
#include "Math/Transform.h"

#include "Math/BoundingBox.h"
#include "Math/Intersection.h"
#include "Math/Plane.h"
#include "Math/Ray.h"
#include "Math/Sphere.h"

#include "Math/Frustum.h"

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
    sizeof(engine::math::Vector4) ==
    sizeof(float) * 4U
);

static_assert(
    sizeof(engine::math::Color) ==
    sizeof(float) * 4U
);

static_assert(
    sizeof(engine::math::Color32) ==
    sizeof(std::uint8_t) * 4U
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
    alignof(engine::math::Vector4) ==
    alignof(float)
);

static_assert(
    alignof(engine::math::Color) ==
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
    std::is_standard_layout_v<
        engine::math::Vector4
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Color
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Color32
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

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Vector4
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Color
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Color32
    >
);

static_assert(
    sizeof(engine::math::Quaternion) ==
    sizeof(float) * 4U
);

static_assert(
    sizeof(engine::math::Matrix4) ==
    sizeof(float) * 16U
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Quaternion
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Matrix4
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Quaternion
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Matrix4
    >
);

static_assert(
    sizeof(engine::math::Transform) ==
    sizeof(float) * 10U
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Transform
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Transform
    >
);

static_assert(
    sizeof(engine::math::Ray) ==
    sizeof(float) * 6U
);

static_assert(
    sizeof(engine::math::Plane) ==
    sizeof(float) * 4U
);

static_assert(
    sizeof(engine::math::Sphere) ==
    sizeof(float) * 4U
);

static_assert(
    sizeof(engine::math::BoundingBox) ==
    sizeof(float) * 6U
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Ray
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Plane
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Sphere
    >
);

static_assert(
    std::is_standard_layout_v<
        engine::math::BoundingBox
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Ray
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Plane
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Sphere
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::BoundingBox
    >
);

static_assert(
    sizeof(engine::math::Frustum) ==
    sizeof(engine::math::Plane) *
    engine::math::FrustumPlaneCount
);

static_assert(
    std::is_standard_layout_v<
        engine::math::Frustum
    >
);

static_assert(
    std::is_trivially_copyable_v<
        engine::math::Frustum
    >
);

static_assert(
    static_cast<std::uint8_t>(
        engine::math::ContainmentType::Outside
    ) == 0U
);

static_assert(
    static_cast<std::uint8_t>(
        engine::math::ContainmentType::Inside
    ) == 1U
);

static_assert(
    static_cast<std::uint8_t>(
        engine::math::ContainmentType::Intersecting
    ) == 2U
);

namespace engine::math::detail
{
    void AnchorMathModule() noexcept
    {
    }

    constexpr engine::math::BoundingBox BoundsTest =
        engine::math::BoundingBox::FromOriginSize(
            {1.0f, 2.0f, 3.0f},
            {4.0f, 6.0f, 8.0f}
        );

    static_assert(
        BoundsTest.minimum.x == 1.0f
    );

    static_assert(
        BoundsTest.maximum.x == 5.0f
    );

    static_assert(
        BoundsTest.ContainsPoint(
            {3.0f, 4.0f, 6.0f}
        )
    );

    constexpr engine::math::Ray RayTest
    {
            {1.0f, 2.0f, 3.0f},
            {0.0f, 0.0f, 2.0f}
    };

    constexpr engine::math::Vector3 RayPointTest =
        RayTest.PointAt(2.0f);

    static_assert(RayPointTest.x == 1.0f);
    static_assert(RayPointTest.y == 2.0f);
    static_assert(RayPointTest.z == 7.0f);
}

namespace
{
    constexpr engine::math::Matrix4 TranslationTest =
        engine::math::Matrix4::CreateTranslation(
            {2.0f, 3.0f, 4.0f}
        );

    constexpr engine::math::Vector3 TranslatedPoint =
        TranslationTest.TransformPoint(
            {1.0f, 2.0f, 3.0f}
        );

    static_assert(TranslatedPoint.x == 3.0f);
    static_assert(TranslatedPoint.y == 5.0f);
    static_assert(TranslatedPoint.z == 7.0f);

    constexpr engine::math::Matrix4 CompositionTest =
        engine::math::Matrix4::CreateScale(
            {2.0f, 3.0f, 4.0f}
        ) *
        engine::math::Matrix4::CreateTranslation(
            {1.0f, 2.0f, 3.0f}
        );

    constexpr engine::math::Vector3 ComposedPoint =
        CompositionTest.TransformPoint(
            {1.0f, 1.0f, 1.0f}
        );

    // Сначала scale, затем translation.
    static_assert(ComposedPoint.x == 3.0f);
    static_assert(ComposedPoint.y == 5.0f);
    static_assert(ComposedPoint.z == 7.0f);
}