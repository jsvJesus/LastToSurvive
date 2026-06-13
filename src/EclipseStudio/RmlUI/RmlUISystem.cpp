#include "r3dPCH.h"
#include "r3d.h"

#include "RmlUISystem.h"

#include <RmlUi/Core/Element.h>

#include <windowsx.h>
#include <string>
#include <cstring>

RmlUISystem::FAppSelectClickListener::FAppSelectClickListener(RmlUISystem* InOwner)
	: Owner(InOwner)
{
}

void RmlUISystem::FAppSelectClickListener::ProcessEvent(Rml::Event& Event)
{
	if (!Owner || !Owner->AppSelectCallback)
		return;

	Rml::Element* Element = Event.GetTargetElement();

	if (!Element)
		return;

	const Rml::String& Id = Element->GetId();

	if (Id == "btn_game_public")
		Owner->AppSelectCallback("game_public");
	else if (Id == "btn_game_dev")
		Owner->AppSelectCallback("game_dev");
	else if (Id == "btn_level_editor")
		Owner->AppSelectCallback("level_editor");
	else if (Id == "btn_particle_editor")
		Owner->AppSelectCallback("particle_editor");
	else if (Id == "btn_physics_editor")
		Owner->AppSelectCallback("physics_editor");
	else if (Id == "btn_character_editor")
		Owner->AppSelectCallback("character_editor");
}

void RmlUISystem::FAppSelectClickListener::OnDetach(Rml::Element* Element)
{
	(void)Element;
}

RmlUISystem::RmlUISystem()
{
}

RmlUISystem::~RmlUISystem()
{
	Shutdown();
}

std::wstring RmlUISystem::GetStudioDataRoot()
{
	wchar_t ModulePath[MAX_PATH]{};
	GetModuleFileNameW(nullptr, ModulePath, MAX_PATH);

	std::wstring Path = ModulePath;

	const size_t Slash = Path.find_last_of(L"\\/");
	if (Slash != std::wstring::npos)
		Path.resize(Slash);

	return Path + L"\\Data";
}

bool RmlUISystem::Init(HWND InHwnd, IDirect3DDevice9* InDevice)
{
	if (bInitialized)
		return true;

	if (!InHwnd || !InDevice)
	{
		OutputDebugStringA("[RmlUI] Init failed: invalid HWND or D3D9 device\n");
		return false;
	}

	Hwnd = InHwnd;
	UpdateClientSize();

	const std::wstring DataRoot = GetStudioDataRoot();

	SystemInterface = std::make_unique<RmlSystemInterface>();
	FileInterface = std::make_unique<RmlFileInterface>(DataRoot.c_str());
	RenderInterface = std::make_unique<RmlRenderDX9>();

	if (!RenderInterface->Init(InDevice, DataRoot.c_str()))
	{
		OutputDebugStringA("[RmlUI] Init failed: DX9 render interface failed\n");
		Shutdown();
		return false;
	}

	Rml::SetSystemInterface(SystemInterface.get());
	Rml::SetFileInterface(FileInterface.get());
	Rml::SetRenderInterface(RenderInterface.get());

	if (!Rml::Initialise())
	{
		OutputDebugStringA("[RmlUI] Rml::Initialise failed\n");
		Shutdown();
		return false;
	}

	bCoreInitializedHere = true;

	Rml::LoadFontFace("Rml/Fonts/NotoSans-Regular.ttf");
	Rml::LoadFontFace("Rml/Fonts/Roboto-Regular.ttf");
	Rml::LoadFontFace("C:/Windows/Fonts/arial.ttf");

	Context = Rml::CreateContext("Studio", Rml::Vector2i(ClientWidth, ClientHeight));

	if (!Context)
	{
		OutputDebugStringA("[RmlUI] CreateContext failed\n");
		Shutdown();
		return false;
	}

	Context->EnableMouseCursor(true);

	AppSelectClickListener = std::make_unique<FAppSelectClickListener>(this);

	bInitialized = true;

	if (!LoadAppSelect())
	{
		OutputDebugStringA("[RmlUI] AppSelect load failed, fallback to old Scaleform UI\n");
	}

	OutputDebugStringA("[RmlUI] Initialized\n");
	return true;
}

void RmlUISystem::Shutdown()
{
	if (Context)
	{
		if (AppSelectDocument)
		{
			DetachAppSelectEvents();
			Context->UnloadDocument(AppSelectDocument);
			AppSelectDocument = nullptr;
		}

		Rml::RemoveContext(Context->GetName());
		Context = nullptr;
	}

	if (bCoreInitializedHere)
	{
		Rml::Shutdown();
		bCoreInitializedHere = false;
	}

	if (RenderInterface)
	{
		RenderInterface->Shutdown();
		RenderInterface.reset();
	}

	FileInterface.reset();
	SystemInterface.reset();
	AppSelectClickListener.reset();

	Hwnd = nullptr;
	bInitialized = false;
	bAppSelectVisible = false;

	OutputDebugStringA("[RmlUI] Shutdown\n");
}

