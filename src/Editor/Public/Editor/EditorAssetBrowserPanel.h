#pragma once

#include <Platform/Window.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace lts::editor
{
    class EditorAssetBrowserPanel final
    {
    public:
        EditorAssetBrowserPanel() noexcept = default;
        ~EditorAssetBrowserPanel() noexcept;

        EditorAssetBrowserPanel(
            const EditorAssetBrowserPanel&) = delete;

        EditorAssetBrowserPanel& operator=(
            const EditorAssetBrowserPanel&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::platform::NativeWindowHandle mainWindow) noexcept;

        void Shutdown() noexcept;

        void Update() noexcept;

        [[nodiscard]]
        bool ConsumeActivatedAsset(
            std::filesystem::path& assetPath);

    private:
        struct AssetEntry final
        {
            std::filesystem::path sourcePath;
            std::wstring displayName;
        };

        [[nodiscard]]
        bool CreateControls() noexcept;

        [[nodiscard]]
        bool InstallWindowSubclass() noexcept;

        void RestoreWindowSubclass() noexcept;

        void DestroyControls() noexcept;

        void UpdateLayout() noexcept;

        void ScanAssets() noexcept;

        void RebuildVisibleList() noexcept;

        void QueueSelectedAsset() noexcept;

        void ProcessRequestedAsset() noexcept;

        [[nodiscard]]
        bool PrepareRuntimeAsset(
            const AssetEntry& entry,
            std::filesystem::path& runtimePath,
            std::wstring& error) const;

        [[nodiscard]]
        static LRESULT CALLBACK WindowProcedure(
            HWND window,
            UINT message,
            WPARAM wParam,
            LPARAM lParam) noexcept;

        void* mainWindow_ = nullptr;
        void* anchorWindow_ = nullptr;

        void* panelWindow_ = nullptr;
        void* filterEdit_ = nullptr;
        void* refreshButton_ = nullptr;
        void* assetList_ = nullptr;
        void* font_ = nullptr;

        void* previousWindowProcedure_ = nullptr;

        std::filesystem::path dataRoot_;

        std::vector<AssetEntry> assets_;

        std::filesystem::path pendingAssetPath_;

        std::size_t requestedAssetIndex_ =
            static_cast<std::size_t>(-1);

        bool scanRequested_ = false;
        bool activationRequested_ = false;
        bool pendingAssetReady_ = false;

        bool subclassInstalled_ = false;
        bool initialized_ = false;
    };
}