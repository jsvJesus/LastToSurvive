#include "UI/RmlUiHost.h"

#include <RmlUi/Core.h>

#include <Windows.h>

#include <array>
#include <system_error>

namespace engine::ui
{
    RmlUiHost::~RmlUiHost() noexcept { Shutdown(); }

    bool RmlUiHost::Initialize(
        const UiDomain domain,
        const std::filesystem::path& gameRoot,
        Rml::RenderInterface& renderInterface,
        const int width,
        const int height)
    {
        if (context_ != nullptr || width <= 0 || height <= 0) return false;
        fileInterface_ = std::make_unique<RmlUiFileInterface>(gameRoot, domain);
        systemInterface_ = std::make_unique<RmlUiSystemInterface>();
        shaderRoot_ = fileInterface_->GetGameRoot() / "Data" / "Shaders" /
            (domain == UiDomain::Editor ? "Editor" : "Game");

        Rml::SetFileInterface(fileInterface_.get());
        Rml::SetSystemInterface(systemInterface_.get());
        Rml::SetRenderInterface(&renderInterface);
        if (!Rml::Initialise())
        {
            Rml::SetRenderInterface(nullptr);
            Rml::SetSystemInterface(nullptr);
            Rml::SetFileInterface(nullptr);
            systemInterface_.reset();
            fileInterface_.reset();
            return false;
        }
        ownsRml_ = true;
        context_ = Rml::CreateContext("LTS", Rml::Vector2i(width, height));
        if (context_ == nullptr)
        {
            Shutdown();
            return false;
        }

        if (!Rml::LoadFontFace("Assets/Fonts/Roboto-Regular.ttf"))
        {
            Shutdown();
            return false;
        }
        return true;
    }

    void RmlUiHost::Shutdown() noexcept
    {
        if (context_ != nullptr)
        {
            Rml::RemoveContext(context_->GetName());
            context_ = nullptr;
        }
        if (ownsRml_)
        {
            Rml::Shutdown();
            ownsRml_ = false;
        }
        Rml::SetRenderInterface(nullptr);
        Rml::SetSystemInterface(nullptr);
        Rml::SetFileInterface(nullptr);
        shaderRoot_.clear();
        systemInterface_.reset();
        fileInterface_.reset();
    }

    Rml::ElementDocument* RmlUiHost::LoadDocument(const Rml::String& path)
    {
        if (context_ == nullptr) return nullptr;
        auto* document = context_->LoadDocument(path);
        if (document != nullptr) document->Show();
        return document;
    }

    void RmlUiHost::Update() { if (context_ != nullptr) context_->Update(); }
    void RmlUiHost::Render() { if (context_ != nullptr) context_->Render(); }

    void RmlUiHost::Resize(const int width, const int height)
    {
        if (context_ != nullptr && width > 0 && height > 0)
            context_->SetDimensions(Rml::Vector2i(width, height));
    }

    bool RmlUiHost::ProcessInput(const engine::platform::InputSystem& input)
    {
        return context_ != nullptr && inputMapper_.ProcessEvents(*context_, input);
    }

    bool RmlUiHost::IsInitialized() const noexcept { return context_ != nullptr; }
    Rml::Context* RmlUiHost::GetContext() noexcept { return context_; }
    const std::filesystem::path& RmlUiHost::GetShaderRoot() const noexcept { return shaderRoot_; }

    std::filesystem::path RmlUiHost::DiscoverGameRoot()
    {
        std::array<wchar_t, 32768> buffer{};
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                                static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size()) return {};
        std::filesystem::path candidate = std::filesystem::path(buffer.data()).parent_path();
        std::error_code error;
        for (int depth = 0; depth < 6 && !candidate.empty(); ++depth)
        {
            if (std::filesystem::is_directory(candidate / "Data", error))
                return std::filesystem::weakly_canonical(candidate, error);
            if (std::filesystem::is_directory(candidate / "game" / "Data", error))
                return std::filesystem::weakly_canonical(candidate / "game", error);
            candidate = candidate.parent_path();
        }
        return {};
    }
}
