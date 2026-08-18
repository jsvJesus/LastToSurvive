#pragma once

#include "Editor/Commands/CommandHistory.h"
#include "Editor/LevelEditor/Scene/SceneDocument.h"

#include <Platform/Window.h>

#include <Windows.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace lts::editor
{
    struct EditorLevelUpdateResult final
    {
        bool sceneReplaced = false;
        bool documentSaved = false;
        bool closeApproved = false;
    };

    class LevelDocument final
    {
    public:
        LevelDocument() noexcept = default;
        ~LevelDocument() noexcept;

        LevelDocument(
            const LevelDocument&) = delete;

        LevelDocument& operator=(
            const LevelDocument&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::platform::NativeWindowHandle mainWindow,
            const SceneDocument& sceneDocument) noexcept;

        void Shutdown() noexcept;

        [[nodiscard]]
        EditorLevelUpdateResult Update(
            SceneDocument& sceneDocument,
            CommandHistory& commandHistory) noexcept;

        void SynchronizeWindowTitle(
            const SceneDocument& sceneDocument) noexcept;

        [[nodiscard]]
            const std::filesystem::path&
                GetCurrentPath() const noexcept;

        [[nodiscard]] bool SetWindowInterceptionEnabled(bool enabled) noexcept;

        void RequestNewLevel() noexcept;
        void RequestOpenLevel() noexcept;
        void RequestSaveLevel() noexcept;
        void RequestSaveLevelAs() noexcept;
        void RequestCloseLevel() noexcept;

    private:
        enum class PendingCommand : std::uint8_t
        {
            None = 0,
            NewLevel,
            OpenLevel,
            Save,
            SaveAs,
            Exit
        };

        enum class OperationResult : std::uint8_t
        {
            Success = 0,
            Cancelled,
            Failed
        };

        [[nodiscard]]
        bool InstallWindowSubclass() noexcept;

        void RestoreWindowSubclass() noexcept;

        void QueueCommand(
            PendingCommand command) noexcept;

        void PollShortcuts() noexcept;

        [[nodiscard]]
        bool WasPressed(
            int virtualKey) noexcept;

        [[nodiscard]]
        OperationResult ConfirmSaveChanges(
            SceneDocument& sceneDocument) noexcept;

        [[nodiscard]]
        OperationResult CreateNewLevel(
            SceneDocument& sceneDocument,
            CommandHistory& commandHistory) noexcept;

        [[nodiscard]]
        OperationResult OpenLevel(
            SceneDocument& sceneDocument,
            CommandHistory& commandHistory) noexcept;

        [[nodiscard]]
        OperationResult SaveLevel(
            SceneDocument& sceneDocument) noexcept;

        [[nodiscard]]
        OperationResult SaveLevelAs(
            SceneDocument& sceneDocument) noexcept;

        [[nodiscard]]
        bool ShowFileDialog(
            bool saveDialog,
            std::filesystem::path& path,
            bool& cancelled,
            std::wstring& error) noexcept;

        void ResetUntitledMetadata() noexcept;
        void UpdateNameFromPath() noexcept;

        void ShowError(
            std::wstring_view title,
            std::wstring_view message) const noexcept;

        [[nodiscard]]
        static LRESULT CALLBACK WindowProcedure(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam) noexcept;

        void* mainWindow_ = nullptr;
        void* previousWindowProcedure_ = nullptr;

        std::filesystem::path currentPath_;

        std::wstring levelName_ =
            L"Untitled";

        std::wstring levelGuid_;
        std::wstring lastWindowTitle_;

        std::array<bool, 256U>
            previousKeyDown_{};

        PendingCommand pendingCommand_ =
            PendingCommand::None;

        bool allowClose_ = false;
        bool subclassInstalled_ = false;
        bool comNeedsUninitialize_ = false;
        bool initialized_ = false;
    };
}