void RmlUISystem::UpdateClientSize()
{
	if (!Hwnd)
	{
		ClientWidth = 1;
		ClientHeight = 1;
		return;
	}

	RECT Rc{};
	GetClientRect(Hwnd, &Rc);

	ClientWidth = (int)(Rc.right - Rc.left);
	ClientHeight = (int)(Rc.bottom - Rc.top);

	if (ClientWidth < 1)
		ClientWidth = 1;

	if (ClientHeight < 1)
		ClientHeight = 1;

	if (Context)
		Context->SetDimensions(Rml::Vector2i(ClientWidth, ClientHeight));
}

bool RmlUISystem::LoadAppSelect()
{
	if (!bInitialized || !Context)
		return false;

	if (AppSelectDocument)
		return true;

	AppSelectDocument = Context->LoadDocument("Rml/Studio/AppSelect.rml");

	if (!AppSelectDocument)
	{
		OutputDebugStringA("[RmlUI] Failed to load Data/Rml/Studio/AppSelect.rml\n");
		return false;
	}

	AttachAppSelectEvents();

	AppSelectDocument->Show();
	bAppSelectVisible = true;

	OutputDebugStringA("[RmlUI] AppSelect loaded\n");
	return true;
}

void RmlUISystem::AttachAppSelectEvents()
{
	if (!AppSelectDocument || !AppSelectClickListener)
		return;

	const char* ButtonIds[] =
	{
		"btn_game_public",
		"btn_game_dev",
		"btn_level_editor",
		"btn_particle_editor",
		"btn_physics_editor",
		"btn_character_editor"
	};

	for (const char* Id : ButtonIds)
	{
		Rml::Element* Element = AppSelectDocument->GetElementById(Id);

		if (Element)
			Element->AddEventListener("click", AppSelectClickListener.get());
		else
		{
			std::string Text = "[RmlUI] AppSelect button missing: ";
			Text += Id;
			Text += "\n";
			OutputDebugStringA(Text.c_str());
		}
	}
}

void RmlUISystem::DetachAppSelectEvents()
{
	if (!AppSelectDocument || !AppSelectClickListener)
		return;

	const char* ButtonIds[] =
	{
		"btn_game_public",
		"btn_game_dev",
		"btn_level_editor",
		"btn_particle_editor",
		"btn_physics_editor",
		"btn_character_editor"
	};

	for (const char* Id : ButtonIds)
	{
		Rml::Element* Element = AppSelectDocument->GetElementById(Id);

		if (Element)
			Element->RemoveEventListener("click", AppSelectClickListener.get());
	}
}

void RmlUISystem::ShowAppSelect()
{
	if (!LoadAppSelect())
		return;

	if (AppSelectDocument)
	{
		AppSelectDocument->Show();
		bAppSelectVisible = true;
	}
}

void RmlUISystem::HideAppSelect()
{
	if (AppSelectDocument)
		AppSelectDocument->Hide();

	bAppSelectVisible = false;
}

void RmlUISystem::Update(float DeltaSeconds)
{
	(void)DeltaSeconds;

	if (!bInitialized || !Context)
		return;

	UpdateClientSize();
	Context->Update();
}

void RmlUISystem::Render()
{
	if (!bInitialized || !Context || !RenderInterface)
		return;

	if (!bAppSelectVisible || !AppSelectDocument)
		return;

	RenderInterface->BeginFrame(ClientWidth, ClientHeight);
	Context->Render();
	RenderInterface->EndFrame();
}

void RmlUISystem::OnDeviceLost()
{
	if (RenderInterface)
		RenderInterface->OnDeviceLost();
}

void RmlUISystem::OnDeviceReset()
{
	UpdateClientSize();

	if (RenderInterface)
		RenderInterface->OnDeviceReset(ClientWidth, ClientHeight);
}

void RmlUISystem::SetAppSelectCallback(FAppSelectCallback Callback)
{
	AppSelectCallback = std::move(Callback);
}

bool RmlUISystem::IsInitialized() const
{
	return bInitialized;
}

bool RmlUISystem::IsAppSelectReady() const
{
	return bInitialized && AppSelectDocument != nullptr;
}

bool RmlUISystem::IsAppSelectVisible() const
{
	return bInitialized && AppSelectDocument != nullptr && bAppSelectVisible;
}

