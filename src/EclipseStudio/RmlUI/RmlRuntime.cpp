#include "r3dPCH.h"
#include "r3d.h"

#include "RmlRuntime.h"

#include "RmlRenderDX9.h"
#include "RmlSystemInterface.h"
#include "RmlFileInterface.h"

#include <RmlUi/Core/Context.h>

#include <windowsx.h>

#include <algorithm>
#include <string>
#include <vector>

RmlRuntime& RmlRuntime::Get()
{
	static RmlRuntime Instance;
	return Instance;
}

bool RmlRuntime::Acquire(
	HWND WindowHandle,
	IDirect3DDevice9* InDevice
)
{
	if (!WindowHandle || !InDevice)
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Init] Invalid HWND or D3D9 device\n"
		);

		return false;
	}

	if (bInitialized)
	{
		if (Hwnd != WindowHandle || Device != InDevice)
		{
			r3dOutToLog(
				"[RmlUI][Runtime][Init] Runtime already uses another "
				"window or D3D9 device\n"
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

	if (!InitializeCore(WindowHandle, InDevice))
		return false;

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

	if (ReferenceCount > 0)
		return;

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
			"[RmlUI][Runtime][Init] DX9 render interface failed\n"
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

		Rml::SetRenderInterface(nullptr);
		Rml::SetFileInterface(nullptr);
		Rml::SetSystemInterface(nullptr);

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

	const char* Fonts[] =
	{
		"Rml/Fonts/NotoSans-Regular.ttf",
		"Rml/Fonts/NotoSans-Bold.ttf",
		"Rml/Fonts/Roboto-Regular.ttf"
	};

	for (const char* Font : Fonts)
	{
		if (!Rml::LoadFontFace(Font))
		{
			r3dOutToLog(
				"[RmlUI][Runtime][Init] Font load warning: %s\n",
				Font
			);
		}
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

	if (!Contexts.empty())
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Shutdown] Warning: destroying %d "
			"remaining contexts\n",
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

			Rml::RemoveContext(ContextName);
		}

		Contexts.clear();
	}

	if (bCoreInitialized)
	{
		Rml::Shutdown();
		bCoreInitialized = false;
	}

	Rml::SetRenderInterface(nullptr);
	Rml::SetFileInterface(nullptr);
	Rml::SetSystemInterface(nullptr);

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
			"[RmlUI][Runtime][Context] Create failed: "
			"runtime is not initialized\n"
		);

		return nullptr;
	}

	const char* SafeBaseName =
		BaseName && BaseName[0]
		? BaseName
		: "Context";

	Rml::String ContextName = SafeBaseName;
	ContextName += "_";
	ContextName += std::to_string(++ContextSerial);

	Rml::Vector2i SafeDimensions(
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
			"[RmlUI][Runtime][Context] Create failed: %s\n",
			ContextName.c_str()
		);

		return nullptr;
	}

	Context->EnableMouseCursor(true);

	Contexts.insert(Context);

	r3dOutToLog(
		"[RmlUI][Runtime][Context] Created: %s (%dx%d)\n",
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

	if (ActiveContext == Context)
		ActiveContext = nullptr;

	const Rml::String ContextName =
		Context->GetName();

	Contexts.erase(Context);

	Rml::RemoveContext(ContextName);

	r3dOutToLog(
		"[RmlUI][Runtime][Context] Destroyed: %s\n",
		ContextName.c_str()
	);

	Context = nullptr;
}

void RmlRuntime::SetActiveContext(
	Rml::Context* Context
)
{
	if (!Context)
	{
		ActiveContext = nullptr;
		return;
	}

	if (!IsRegisteredContext(Context))
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Input] Attempt to activate "
			"unregistered context\n"
		);

		return;
	}

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
	if (ActiveContext != Context)
		return;

	r3dOutToLog(
		"[RmlUI][Runtime][Input] Context released input: %s\n",
		Context
		? Context->GetName().c_str()
		: "null"
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

	if (!bInitialized || !Context)
		return false;

	if (Context != ActiveContext)
		return false;

	switch (Message)
	{
	case WM_MOUSEMOVE:
	{
		TRACKMOUSEEVENT TrackEvent{};
		TrackEvent.cbSize = sizeof(TrackEvent);
		TrackEvent.dwFlags = TME_LEAVE;
		TrackEvent.hwndTrack = WindowHandle;

		TrackMouseEvent(&TrackEvent);

		const int X = GET_X_LPARAM(LParam);
		const int Y = GET_Y_LPARAM(LParam);

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
	case WM_XBUTTONDOWN:
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
	case WM_XBUTTONUP:
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
			Rml::Vector2f(0.0f, Delta),
			GetKeyModifiers()
		);

		return true;
	}

	case WM_MOUSEHWHEEL:
	{
		const short Wheel =
			GET_WHEEL_DELTA_WPARAM(WParam);

		const float Delta =
			-static_cast<float>(Wheel) /
			static_cast<float>(WHEEL_DELTA);

		Context->ProcessMouseWheel(
			Rml::Vector2f(Delta, 0.0f),
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

		if (Key != Rml::Input::KI_UNKNOWN)
		{
			Context->ProcessKeyDown(
				Key,
				GetKeyModifiers()
			);

			return true;
		}

		return false;
	}

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		const Rml::Input::KeyIdentifier Key =
			TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
		{
			Context->ProcessKeyUp(
				Key,
				GetKeyModifiers()
			);

			return true;
		}

		return false;
	}

	case WM_CHAR:
	{
		if (WParam >= 32)
		{
			Context->ProcessTextInput(
				static_cast<Rml::Character>(WParam)
			);
		}

		return true;
	}

	case WM_KILLFOCUS:
		Context->ProcessMouseLeave();
		return false;

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

	if (bRenderFrameOpen)
	{
		r3dOutToLog(
			"[RmlUI][Runtime][Render] Nested render rejected\n"
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

void RmlRuntime::OnDeviceLost()
{
	if (!bInitialized || !RenderInterface)
		return;

	bRenderFrameOpen = false;

	RenderInterface->OnDeviceLost();

	r3dOutToLog(
		"[RmlUI][Runtime][Render] Device lost\n"
	);
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

	r3dOutToLog(
		"[RmlUI][Runtime][Render] Device reset: %dx%d\n",
		Width,
		Height
	);
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

	std::wstring Path = ModulePath;

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
			Rml::Input::KI_A + (WParam - 'A')
		);
	}

	if (WParam >= '0' && WParam <= '9')
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_0 + (WParam - '0')
		);
	}

	if (WParam >= VK_F1 && WParam <= VK_F12)
	{
		return static_cast<Rml::Input::KeyIdentifier>(
			Rml::Input::KI_F1 + (WParam - VK_F1)
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

	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		return 3;

	default:
		return 0;
	}
}