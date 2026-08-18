#include "Editor/Commands/CommandSystem.h"

#include "Editor/Commands/CommandHistory.h"
#include "Editor/LevelEditor/Documents/LevelDocument.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"
#include "Editor/LevelEditor/Viewport/TransformController.h"

namespace lts::editor
{
    bool CommandSystem::CanExecute(
        const EditorCommand command,
        const SceneDocument& scene,
        const CommandHistory& history) const noexcept
    {
        switch (command)
        {
            case EditorCommand::Undo: return history.CanUndo();
            case EditorCommand::Redo: return history.CanRedo();
            case EditorCommand::DuplicateSelection:
            case EditorCommand::DeleteSelection:
                return scene.GetSelectedEntity() != nullptr;
            default:
                return true;
        }
    }

    bool CommandSystem::Execute(
        const EditorCommand command,
        SceneDocument& scene,
        CommandHistory& history,
        LevelDocument& levelDocument,
        TransformController& transformController) const
    {
        if (!CanExecute(command, scene, history)) return false;

        switch (command)
        {
            case EditorCommand::NewLevel:
                levelDocument.RequestNewLevel();
                return true;
            case EditorCommand::OpenLevel:
                levelDocument.RequestOpenLevel();
                return true;
            case EditorCommand::SaveLevel:
                levelDocument.RequestSaveLevel();
                return true;
            case EditorCommand::SaveLevelAs:
                levelDocument.RequestSaveLevelAs();
                return true;
            case EditorCommand::Undo:
                return history.Undo(scene);
            case EditorCommand::Redo:
                return history.Redo(scene);
            case EditorCommand::DuplicateSelection:
            {
                const EditorSceneSnapshot before = scene.CreateSnapshot();
                if (!scene.DuplicateSelectedEntity()) return false;
                return history.Push(before, scene.CreateSnapshot());
            }
            case EditorCommand::DeleteSelection:
            {
                const EditorSceneSnapshot before = scene.CreateSnapshot();
                if (!scene.DeleteSelectedEntity()) return false;
                return history.Push(before, scene.CreateSnapshot());
            }
            case EditorCommand::SelectTool:
                transformController.SetOperation(EditorTransformOperation::Select);
                return true;
            case EditorCommand::MoveTool:
                transformController.SetOperation(EditorTransformOperation::Move);
                return true;
            case EditorCommand::RotateTool:
                transformController.SetOperation(EditorTransformOperation::Rotate);
                return true;
            case EditorCommand::ScaleTool:
                transformController.SetOperation(EditorTransformOperation::Scale);
                return true;
        }
        return false;
    }
}
