#pragma once

#include <RmlUi/Core/EventListener.h>

#include <functional>
#include <memory>
#include <vector>

namespace Rml
{
    class Element;
    class ElementDocument;
}

namespace lts::editor
{
    enum class LevelEditorUiAction
    {
        Back,
        NewLevel,
        OpenLevel,
        SaveLevel,
        Undo,
        Redo,
        Select,
        Move,
        Rotate,
        Scale,
        ToggleSpace,
        Play,
        FileMenu,
        EditMenu,
        WindowMenu,
        ToolsMenu,
        BuildMenu,
        HelpMenu,
        Snap,
        PlayOptions
    };

    class LevelEditorUiController final
    {
    public:
        using Callback = std::function<void(LevelEditorUiAction)>;

        [[nodiscard]] bool Attach(Rml::ElementDocument& document, Callback callback);
        void Detach() noexcept;

    private:
        class Listener final : public Rml::EventListener
        {
        public:
            Listener(LevelEditorUiAction action, Callback& callback) noexcept;
            void ProcessEvent(Rml::Event& event) override;

        private:
            LevelEditorUiAction action_;
            Callback& callback_;
        };

        struct Binding final
        {
            Rml::Element* element = nullptr;
            std::unique_ptr<Listener> listener;
        };

        Callback callback_;
        std::vector<Binding> bindings_;
    };
}
