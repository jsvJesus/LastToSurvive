#include "Editor/LevelEditorUiController.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

#include <array>
#include <utility>

namespace lts::editor
{
    LevelEditorUiController::Listener::Listener(
        const LevelEditorUiAction action,
        Callback& callback) noexcept
        : action_(action), callback_(callback)
    {
    }

    void LevelEditorUiController::Listener::ProcessEvent(Rml::Event& event)
    {
        event.StopPropagation();
        if (callback_)
        {
            callback_(action_);
        }
    }

    bool LevelEditorUiController::Attach(
        Rml::ElementDocument& document,
        Callback callback)
    {
        Detach();
        callback_ = std::move(callback);

        constexpr std::array mappings{
            std::pair{"back-to-editor-menu", LevelEditorUiAction::Back},
            std::pair{"new-level", LevelEditorUiAction::NewLevel},
            std::pair{"open-level", LevelEditorUiAction::OpenLevel},
            std::pair{"save-level", LevelEditorUiAction::SaveLevel},
            std::pair{"undo", LevelEditorUiAction::Undo},
            std::pair{"redo", LevelEditorUiAction::Redo},
            std::pair{"select-tool", LevelEditorUiAction::Select},
            std::pair{"move-tool", LevelEditorUiAction::Move},
            std::pair{"rotate-tool", LevelEditorUiAction::Rotate},
            std::pair{"scale-tool", LevelEditorUiAction::Scale},
            std::pair{"coordinate-space", LevelEditorUiAction::ToggleSpace},
            std::pair{"play-level", LevelEditorUiAction::Play},
            std::pair{"file-menu", LevelEditorUiAction::FileMenu},
            std::pair{"edit-menu", LevelEditorUiAction::EditMenu},
            std::pair{"window-menu", LevelEditorUiAction::WindowMenu},
            std::pair{"tools-menu", LevelEditorUiAction::ToolsMenu},
            std::pair{"build-menu", LevelEditorUiAction::BuildMenu},
            std::pair{"help-menu", LevelEditorUiAction::HelpMenu},
            std::pair{"snap-settings", LevelEditorUiAction::Snap},
            std::pair{"play-options", LevelEditorUiAction::PlayOptions}
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

    void LevelEditorUiController::Detach() noexcept
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