int RmlUISystem::GetKeyModifiers()
{
	int Modifiers = 0;

	if (GetKeyState(VK_CONTROL) & 0x8000)
		Modifiers |= Rml::Input::KM_CTRL;

	if (GetKeyState(VK_SHIFT) & 0x8000)
		Modifiers |= Rml::Input::KM_SHIFT;

	if (GetKeyState(VK_MENU) & 0x8000)
		Modifiers |= Rml::Input::KM_ALT;

	if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0)
		Modifiers |= Rml::Input::KM_CAPSLOCK;

	if ((GetKeyState(VK_NUMLOCK) & 0x0001) != 0)
		Modifiers |= Rml::Input::KM_NUMLOCK;

	if ((GetKeyState(VK_SCROLL) & 0x0001) != 0)
		Modifiers |= Rml::Input::KM_SCROLLLOCK;

	return Modifiers;
}

Rml::Input::KeyIdentifier RmlUISystem::TranslateKey(WPARAM WParam)
{
	if (WParam >= 'A' && WParam <= 'Z')
		return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_A + (WParam - 'A'));

	if (WParam >= '0' && WParam <= '9')
		return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_0 + (WParam - '0'));

	if (WParam >= VK_F1 && WParam <= VK_F12)
		return static_cast<Rml::Input::KeyIdentifier>(Rml::Input::KI_F1 + (WParam - VK_F1));

	switch (WParam)
	{
	case VK_SPACE: return Rml::Input::KI_SPACE;
	case VK_BACK: return Rml::Input::KI_BACK;
	case VK_TAB: return Rml::Input::KI_TAB;
	case VK_RETURN: return Rml::Input::KI_RETURN;
	case VK_ESCAPE: return Rml::Input::KI_ESCAPE;
	case VK_PRIOR: return Rml::Input::KI_PRIOR;
	case VK_NEXT: return Rml::Input::KI_NEXT;
	case VK_END: return Rml::Input::KI_END;
	case VK_HOME: return Rml::Input::KI_HOME;
	case VK_LEFT: return Rml::Input::KI_LEFT;
	case VK_UP: return Rml::Input::KI_UP;
	case VK_RIGHT: return Rml::Input::KI_RIGHT;
	case VK_DOWN: return Rml::Input::KI_DOWN;
	case VK_INSERT: return Rml::Input::KI_INSERT;
	case VK_DELETE: return Rml::Input::KI_DELETE;
	case VK_SHIFT: return Rml::Input::KI_LSHIFT;
	case VK_CONTROL: return Rml::Input::KI_LCONTROL;
	case VK_MENU: return Rml::Input::KI_LMENU;
	case VK_OEM_PLUS: return Rml::Input::KI_OEM_PLUS;
	case VK_OEM_MINUS: return Rml::Input::KI_OEM_MINUS;
	case VK_OEM_COMMA: return Rml::Input::KI_OEM_COMMA;
	case VK_OEM_PERIOD: return Rml::Input::KI_OEM_PERIOD;
	default: break;
	}

	return Rml::Input::KI_UNKNOWN;
}

int RmlUISystem::TranslateMouseButton(UINT Message)
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

bool RmlUISystem::ProcessWin32Message(HWND InHwnd, UINT Message, WPARAM WParam, LPARAM LParam, LRESULT* OutResult)
{
	if (OutResult)
		*OutResult = 0;

	if (!bInitialized || !Context || !bAppSelectVisible)
		return false;

	switch (Message)
	{
	case WM_SIZE:
		UpdateClientSize();
		return false;

	case WM_MOUSEMOVE:
		{
			const int X = GET_X_LPARAM(LParam);
			const int Y = GET_Y_LPARAM(LParam);

			Context->ProcessMouseMove(X, Y, GetKeyModifiers());
			return true;
		}

	case WM_MOUSELEAVE:
	{
		Context->ProcessMouseLeave();
		return false;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	{
		SetCapture(InHwnd);

		const int Button = TranslateMouseButton(Message);
		Context->ProcessMouseButtonDown(Button, GetKeyModifiers());

		return true;
	}

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		ReleaseCapture();

		const int Button = TranslateMouseButton(Message);
		Context->ProcessMouseButtonUp(Button, GetKeyModifiers());

		return true;
	}

	case WM_MOUSEWHEEL:
	{
		const short Wheel = GET_WHEEL_DELTA_WPARAM(WParam);
		const float Delta = -static_cast<float>(Wheel) / static_cast<float>(WHEEL_DELTA);

		Context->ProcessMouseWheel(Rml::Vector2f(0.0f, Delta), GetKeyModifiers());
		return true;
	}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		const Rml::Input::KeyIdentifier Key = TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
			Context->ProcessKeyDown(Key, GetKeyModifiers());

		return true;
	}

	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		const Rml::Input::KeyIdentifier Key = TranslateKey(WParam);

		if (Key != Rml::Input::KI_UNKNOWN)
			Context->ProcessKeyUp(Key, GetKeyModifiers());

		return true;
	}

	case WM_CHAR:
	{
		if (WParam >= 32)
			Context->ProcessTextInput(static_cast<Rml::Character>(WParam));

		return true;
	}

	default:
		break;
	}

	return false;
}