#pragma once

#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace engine::ui
{
    class ImGuiHost final
    {
    public:
        ImGuiHost() noexcept = default;
        ~ImGuiHost() noexcept;

        ImGuiHost(const ImGuiHost&) = delete;
        ImGuiHost& operator=(const ImGuiHost&) = delete;

        [[nodiscard]] bool Initialize(
            void* nativeWindow,
            ID3D11Device* device,
            ID3D11DeviceContext* context) noexcept;

        void Shutdown() noexcept;
        void BeginFrame() noexcept;
        void Render() noexcept;

        [[nodiscard]] bool ProcessNativeMessage(
            void* nativeWindow,
            std::uint32_t message,
            std::uintptr_t wordParameter,
            std::intptr_t longParameter) noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept;

    private:
        bool initialized_ = false;
    };
}
