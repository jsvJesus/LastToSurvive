#pragma once

namespace lts::editor
{
    class CommandHistory;
    class LevelDocument;
    class SceneDocument;
    class TransformController;

    enum class EditorCommand
    {
        NewLevel,
        OpenLevel,
        SaveLevel,
        SaveLevelAs,
        Undo,
        Redo,
        DuplicateSelection,
        DeleteSelection,
        SelectTool,
        MoveTool,
        RotateTool,
        ScaleTool
    };

    class CommandSystem final
    {
    public:
        [[nodiscard]] bool CanExecute(
            EditorCommand command,
            const SceneDocument& scene,
            const CommandHistory& history) const noexcept;

        bool Execute(
            EditorCommand command,
            SceneDocument& scene,
            CommandHistory& history,
            LevelDocument& levelDocument,
            TransformController& transformController) const;
    };
}
