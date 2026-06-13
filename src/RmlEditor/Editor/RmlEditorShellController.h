#pragma once

#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>

#include <functional>
#include <string>
#include <unordered_map>

class RmlEditorShellController final : public Rml::EventListener
{
public:
    using CommandCallback = std::function<void()>;

    void SetOpenCallback(CommandCallback Callback);
    void SetReloadCallback(CommandCallback Callback);

    void Attach(Rml::ElementDocument* Document);
    void Detach();

    void ProcessEvent(Rml::Event& Event) override;

private:
    enum class Command
    {
        Open,
        Reload
    };

    Rml::ElementDocument* AttachedDocument = nullptr;
    std::unordered_map<std::string, Command> CommandsByElementId;

    CommandCallback OpenCallback;
    CommandCallback ReloadCallback;

    void AttachCommand(
        const char* ElementId,
        Command ButtonCommand
    );
};
