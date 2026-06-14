#include "RmlElementPicker.h"

#include "../App/RmlEditorLog.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    std::string PixelValue(float Value)
    {
        if (!std::isfinite(Value))
            Value = 0.0f;

        return std::to_string(Value) + "px";
    }
}

bool RmlElementPicker::Initialize(Rml::Context* Context)
{
    Shutdown();
    PickerContext = Context;

    if (!PickerContext)
        return false;

    return CreateOverlayDocument();
}

void RmlElementPicker::Shutdown()
{
    ClearHover();
    ClearSelection();
    DestroyOverlayDocument();

    PickerContext = nullptr;
    ActiveDocument = nullptr;
    Enabled = false;
}

void RmlElementPicker::SetDocument(
    Rml::ElementDocument* Document
)
{
    ClearHover();
    ClearSelection();
    ActiveDocument = Document;

    if (!ActiveDocument)
    {
        Enabled = false;

        if (OverlayDocument)
            OverlayDocument->Hide();

        return;
    }

    if (OverlayDocument)
        OverlayDocument->PullToFront();

    UpdateOverlay();
}

void RmlElementPicker::SetEnabled(bool InEnabled)
{
    Enabled =
        InEnabled &&
        PickerContext != nullptr &&
        ActiveDocument != nullptr &&
        OverlayDocument != nullptr;

    if (!Enabled)
    {
        ClearHover();

        if (OverlayDocument)
            OverlayDocument->Hide();

        return;
    }

    OverlayDocument->Show(
        Rml::ModalFlag::None,
        Rml::FocusFlag::None
    );

    OverlayDocument->PullToFront();
    UpdateOverlay();
}

bool RmlElementPicker::IsEnabled() const
{
    return Enabled;
}

bool RmlElementPicker::UpdateHover(Rml::Vector2i Point)
{
    if (!Enabled)
        return AssignHover(nullptr);

    return AssignHover(FindSelectableElement(Point));
}

bool RmlElementPicker::SelectAt(Rml::Vector2i Point)
{
    if (!Enabled)
        return false;

    Rml::Element* Element = FindSelectableElement(Point);

    AssignHover(Element);
    return AssignSelection(Element);
}

void RmlElementPicker::ClearHover()
{
    AssignHover(nullptr);
}

void RmlElementPicker::ClearSelection()
{
    AssignSelection(nullptr);
}

bool RmlElementPicker::RestoreSelectionById(
    const std::string& ElementId
)
{
    if (!ActiveDocument || ElementId.empty())
        return false;

    Rml::Element* Element =
        ActiveDocument->GetElementById(ElementId);

    if (!IsSelectableElement(Element, ActiveDocument))
        return false;

    AssignSelection(Element);
    return true;
}

void RmlElementPicker::Update()
{
    bool OverlayChanged = false;

    if (HadHover && !HoveredElement)
    {
        HoveredElement.reset();
        HadHover = false;
        OverlayChanged = true;
    }

    if (HadSelection && !SelectedElement)
    {
        SelectedElement.reset();
        SelectedElementId.clear();
        SelectedElementAddress.clear();
        HadSelection = false;

        ++SelectionRevision;
        OverlayChanged = true;
    }

    if (OverlayChanged || Enabled)
        UpdateOverlay();
}

Rml::Element* RmlElementPicker::GetHoveredElement() const
{
    return HoveredElement.get();
}

Rml::Element* RmlElementPicker::GetSelectedElement() const
{
    return SelectedElement.get();
}

const std::string&
RmlElementPicker::GetSelectedElementId() const
{
    return SelectedElementId;
}

const std::string&
RmlElementPicker::GetSelectedElementAddress() const
{
    return SelectedElementAddress;
}

std::uint64_t RmlElementPicker::GetSelectionRevision() const
{
    return SelectionRevision;
}

bool RmlElementPicker::CreateOverlayDocument()
{
    if (!PickerContext)
        return false;

    static const char* OverlayRml =
        "<rml>"
        "<head>"
        "<style>"
        "* { box-sizing: border-box; }"
        "html, body {"
        " width: 100%;"
        " height: 100%;"
        " margin: 0;"
        " padding: 0;"
        " background-color: transparent;"
        "}"
        "#rml_editor_hover_box {"
        " position: absolute;"
        " display: none;"
        " border-width: 1px;"
        " border-color: #58a6ff;"
        " background-color: transparent;"
        " z-index: 100000;"
        "}"
        "#rml_editor_selection_box {"
        " position: absolute;"
        " display: none;"
        " border-width: 2px;"
        " border-color: #f29a1d;"
        " background-color: transparent;"
        " z-index: 100001;"
        "}"
        "</style>"
        "</head>"
        "<body>"
        "<div id=\"rml_editor_hover_box\"></div>"
        "<div id=\"rml_editor_selection_box\"></div>"
        "</body>"
        "</rml>";

    OverlayDocument = PickerContext->LoadDocumentFromMemory(
        OverlayRml,
        "[RmlEditor Element Picker Overlay]"
    );

    if (!OverlayDocument)
    {
        RmlEditorLog::Write(
            "[RmlEditor] Failed to create element picker overlay document"
        );

        return false;
    }

    OverlayDocument->Hide();
    return true;
}

