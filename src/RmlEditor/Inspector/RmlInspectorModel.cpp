#include "RmlInspectorModel.h"

#include <RmlUi/Core/Box.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Property.h>
#include <RmlUi/Core/Variant.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
    std::string EscapeRml(const std::string& Text)
    {
        std::string Result;
        Result.reserve(Text.size());

        for (const char Character : Text)
        {
            switch (Character)
            {
            case '&':
                Result += "&amp;";
                break;

            case '<':
                Result += "&lt;";
                break;

            case '>':
                Result += "&gt;";
                break;

            case '"':
                Result += "&quot;";
                break;

            case '\'':
                Result += "&#39;";
                break;

            case '\r':
                break;

            case '\n':
                Result += " ";
                break;

            default:
                Result.push_back(Character);
                break;
            }
        }

        return Result;
    }

    std::string FormatNumber(float Value)
    {
        if (!std::isfinite(Value))
            return "0";

        const float Rounded = std::round(Value);

        if (std::fabs(Value - Rounded) < 0.01f)
        {
            return std::to_string(
                static_cast<int>(Rounded)
            );
        }

        std::ostringstream Stream;

        Stream
            << std::fixed
            << std::setprecision(2)
            << Value;

        return Stream.str();
    }

    std::string PropertyText(
        Rml::Element* Element,
        const char* PropertyName,
        const char* Fallback = "-"
    )
    {
        if (!Element || !PropertyName)
            return Fallback;

        const Rml::Property* Property =
            Element->GetProperty(PropertyName);

        if (!Property)
            return Fallback;

        const std::string Value = Property->ToString();

        return Value.empty()
            ? Fallback
            : Value;
    }

    std::string BuildAttributesText(Rml::Element* Element)
    {
        if (!Element)
            return "(none)";

        std::ostringstream Stream;
        bool First = true;

        for (const auto& Entry : Element->GetAttributes())
        {
            if (!First)
                Stream << ", ";

            First = false;

            Stream
                << Entry.first
                << "=\""
                << Entry.second.Get<Rml::String>()
                << "\"";
        }

        if (First)
            return "(none)";

        return Stream.str();
    }

    void AppendReadOnlyRow(
        std::ostringstream& Stream,
        const char* Label,
        const std::string& Value,
        const char* ExtraClass = nullptr
    )
    {
        Stream
            << "<div class=\"inspector_row\">"
            << "<div class=\"inspector_label\">"
            << EscapeRml(Label ? Label : "")
            << "</div>"
            << "<div class=\"inspector_field";

        if (ExtraClass && ExtraClass[0] != '\0')
            Stream << " " << ExtraClass;

        Stream
            << "\">"
            << EscapeRml(Value)
            << "</div>"
            << "</div>";
    }

    void AppendGridRow(
        std::ostringstream& Stream,
        const char* LeftLabel,
        const std::string& LeftValue,
        const char* RightLabel,
        const std::string& RightValue
    )
    {
        Stream
            << "<div class=\"grid_label\">"
            << EscapeRml(LeftLabel ? LeftLabel : "")
            << "</div>"
            << "<div class=\"grid_field\">"
            << EscapeRml(LeftValue)
            << "</div>"
            << "<div class=\"grid_label right\">"
            << EscapeRml(RightLabel ? RightLabel : "")
            << "</div>"
            << "<div class=\"grid_field\">"
            << EscapeRml(RightValue)
            << "</div>";
    }

    void AppendColorRow(
        std::ostringstream& Stream,
        const char* Label,
        const std::string& Value
    )
    {
        const std::string SafeColor =
            Value.empty() ||
            Value == "none" ||
            Value == "-"
                ? "#00000000"
                : Value;

        Stream
            << "<div class=\"inspector_row\">"
            << "<div class=\"inspector_label\">"
            << EscapeRml(Label ? Label : "")
            << "</div>"
            << "<div class=\"inspector_field color_field\">"
            << "<span class=\"color_swatch\" "
            << "style=\"background-color: "
            << EscapeRml(SafeColor)
            << ";\"></span>"
            << "<span>"
            << EscapeRml(Value)
            << "</span>"
            << "</div>"
            << "</div>";
    }

    const char* AlignClass(
        const std::string& Current,
        const char* Expected
    )
    {
        return Current == Expected
            ? "align_button active"
            : "align_button";
    }
}

void RmlInspectorModel::Attach(
    Rml::ElementDocument* EditorDocument
)
{
    ShellDocument = EditorDocument;
    ShowNoSelection();
}

void RmlInspectorModel::Detach()
{
    ShellDocument = nullptr;
}

void RmlInspectorModel::ShowNoSelection()
{
    SetInspectorRml(
        "<div class=\"inspector_section\">"
        "<div class=\"inspector_section_title\">Element</div>"
        "<div class=\"inspector_row\">"
        "<div class=\"inspector_field\">"
        "No element selected."
        "</div>"
        "</div>"
        "<div class=\"inspector_row\">"
        "<div class=\"inspector_field\">"
        "Enable Pick Element and click inside Live Preview."
        "</div>"
        "</div>"
        "</div>"
    );
}

