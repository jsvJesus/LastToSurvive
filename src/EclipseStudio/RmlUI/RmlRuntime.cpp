#include "r3dPCH.h"
#include "r3d.h"

#include "RmlRuntime.h"

#include "RmlRenderDX9.h"
#include "RmlSystemInterface.h"
#include "RmlFileInterface.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Debugger.h>

#include <imm.h>
#include <windowsx.h>

#include <algorithm>
#include <string>
#include <vector>

#pragma comment(lib, "imm32.lib")

#ifndef WM_UNICHAR
#define WM_UNICHAR 0x0109
#endif

#ifndef UNICODE_NOCHAR
#define UNICODE_NOCHAR 0xFFFF
#endif

namespace
{
	Rml::String RmlRuntimeConvertToUTF8(
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

	int RmlRuntimeIMEGetCursorPosition(
		HIMC Context
	)
	{
		return ImmGetCompositionString(
			Context,
			GCS_CURSORPOS,
			nullptr,
			0
		);
	}

	std::wstring RmlRuntimeIMEGetCompositionString(
		HIMC Context,
		bool bFinalize
	)
	{
		const DWORD Type =
			bFinalize
			? GCS_RESULTSTR
			: GCS_COMPSTR;

		const LONG ByteCount =
			ImmGetCompositionStringW(
				Context,
				Type,
				nullptr,
				0
			);

		if (ByteCount <= 0)
			return std::wstring();

		const int CharacterCount =
			static_cast<int>(
				ByteCount /
				sizeof(wchar_t)
			);

		std::wstring Result;
		Result.resize(
			static_cast<size_t>(
				CharacterCount
			)
		);

		ImmGetCompositionStringW(
			Context,
			Type,
			&Result[0],
			ByteCount
		);

		return Result;
	}

	void RmlRuntimeCompleteIMEComposition(
		HWND WindowHandle
	)
	{
		HIMC Context =
			ImmGetContext(
				WindowHandle
			);

		if (!Context)
			return;

		ImmNotifyIME(
			Context,
			NI_COMPOSITIONSTR,
			CPS_COMPLETE,
			0
		);

		ImmReleaseContext(
			WindowHandle,
			Context
		);
	}
}

RmlRuntime& RmlRuntime::Get()
{
	static RmlRuntime Instance;
	return Instance;
}

RmlRuntime::RmlRuntime()
{
}

RmlRuntime::~RmlRuntime()
{
}

bool RmlRuntime::Acquire(
	HWND WindowHandle,
	IDirect3DDevice9* InDevice
)
{
	if (!WindowHandle || !InDevice)
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Init] Invalid window or D3D9 device\n"
		);

		return false;
	}

	if (bInitialized)
	{
		if (
			Hwnd != WindowHandle ||
			Device != InDevice
		)
		{
			r3dOutToLog(
				"[RmlUI][Runtime][Init] Runtime already uses "
				"another window or device\n"
			);

			return false;
		}

		++ReferenceCount;

		r3dOutToLog(
			"[RmlUI][Runtime][Init] Reused, references=%d\n",
			ReferenceCount
		);

		return true;
	}

	if (!InitializeCore(
		WindowHandle,
		InDevice
	))
	{
		return false;
	}

	ReferenceCount = 1;

	r3dOutToLog(
		"[RmlUI][Runtime][Init] Ready, references=1\n"
	);

	return true;
}

void RmlRuntime::Release()
{
	if (ReferenceCount <= 0)
		return;

	--ReferenceCount;

	r3dOutToLog(
		"[RmlUI][Runtime][Shutdown] Release, references=%d\n",
		ReferenceCount
	);

	if (ReferenceCount == 0)
		ShutdownCore();
}

