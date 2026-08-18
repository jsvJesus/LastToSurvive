#pragma once

#include "UI/RmlUiFileInterface.h"
#include "UI/RmlUiInputMapper.h"
#include "UI/RmlUiSystemInterface.h"

#include <RmlUi/Core/Types.h>

#include <filesystem>
#include <memory>

namespace Rml
{
    class Context;
    class ElementDocument;
    class RenderInterface;
}

namespace engine::ui
{
    class RmlUiHost final
    {
    public:
        RmlUiHost() = default;
        ~RmlUiHost() noexcept;

        RmlUiHost(const RmlUiHost&) = delete;
        RmlUiHost& operator=(const RmlUiHost&) = delete;

        [[nodiscard]] bool Initialize(
            UiDomain domain,
            const std::filesystem::path& gameRoot,
            Rml::RenderInterface& renderInterface,
            int width,
            int height);
        void Shutdown() noexcept;

        [[nodiscard]] Rml::ElementDocument* LoadDocument(const Rml::String& path);
        void Update();
        void Render();
        void Resize(int width, int height);

        [[nodiscard]] bool ProcessInput(const engine::platform::InputSystem& input);
        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] Rml::Context* GetContext() noexcept;
        [[nodiscard]] const std::filesystem::path& GetShaderRoot() const noexcept;

        [[nodiscard]] static std::filesystem::path DiscoverGameRoot();

    private:
        std::unique_ptr<RmlUiFileInterface> fileInterface_;
        std::unique_ptr<RmlUiSystemInterface> systemInterface_;
        RmlUiInputMapper inputMapper_;
        Rml::Context* context_ = nullptr;
        std::filesystem::path shaderRoot_;
        bool ownsRml_ = false;
    };
}
