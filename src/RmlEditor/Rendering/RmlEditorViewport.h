#pragma once

#include <RmlUi/Core/Types.h>

class RmlEditorViewport final
{
public:
    struct Rectangle
    {
        int X = 0;
        int Y = 0;
        int Width = 0;
        int Height = 0;
    };

    void SetLogicalSize(int Width, int Height);
    void UpdateFromElement(Rml::Element* Element);
    void Clear();

    bool IsValid() const;
    bool ContainsScreenPoint(int X, int Y) const;
    Rml::Vector2i ScreenToLogical(int X, int Y) const;

    const Rectangle& GetPhysicalRectangle() const;
    int GetLogicalWidth() const;
    int GetLogicalHeight() const;
    float GetScaleX() const;
    float GetScaleY() const;

private:
    Rectangle PhysicalRectangle;

    int LogicalWidth = 1920;
    int LogicalHeight = 1080;

    float ScaleX = 1.0f;
    float ScaleY = 1.0f;
};
