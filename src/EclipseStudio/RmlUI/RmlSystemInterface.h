#pragma once

#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/TextInputHandler.h>
#include <windows.h>

class RmlSystemInterface final : public Rml::SystemInterface
	, public Rml::TextInputHandler
{
public:
    RmlSystemInterface();

	void SetWindow(HWND WindowHandle);

    double GetElapsedTime() override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
    void JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path) override;
	void SetMouseCursor(const Rml::String& cursor_name) override;
	void SetClipboardText(const Rml::String& text) override;
	void GetClipboardText(Rml::String& text) override;
	void ActivateKeyboard(Rml::Vector2f caret_position, float line_height) override;

	void OnActivate(Rml::TextInputContext* input_context) override;
	void OnDeactivate(Rml::TextInputContext* input_context) override;
	void OnDestroy(Rml::TextInputContext* input_context) override;

	bool IsComposing() const;
	void StartComposition();
	void CancelComposition();
	void SetComposition(Rml::StringView composition);
	void ConfirmComposition(Rml::StringView composition);
	void SetCompositionCursorPosition(int cursor_pos, bool update);

private:
    LARGE_INTEGER Frequency{};
    LARGE_INTEGER StartTime{};
	HWND WindowHandle = nullptr;

	HCURSOR CursorDefault = nullptr;
	HCURSOR CursorMove = nullptr;
	HCURSOR CursorPointer = nullptr;
	HCURSOR CursorResize = nullptr;
	HCURSOR CursorCross = nullptr;
	HCURSOR CursorText = nullptr;
	HCURSOR CursorUnavailable = nullptr;

	Rml::TextInputContext* InputContext = nullptr;
	bool bComposing = false;
	int CompositionCursorPosition = 0;
	int CompositionRangeStart = 0;
	int CompositionRangeEnd = 0;

    static bool IsAbsolutePath(const Rml::String& path);
	static Rml::String ConvertToUTF8(const std::wstring& text);
	static std::wstring ConvertToUTF16(const Rml::String& text);

	void EndComposition();
	void SetCompositionString(Rml::StringView composition);
	void UpdateCompositionCursorPosition();
};
