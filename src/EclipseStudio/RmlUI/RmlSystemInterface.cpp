#include "RmlSystemInterface.h"

#include <Core/Log.h>
#include <Platform/Clock.h>

#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/TextInputContext.h>

#include <algorithm>
#include <imm.h>
#include <cstring>
#include <string>

#pragma comment(lib, "imm32.lib")

RmlSystemInterface::RmlSystemInterface()
{
	StartTime =
		engine::platform::Clock::Now();

	CursorDefault =
		LoadCursor(
			nullptr,
			IDC_ARROW
		);

	CursorMove =
		LoadCursor(
			nullptr,
			IDC_SIZEALL
		);

	CursorPointer =
		LoadCursor(
			nullptr,
			IDC_HAND
		);

	CursorResize =
		LoadCursor(
			nullptr,
			IDC_SIZENWSE
		);

	CursorCross =
		LoadCursor(
			nullptr,
			IDC_CROSS
		);

	CursorText =
		LoadCursor(
			nullptr,
			IDC_IBEAM
		);

	CursorUnavailable =
		LoadCursor(
			nullptr,
			IDC_NO
		);
}

void RmlSystemInterface::SetWindow(
	HWND InWindowHandle
)
{
	WindowHandle =
		InWindowHandle;
}

double RmlSystemInterface::GetElapsedTime()
{
	return engine::platform::Clock::ElapsedSeconds(
		StartTime,
		engine::platform::Clock::Now());
}

bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	engine::core::LogLevel Level =
		engine::core::LogLevel::Information;

    switch (type)
    {
	case Rml::Log::LT_ERROR:
		Level = engine::core::LogLevel::Error;
		break;

	case Rml::Log::LT_ASSERT:
		Level = engine::core::LogLevel::Critical;
		break;

	case Rml::Log::LT_WARNING:
		Level = engine::core::LogLevel::Warning;
		break;

	case Rml::Log::LT_DEBUG:
		Level = engine::core::LogLevel::Debug;
		break;

	case Rml::Log::LT_ALWAYS:
	case Rml::Log::LT_INFO:
    default: break;
    }

	engine::core::GetLogger().Write(
		Level,
		"RmlUI",
		message);

    return true;
}

bool RmlSystemInterface::IsAbsolutePath(
    const Rml::String& Path
)
{
    if (Path.empty())
        return false;

    /*
     * Пользовательские и стандартные URI:
     *
     * rml://character-preview
     * http://...
     * https://...
     */
    if (
        Path.find("://") !=
        Rml::String::npos
    )
    {
        return true;
    }

    /*
     * Windows:
     * C:\Folder\File
     */
    if (
        Path.size() >= 2 &&
        Path[1] == ':'
    )
    {
        return true;
    }

    /*
     * Абсолютный путь от корня.
     */
    if (
        Path[0] == '/' ||
        Path[0] == '\\'
    )
    {
        return true;
    }

    return false;
}

void RmlSystemInterface::JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path)
{
    if (IsAbsolutePath(path))
    {
        translated_path = path;
        return;
    }

    const size_t SlashA = document_path.find_last_of('/');
    const size_t SlashB = document_path.find_last_of('\\');
    const size_t Slash = (std::max)(
        SlashA == Rml::String::npos ? 0 : SlashA,
        SlashB == Rml::String::npos ? 0 : SlashB
    );

    if (Slash != 0 && Slash != Rml::String::npos)
        translated_path = document_path.substr(0, Slash + 1) + path;
    else
        translated_path = path;
}

void RmlSystemInterface::OnActivate(
	Rml::TextInputContext* InInputContext
)
{
	InputContext =
		InInputContext;
}

void RmlSystemInterface::OnDeactivate(
	Rml::TextInputContext* InInputContext
)
{
	if (InputContext == InInputContext)
		InputContext = nullptr;
}

void RmlSystemInterface::OnDestroy(
	Rml::TextInputContext* InInputContext
)
{
	if (InputContext == InInputContext)
		InputContext = nullptr;
}

bool RmlSystemInterface::IsComposing() const
{
	return bComposing;
}

void RmlSystemInterface::StartComposition()
{
	if (bComposing)
		return;

	bComposing =
		true;

	CompositionCursorPosition =
		-1;
}

void RmlSystemInterface::EndComposition()
{
	if (InputContext)
	{
		InputContext->SetCompositionRange(
			0,
			0
		);
	}

	bComposing =
		false;

	CompositionCursorPosition =
		-1;

	CompositionRangeStart =
		0;

	CompositionRangeEnd =
		0;
}

