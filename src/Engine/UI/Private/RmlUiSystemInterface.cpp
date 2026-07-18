#include "UI/RmlUiSystemInterface.h"

#include <Windows.h>

#include <cstring>

namespace engine::ui
{
    RmlUiSystemInterface::RmlUiSystemInterface() noexcept : startTime_(std::chrono::steady_clock::now()) {}

    double RmlUiSystemInterface::GetElapsedTime()
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - startTime_).count();
    }

    bool RmlUiSystemInterface::LogMessage(const Rml::Log::Type, const Rml::String& message)
    {
        OutputDebugStringA(("[RmlUi] " + message + "\n").c_str());
        return true;
    }

    void RmlUiSystemInterface::SetMouseCursor(const Rml::String& cursorName)
    {
        const wchar_t* cursor = IDC_ARROW;
        if (cursorName == "pointer") cursor = IDC_HAND;
        else if (cursorName == "text") cursor = IDC_IBEAM;
        else if (cursorName == "cross") cursor = IDC_CROSS;
        SetCursor(LoadCursorW(nullptr, cursor));
    }

    void RmlUiSystemInterface::SetClipboardText(const Rml::String& text)
    {
        if (!OpenClipboard(nullptr)) return;
        EmptyClipboard();
        const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(count) * sizeof(wchar_t));
        if (memory != nullptr)
        {
            auto* output = static_cast<wchar_t*>(GlobalLock(memory));
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, output, count);
            GlobalUnlock(memory);
            if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) GlobalFree(memory);
        }
        CloseClipboard();
    }

    void RmlUiSystemInterface::GetClipboardText(Rml::String& text)
    {
        text.clear();
        if (!OpenClipboard(nullptr)) return;
        const HANDLE data = GetClipboardData(CF_UNICODETEXT);
        const auto* input = data != nullptr ? static_cast<const wchar_t*>(GlobalLock(data)) : nullptr;
        if (input != nullptr)
        {
            const int count = WideCharToMultiByte(CP_UTF8, 0, input, -1, nullptr, 0, nullptr, nullptr);
            if (count > 1)
            {
                text.resize(static_cast<std::size_t>(count - 1));
                WideCharToMultiByte(CP_UTF8, 0, input, -1, text.data(), count, nullptr, nullptr);
            }
            GlobalUnlock(data);
        }
        CloseClipboard();
    }
}
