#include "UI/RmlUiFileInterface.h"

#include <cstdio>
#include <system_error>

namespace engine::ui
{
    namespace
    {
        [[nodiscard]] bool IsWithin(const std::filesystem::path& child,
                                    const std::filesystem::path& parent) noexcept
        {
            auto childPart = child.begin();
            auto parentPart = parent.begin();
            for (; parentPart != parent.end(); ++parentPart, ++childPart)
            {
                if (childPart == child.end() || *childPart != *parentPart)
                {
                    return false;
                }
            }
            return true;
        }
    }

    RmlUiFileInterface::RmlUiFileInterface(std::filesystem::path gameRoot,
                                           const UiDomain domain)
        : gameRoot_(std::filesystem::weakly_canonical(std::move(gameRoot))),
          uiRoot_(gameRoot_ / "Data" / "UI" /
                  (domain == UiDomain::Editor ? "Editor" : "Game")),
          assetRoot_(gameRoot_ / "Data" / "UI" / "Assets")
    {
        uiRoot_ = std::filesystem::weakly_canonical(uiRoot_);
        assetRoot_ = std::filesystem::weakly_canonical(assetRoot_);
    }

    Rml::FileHandle RmlUiFileInterface::Open(const Rml::String& path)
    {
        const std::filesystem::path resolved = Resolve(path);
        if (resolved.empty())
        {
            return Rml::FileHandle{};
        }
        std::FILE* file = nullptr;
        if (_wfopen_s(&file, resolved.c_str(), L"rb") != 0)
        {
            return Rml::FileHandle{};
        }
        return reinterpret_cast<Rml::FileHandle>(file);
    }

    void RmlUiFileInterface::Close(const Rml::FileHandle file)
    {
        if (file != Rml::FileHandle{})
        {
            std::fclose(reinterpret_cast<std::FILE*>(file));
        }
    }

    std::size_t RmlUiFileInterface::Read(void* buffer, const std::size_t size,
                                         const Rml::FileHandle file)
    {
        return file != Rml::FileHandle{}
            ? std::fread(buffer, 1, size, reinterpret_cast<std::FILE*>(file))
            : 0;
    }

    bool RmlUiFileInterface::Seek(const Rml::FileHandle file, const long offset,
                                  const int origin)
    {
        return file != Rml::FileHandle{} &&
               std::fseek(reinterpret_cast<std::FILE*>(file), offset, origin) == 0;
    }

    std::size_t RmlUiFileInterface::Tell(const Rml::FileHandle file)
    {
        if (file == Rml::FileHandle{})
        {
            return 0;
        }
        const long position = std::ftell(reinterpret_cast<std::FILE*>(file));
        return position >= 0 ? static_cast<std::size_t>(position) : 0;
    }

    const std::filesystem::path& RmlUiFileInterface::GetGameRoot() const noexcept { return gameRoot_; }
    const std::filesystem::path& RmlUiFileInterface::GetUiRoot() const noexcept { return uiRoot_; }
    const std::filesystem::path& RmlUiFileInterface::GetAssetRoot() const noexcept { return assetRoot_; }

    std::filesystem::path RmlUiFileInterface::Resolve(const Rml::String& path) const
    {
        std::filesystem::path requested = std::filesystem::u8path(path);
        if (requested.is_absolute())
        {
            return {};
        }
        const std::string generic = requested.generic_string();
        std::filesystem::path base = uiRoot_;
        if (generic.rfind("Assets/", 0) == 0)
        {
            base = assetRoot_;
            requested = std::filesystem::u8path(generic.substr(7));
        }
        std::error_code error;
        const auto resolved = std::filesystem::weakly_canonical(base / requested, error);
        return !error && IsAllowed(resolved) ? resolved : std::filesystem::path{};
    }

    bool RmlUiFileInterface::IsAllowed(const std::filesystem::path& path) const noexcept
    {
        return IsWithin(path, uiRoot_) || IsWithin(path, assetRoot_);
    }
}
