#include "RmlEditorShellController.h"

#include "../App/RmlEditorLog.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/ID.h>

#include <utility>

void RmlEditorShellController::SetOpenCallback(
    CommandCallback Callback
)
{
    OpenCallback = std::move(Callback);
}

void RmlEditorShellController::SetReloadCallback(
    CommandCallback Callback
)
{
    ReloadCallback = std::move(Callback);
}

void RmlEditorShellController::Attach(
    Rml::ElementDocument* Document
)
{
    Detach();

    AttachedDocument = Document;

    if (!AttachedDocument)
        return;

    AttachCommand("menu_file", Command::Open);
    AttachCommand("toolbar_open", Command::Open);
    AttachCommand("preview_open_button", Command::Open);

    AttachCommand("menu_reload", Command::Reload);
    AttachCommand("toolbar_reload", Command::Reload);
}

void RmlEditorShellController::Detach()
{
    if (!AttachedDocument)
    {
        CommandsByElementId.clear();
        return;
    }

    for (const auto& Entry : CommandsByElementId)
    {
        if (Rml::Element* Element =
                AttachedDocument->GetElementById(Entry.first))
        {
            Element->RemoveEventListener(Rml::EventId::Click, this);
        }
    }

    CommandsByElementId.clear();
    AttachedDocument = nullptr;
}

void RmlEditorShellController::ProcessEvent(
    Rml::Event& Event
)
{
    if (Event.GetId() != Rml::EventId::Click)
        return;

    Rml::Element* Target = Event.GetTargetElement();

    if (!Target)
        return;

    const std::string ElementId = Target->GetId();
    const auto CommandIterator =
        CommandsByElementId.find(ElementId);

    if (CommandIterator == CommandsByElementId.end())
        return;

    Event.StopPropagation();

    switch (CommandIterator->second)
    {
    case Command::Open:
        if (OpenCallback)
            OpenCallback();
        break;

    case Command::Reload:
        if (ReloadCallback)
            ReloadCallback();
        break;
    }
}

void RmlEditorShellController::AttachCommand(
    const char* ElementId,
    Command ButtonCommand
)
{
    if (!AttachedDocument || !ElementId)
        return;

    Rml::Element* Element = AttachedDocument->GetElementById(ElementId);

    if (!Element)
    {
        RmlEditorLog::Write(
            "[RmlEditor] Shell command element missing: %s",
            ElementId
        );

        return;
    }

    Element->AddEventListener(Rml::EventId::Click, this);
    CommandsByElementId[ElementId] = ButtonCommand;
}
