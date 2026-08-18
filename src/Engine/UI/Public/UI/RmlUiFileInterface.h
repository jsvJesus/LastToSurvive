#pragma once

#include "UI/UiDomain.h"

#include <RmlUi/Core/FileInterface.h>

#include <filesystem>

namespace engine::ui
{
    class RmlUiFileInterface final : public Rml::FileInterface
    {
    public:
        RmlUiFileInterface(std::filesystem::path gameRoot, UiDomain domain);

        Rml::FileHandle Open(const Rml::String& path) override;
        void Close(Rml::FileHandle file) override;
        std::size_t Read(void* buffer, std::size_t size, Rml::FileHandle file) override;
        bool Seek(Rml::FileHandle file, long offset, int origin) override;
        std::size_t Tell(Rml::FileHandle file) override;

        [[nodiscard]] const std::filesystem::path& GetGameRoot() const noexcept;
        [[nodiscard]] const std::filesystem::path& GetUiRoot() const noexcept;
        [[nodiscard]] const std::filesystem::path& GetAssetRoot() const noexcept;

    private:
        [[nodiscard]] std::filesystem::path Resolve(const Rml::String& path) const;
        [[nodiscard]] bool IsAllowed(const std::filesystem::path& path) const noexcept;

        std::filesystem::path gameRoot_;
        std::filesystem::path uiRoot_;
        std::filesystem::path assetRoot_;
    };
}
