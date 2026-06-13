#include "RmlEditorRmlHost.h"

#include "../App/RmlEditorLog.h"

#include <windowsx.h>

#include <algorithm>

RmlEditorRmlHost::RmlEditorRmlHost() = default;

RmlEditorRmlHost::~RmlEditorRmlHost()
{
	Shutdown();
}

std::wstring RmlEditorRmlHost::GetDataRoot()
{
	wchar_t ModulePath[MAX_PATH] = {};

	const DWORD Length = GetModuleFileNameW(
		nullptr,
		ModulePath,
		static_cast<DWORD>(_countof(ModulePath))
	);

	if (Length == 0 || Length >= _countof(ModulePath))
		return L"Data";

	std::wstring Result = ModulePath;

	const size_t Slash = Result.find_last_of(L"\\/");

	if (Slash != std::wstring::npos)
		Result.resize(Slash);
	else
		Result.clear();

	Result += L"\\Data";
	return Result;
}

bool RmlEditorRmlHost::Initialize(
	HWND Window,
	IDirect3DDevice9* Device
)
{
	if (Initialized)
		return true;

	if (!Window || !Device)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Rml host initialization failed: invalid window or device"
		);

		return false;
	}

	WindowHandle = Window;
	DataRoot = GetDataRoot();

	UpdateClientSize();

	SystemInterface =
		std::make_unique<RmlEditorSystemInterface>();

	FileInterface =
		std::make_unique<RmlEditorFileInterface>(
			DataRoot.c_str()
		);

	RenderInterface =
		std::make_unique<RmlEditorRenderDX9>();

	if (!RenderInterface->Initialize(
		Device,
		DataRoot.c_str()
	))
	{
		Shutdown();
		return false;
	}

	Rml::SetSystemInterface(SystemInterface.get());
	Rml::SetFileInterface(FileInterface.get());
	Rml::SetRenderInterface(RenderInterface.get());

	if (!Rml::Initialise())
	{
		RmlEditorLog::Write(
			"[RmlEditor] Rml::Initialise failed"
		);

		Shutdown();
		return false;
	}

	RmlInitialized = true;

	LoadFonts();

	EditorContext = Rml::CreateContext(
		"RmlEditor.EditorContext",
		Rml::Vector2i(
			ClientWidth,
			ClientHeight
		)
	);

	if (!EditorContext)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to create EditorContext"
		);

		Shutdown();
		return false;
	}

	PreviewContext = Rml::CreateContext(
		"RmlEditor.PreviewContext",
		Rml::Vector2i(1280, 720)
	);

	if (!PreviewContext)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to create PreviewContext"
		);

		Shutdown();
		return false;
	}

	EditorContext->EnableMouseCursor(true);
	PreviewContext->EnableMouseCursor(false);

	Initialized = true;

	if (!LoadEditorShell())
	{
		Shutdown();
		return false;
	}

	RmlEditorLog::Write(
		"[RmlEditor] Rml host initialized"
	);

	RmlEditorLog::Write(
		"[RmlEditor] EditorContext and PreviewContext created"
	);

	return true;
}

void RmlEditorRmlHost::LoadFonts()
{
	const char* FontPaths[] =
	{
		"Rml/Fonts/NotoSans-Regular.ttf",
		"Rml/Fonts/Roboto-Regular.ttf",
		"C:/Windows/Fonts/arial.ttf"
	};

	bool AnyFontLoaded = false;

	for (const char* FontPath : FontPaths)
	{
		if (Rml::LoadFontFace(FontPath))
		{
			AnyFontLoaded = true;

			RmlEditorLog::Write(
				"[RmlEditor] Font loaded: %s",
				FontPath
			);
		}
	}

	if (!AnyFontLoaded)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Warning: no fonts were loaded"
		);
	}
}

bool RmlEditorRmlHost::LoadEditorShell()
{
	if (!EditorContext)
		return false;

	EditorDocument = EditorContext->LoadDocument(
		"RmlEditor/EditorShell.rml"
	);

	if (!EditorDocument)
	{
		RmlEditorLog::Write(
			"[RmlEditor] Failed to load Data/RmlEditor/EditorShell.rml"
		);

		return false;
	}

	EditorDocument->Show();

	RmlEditorLog::Write(
		"[RmlEditor] EditorShell.rml loaded"
	);

	return true;
}

void RmlEditorRmlHost::Shutdown()
{
	Initialized = false;
	MouseTrackingEnabled = false;

	if (EditorContext && EditorDocument)
	{
		EditorContext->UnloadDocument(EditorDocument);
		EditorDocument = nullptr;
	}

	if (PreviewContext)
	{
		const Rml::String ContextName =
			PreviewContext->GetName();

		Rml::RemoveContext(ContextName);
		PreviewContext = nullptr;
	}

	if (EditorContext)
	{
		const Rml::String ContextName =
			EditorContext->GetName();

		Rml::RemoveContext(ContextName);
		EditorContext = nullptr;
	}

	if (RmlInitialized)
	{
		Rml::Shutdown();
		RmlInitialized = false;
	}

	if (RenderInterface)
	{
		RenderInterface->Shutdown();
		RenderInterface.reset();
	}

	FileInterface.reset();
	SystemInterface.reset();

	WindowHandle = nullptr;
	DataRoot.clear();

	RmlEditorLog::Write(
		"[RmlEditor] Rml host shutdown"
	);
}