bool RmlRuntime::InitializeCore(
	HWND WindowHandle,
	IDirect3DDevice9* InDevice
)
{
	Hwnd = WindowHandle;
	Device = InDevice;
	DataRoot = BuildDataRoot();

	SystemInterface =
		std::make_unique<RmlSystemInterface>();

	SystemInterface->SetWindow(
		WindowHandle
	);

	FileInterface =
		std::make_unique<RmlFileInterface>(
			DataRoot.c_str()
		);

	RenderInterface =
		std::make_unique<RmlRenderDX9>();

	if (!RenderInterface->Init(
		InDevice,
		DataRoot.c_str()
	))
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Init] DX9 renderer initialization failed\n"
		);

		RenderInterface.reset();
		FileInterface.reset();
		SystemInterface.reset();

		Hwnd = nullptr;
		Device = nullptr;
		DataRoot.clear();

		return false;
	}

	Rml::SetSystemInterface(
		SystemInterface.get()
	);

	Rml::SetTextInputHandler(
		SystemInterface.get()
	);

	Rml::SetFileInterface(
		FileInterface.get()
	);

	Rml::SetRenderInterface(
		RenderInterface.get()
	);

	if (!Rml::Initialise())
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Init] Rml::Initialise failed\n"
		);

		RenderInterface->Shutdown();

		RenderInterface.reset();
		FileInterface.reset();
		SystemInterface.reset();

		Hwnd = nullptr;
		Device = nullptr;
		DataRoot.clear();

		return false;
	}

	bCoreInitialized = true;
	bInitialized = true;

	Rml::LoadFontFace(
		"Z:/WarZ/External/RmlUI/Fonts/NotoSans-Regular.ttf"
	);

	Rml::LoadFontFace(
		"Z:/WarZ/External/RmlUI/Fonts/NotoSans-Bold.ttf"
	);

	Rml::LoadFontFace(
		"Z:/WarZ/External/RmlUI/Fonts/Roboto-Regular.ttf"
	);

	Rml::LoadFontFace(
		"C:/Windows/Fonts/arial.ttf"
	);

	{
		const Rml::String PlayRegularPath =
			RmlRuntimeConvertToUTF8(
				DataRoot +
				L"\\Rml\\Fonts\\1_PlayRegular_Play.ttf"
			);

		const bool bPlayRegularLoaded =
			Rml::LoadFontFace(
				PlayRegularPath
			);

		r3dOutToLog(
			"[RmlUI][Runtime][Fonts] Play Regular: %s (%s)\n",
			bPlayRegularLoaded ? "loaded" : "FAILED",
			PlayRegularPath.c_str()
		);
	}

	r3dOutToLog(
		"[RmlUI][Runtime][Init] Core initialized. DataRoot=%ls\n",
		DataRoot.c_str()
	);

	return true;
}

void RmlRuntime::ShutdownCore()
{
	ActiveContext = nullptr;

	if (bDebuggerInitialized)
	{
		Rml::Debugger::SetVisible(false);
		Rml::Debugger::Shutdown();

		bDebuggerInitialized = false;
		DebuggerContext = nullptr;
	}

	if (!Contexts.empty())
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Shutdown] Removing %d remaining contexts\n",
			static_cast<int>(Contexts.size())
		);

		std::vector<Rml::Context*> RemainingContexts;

		RemainingContexts.reserve(
			Contexts.size()
		);

		for (Rml::Context* Context : Contexts)
			RemainingContexts.push_back(Context);

		for (Rml::Context* Context : RemainingContexts)
		{
			if (!Context)
				continue;

			const Rml::String ContextName =
				Context->GetName();

			Rml::RemoveContext(
				ContextName
			);
		}

		Contexts.clear();
	}

	if (bCoreInitialized)
	{
		if (
			Rml::GetTextInputHandler() ==
			SystemInterface.get()
		)
		{
			Rml::SetTextInputHandler(
				nullptr
			);
		}

		Rml::Shutdown();
		bCoreInitialized = false;
	}

	if (RenderInterface)
	{
		RenderInterface->Shutdown();
		RenderInterface.reset();
	}

	FileInterface.reset();
	SystemInterface.reset();

	Hwnd = nullptr;
	Device = nullptr;

	DataRoot.clear();

	ContextSerial = 0;
	ReferenceCount = 0;

	bInitialized = false;
	bRenderFrameOpen = false;

	r3dOutToLog(
		"[RmlUI][Runtime][Shutdown] Core shutdown complete\n"
	);
}

