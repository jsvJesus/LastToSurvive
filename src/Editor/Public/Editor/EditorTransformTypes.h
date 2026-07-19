#pragma once

#include <cstdint>

namespace lts::editor
{
    enum class EditorTransformOperation : std::uint8_t
    {
        Select = 0,
        Move,
        Rotate,
        Scale
    };

    enum class EditorTransformSpace : std::uint8_t
    {
        World = 0,
        Local
    };

    enum class EditorTransformAxis : std::uint8_t
    {
        None = 0,
        X,
        Y,
        Z
    };

    struct EditorTransformVisualState final
    {
        EditorTransformOperation operation =
            EditorTransformOperation::Select;

        EditorTransformSpace space =
            EditorTransformSpace::World;

        EditorTransformAxis hotAxis =
            EditorTransformAxis::None;

        EditorTransformAxis activeAxis =
            EditorTransformAxis::None;
    };
}