void RmlElementPicker::DestroyOverlayDocument()
{
    if (PickerContext && OverlayDocument)
    {
        PickerContext->UnloadDocument(OverlayDocument);
        PickerContext->Update();
    }

    OverlayDocument = nullptr;
}

void RmlElementPicker::UpdateOverlay()
{
    if (!OverlayDocument)
        return;

    if (!Enabled)
    {
        OverlayDocument->Hide();
        return;
    }

    UpdateOverlayElement(
        "rml_editor_hover_box",
        HoveredElement.get()
    );

    UpdateOverlayElement(
        "rml_editor_selection_box",
        SelectedElement.get()
    );
}

void RmlElementPicker::UpdateOverlayElement(
    const char* OverlayElementId,
    Rml::Element* TargetElement
)
{
    if (!OverlayDocument || !OverlayElementId)
        return;

    Rml::Element* OverlayElement =
        OverlayDocument->GetElementById(OverlayElementId);

    if (!OverlayElement)
        return;

    if (!TargetElement ||
        TargetElement->GetOwnerDocument() != ActiveDocument ||
        !TargetElement->IsVisible(true))
    {
        OverlayElement->SetProperty("display", "none");
        return;
    }

    const float Left = TargetElement->GetAbsoluteLeft();
    const float Top = TargetElement->GetAbsoluteTop();

    const float Width = std::max(
        0.0f,
        TargetElement->GetOffsetWidth()
    );

    const float Height = std::max(
        0.0f,
        TargetElement->GetOffsetHeight()
    );

    if (Width <= 0.0f || Height <= 0.0f)
    {
        OverlayElement->SetProperty("display", "none");
        return;
    }

    OverlayElement->SetProperty("display", "block");
    OverlayElement->SetProperty("left", PixelValue(Left));
    OverlayElement->SetProperty("top", PixelValue(Top));
    OverlayElement->SetProperty("width", PixelValue(Width));
    OverlayElement->SetProperty("height", PixelValue(Height));
}

Rml::Element* RmlElementPicker::FindSelectableElement(
    Rml::Vector2i Point
) const
{
    if (!PickerContext || !ActiveDocument)
        return nullptr;

    Rml::Element* Element =
        PickerContext->GetElementAtPoint(
            Rml::Vector2f(
                static_cast<float>(Point.x),
                static_cast<float>(Point.y)
            ),
            OverlayDocument
        );

    while (Element)
    {
        if (IsSelectableElement(Element, ActiveDocument))
            return Element;

        if (Element == ActiveDocument)
            break;

        Element = Element->GetParentNode();
    }

    return nullptr;
}

bool RmlElementPicker::AssignHover(Rml::Element* Element)
{
    if (HoveredElement.get() == Element &&
        (Element != nullptr || !HadHover))
    {
        return false;
    }

    HoveredElement.reset();
    HadHover = false;

    if (Element)
    {
        HoveredElement = Element->GetObserverPtr();
        HadHover = true;
    }

    UpdateOverlay();
    return true;
}

bool RmlElementPicker::AssignSelection(
    Rml::Element* Element
)
{
    if (SelectedElement.get() == Element &&
        (Element != nullptr || !HadSelection))
    {
        return false;
    }

    SelectedElement.reset();
    SelectedElementId.clear();
    SelectedElementAddress.clear();
    HadSelection = false;

    if (Element)
    {
        SelectedElement = Element->GetObserverPtr();
        SelectedElementId = Element->GetId();
        SelectedElementAddress =
            Element->GetAddress(false, true);

        HadSelection = true;
    }

    ++SelectionRevision;

    UpdateOverlay();
    return true;
}

bool RmlElementPicker::IsSelectableElement(
    Rml::Element* Element,
    Rml::ElementDocument* Document
)
{
    if (!Element || !Document || Element == Document)
        return false;

    if (Element->GetOwnerDocument() != Document)
        return false;

    const Rml::String& TagName = Element->GetTagName();

    if (TagName.empty() ||
        TagName == "#text" ||
        TagName == "head" ||
        TagName == "title" ||
        TagName == "link" ||
        TagName == "style" ||
        TagName == "script")
    {
        return false;
    }

    if (!Element->IsVisible(true))
        return false;

    return
        Element->GetOffsetWidth() > 0.0f &&
        Element->GetOffsetHeight() > 0.0f;
}