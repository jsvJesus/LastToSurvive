#include "r3dPCH.h"
#include "r3d.h"

#include "RmlUISystem.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>

#include <windowsx.h>
#include <string>
#include <cstring>
#include <cstdio>

namespace
{
	Rml::String WideToUtf8(const wchar_t* Text)
	{
		if (!Text || !Text[0])
			return Rml::String();

		const int Required = WideCharToMultiByte(CP_UTF8, 0, Text, -1, nullptr, 0, nullptr, nullptr);
		if (Required <= 1)
			return Rml::String();

		Rml::String Result;
		Result.resize(Required);

		WideCharToMultiByte(CP_UTF8, 0, Text, -1, &Result[0], Required, nullptr, nullptr);
		Result.resize(Required - 1);
		return Result;
	}

	Rml::String EscapeRmlText(const Rml::String& Text)
	{
		Rml::String Result;
		Result.reserve(Text.size());

		for (char Ch : Text)
		{
			switch (Ch)
			{
			case '&': Result += "&amp;"; break;
			case '<': Result += "&lt;"; break;
			case '>': Result += "&gt;"; break;
			default: Result += Ch; break;
			}
		}

		return Result;
	}

	void SetElementText(Rml::ElementDocument* Document, const char* ElementId, const Rml::String& Text)
	{
		if (!Document || !ElementId)
			return;

		Rml::Element* Element = Document->GetElementById(ElementId);
		if (Element)
			Element->SetInnerRML(EscapeRmlText(Text));
	}
}

RmlUISystem::FAppSelectClickListener::FAppSelectClickListener(RmlUISystem* InOwner)
	: Owner(InOwner)
{
}

void RmlUISystem::FAppSelectClickListener::ProcessEvent(Rml::Event& Event)
{
	if (!Owner || !Owner->AppSelectCallback)
		return;

	Rml::Element* Element = Event.GetCurrentElement();

	if (!Element)
		Element = Event.GetTargetElement();

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
	else if (Id == "btn_exit")
		Owner->AppSelectCallback("exit");
}

void RmlUISystem::FAppSelectClickListener::OnDetach(Rml::Element* Element)
{
	(void)Element;
}

RmlUISystem::FAppMainClickListener::FAppMainClickListener(RmlUISystem* InOwner)
	: Owner(InOwner)
{
}

void RmlUISystem::FAppMainClickListener::ProcessEvent(Rml::Event& Event)
{
	if (!Owner || !Owner->AppMainCallback)
		return;

	Rml::Element* Element = Event.GetCurrentElement();

	if (!Element)
		Element = Event.GetTargetElement();

	if (!Element)
		return;

	const Rml::String& Id = Element->GetId();

	if (Id == "btn_appmain_live_maps")
	{
		Owner->AppMainCallback("tab", "0");
	}
	else if (Id == "btn_appmain_editor_maps")
	{
		Owner->AppMainCallback("tab", "1");
	}
	else if (Id == "btn_appmain_create_map")
	{
		Owner->AppMainCallback("tab", "2");
	}
	else if (Id == "btn_appmain_load_level")
	{
		Owner->AppMainCallback("load", "");
	}
	else if (Id == "btn_appmain_create_level")
	{
		Rml::String Name = Owner->GetAppMainCreateLevelName();
		Owner->AppMainCallback("create", Name.c_str());
	}
	else if (Id == "btn_appmain_back")
	{
		Owner->AppMainCallback("back", "");
	}
	else if (Id == "btn_appmain_exit")
	{
		Owner->AppMainCallback("exit", "");
	}
	else if (Id == "btn_appmain_terrain_toggle")
	{
		Owner->AppMainCallback("terrain_toggle", "");
	}
	else if (Id == "btn_appmain_terrain2_toggle")
	{
		Owner->AppMainCallback("terrain2_toggle", "");
	}
	else if (Id == "btn_appmain_terrain_size")
	{
		Owner->AppMainCallback("terrain_size", "");
	}
	else if (Id == "btn_appmain_splat_size")
	{
		Owner->AppMainCallback("splat_size", "");
	}
	else if (Id.compare(0, 12, "appmain_map_") == 0)
	{
		Owner->AppMainCallback("select", Id.c_str() + 12);
	}
}

