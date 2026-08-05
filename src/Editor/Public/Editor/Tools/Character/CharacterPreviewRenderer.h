#pragma once

#include <Graphics/GraphicsResult.h>

#include <cstdint>
#include <memory>
#include <string>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    struct CharacterDefinition;

    class CharacterPreviewRenderer final
    {
    public:
        CharacterPreviewRenderer() noexcept;
        ~CharacterPreviewRenderer() noexcept;

        CharacterPreviewRenderer(
            const CharacterPreviewRenderer&) = delete;

        CharacterPreviewRenderer& operator=(
            const CharacterPreviewRenderer&) = delete;

        [[nodiscard]]
        bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]]
        bool LoadCharacter(
            engine::graphics::RenderDevice& device,
            const CharacterDefinition& character) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Resize(
            engine::graphics::RenderDevice& device,
            std::uint32_t width,
            std::uint32_t height) noexcept;

        [[nodiscard]]
        engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            float yawDegrees,
            float pitchDegrees,
            float distanceMultiplier) noexcept;

        [[nodiscard]]
        void* GetImGuiTextureId(
            const engine::graphics::RenderDevice& device) const noexcept;

        [[nodiscard]]
        bool IsInitialized() const noexcept;

        [[nodiscard]]
        bool HasCharacter() const noexcept;

        [[nodiscard]]
        const std::string& GetStatus() const noexcept;

    private:
        class Impl;

        std::unique_ptr<Impl> impl_;
    };
}