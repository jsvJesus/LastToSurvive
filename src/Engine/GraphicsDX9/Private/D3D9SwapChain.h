#pragma once

#include "D3D9ComPtr.h"

#include "Graphics/SwapChain.h"

#include <memory>

struct IDirect3DDevice9;
struct IDirect3DSwapChain9;

namespace engine::graphics::d3d9::detail
{
    class D3D9SwapChain final : public SwapChain
    {
    public:
        [[nodiscard]] static GraphicsResult Create(
            IDirect3DDevice9* device,
            const SwapChainDesc& desc,
            std::unique_ptr<SwapChain>& outSwapChain) noexcept;

        ~D3D9SwapChain() noexcept override = default;

        [[nodiscard]] GraphicsBackend GetBackend() const noexcept override;
        [[nodiscard]] SwapChainHandle GetHandle() const noexcept override;
        [[nodiscard]] const SwapChainDesc& GetDesc() const noexcept override;

        [[nodiscard]] GraphicsResult Resize(
            std::uint32_t width,
            std::uint32_t height) noexcept override;

        [[nodiscard]] GraphicsResult Present(
            PresentStatus& outStatus) noexcept override;

    private:
        D3D9SwapChain(
            IDirect3DDevice9* device,
            SwapChainDesc desc,
            SwapChainHandle handle) noexcept;

        [[nodiscard]] GraphicsResult RefreshNativeSwapChain() noexcept;

        IDirect3DDevice9* device_ = nullptr;
        SwapChainDesc desc_;
        SwapChainHandle handle_;
        ComPtr<IDirect3DSwapChain9> native_;
    };
}
