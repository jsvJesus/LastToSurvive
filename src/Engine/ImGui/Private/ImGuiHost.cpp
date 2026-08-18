#include "ImGui/ImGuiHost.h"

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#include <Windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND window,
    UINT message,
    WPARAM wordParameter,
    LPARAM longParameter);

namespace engine::ui
{
    ImGuiHost::~ImGuiHost() noexcept
    {
        Shutdown();
    }

    bool ImGuiHost::Initialize(
        void* const nativeWindow,
        ID3D11Device* const device,
        ID3D11DeviceContext* const context,
        const char* const iniFilename) noexcept
    {
        if (initialized_)
        {
            return true;
        }
        if (nativeWindow == nullptr || device == nullptr || context == nullptr)
        {
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        iniFilename_ = iniFilename != nullptr ? iniFilename : "imgui.ini";
        io.IniFilename = iniFilename_.c_str();
        static_cast<void>(io.Fonts->AddFontFromFileTTF(
            "Data/UI/Assets/Fonts/Roboto-Regular.ttf",
            15.0F));

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0F;
        style.ChildRounding = 0.0F;
        style.FrameRounding = 2.0F;
        style.PopupRounding = 2.0F;
        style.TabRounding = 0.0F;
        style.WindowPadding = ImVec2(8.0F, 7.0F);
        style.FramePadding = ImVec2(8.0F, 5.0F);
        style.ItemSpacing = ImVec2(7.0F, 5.0F);
        style.ScrollbarSize = 13.0F;
        style.WindowBorderSize = 1.0F;
        style.TabBorderSize = 0.0F;

        ImVec4* const colors = style.Colors;
        colors[ImGuiCol_Text] = ImVec4(0.82F, 0.84F, 0.87F, 1.0F);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.45F, 0.48F, 0.52F, 1.0F);
        colors[ImGuiCol_WindowBg] = ImVec4(0.055F, 0.063F, 0.075F, 1.0F);
        colors[ImGuiCol_ChildBg] = ImVec4(0.055F, 0.063F, 0.075F, 1.0F);
        colors[ImGuiCol_PopupBg] = ImVec4(0.075F, 0.085F, 0.10F, 1.0F);
        colors[ImGuiCol_Border] = ImVec4(0.18F, 0.20F, 0.23F, 1.0F);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10F, 0.115F, 0.135F, 1.0F);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16F, 0.18F, 0.21F, 1.0F);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.13F, 0.32F, 0.39F, 1.0F);
        colors[ImGuiCol_TitleBg] = ImVec4(0.075F, 0.085F, 0.10F, 1.0F);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.09F, 0.105F, 0.12F, 1.0F);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.065F, 0.075F, 0.09F, 1.0F);
        colors[ImGuiCol_Button] = ImVec4(0.105F, 0.12F, 0.14F, 1.0F);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.16F, 0.19F, 0.22F, 1.0F);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.12F, 0.34F, 0.42F, 1.0F);
        colors[ImGuiCol_Header] = ImVec4(0.11F, 0.28F, 0.34F, 1.0F);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.15F, 0.36F, 0.43F, 1.0F);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.18F, 0.42F, 0.50F, 1.0F);
        colors[ImGuiCol_Tab] = ImVec4(0.075F, 0.085F, 0.10F, 1.0F);
        colors[ImGuiCol_TabHovered] = ImVec4(0.14F, 0.32F, 0.38F, 1.0F);
        colors[ImGuiCol_TabSelected] = ImVec4(0.105F, 0.20F, 0.24F, 1.0F);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.12F, 0.55F, 0.68F, 0.55F);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.025F, 0.030F, 0.036F, 1.0F);

        if (!ImGui_ImplWin32_Init(nativeWindow))
        {
            ImGui::DestroyContext();
            return false;
        }
        if (!ImGui_ImplDX11_Init(device, context))
        {
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        initialized_ = true;
        return true;
    }

    void ImGuiHost::Shutdown() noexcept
    {
        if (!initialized_)
        {
            return;
        }
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        iniFilename_.clear();
        initialized_ = false;
    }

    void ImGuiHost::BeginFrame() noexcept
    {
        if (!initialized_)
        {
            return;
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiHost::Render() noexcept
    {
        if (!initialized_)
        {
            return;
        }
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    bool ImGuiHost::ProcessNativeMessage(
        void* const nativeWindow,
        const std::uint32_t message,
        const std::uintptr_t wordParameter,
        const std::intptr_t longParameter) noexcept
    {
        return initialized_ && ImGui_ImplWin32_WndProcHandler(
            static_cast<HWND>(nativeWindow),
            message,
            static_cast<WPARAM>(wordParameter),
            static_cast<LPARAM>(longParameter)) != 0;
    }

    bool ImGuiHost::IsInitialized() const noexcept
    {
        return initialized_;
    }
}
