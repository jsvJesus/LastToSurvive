#pragma once

#include <RmlUi/Core/ObserverPtr.h>
#include <RmlUi/Core/Types.h>

#include <cstdint>
#include <string>

namespace Rml
{
    class Context;
    class Element;
    class ElementDocument;
}

class RmlElementPicker final
{
public:
    bool Initialize(Rml::Context* Context);
    void Shutdown();

    void SetDocument(Rml::ElementDocument* Document);

    void SetEnabled(bool Enabled);
    bool IsEnabled() const;

    bool UpdateHover(Rml::Vector2i Point);
    bool SelectAt(Rml::Vector2i Point);

    void ClearHover();
    void ClearSelection();

    bool RestoreSelectionById(const std::string& ElementId);
    void Update();

    Rml::Element* GetHoveredElement() const;
    Rml::Element* GetSelectedElement() const;

    const std::string& GetSelectedElementId() const;
    const std::string& GetSelectedElementAddress() const;

    std::uint64_t GetSelectionRevision() const;

private:
    Rml::Context* PickerContext = nullptr;
    Rml::ElementDocument* ActiveDocument = nullptr;
    Rml::ElementDocument* OverlayDocument = nullptr;

    Rml::ObserverPtr<Rml::Element> HoveredElement;
    Rml::ObserverPtr<Rml::Element> SelectedElement;

    std::string SelectedElementId;
    std::string SelectedElementAddress;

    std::uint64_t SelectionRevision = 0;

    bool Enabled = false;
    bool HadHover = false;
    bool HadSelection = false;

    bool CreateOverlayDocument();
    void DestroyOverlayDocument();
    void UpdateOverlay();

    void UpdateOverlayElement(
        const char* OverlayElementId,
        Rml::Element* TargetElement
    );

    Rml::Element* FindSelectableElement(Rml::Vector2i Point) const;
    bool AssignHover(Rml::Element* Element);
    bool AssignSelection(Rml::Element* Element);

    static bool IsSelectableElement(
        Rml::Element* Element,
        Rml::ElementDocument* Document
    );
};