#pragma once

#include <string>

namespace Rml
{
    class Element;
    class ElementDocument;
}

class RmlInspectorModel final
{
public:
    void Attach(Rml::ElementDocument* EditorDocument);
    void Detach();

    void ShowNoSelection();
    void ShowElement(Rml::Element* Element);

private:
    Rml::ElementDocument* ShellDocument = nullptr;

    void SetInspectorRml(const std::string& RmlText);
};