void RmlEditorRmlHost::UpdateClientSize()
{
	if (!WindowHandle)
	{
		ClientWidth = 1;
		ClientHeight = 1;
		return;
	}

	RECT ClientRectangle{};
	GetClientRect(WindowHandle, &ClientRectangle);

	ClientWidth = std::max(
		1,
		static_cast<int>(
			ClientRectangle.right -
			ClientRectangle.left
		)
	);

	ClientHeight = std::max(
		1,
		static_cast<int>(
			ClientRectangle.bottom -
			ClientRectangle.top
		)
	);

	if (EditorContext)
	{
		EditorContext->SetDimensions(
			Rml::Vector2i(
				ClientWidth,
				ClientHeight
			)
		);
	}
}

void RmlEditorRmlHost::Update(float DeltaSeconds)
{
	(void)DeltaSeconds;

	if (!Initialized)
		return;

	UpdateClientSize();

	if (EditorContext)
		EditorContext->Update();

	if (PreviewContext)
		PreviewContext->Update();
}

void RmlEditorRmlHost::Render()
{
	if (!Initialized ||
		!EditorContext ||
		!RenderInterface)
	{
		return;
	}

	RenderInterface->BeginFrame(
		ClientWidth,
		ClientHeight
	);

	EditorContext->Render();

	// PreviewContext существует отдельно.
	// Его вывод в центральный viewport будет добавлен
	// на следующем этапе через RmlEditorViewport.

	RenderInterface->EndFrame();
}

void RmlEditorRmlHost::OnDeviceLost()
{
	if (RenderInterface)
		RenderInterface->OnDeviceLost();
}

void RmlEditorRmlHost::OnDeviceReset()
{
	UpdateClientSize();

	if (RenderInterface)
	{
		RenderInterface->OnDeviceReset(
			ClientWidth,
			ClientHeight
		);
	}
}

void RmlEditorRmlHost::BeginMouseTracking(HWND Window)
{
	if (MouseTrackingEnabled)
		return;

	TRACKMOUSEEVENT TrackEvent{};

	TrackEvent.cbSize = sizeof(TrackEvent);
	TrackEvent.dwFlags = TME_LEAVE;
	TrackEvent.hwndTrack = Window;

	if (TrackMouseEvent(&TrackEvent))
		MouseTrackingEnabled = true;
}

int RmlEditorRmlHost::GetKeyModifiers()
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

Rml::Input::KeyIdentifier
RmlEditorRmlHost::TranslateKey(WPARAM Key)
{
	if (Key >= 'A' && Key <= 'Z')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_A +
			(Key - 'A')
		);
	}

	if (Key >= '0' && Key <= '9')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_0 +
			(Key - '0')
		);
	}

	if (Key >= VK_F1 && Key <= VK_F12)
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_F1 +
			(Key - VK_F1)
		);
	}

	switch (Key)
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

int RmlEditorRmlHost::TranslateMouseButton(UINT Message)
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

bool RmlEditorRmlHost::ProcessWindowMessage(
	HWND Window,
	UINT Message,
	WPARAM WParam,
	LPARAM LParam,
	LRESULT* Result
)
{
	if (Result)
		*Result = 0;

	if (!Initialized || !EditorContext)
		return false;

	switch (Message)
	{
	case WM_MOUSEMOVE:
	{
		BeginMouseTracking(Window);

		const int MouseX = GET_X_LPARAM(LParam);
		const int MouseY = GET_Y_LPARAM(LParam);

		EditorContext->ProcessMouseMove(
			MouseX,
			MouseY,
			GetKeyModifiers()
		);

		return true;
	}

	case WM_MOUSELEAVE:
	{
		MouseTrackingEnabled = false;
		EditorContext->ProcessMouseLeave();
		return true;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	{
		SetCapture(Window);

		EditorContext->ProcessMouseButtonDown(
			TranslateMouseButton(Message),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		if (GetCapture() == Window)
			ReleaseCapture();

		EditorContext->ProcessMouseButtonUp(
			TranslateMouseButton(Message),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_MOUSEWHEEL:
	{
		const short WheelDelta =
			GET_WHEEL_DELTA_WPARAM(WParam);

		const float Delta =
			-static_cast<float>(WheelDelta) /
			static_cast<float>(WHEEL_DELTA);

		EditorContext->ProcessMouseWheel(
			Rml::Vector2f(0.0f, Delta),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
		{
			EditorContext->ProcessKeyDown(
				Key,
				GetKeyModifiers()
			);

			return true;
		}

		break;
	}

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
		{
			EditorContext->ProcessKeyUp(
				Key,
				GetKeyModifiers()
			);

			return true;
		}

		break;
	}

	case WM_CHAR:
	{
		if (WParam >= 32)
		{
			EditorContext->ProcessTextInput(
				static_cast<Rml::Character>(WParam)
			);
		}

		return true;
	}

	default:
		break;
	}

	return false;
}

bool RmlEditorRmlHost::IsInitialized() const
{
	return Initialized;
}

Rml::Context* RmlEditorRmlHost::GetEditorContext() const
{
	return EditorContext;
}

Rml::Context* RmlEditorRmlHost::GetPreviewContext() const
{
	return PreviewContext;
}