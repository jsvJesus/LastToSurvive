#pragma once

#include <RmlUi/Core/EventListener.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Rml
{
    class Element;
    class ElementDocument;
}

namespace lts::editor
{
    enum class EditorLauncherAction
    {
        TestGame,
        LevelEditor,
        CharacterEditor,
        PhysicsEditor,
        FbxImporter,
        IconGenerator,
        Exit
    };

    class EditorLauncherController final
    {
    public:
        using Callback = std::function<void(EditorLauncherAction)>;

        [[nodiscard]] bool Attach(
            Rml::ElementDocument& document,
            Callback callback);
        void Detach() noexcept;

    private:
        class Listener final : public Rml::EventListener
        {
        public:
            Listener(EditorLauncherAction action, Callback& callback) noexcept;
            void ProcessEvent(Rml::Event& event) override;

        private:
            EditorLauncherAction action_;
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
