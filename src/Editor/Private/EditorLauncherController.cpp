#include "Editor/EditorLauncherController.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include <array>

namespace lts::editor
{
    EditorLauncherController::Listener::Listener(
        const EditorLauncherAction action,
        Callback& callback) noexcept
        : action_(action), callback_(callback)
    {
    }

    void EditorLauncherController::Listener::ProcessEvent(Rml::Event& event)
    {
        event.StopPropagation();
        if (callback_)
        {
            callback_(action_);
        }
    }

    bool EditorLauncherController::Attach(
        Rml::ElementDocument& document,
        Callback callback)
    {
        Detach();
        callback_ = std::move(callback);

        constexpr std::array mappings{
            std::pair{"launch-test-game", EditorLauncherAction::TestGame},
            std::pair{"open-level-editor", EditorLauncherAction::LevelEditor},
            std::pair{"open-character-editor", EditorLauncherAction::CharacterEditor},
            std::pair{"open-physics-editor", EditorLauncherAction::PhysicsEditor},
            std::pair{"open-fbx-importer", EditorLauncherAction::FbxImporter},
            std::pair{"open-icon-generator", EditorLauncherAction::IconGenerator},
            std::pair{"exit-editor", EditorLauncherAction::Exit}
        };

        for (const auto& mapping : mappings)
        {
            auto* element = document.GetElementById(mapping.first);
            if (element == nullptr)
            {
                Detach();
                return false;
            }

            Binding binding;
            binding.element = element;
            binding.listener = std::make_unique<Listener>(mapping.second, callback_);
            element->AddEventListener("click", binding.listener.get());
            bindings_.push_back(std::move(binding));
        }
        return true;
    }

    void EditorLauncherController::Detach() noexcept
    {
        for (auto& binding : bindings_)
        {
            if (binding.element != nullptr && binding.listener != nullptr)
            {
                binding.element->RemoveEventListener("click", binding.listener.get());
            }
        }
        bindings_.clear();
        callback_ = {};
    }
}