void RmlSystemInterface::CancelComposition()
{
	if (!bComposing)
		return;

	if (InputContext)
	{
		InputContext->SetText(
			Rml::StringView(),
			CompositionRangeStart,
			CompositionRangeEnd
		);

		InputContext->SetCursorPosition(
			CompositionRangeStart
		);
	}

	EndComposition();
}

void RmlSystemInterface::SetComposition(
	Rml::StringView Composition
)
{
	if (!bComposing)
		StartComposition();

	SetCompositionString(
		Composition
	);

	UpdateCompositionCursorPosition();

	if (
		CompositionCursorPosition != -1 &&
		InputContext
	)
	{
		InputContext->SetCompositionRange(
			CompositionRangeStart,
			CompositionRangeEnd
		);
	}
}

void RmlSystemInterface::ConfirmComposition(
	Rml::StringView Composition
)
{
	if (!bComposing)
		StartComposition();

	SetCompositionString(
		Composition
	);

	if (InputContext)
	{
		InputContext->SetCompositionRange(
			CompositionRangeStart,
			CompositionRangeEnd
		);

		InputContext->CommitComposition(
			Composition
		);
	}

	SetCompositionCursorPosition(
		CompositionRangeEnd -
			CompositionRangeStart,
		true
	);

	EndComposition();
}

void RmlSystemInterface::SetCompositionCursorPosition(
	int CursorPosition,
	bool bUpdate
)
{
	if (!bComposing)
		StartComposition();

	CompositionCursorPosition =
		CursorPosition;

	if (bUpdate)
		UpdateCompositionCursorPosition();
}

void RmlSystemInterface::SetCompositionString(
	Rml::StringView Composition
)
{
	if (!InputContext)
		return;

	if (
		CompositionRangeStart == 0 &&
		CompositionRangeEnd == 0
	)
	{
		InputContext->GetSelectionRange(
			CompositionRangeStart,
			CompositionRangeEnd
		);
	}

	InputContext->SetText(
		Composition,
		CompositionRangeStart,
		CompositionRangeEnd
	);

	const size_t Length =
		Rml::StringUtilities::LengthUTF8(
			Composition
		);

	CompositionRangeEnd =
		CompositionRangeStart +
		static_cast<int>(
			Length
		);
}

void RmlSystemInterface::UpdateCompositionCursorPosition()
{
	if (
		!InputContext ||
		(
			CompositionRangeStart == 0 &&
			CompositionRangeEnd == 0
		)
	)
	{
		return;
	}

	if (CompositionCursorPosition != -1)
	{
		InputContext->SetCursorPosition(
			CompositionRangeStart +
				CompositionCursorPosition
		);
	}
	else
	{
		InputContext->SetSelectionRange(
			CompositionRangeStart,
			CompositionRangeEnd
		);
	}
}

void RmlSystemInterface::SetMouseCursor(
	const Rml::String& CursorName
)
{
	if (!WindowHandle)
		return;

	HCURSOR CursorHandle =
		nullptr;

	if (
		CursorName.empty() ||
		CursorName == "arrow"
	)
	{
		CursorHandle =
			CursorDefault;
	}
	else if (CursorName == "move")
	{
		CursorHandle =
			CursorMove;
	}
	else if (CursorName == "pointer")
	{
		CursorHandle =
			CursorPointer;
	}
	else if (CursorName == "resize")
	{
		CursorHandle =
			CursorResize;
	}
	else if (CursorName == "cross")
	{
		CursorHandle =
			CursorCross;
	}
	else if (CursorName == "text")
	{
		CursorHandle =
			CursorText;
	}
	else if (CursorName == "unavailable")
	{
		CursorHandle =
			CursorUnavailable;
	}
	else if (
		Rml::StringUtilities::StartsWith(
			CursorName,
			"rmlui-scroll"
		)
	)
	{
		CursorHandle =
			CursorMove;
	}

	if (!CursorHandle)
		return;

	SetCursor(
		CursorHandle
	);

	SetClassLongPtr(
		WindowHandle,
		GCLP_HCURSOR,
		reinterpret_cast<LONG_PTR>(
			CursorHandle
		)
	);
}