Rml::Context* RmlRuntime::CreateContext(
	const char* BaseName,
	const Rml::Vector2i& Dimensions
)
{
	if (!bInitialized)
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Context] Runtime is not initialized\n"
		);

		return nullptr;
	}

	const char* SafeBaseName =
		BaseName && BaseName[0]
		? BaseName
		: "Context";

	Rml::String ContextName =
		SafeBaseName;

	ContextName += "_";
	ContextName += std::to_string(
		++ContextSerial
	);

	const Rml::Vector2i SafeDimensions(
		std::max(1, Dimensions.x),
		std::max(1, Dimensions.y)
	);

	Rml::Context* Context =
		Rml::CreateContext(
			ContextName,
			SafeDimensions
		);

	if (!Context)
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Context] Failed to create %s\n",
			ContextName.c_str()
		);

		return nullptr;
	}

	Context->EnableMouseCursor(true);

	Contexts.insert(Context);

	r3dOutToLog(
		"[RmlUI][Runtime][Context] Created %s (%dx%d)\n",
		ContextName.c_str(),
		SafeDimensions.x,
		SafeDimensions.y
	);

	return Context;
}

void RmlRuntime::DestroyContext(
	Rml::Context*& Context
)
{
	if (!Context)
		return;

	if (!IsRegisteredContext(Context))
	{
		Context = nullptr;
		return;
	}

	if (ActiveContext == Context)
		ActiveContext = nullptr;

	if (
		bDebuggerInitialized &&
		DebuggerContext == Context
	)
	{
		Rml::Debugger::SetVisible(false);
		Rml::Debugger::Shutdown();

		bDebuggerInitialized = false;
		DebuggerContext = nullptr;
	}

	const Rml::String ContextName =
		Context->GetName();

	Contexts.erase(Context);

	Rml::RemoveContext(
		ContextName
	);

	r3dOutToLog(
		"[RmlUI][Runtime][Context] Destroyed %s\n",
		ContextName.c_str()
	);

	Context = nullptr;
}

void RmlRuntime::SetActiveContext(
	Rml::Context* Context
)
{
	if (!Context)
		return;

	if (!IsRegisteredContext(Context))
		return;

	if (ActiveContext == Context)
		return;

	ActiveContext = Context;

	r3dOutToLog(
		"[RmlUI][Runtime][Input] Active context: %s\n",
		Context->GetName().c_str()
	);
}

void RmlRuntime::ClearActiveContext(
	Rml::Context* Context
)
{
	if (!Context)
		return;

	if (ActiveContext != Context)
		return;

	r3dOutToLog(
		"[RmlUI][Runtime][Input] Released context: %s\n",
		Context->GetName().c_str()
	);

	ActiveContext = nullptr;
}

bool RmlRuntime::IsActiveContext(
	const Rml::Context* Context
) const
{
	return
		Context &&
		ActiveContext == Context;
}

bool RmlRuntime::IsRegisteredContext(
	const Rml::Context* Context
) const
{
	if (!Context)
		return false;

	return Contexts.find(
		const_cast<Rml::Context*>(Context)
	) != Contexts.end();
}

