#pragma once

#include "Assets/TextureAsset.h"

#include "Graphics/GraphicsBackend.h"
#include "Graphics/GraphicsResult.h"
#include "Graphics/RenderDevice.h"
#include "Graphics/ResourceHandle.h"
#include "Graphics/Texture.h"

namespace engine::assets
{
    enum class RequestedColorSpace : std::uint8_t { Preserve = 0, Linear, Srgb };
    struct GpuTextureUploadOptions final { RequestedColorSpace requestedColorSpace = RequestedColorSpace::Preserve; };
    // Backend-neutral GPU texture.
    //
    // Ресурс необходимо освободить через Release() до Shutdown()
    // соответствующего RenderDevice.
    class GpuTexture final
    {
    public:
        GpuTexture() noexcept = default;
        ~GpuTexture() noexcept = default;

        GpuTexture(const GpuTexture&) = delete;
        GpuTexture& operator=(const GpuTexture&) = delete;

        GpuTexture(GpuTexture&&) = delete;
        GpuTexture& operator=(GpuTexture&&) = delete;

        [[nodiscard]] engine::graphics::GraphicsResult Upload(
            engine::graphics::RenderDevice& device,
            const TextureAsset& textureAsset,
            GpuTextureUploadOptions options = {}) noexcept;

        // Создаёт новую GPU texture до уничтожения старой.
        // При ошибке создания старый ресурс остаётся действительным.
        [[nodiscard]] engine::graphics::GraphicsResult Replace(
            engine::graphics::RenderDevice& device,
            const TextureAsset& textureAsset,
            GpuTextureUploadOptions options = {}) noexcept;

        [[nodiscard]] engine::graphics::GraphicsResult Release(
            engine::graphics::RenderDevice& device) noexcept;

        // Использовать только тогда, когда RenderDevice уже уничтожил
        // собственный registry во время Shutdown().
        void Abandon() noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] engine::graphics::TextureHandle
            GetHandle() const noexcept;

        [[nodiscard]] engine::graphics::GraphicsBackend
            GetBackend() const noexcept;

        [[nodiscard]] const engine::graphics::TextureDesc&
            GetDesc() const noexcept;

    private:
        void ClearState() noexcept;

        engine::graphics::TextureHandle handle_;

        engine::graphics::TextureDesc desc_;

        engine::graphics::GraphicsBackend backend_ =
            engine::graphics::GraphicsBackend::None;
    };
}
