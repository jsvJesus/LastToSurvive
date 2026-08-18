#pragma once

#include <Graphics/GraphicsResult.h>
#include <Graphics/ResourceHandle.h>

#include <cstdint>
#include <memory>

namespace engine::graphics
{
    class CommandContext;
    class RenderDevice;
}

namespace lts::editor
{
    struct ColorCorrectionSettings final
    {
        bool enabled = true;
        float exposure = 0.15F;
        float contrast = 1.12F;
        float saturation = 1.14F;
        float gamma = 1.0F;
        float vibrance = 0.22F;
        float temperature = 0.05F;
        float tint = 0.02F;
        float filmicStrength = 0.35F;
        float lift = 0.0F;
        float gain = 1.0F;
        float sharpen = 0.25F;
        float vignette = 0.12F;
        float vignetteSoftness = 0.55F;
        float bloomStrength = 0.08F;
        float bloomThreshold = 0.72F;
        float bloomRadius = 2.0F;
        float colorFilter[3]{1.0F, 1.0F, 1.0F};
    };

    class ColorCorrectionRenderer final
    {
    public:
        ColorCorrectionRenderer() noexcept;
        ~ColorCorrectionRenderer() noexcept;

        ColorCorrectionRenderer(const ColorCorrectionRenderer&) = delete;
        ColorCorrectionRenderer& operator=(const ColorCorrectionRenderer&) = delete;

        [[nodiscard]] bool Initialize(
            engine::graphics::RenderDevice& device) noexcept;

        void Shutdown(
            engine::graphics::RenderDevice& device) noexcept;

        [[nodiscard]] engine::graphics::GraphicsResult Render(
            engine::graphics::CommandContext& context,
            engine::graphics::TextureHandle source,
            std::uint32_t width,
            std::uint32_t height,
            const ColorCorrectionSettings& settings) noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
