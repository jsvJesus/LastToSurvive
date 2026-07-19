#pragma once

namespace lts::editor
{
    class EditorCommandHistory;
    class EditorLevelDocument;
    class EditorSceneDocument;
    class EditorTransformController;

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

    class EditorCommandSystem final
    {
    public:
        [[nodiscard]] bool CanExecute(
            EditorCommand command,
            const EditorSceneDocument& scene,
            const EditorCommandHistory& history) const noexcept;

        bool Execute(
            EditorCommand command,
            EditorSceneDocument& scene,
            EditorCommandHistory& history,
            EditorLevelDocument& levelDocument,
            EditorTransformController& transformController) const;
    };
}