Rml::String RmlSystemInterface::ConvertToUTF8(
	const std::wstring& Text
)
{
	if (Text.empty())
		return Rml::String();

	const int Required =
		WideCharToMultiByte(
			CP_UTF8,
			0,
			Text.data(),
			static_cast<int>(
				Text.size()
			),
			nullptr,
			0,
			nullptr,
			nullptr
		);

	if (Required <= 0)
		return Rml::String();

	Rml::String Result;
	Result.resize(
		static_cast<size_t>(
			Required
		)
	);

	WideCharToMultiByte(
		CP_UTF8,
		0,
		Text.data(),
		static_cast<int>(
			Text.size()
		),
		&Result[0],
		Required,
		nullptr,
		nullptr
	);

	return Result;
}

std::wstring RmlSystemInterface::ConvertToUTF16(
	const Rml::String& Text
)
{
	if (Text.empty())
		return std::wstring();

	const int Required =
		MultiByteToWideChar(
			CP_UTF8,
			0,
			Text.data(),
			static_cast<int>(
				Text.size()
			),
			nullptr,
			0
		);

	if (Required <= 0)
		return std::wstring();

	std::wstring Result;
	Result.resize(
		static_cast<size_t>(
			Required
		)
	);

	MultiByteToWideChar(
		CP_UTF8,
		0,
		Text.data(),
		static_cast<int>(
			Text.size()
		),
		&Result[0],
		Required
	);

	return Result;
}

void RmlSystemInterface::SetClipboardText(
	const Rml::String& Text
)
{
	if (
		!WindowHandle ||
		!OpenClipboard(
			WindowHandle
		)
	)
	{
		return;
	}

	EmptyClipboard();

	const std::wstring WideText =
		ConvertToUTF16(
			Text
		);

	const SIZE_T ByteCount =
		sizeof(wchar_t) *
		(
			WideText.size() +
			1
		);

	HGLOBAL ClipboardData =
		GlobalAlloc(
			GMEM_MOVEABLE,
			ByteCount
		);

	if (!ClipboardData)
	{
		CloseClipboard();
		return;
	}

	void* Memory =
		GlobalLock(
			ClipboardData
		);

	if (!Memory)
	{
		GlobalFree(
			ClipboardData
		);

		CloseClipboard();
		return;
	}

	std::memcpy(
		Memory,
		WideText.c_str(),
		ByteCount
	);

	GlobalUnlock(
		ClipboardData
	);

	if (
		!SetClipboardData(
			CF_UNICODETEXT,
			ClipboardData
		)
	)
	{
		GlobalFree(
			ClipboardData
		);
	}

	CloseClipboard();
}

void RmlSystemInterface::GetClipboardText(
	Rml::String& Text
)
{
	if (
		!WindowHandle ||
		!OpenClipboard(
			WindowHandle
		)
	)
	{
		return;
	}

	HANDLE ClipboardData =
		GetClipboardData(
			CF_UNICODETEXT
		);

	if (!ClipboardData)
	{
		CloseClipboard();
		return;
	}

	const wchar_t* WideText =
		static_cast<const wchar_t*>(
			GlobalLock(
				ClipboardData
			)
		);

	if (WideText)
	{
		Text =
			ConvertToUTF8(
				WideText
			);

		GlobalUnlock(
			ClipboardData
		);
	}

	CloseClipboard();
}

void RmlSystemInterface::ActivateKeyboard(
	Rml::Vector2f CaretPosition,
	float LineHeight
)
{
	if (!WindowHandle)
		return;

	HIMC Context =
		ImmGetContext(
			WindowHandle
		);

	if (!Context)
		return;

	const LONG X =
		static_cast<LONG>(
			CaretPosition.x
		);

	const LONG Y =
		static_cast<LONG>(
			CaretPosition.y
		);

	const LONG Height =
		static_cast<LONG>(
			LineHeight
		) +
		2;

	COMPOSITIONFORM CompositionForm{};
	CompositionForm.dwStyle =
		CFS_FORCE_POSITION;

	CompositionForm.ptCurrentPos =
		{
			X,
			Y
		};

	ImmSetCompositionWindow(
		Context,
		&CompositionForm
	);

	CANDIDATEFORM CandidateForm{};
	CandidateForm.dwStyle =
		CFS_EXCLUDE;

	CandidateForm.ptCurrentPos =
		{
			X,
			Y
		};

	CandidateForm.rcArea =
		{
			X,
			Y,
			X + 1,
			Y + Height
		};

	ImmSetCandidateWindow(
		Context,
		&CandidateForm
	);

	ImmReleaseContext(
		WindowHandle,
		Context
	);
}