bool RmlRuntime::ProcessWin32Message(
	Rml::Context* Context,
	HWND WindowHandle,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam,
	LRESULT* OutResult
)
{
	if (OutResult)
		*OutResult = 0;

	if (
		!bInitialized ||
		!Context ||
		ActiveContext != Context
	)
	{
		return false;
	}

	if (
		SystemInterface &&
		SystemInterface->IsComposing() &&
		Message >= WM_LBUTTONDOWN &&
		Message <= WM_MBUTTONDBLCLK
	)
	{
		RmlRuntimeCompleteIMEComposition(
			WindowHandle
		);
	}

	switch (Message)
	{
	case WM_MOUSEMOVE:
	{
		const int X =
			GET_X_LPARAM(LParam);

		const int Y =
			GET_Y_LPARAM(LParam);

		Context->ProcessMouseMove(
			X,
			Y,
			GetKeyModifiers()
		);

		return true;
	}

	case WM_MOUSELEAVE:
		Context->ProcessMouseLeave();
		return false;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	{
		SetCapture(WindowHandle);

		Context->ProcessMouseButtonDown(
			TranslateMouseButton(Message),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		if (GetCapture() == WindowHandle)
			ReleaseCapture();

		Context->ProcessMouseButtonUp(
			TranslateMouseButton(Message),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_MOUSEWHEEL:
	{
		const short Wheel =
			GET_WHEEL_DELTA_WPARAM(WParam);

		const float Delta =
			-static_cast<float>(Wheel) /
			static_cast<float>(WHEEL_DELTA);

		Context->ProcessMouseWheel(
			Rml::Vector2f(
				0.0f,
				Delta
			),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		if (
			WParam == VK_F4 &&
			(GetKeyState(VK_MENU) & 0x8000)
		)
		{
			return false;
		}

		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key == Rml::Input::KI_UNKNOWN)
			return false;

		Context->ProcessKeyDown(
			Key,
			GetKeyModifiers()
		);

		return true;
	}

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key == Rml::Input::KI_UNKNOWN)
			return false;

		Context->ProcessKeyUp(
			Key,
			GetKeyModifiers()
		);

		return true;
	}

	case WM_CHAR:
	{
		static wchar_t FirstU16CodeUnit =
			0;

		const wchar_t Character16 =
			static_cast<wchar_t>(
				WParam
			);

		if (
			Character16 >= 0xD800 &&
			Character16 < 0xDC00
		)
		{
			FirstU16CodeUnit =
				Character16;

			return true;
		}

		if (
			Character16 >= 0xDC00 &&
			Character16 < 0xE000 &&
			FirstU16CodeUnit != 0
		)
		{
			const Rml::String Text =
				RmlRuntimeConvertToUTF8(
					std::wstring{
						FirstU16CodeUnit,
						Character16
					}
				);

			FirstU16CodeUnit =
				0;

			if (!Text.empty())
			{
				Context->ProcessTextInput(
					Text
				);
			}

			return true;
		}

		FirstU16CodeUnit =
			0;

		Rml::Character Character =
			static_cast<Rml::Character>(
				Character16
			);

		if (Character16 == L'\r')
		{
			Character =
				static_cast<Rml::Character>(
					'\n'
				);
		}

		if (
			(
				static_cast<char32_t>(
					Character
				) >= 32 ||
				Character ==
					static_cast<Rml::Character>(
						'\n'
					)
			) &&
			Character !=
				static_cast<Rml::Character>(
					127
				)
		)
		{
			Context->ProcessTextInput(
				Character
			);
		}

		return true;
	}

	case WM_UNICHAR:
	{
		if (WParam == UNICODE_NOCHAR)
		{
			if (OutResult)
				*OutResult = TRUE;

			return true;
		}

		Context->ProcessTextInput(
			static_cast<Rml::Character>(
				WParam
			)
		);

		return true;
	}

	case WM_IME_STARTCOMPOSITION:
	{
		if (SystemInterface)
			SystemInterface->StartComposition();

		return true;
	}

	case WM_IME_ENDCOMPOSITION:
	{
		if (
			SystemInterface &&
			SystemInterface->IsComposing()
		)
		{
			SystemInterface->ConfirmComposition(
				Rml::StringView()
			);
		}

		return true;
	}

	case WM_IME_COMPOSITION:
	{
		if (!SystemInterface)
			return false;

		HIMC IMEContext =
			ImmGetContext(
				WindowHandle
			);

		if (!IMEContext)
			return false;

		if (!SystemInterface->IsComposing())
			SystemInterface->StartComposition();

		if (LParam & GCS_CURSORPOS)
		{
			int CursorPosition =
				RmlRuntimeIMEGetCursorPosition(
					IMEContext
				);

			const std::wstring Composition =
				RmlRuntimeIMEGetCompositionString(
					IMEContext,
					false
				);

			const Rml::String Prefix =
				RmlRuntimeConvertToUTF8(
					Composition.substr(
						0,
						static_cast<size_t>(
							std::max(
								0,
								CursorPosition
							)
						)
					)
				);

			CursorPosition =
				static_cast<int>(
					Rml::StringUtilities::LengthUTF8(
						Prefix
					)
				);

			SystemInterface->SetCompositionCursorPosition(
				CursorPosition,
				true
			);
		}

		if (LParam & CS_NOMOVECARET)
		{
			SystemInterface->SetCompositionCursorPosition(
				-1,
				false
			);
		}

		if (LParam & GCS_RESULTSTR)
		{
			const std::wstring Composition =
				RmlRuntimeIMEGetCompositionString(
					IMEContext,
					true
				);

			SystemInterface->ConfirmComposition(
				RmlRuntimeConvertToUTF8(
					Composition
				)
			);
		}

		if (LParam & GCS_COMPSTR)
		{
			const std::wstring Composition =
				RmlRuntimeIMEGetCompositionString(
					IMEContext,
					false
				);

			SystemInterface->SetComposition(
				RmlRuntimeConvertToUTF8(
					Composition
				)
			);
		}

		if (LParam == 0)
		{
			SystemInterface->CancelComposition();
		}

		ImmReleaseContext(
			WindowHandle,
			IMEContext
		);

		return true;
	}

	case WM_IME_CHAR:
	case WM_IME_REQUEST:
		return true;

	default:
		break;
	}

	return false;
}

void RmlRuntime::RenderContext(
	Rml::Context* Context,
	int Width,
	int Height
)
{
	if (
		!bInitialized ||
		!RenderInterface ||
		!Context ||
		!IsRegisteredContext(Context)
	)
	{
		return;
	}

	if (
		!r3dRenderer ||
		!r3dRenderer->pd3ddev ||
		!r3dRenderer->DeviceAvailable ||
		r3dRenderer->IsDeviceLost()
	)
	{
		return;
	}

	if (bRenderFrameOpen)
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Render] Nested rendering rejected\n"
		);

		return;
	}

	bRenderFrameOpen = true;

	RenderInterface->BeginFrame(
		std::max(1, Width),
		std::max(1, Height)
	);

	Context->Render();

	RenderInterface->EndFrame();

	bRenderFrameOpen = false;
}

