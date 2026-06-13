#include "RmlEditorViewport.h"

#include <RmlUi/Core/Element.h>

#include <algorithm>
#include <cmath>

void RmlEditorViewport::SetLogicalSize(int Width, int Height)
{
    LogicalWidth = std::max(1, Width);
    LogicalHeight = std::max(1, Height);

    if (PhysicalRectangle.Width > 0 && PhysicalRectangle.Height > 0)
    {
        ScaleX =
            static_cast<float>(PhysicalRectangle.Width) /
            static_cast<float>(LogicalWidth);

        ScaleY =
            static_cast<float>(PhysicalRectangle.Height) /
            static_cast<float>(LogicalHeight);
    }
}

void RmlEditorViewport::UpdateFromElement(Rml::Element* Element)
{
    if (!Element)
    {
        Clear();
        return;
    }

    const Rml::Vector2f Offset =
        Element->GetAbsoluteOffset(Rml::BoxArea::Content);

    PhysicalRectangle.X =
        static_cast<int>(std::floor(Offset.x + 0.5f));

    PhysicalRectangle.Y =
        static_cast<int>(std::floor(Offset.y + 0.5f));

    PhysicalRectangle.Width =
        std::max(
            0,
            static_cast<int>(
                std::floor(Element->GetClientWidth() + 0.5f)
            )
        );

    PhysicalRectangle.Height =
        std::max(
            0,
            static_cast<int>(
                std::floor(Element->GetClientHeight() + 0.5f)
            )
        );

    ScaleX =
        PhysicalRectangle.Width > 0
            ? static_cast<float>(PhysicalRectangle.Width) /
                static_cast<float>(LogicalWidth)
            : 1.0f;

    ScaleY =
        PhysicalRectangle.Height > 0
            ? static_cast<float>(PhysicalRectangle.Height) /
                static_cast<float>(LogicalHeight)
            : 1.0f;
}

void RmlEditorViewport::Clear()
{
    PhysicalRectangle = Rectangle{};
    ScaleX = 1.0f;
    ScaleY = 1.0f;
}

bool RmlEditorViewport::IsValid() const
{
    return
        PhysicalRectangle.Width > 0 &&
        PhysicalRectangle.Height > 0 &&
        LogicalWidth > 0 &&
        LogicalHeight > 0;
}

bool RmlEditorViewport::ContainsScreenPoint(int X, int Y) const
{
    return
        IsValid() &&
        X >= PhysicalRectangle.X &&
        Y >= PhysicalRectangle.Y &&
        X < PhysicalRectangle.X + PhysicalRectangle.Width &&
        Y < PhysicalRectangle.Y + PhysicalRectangle.Height;
}

Rml::Vector2i RmlEditorViewport::ScreenToLogical(int X, int Y) const
{
    if (!IsValid())
        return Rml::Vector2i(0, 0);

    const int LocalX =
        std::max(0, X - PhysicalRectangle.X);

    const int LocalY =
        std::max(0, Y - PhysicalRectangle.Y);

    const float LogicalX =
        ScaleX > 0.0f
            ? static_cast<float>(LocalX) / ScaleX
            : 0.0f;

    const float LogicalY =
        ScaleY > 0.0f
            ? static_cast<float>(LocalY) / ScaleY
            : 0.0f;

    return Rml::Vector2i(
        std::max(
            0,
            std::min(
                LogicalWidth - 1,
                static_cast<int>(std::floor(LogicalX + 0.5f))
            )
        ),
        std::max(
            0,
            std::min(
                LogicalHeight - 1,
                static_cast<int>(std::floor(LogicalY + 0.5f))
            )
        )
    );
}

const RmlEditorViewport::Rectangle&
RmlEditorViewport::GetPhysicalRectangle() const
{
    return PhysicalRectangle;
}

int RmlEditorViewport::GetLogicalWidth() const
{
    return LogicalWidth;
}

int RmlEditorViewport::GetLogicalHeight() const
{
    return LogicalHeight;
}

float RmlEditorViewport::GetScaleX() const
{
    return ScaleX;
}

float RmlEditorViewport::GetScaleY() const
{
    return ScaleY;
}