void RmlUISystem::FAppMainClickListener::OnDetach(Rml::Element* Element)
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

bool RmlUISystem::Init(HWND InHwnd, IDirect3DDevice9* InDevice, bool bLoadAppSelectOnInit)
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
	AppMainClickListener = std::make_unique<FAppMainClickListener>(this);

	bInitialized = true;

	if (bLoadAppSelectOnInit && !LoadAppSelect())
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

		if (LoadingScreenDocument)
		{
			Context->UnloadDocument(LoadingScreenDocument);
			LoadingScreenDocument = nullptr;
		}

		if (AppMainDocument)
		{
			DetachAppMainEvents();
			Context->UnloadDocument(AppMainDocument);
			AppMainDocument = nullptr;
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
	bAppMainVisible = false;
	bLoadingScreenVisible = false;

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
		"btn_character_editor",
		"btn_exit"
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
		"btn_character_editor",
		"btn_exit"
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

bool RmlUISystem::LoadLoadingScreen()
{
	if (!bInitialized || !Context)
		return false;

	if (LoadingScreenDocument)
		return true;

	LoadingScreenDocument = Context->LoadDocument("Rml/Studio/LoadingScreen.rml");

	if (!LoadingScreenDocument)
	{
		OutputDebugStringA("[RmlUI] Failed to load Data/Rml/Studio/LoadingScreen.rml\n");
		return false;
	}

	LoadingScreenDocument->Hide();
	bLoadingScreenVisible = false;

	OutputDebugStringA("[RmlUI] LoadingScreen loaded\n");
	return true;
}

void RmlUISystem::ShowLoadingScreen()
{
	if (!LoadLoadingScreen())
		return;

	if (LoadingScreenDocument)
	{
		LoadingScreenDocument->Show();
		bLoadingScreenVisible = true;
		ApplyLoadingScreenProgress(LoadingProgressVisual);
	}
}

void RmlUISystem::HideLoadingScreen()
{
	if (LoadingScreenDocument)
		LoadingScreenDocument->Hide();

	bLoadingScreenVisible = false;
}

void RmlUISystem::SetLoadingScreenData(const wchar_t* Name, const wchar_t* Description, const wchar_t* Tip)
{
	if (!LoadLoadingScreen())
		return;

	SetElementText(LoadingScreenDocument, "loading_title", WideToUtf8(Name ? Name : L"Loading"));
	SetElementText(LoadingScreenDocument, "loading_description", WideToUtf8(Description ? Description : L""));
	SetElementText(LoadingScreenDocument, "loading_tip", WideToUtf8(Tip ? Tip : L""));
}

void RmlUISystem::SetLoadingScreenProgress(float Progress)
{
	if (!LoadLoadingScreen())
		return;

	if (Progress < 0.0f)
		Progress = 0.0f;
	else if (Progress > 1.0f)
		Progress = 1.0f;

	LoadingProgressTarget = Progress;
}

void RmlUISystem::ApplyLoadingScreenProgress(float Progress)
{
	if (!LoadingScreenDocument)
		return;

	if (Progress < 0.0f)
		Progress = 0.0f;
	else if (Progress > 1.0f)
		Progress = 1.0f;

	char PercentText[32] = {};
	sprintf_s(PercentText, "%.0f%%", Progress * 100.0f);
	SetElementText(LoadingScreenDocument, "loading_percent", PercentText);

	Rml::Element* Bar = LoadingScreenDocument->GetElementById("loading_bar_fill");
	if (Bar)
	{
		char WidthText[32] = {};
		sprintf_s(WidthText, "%.2f%%", Progress * 100.0f);
		Bar->SetProperty("width", WidthText);
	}
}

void RmlUISystem::Update(float DeltaSeconds)
{
	if (!bInitialized || !Context)
		return;

	UpdateClientSize();

	if (bLoadingScreenVisible && LoadingScreenDocument)
	{
		if (DeltaSeconds <= 0.0f)
			DeltaSeconds = 0.016f;

		const float Speed = 8.0f;
		const float Alpha = R3D_CLAMP(DeltaSeconds * Speed, 0.0f, 1.0f);

		LoadingProgressVisual += (LoadingProgressTarget - LoadingProgressVisual) * Alpha;

		if (fabsf(LoadingProgressTarget - LoadingProgressVisual) < 0.001f)
			LoadingProgressVisual = LoadingProgressTarget;

		ApplyLoadingScreenProgress(LoadingProgressVisual);
	}

	Context->Update();
}

void RmlUISystem::Render()
{
	if (!bInitialized || !Context || !RenderInterface)
		return;

	const bool bHasVisibleDocument =
		(bAppSelectVisible && AppSelectDocument) ||
		(bLoadingScreenVisible && LoadingScreenDocument) ||
		(bAppMainVisible && AppMainDocument);

	if (!bHasVisibleDocument)
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

bool RmlUISystem::IsLoadingScreenReady() const
{
	return bInitialized && LoadingScreenDocument != nullptr;
}

bool RmlUISystem::IsLoadingScreenVisible() const
{
	return bInitialized && LoadingScreenDocument != nullptr && bLoadingScreenVisible;
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

	if (!bInitialized || !Context || (!bAppSelectVisible && !bAppMainVisible))
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

bool RmlUISystem::LoadAppMain()
{
	if (!bInitialized || !Context)
		return false;

	if (AppMainDocument)
		return true;

	AppMainDocument = Context->LoadDocument("Rml/Studio/AppMain.rml");

	if (!AppMainDocument)
	{
		OutputDebugStringA("[RmlUI] Failed to load Data/Rml/Studio/AppMain.rml\n");
		return false;
	}

	AttachAppMainEvents();

	AppMainDocument->Hide();
	bAppMainVisible = false;

	OutputDebugStringA("[RmlUI] AppMain loaded\n");
	return true;
}

void RmlUISystem::ShowAppMain()
{
	if (!LoadAppMain())
		return;

	if (AppMainDocument)
	{
		AppMainDocument->Show();
		bAppMainVisible = true;
	}
}

void RmlUISystem::HideAppMain()
{
	if (AppMainDocument)
		AppMainDocument->Hide();

	bAppMainVisible = false;
}

void RmlUISystem::AttachAppMainEvents()
{
	if (!AppMainDocument || !AppMainClickListener)
		return;

	const char* ButtonIds[] =
	{
		"btn_appmain_live_maps",
		"btn_appmain_editor_maps",
		"btn_appmain_create_map",
		"btn_appmain_load_level",
		"btn_appmain_create_level",
		"btn_appmain_back",
		"btn_appmain_exit",
		"btn_appmain_terrain_toggle",
		"btn_appmain_terrain2_toggle",
		"btn_appmain_terrain_size",
		"btn_appmain_splat_size"
	};

	for (const char* Id : ButtonIds)
	{
		Rml::Element* Element = AppMainDocument->GetElementById(Id);

		if (Element)
			Element->AddEventListener("click", AppMainClickListener.get());
		else
		{
			std::string Text = "[RmlUI] AppMain button missing: ";
			Text += Id;
			Text += "\n";
			OutputDebugStringA(Text.c_str());
		}
	}
}

void RmlUISystem::DetachAppMainEvents()
{
	if (!AppMainDocument || !AppMainClickListener)
		return;

	const char* ButtonIds[] =
	{
		"btn_appmain_live_maps",
		"btn_appmain_editor_maps",
		"btn_appmain_create_map",
		"btn_appmain_load_level",
		"btn_appmain_create_level",
		"btn_appmain_quit"
	};

	for (const char* Id : ButtonIds)
	{
		Rml::Element* Element = AppMainDocument->GetElementById(Id);

		if (Element)
			Element->RemoveEventListener("click", AppMainClickListener.get());
	}
}

void RmlUISystem::SetAppMainCallback(FAppMainCallback Callback)
{
	AppMainCallback = std::move(Callback);
}

void RmlUISystem::SetAppMainTab(int TabIndex)
{
	if (!LoadAppMain())
		return;

	Rml::Element* MapsPanel = AppMainDocument->GetElementById("appmain_maps_panel");
	Rml::Element* CreatePanel = AppMainDocument->GetElementById("appmain_create_panel");

	if (MapsPanel)
		MapsPanel->SetProperty("display", TabIndex == 2 ? "none" : "block");

	if (CreatePanel)
		CreatePanel->SetProperty("display", TabIndex == 2 ? "block" : "none");

	if (TabIndex == 0)
		SetElementText(AppMainDocument, "appmain_section_title", "Live Maps");
	else if (TabIndex == 1)
		SetElementText(AppMainDocument, "appmain_section_title", "Editor Maps");
	else
		SetElementText(AppMainDocument, "appmain_section_title", "");
}

void RmlUISystem::SetAppMainMaps(const char** Names, int Count)
{
	if (!LoadAppMain())
		return;

	Rml::Element* List = AppMainDocument->GetElementById("appmain_map_list");
	if (!List)
		return;

	Rml::String RmlText;

	if (!Names || Count <= 0)
	{
		RmlText = "<div id=\"appmain_empty_text\">No maps found</div>";
		List->SetInnerRML(RmlText);
		return;
	}

	for (int i = 0; i < Count; ++i)
	{
		if (!Names[i] || !Names[i][0])
			continue;

		char IdText[64] = {};
		sprintf_s(IdText, "appmain_map_%d", i);

		RmlText += "<button id=\"";
		RmlText += IdText;
		RmlText += "\" class=\"map_item\">";
		RmlText += EscapeRmlText(Names[i]);
		RmlText += "</button>";
	}

	List->SetInnerRML(RmlText);

	for (int i = 0; i < Count; ++i)
	{
		char IdText[64] = {};
		sprintf_s(IdText, "appmain_map_%d", i);

		Rml::Element* Element = AppMainDocument->GetElementById(IdText);
		if (Element)
			Element->AddEventListener("click", AppMainClickListener.get());
	}
}

void RmlUISystem::SetAppMainSelectedLevel(const char* Name)
{
	if (!LoadAppMain())
		return;

	if (Name && Name[0])
		SetElementText(AppMainDocument, "appmain_selected_value", Name);
	else
		SetElementText(AppMainDocument, "appmain_selected_value", "No map selected");
}

Rml::String RmlUISystem::GetAppMainCreateLevelName() const
{
	if (!AppMainDocument)
		return "NewLevel";

	Rml::Element* Element = AppMainDocument->GetElementById("appmain_create_name");
	if (!Element)
		return "NewLevel";

	Rml::ElementFormControlInput* Input = dynamic_cast<Rml::ElementFormControlInput*>(Element);
	if (!Input)
		return "NewLevel";

	Rml::String Value = Input->GetValue();

	if (Value.empty())
		return "NewLevel";

	return Value;
}

bool RmlUISystem::IsAppMainReady() const
{
	return bInitialized && AppMainDocument != nullptr;
}

bool RmlUISystem::IsAppMainVisible() const
{
	return bInitialized && AppMainDocument != nullptr && bAppMainVisible;
}

static float RmlGetInputFloat(Rml::ElementDocument* Document, const char* Id, float DefaultValue)
{
	if (!Document || !Id)
		return DefaultValue;

	Rml::Element* Element = Document->GetElementById(Id);
	if (!Element)
		return DefaultValue;

	Rml::ElementFormControlInput* Input = dynamic_cast<Rml::ElementFormControlInput*>(Element);
	if (!Input)
		return DefaultValue;

	Rml::String Value = Input->GetValue();
	if (Value.empty())
		return DefaultValue;

	return (float)atof(Value.c_str());
}

static void RmlGetInputText(Rml::ElementDocument* Document, const char* Id, char* OutText, int OutTextSize, const char* DefaultValue)
{
	if (!OutText || OutTextSize <= 0)
		return;

	OutText[0] = 0;

	if (!Document || !Id)
	{
		r3dscpy(OutText, DefaultValue ? DefaultValue : "");
		return;
	}

	Rml::Element* Element = Document->GetElementById(Id);
	if (!Element)
	{
		r3dscpy(OutText, DefaultValue ? DefaultValue : "");
		return;
	}

	Rml::ElementFormControlInput* Input = dynamic_cast<Rml::ElementFormControlInput*>(Element);
	if (!Input)
	{
		r3dscpy(OutText, DefaultValue ? DefaultValue : "");
		return;
	}

	Rml::String Value = Input->GetValue();
	if (Value.empty())
		r3dscpy(OutText, DefaultValue ? DefaultValue : "");
	else
		r3dscpy(OutText, Value.c_str());
}

void RmlUISystem::SetAppMainCreateOptions(
	bool bHaveTerrain,
	bool bTerrainV2,
	int TerrainSizeIndex,
	int SplatSizeIndex,
	float CellSize,
	float Height
)
{
	if (!LoadAppMain())
		return;

	if (TerrainSizeIndex < 0)
		TerrainSizeIndex = 0;

	if (TerrainSizeIndex > 4)
		TerrainSizeIndex = 4;

	if (SplatSizeIndex < 0)
		SplatSizeIndex = 0;

	if (SplatSizeIndex > TerrainSizeIndex)
		SplatSizeIndex = TerrainSizeIndex;

	const int SizeValues[5] = { 256, 512, 1024, 2048, 4096 };

	char Text[128] = {};

	sprintf_s(Text, "Terrain: %s", bHaveTerrain ? "ON" : "OFF");
	SetElementText(AppMainDocument, "btn_appmain_terrain_toggle", Text);

	sprintf_s(Text, "Terrain V2: %s", bTerrainV2 ? "ON" : "OFF");
	SetElementText(AppMainDocument, "btn_appmain_terrain2_toggle", Text);

	sprintf_s(Text, "Terrain Size: %d", SizeValues[TerrainSizeIndex]);
	SetElementText(AppMainDocument, "btn_appmain_terrain_size", Text);

	sprintf_s(Text, "Splat Size: %d", SizeValues[SplatSizeIndex]);
	SetElementText(AppMainDocument, "btn_appmain_splat_size", Text);

	Rml::Element* Cell = AppMainDocument->GetElementById("appmain_cell_size");
	if (Cell)
	{
		sprintf_s(Text, "%.2f", CellSize);
		Cell->SetAttribute("value", Text);
	}

	Rml::Element* HeightElement = AppMainDocument->GetElementById("appmain_height");
	if (HeightElement)
	{
		sprintf_s(Text, "%.2f", Height);
		HeightElement->SetAttribute("value", Text);
	}
}

bool RmlUISystem::GetAppMainCreateData(
	char* OutName,
	int OutNameSize,
	bool& bOutHaveTerrain,
	bool& bOutTerrainV2,
	int& OutTerrainSizeIndex,
	int& OutSplatSizeIndex,
	float& OutCellSize,
	float& OutHeight
) const
{
	if (!AppMainDocument)
		return false;

	RmlGetInputText(AppMainDocument, "appmain_create_name", OutName, OutNameSize, "NewLevel");

	bOutHaveTerrain = false;
	bOutTerrainV2 = false;

	OutTerrainSizeIndex = 0;
	OutSplatSizeIndex = 0;

	OutCellSize = RmlGetInputFloat(AppMainDocument, "appmain_cell_size", 1.0f);
	OutHeight = RmlGetInputFloat(AppMainDocument, "appmain_height", 100.0f);

	return true;
}