void RmlRuntime::
SetCharacterPreviewTexture(
	IDirect3DTexture9* Texture
)
{
	if (!RenderInterface)
		return;

	RenderInterface->
		SetCharacterPreviewTexture(
			Texture
		);
}

void RmlRuntime::
SetCharacterPortraitTexture(
	IDirect3DTexture9* Texture
)
{
	if (!RenderInterface)
		return;

	RenderInterface->
		SetCharacterPortraitTexture(
			Texture
		);
}

void RmlRuntime::OnDeviceLost()
{
	if (!bInitialized || !RenderInterface)
		return;

	if (bRenderFrameOpen)
	{
		RenderInterface->EndFrame();
		bRenderFrameOpen = false;
	}

	RenderInterface->OnDeviceLost();
}

void RmlRuntime::OnDeviceReset(
	int Width,
	int Height
)
{
	if (!bInitialized || !RenderInterface)
		return;

	RenderInterface->OnDeviceReset(
		std::max(1, Width),
		std::max(1, Height)
	);
}

bool RmlRuntime::EnsureDebugger(
	Rml::Context* Context
)
{
	if (
		!bInitialized ||
		!Context ||
		!IsRegisteredContext(Context)
	)
	{
		return false;
	}

	if (bDebuggerInitialized)
	{
		if (DebuggerContext != Context)
		{
			Rml::Debugger::SetVisible(false);
			Rml::Debugger::SetContext(Context);

			DebuggerContext = Context;
		}

		return true;
	}

	if (!Rml::Debugger::Initialise(Context))
		return false;

	Rml::Debugger::SetContext(Context);
	Rml::Debugger::SetVisible(false);

	DebuggerContext = Context;
	bDebuggerInitialized = true;

	return true;
}

void RmlRuntime::SetDebuggerVisible(
	Rml::Context* Context,
	bool bVisible
)
{
	if (!Context)
		return;

	if (
		bVisible &&
		!EnsureDebugger(Context)
	)
	{
		return;
	}

	if (!bDebuggerInitialized)
		return;

	if (DebuggerContext != Context)
	{
		Rml::Debugger::SetVisible(false);
		Rml::Debugger::SetContext(Context);

		DebuggerContext = Context;
	}

	Rml::Debugger::SetVisible(
		bVisible
	);
}

bool RmlRuntime::IsDebuggerVisible(
	const Rml::Context* Context
) const
{
	return
		bDebuggerInitialized &&
		DebuggerContext == Context &&
		Rml::Debugger::IsVisible();
}

