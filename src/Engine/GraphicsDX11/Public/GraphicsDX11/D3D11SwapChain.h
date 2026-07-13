#pragma once

#include "Graphics/SwapChain.h"

#include <memory>

struct ID3D11Device;
struct ID3D11Texture2D;
struct ID3D11RenderTargetView;
struct IDXGIFactory2;
struct IDXGISwapChain1;

namespace engine::graphics::d3d11
{
    class D3D11Device;

    class D3D11SwapChain final : public SwapChain
    {
    public:
        ~D3D11SwapChain() noexcept override;

        D3D11SwapChain(const D3D11SwapChain&) = delete;
        D3D11SwapChain& operator=(const D3D11SwapChain&) = delete;

        [[nodiscard]] SwapChainHandle GetHandle() const noexcept override;
        [[nodiscard]] const SwapChainDesc& GetDesc() const noexcept override;

        [[nodiscard]] GraphicsResult Resize(
            std::uint32_t width,
            std::uint32_t height) noexcept override;

        [[nodiscard]] GraphicsResult Present(
            PresentStatus& outStatus) noexcept override;

        [[nodiscard]] IDXGISwapChain1* GetNativeSwapChain() const noexcept;
        [[nodiscard]] ID3D11Texture2D* GetBackBufferTexture() const noexcept;
        [[nodiscard]] ID3D11RenderTargetView*
            GetBackBufferRenderTargetView() const noexcept;

    private:
        friend class D3D11Device;

        D3D11SwapChain();

        [[nodiscard]] static GraphicsResult Create(
            ID3D11Device* device,
            IDXGIFactory2* factory,
            bool tearingSupported,
            const SwapChainDesc& desc,
            std::unique_ptr<SwapChain>& outSwapChain) noexcept;

        [[nodiscard]] GraphicsResult RecreateBackBuffer() noexcept;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}