void RmlInspectorModel::ShowElement(Rml::Element* Element)
{
    if (!Element)
    {
        ShowNoSelection();
        return;
    }

    const Rml::Box& Box = Element->GetBox();

    const std::string Tag = Element->GetTagName();

    const std::string Id =
        Element->GetId().empty()
            ? "(none)"
            : Element->GetId();

    const std::string Classes =
        Element->GetClassNames().empty()
            ? "(none)"
            : Element->GetClassNames();

    const std::string Address =
        Element->GetAddress(false, true);

    const std::string Attributes =
        BuildAttributesText(Element);

    const std::string BackgroundColor =
        PropertyText(
            Element,
            "background-color",
            "transparent"
        );

    const std::string BorderColor =
        PropertyText(
            Element,
            "border-top-color",
            "transparent"
        );

    const std::string TextColor =
        PropertyText(
            Element,
            "color",
            "#ffffff"
        );

    const std::string TextAlign =
        PropertyText(
            Element,
            "text-align",
            "left"
        );

    std::ostringstream Stream;

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Element"
        << "</div>";

    AppendReadOnlyRow(Stream, "Type", Tag);
    AppendReadOnlyRow(Stream, "ID", Id);
    AppendReadOnlyRow(Stream, "Classes", Classes);
    AppendReadOnlyRow(Stream, "DOM", Address);
    AppendReadOnlyRow(Stream, "Attrs", Attributes);

    Stream << "</div>";

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Position"
        << "</div>"
        << "<div class=\"inspector_grid\">";

    AppendGridRow(
        Stream,
        "X",
        FormatNumber(Element->GetAbsoluteLeft()),
        "Y",
        FormatNumber(Element->GetAbsoluteTop())
    );

    AppendGridRow(
        Stream,
        "Width",
        FormatNumber(Element->GetOffsetWidth()),
        "Height",
        FormatNumber(Element->GetOffsetHeight())
    );

    AppendGridRow(
        Stream,
        "Min W",
        PropertyText(Element, "min-width"),
        "Min H",
        PropertyText(Element, "min-height")
    );

    AppendGridRow(
        Stream,
        "Max W",
        PropertyText(Element, "max-width"),
        "Max H",
        PropertyText(Element, "max-height")
    );

    Stream << "</div></div>";

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Margin"
        << "</div>"
        << "<div class=\"inspector_grid\">";

    AppendGridRow(
        Stream,
        "Top",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Margin,
                Rml::BoxEdge::Top
            )
        ),
        "Right",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Margin,
                Rml::BoxEdge::Right
            )
        )
    );

    AppendGridRow(
        Stream,
        "Bottom",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Margin,
                Rml::BoxEdge::Bottom
            )
        ),
        "Left",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Margin,
                Rml::BoxEdge::Left
            )
        )
    );

    Stream << "</div></div>";

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Padding"
        << "</div>"
        << "<div class=\"inspector_grid\">";

    AppendGridRow(
        Stream,
        "Top",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Padding,
                Rml::BoxEdge::Top
            )
        ),
        "Right",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Padding,
                Rml::BoxEdge::Right
            )
        )
    );

    AppendGridRow(
        Stream,
        "Bottom",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Padding,
                Rml::BoxEdge::Bottom
            )
        ),
        "Left",
        FormatNumber(
            Box.GetEdge(
                Rml::BoxArea::Padding,
                Rml::BoxEdge::Left
            )
        )
    );

    Stream << "</div></div>";

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Background"
        << "</div>";

    AppendColorRow(
        Stream,
        "Color",
        BackgroundColor
    );

    AppendReadOnlyRow(
        Stream,
        "Image",
        PropertyText(
            Element,
            "background-image",
            "none"
        )
    );

    Stream << "</div>";

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Border"
        << "</div>";

    AppendColorRow(
        Stream,
        "Color",
        BorderColor
    );

    AppendReadOnlyRow(
        Stream,
        "Width",
        PropertyText(
            Element,
            "border-top-width",
            "0px"
        ),
        "short_field"
    );

    AppendReadOnlyRow(
        Stream,
        "Radius",
        PropertyText(
            Element,
            "border-top-left-radius",
            "0px"
        ),
        "short_field"
    );

    Stream << "</div>";

    Stream
        << "<div class=\"inspector_section\">"
        << "<div class=\"inspector_section_title\">"
        << "Text"
        << "</div>";

    AppendReadOnlyRow(
        Stream,
        "Font",
        PropertyText(
            Element,
            "font-family",
            "-"
        )
    );

    AppendReadOnlyRow(
        Stream,
        "Size",
        PropertyText(
            Element,
            "font-size",
            "-"
        )
    );

    AppendColorRow(
        Stream,
        "Color",
        TextColor
    );

    Stream
        << "<div class=\"inspector_row\">"
        << "<div class=\"inspector_label\">"
        << "Align"
        << "</div>"
        << "<div class=\"align_group\">"
        << "<button class=\""
        << AlignClass(TextAlign, "left")
        << "\">L</button>"
        << "<button class=\""
        << AlignClass(TextAlign, "center")
        << "\">C</button>"
        << "<button class=\""
        << AlignClass(TextAlign, "right")
        << "\">R</button>"
        << "<button class=\""
        << AlignClass(TextAlign, "justify")
        << "\">J</button>"
        << "</div>"
        << "</div>"
        << "</div>";

    Stream
        << "<div class=\"inspector_section states_section\">"
        << "<div class=\"inspector_section_title\">"
        << "States"
        << "</div>"
        << "<div class=\"state_buttons\">"
        << "<button class=\"state_button active\">"
        << ":normal"
        << "</button>"
        << "<button class=\"state_button\">"
        << ":hover"
        << "</button>"
        << "<button class=\"state_button\">"
        << ":active"
        << "</button>"
        << "<button class=\"state_button\">"
        << ":disabled"
        << "</button>"
        << "</div>"
        << "</div>";

    SetInspectorRml(Stream.str());
}

void RmlInspectorModel::SetInspectorRml(
    const std::string& RmlText
)
{
    if (!ShellDocument)
        return;

    Rml::Element* InspectorContent =
        ShellDocument->GetElementById(
            "inspector_content"
        );

    if (!InspectorContent)
        return;

    InspectorContent->SetInnerRML(RmlText);
}