bool RmlRuntime::IsInitialized() const
{
	return bInitialized;
}

int RmlRuntime::GetReferenceCount() const
{
	return ReferenceCount;
}

const std::wstring& RmlRuntime::GetDataRoot() const
{
	return DataRoot;
}

std::wstring RmlRuntime::BuildDataRoot()
{
	wchar_t ModulePath[MAX_PATH]{};

	GetModuleFileNameW(
		nullptr,
		ModulePath,
		MAX_PATH
	);

	std::wstring Path =
		ModulePath;

	const size_t Slash =
		Path.find_last_of(L"\\/");

	if (Slash != std::wstring::npos)
		Path.resize(Slash);

	return Path + L"\\Data";
}

int RmlRuntime::GetKeyModifiers()
{
	int Modifiers = 0;

	if (GetKeyState(VK_CONTROL) & 0x8000)
		Modifiers |= Rml::Input::KM_CTRL;

	if (GetKeyState(VK_SHIFT) & 0x8000)
		Modifiers |= Rml::Input::KM_SHIFT;

	if (GetKeyState(VK_MENU) & 0x8000)
		Modifiers |= Rml::Input::KM_ALT;

	if (GetKeyState(VK_CAPITAL) & 0x0001)
		Modifiers |= Rml::Input::KM_CAPSLOCK;

	if (GetKeyState(VK_NUMLOCK) & 0x0001)
		Modifiers |= Rml::Input::KM_NUMLOCK;

	if (GetKeyState(VK_SCROLL) & 0x0001)
		Modifiers |= Rml::Input::KM_SCROLLLOCK;

	return Modifiers;
}

Rml::Input::KeyIdentifier RmlRuntime::TranslateKey(
	WPARAM WParam
)
{
	if (WParam >= 'A' && WParam <= 'Z')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_A +
			(WParam - 'A')
		);
	}

	if (WParam >= '0' && WParam <= '9')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_0 +
			(WParam - '0')
		);
	}

	if (WParam >= VK_F1 && WParam <= VK_F12)
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_F1 +
			(WParam - VK_F1)
		);
	}

	switch (WParam)
	{
	case VK_SPACE:
		return Rml::Input::KI_SPACE;

	case VK_BACK:
		return Rml::Input::KI_BACK;

	case VK_TAB:
		return Rml::Input::KI_TAB;

	case VK_RETURN:
		return Rml::Input::KI_RETURN;

	case VK_ESCAPE:
		return Rml::Input::KI_ESCAPE;

	case VK_PRIOR:
		return Rml::Input::KI_PRIOR;

	case VK_NEXT:
		return Rml::Input::KI_NEXT;

	case VK_END:
		return Rml::Input::KI_END;

	case VK_HOME:
		return Rml::Input::KI_HOME;

	case VK_LEFT:
		return Rml::Input::KI_LEFT;

	case VK_UP:
		return Rml::Input::KI_UP;

	case VK_RIGHT:
		return Rml::Input::KI_RIGHT;

	case VK_DOWN:
		return Rml::Input::KI_DOWN;

	case VK_INSERT:
		return Rml::Input::KI_INSERT;

	case VK_DELETE:
		return Rml::Input::KI_DELETE;

	case VK_SHIFT:
		return Rml::Input::KI_LSHIFT;

	case VK_CONTROL:
		return Rml::Input::KI_LCONTROL;

	case VK_MENU:
		return Rml::Input::KI_LMENU;

	case VK_OEM_PLUS:
		return Rml::Input::KI_OEM_PLUS;

	case VK_OEM_MINUS:
		return Rml::Input::KI_OEM_MINUS;

	case VK_OEM_COMMA:
		return Rml::Input::KI_OEM_COMMA;

	case VK_OEM_PERIOD:
		return Rml::Input::KI_OEM_PERIOD;

	default:
		return Rml::Input::KI_UNKNOWN;
	}
}

int RmlRuntime::TranslateMouseButton(
	UINT Message
)
{
	switch (Message)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		return 0;

	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		return 1;

	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		return 2;

	default:
		return 0;
	}
}
