#include "r3dPCH.h"
#include "r3d.h"

#include "RmlUISystem.h"
#include "RmlRuntime.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Traits.h>
#include <RmlUi/Debugger.h>

#include <windowsx.h>
#include <algorithm>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>

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

	Rml::Element* CurrentElement = Event.GetCurrentElement();
	Rml::Element* TargetElement = Event.GetTargetElement();

	const char* MapPrefix = "appmain_map_";
	const size_t MapPrefixLen = strlen(MapPrefix);

	Rml::Element* SearchElement = TargetElement;

	while (SearchElement)
	{
		const Rml::String& SearchId = SearchElement->GetId();

		if (SearchId.compare(0, MapPrefixLen, MapPrefix) == 0)
		{
			Owner->AppMainCallback("select", SearchId.c_str() + MapPrefixLen);
			return;
		}

		if (SearchElement == CurrentElement)
			break;

		SearchElement = SearchElement->GetParentNode();
	}

	Rml::Element* Element = CurrentElement;

	if (!Element)
		Element = TargetElement;

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
	else if (Id == "btn_appmain_scroll_up")
	{
		Owner->AppMainCallback("scroll_up", "");
	}
	else if (Id == "btn_appmain_scroll_down")
	{
		Owner->AppMainCallback("scroll_down", "");
	}
}

void RmlUISystem::FAppMainClickListener::OnDetach(Rml::Element* Element)
{
	(void)Element;
}

RmlUISystem::FCharacterClickListener::
FCharacterClickListener(
	RmlUISystem* InOwner
)
	: Owner(InOwner)
{
}

void RmlUISystem::FCharacterClickListener::
ProcessEvent(
	Rml::Event& Event
)
{
	if (!Owner || !Owner->CharacterCallback)
		return;

	Rml::Element* Element =
		Event.GetTargetElement();

	while (Element)
	{
		const Rml::String& Id =
			Element->GetId();

		const char* StatePrefix =
			"btn_char_state_";

		const char* DirectionPrefix =
			"btn_char_direction_";

		const char* AnimationPrefix =
			"char_anim_";

		if (
			Id.compare(
				0,
				strlen(StatePrefix),
				StatePrefix
			) == 0
		)
		{
			Owner->CharacterCallback(
				"state",
				Id.c_str() + strlen(StatePrefix)
			);

			return;
		}

		const char* EquipmentCategoryPrefix =
		"btn_char_equipment_category_";

		const char* EquipmentItemPrefix =
			"char_equipment_item_";

		if (
			Id.compare(
				0,
				strlen(EquipmentCategoryPrefix),
				EquipmentCategoryPrefix
			) == 0
		)
		{
			Owner->CharacterCallback(
				"equipment_category",
				Id.c_str() +
					strlen(EquipmentCategoryPrefix)
			);

			return;
		}

		if (
			Id.compare(
				0,
				strlen(EquipmentItemPrefix),
				EquipmentItemPrefix
			) == 0
		)
		{
			Owner->CharacterCallback(
				"equipment_item",
				Id.c_str() +
					strlen(EquipmentItemPrefix)
			);

			return;
		}

		if (Id == "btn_char_equipment_close")
		{
			Owner->CharacterCallback(
				"show_equipment",
				""
			);

			return;
		}

		if (
			Id.compare(
				0,
				strlen(DirectionPrefix),
				DirectionPrefix
			) == 0
		)
		{
			Owner->CharacterCallback(
				"direction",
				Id.c_str() + strlen(DirectionPrefix)
			);

			return;
		}

		if (
			Id.compare(
				0,
				strlen(AnimationPrefix),
				AnimationPrefix
			) == 0
		)
		{
			Owner->CharacterCallback(
				"animation",
				Id.c_str() + strlen(AnimationPrefix)
			);

			return;
		}

		if (Id == "btn_char_mode_states")
		{
			Owner->CharacterCallback(
				"mode",
				"0"
			);

			return;
		}

		if (Id == "btn_char_mode_all_anims")
		{
			Owner->CharacterCallback(
				"mode",
				"1"
			);

			return;
		}

		if (Id == "btn_char_ui_idle")
		{
			Owner->CharacterCallback(
				"ui_idle",
				""
			);

			return;
		}

		if (Id == "btn_char_reload")
		{
			Owner->CharacterCallback(
				"reload",
				""
			);

			return;
		}

		if (Id == "btn_char_shoot")
		{
			Owner->CharacterCallback(
				"shoot",
				""
			);

			return;
		}

		if (Id == "btn_char_jump")
		{
			Owner->CharacterCallback(
				"jump",
				""
			);

			return;
		}

		if (Id == "btn_char_jump_anim")
		{
			Owner->CharacterCallback(
				"jump_anim",
				""
			);

			return;
		}

		if (Id == "btn_char_in_air")
		{
			Owner->CharacterCallback(
				"in_air",
				""
			);

			return;
		}

		if (Id == "btn_char_play_pause")
		{
			Owner->CharacterCallback(
				"play_pause",
				""
			);

			return;
		}

		if (Id == "btn_char_stop")
		{
			Owner->CharacterCallback(
				"stop",
				""
			);

			return;
		}

		if (Id == "btn_char_default_speed")
		{
			Owner->CharacterCallback(
				"default_speed",
				""
			);

			return;
		}

		if (Id == "btn_char_show_skeleton")
		{
			Owner->CharacterCallback(
				"show_skeleton",
				""
			);

			return;
		}

		if (Id == "btn_char_show_anim_stack")
		{
			Owner->CharacterCallback(
				"show_anim_stack",
				""
			);

			return;
		}

		if (Id == "btn_char_show_equipment")
		{
			Owner->CharacterCallback(
				"show_equipment",
				""
			);

			return;
		}

		if (
			Element ==
			Owner->CharacterEditorDocument
		)
		{
			break;
		}

		Element = Element->GetParentNode();
	}
}

void RmlUISystem::FCharacterClickListener::
OnDetach(
	Rml::Element* Element
)
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

bool RmlUISystem::Init(
	HWND InHwnd,
	IDirect3DDevice9* InDevice,
	bool bLoadAppSelectOnInit
)
{
	if (bInitialized)
		return true;

	if (!InHwnd || !InDevice)
	{
		OutputDebugStringA(
			"[RmlUI] Init failed: invalid HWND or D3D9 device\n"
		);

		return false;
	}

	Hwnd = InHwnd;

	UpdateClientSize();

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	if (Runtime.IsUsingDX11())
	{
		static bool bLoggedDX11RuntimeSkip = false;
		if (!bLoggedDX11RuntimeSkip)
		{
			r3dOutToLog(
				"[RmlUI] DX9 init skipped: runtime is already using DX11\n"
			);
			bLoggedDX11RuntimeSkip = true;
		}

		Hwnd = nullptr;
		return false;
	}

	if (!Runtime.Acquire(
		InHwnd,
		InDevice
	))
	{
		OutputDebugStringA(
			"[RmlUI] Shared runtime initialization failed\n"
		);

		Hwnd = nullptr;
		return false;
	}

	bCoreInitializedHere = true;

	Context = Runtime.CreateContext(
		"Studio",
		Rml::Vector2i(
			ClientWidth,
			ClientHeight
		)
	);

	if (!Context)
	{
		Runtime.Release();

		bCoreInitializedHere = false;
		Hwnd = nullptr;

		OutputDebugStringA(
			"[RmlUI] CreateContext failed\n"
		);

		return false;
	}

	Context->EnableMouseCursor(true);

	bDebuggerInitialized =
		Runtime.EnsureDebugger(Context);

	if (bDebuggerInitialized)
	{
		OutputDebugStringA(
			"[RmlUI] Debugger initialized. Press F10 to toggle.\n"
		);
	}
	else
	{
		OutputDebugStringA(
			"[RmlUI] Debugger initialization failed\n"
		);
	}

	AppSelectClickListener =
		std::make_unique<FAppSelectClickListener>(
			this
		);

	AppMainClickListener =
		std::make_unique<FAppMainClickListener>(
			this
		);

	CharacterClickListener =
		std::make_unique<FCharacterClickListener>(
			this
		);

	bInitialized = true;

	if (
		bLoadAppSelectOnInit &&
		!LoadAppSelect()
	)
	{
		OutputDebugStringA(
			"[RmlUI] AppSelect load failed, fallback to old UI\n"
		);
	}

	OutputDebugStringA(
		"[RmlUI] Initialized through shared runtime\n"
	);

	return true;
}

bool RmlUISystem::Init(
	HWND InHwnd,
	ID3D11Device* InDevice,
	ID3D11DeviceContext* InContext,
	bool bLoadAppSelectOnInit
)
{
	if (bInitialized)
		return true;

	if (!InHwnd || !InDevice || !InContext)
	{
		OutputDebugStringA(
			"[RmlUI] Init DX11 failed: invalid HWND/device/context\n"
		);

		return false;
	}

	Hwnd = InHwnd;

	UpdateClientSize();

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	if (Runtime.IsUsingDX9())
	{
		r3dOutToLog(
			"[RmlUI] DX11 init failed: runtime is already using DX9\n"
		);

		Hwnd = nullptr;
		return false;
	}

	if (!Runtime.Acquire(
		InHwnd,
		InDevice,
		InContext
	))
	{
		OutputDebugStringA(
			"[RmlUI] Shared DX11 runtime initialization failed\n"
		);

		Hwnd = nullptr;
		return false;
	}

	bCoreInitializedHere = true;

	Context = Runtime.CreateContext(
		"Studio",
		Rml::Vector2i(
			ClientWidth,
			ClientHeight
		)
	);

	if (!Context)
	{
		Runtime.Release();

		bCoreInitializedHere = false;
		Hwnd = nullptr;

		OutputDebugStringA(
			"[RmlUI] CreateContext DX11 failed\n"
		);

		return false;
	}

	Context->EnableMouseCursor(true);

	bDebuggerInitialized =
		Runtime.EnsureDebugger(Context);

	AppSelectClickListener =
		std::make_unique<FAppSelectClickListener>(
			this
		);

	AppMainClickListener =
		std::make_unique<FAppMainClickListener>(
			this
		);

	CharacterClickListener =
		std::make_unique<FCharacterClickListener>(
			this
		);

	bInitialized = true;

	if (
		bLoadAppSelectOnInit &&
		!LoadAppSelect()
	)
	{
		OutputDebugStringA(
			"[RmlUI] AppSelect DX11 load failed, fallback to old UI\n"
		);
	}

	OutputDebugStringA(
		"[RmlUI] Initialized through shared DX11 runtime\n"
	);

	return true;
}

void RmlUISystem::Shutdown()
{
	RmlRuntime& Runtime =
		RmlRuntime::Get();

	Runtime.ClearActiveContext(
		Context
	);

	if (Context)
	{
		if (AppSelectDocument)
		{
			DetachAppSelectEvents();

			Context->UnloadDocument(
				AppSelectDocument
			);

			AppSelectDocument = nullptr;
		}

		if (LoadingScreenDocument)
		{
			Context->UnloadDocument(
				LoadingScreenDocument
			);

			LoadingScreenDocument = nullptr;
		}

		if (AppMainDocument)
		{
			DetachAppMainEvents();

			Context->UnloadDocument(
				AppMainDocument
			);

			AppMainDocument = nullptr;
		}

		if (CharacterEditorDocument)
		{
			DetachCharacterEvents();

			Context->UnloadDocument(
				CharacterEditorDocument
			);

			CharacterEditorDocument = nullptr;
		}

		SelectedLiveElement = nullptr;

		Runtime.DestroyContext(
			Context
		);
	}

	if (bCoreInitializedHere)
	{
		Runtime.Release();
		bCoreInitializedHere = false;
	}

	FileInterface.reset();
	SystemInterface.reset();

	AppSelectClickListener.reset();
	AppMainClickListener.reset();
	CharacterClickListener.reset();

	CharacterCallback = nullptr;

	Hwnd = nullptr;

	bInitialized = false;
	bAppSelectVisible = false;
	bAppMainVisible = false;
	bLoadingScreenVisible = false;
	bCharacterEditorVisible = false;
	bDebuggerInitialized = false;
	bLiveEditorVisible = false;

	SelectedLiveElement = nullptr;

	OutputDebugStringA(
		"[RmlUI] Shutdown\n"
	);
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

	RmlRuntime::Get().SetActiveContext(
			Context
		);

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

	if (
		!bAppMainVisible &&
		!bCharacterEditorVisible &&
		!bLiveEditorVisible &&
		!IsDebuggerVisible()
	)
	{
		RmlRuntime::Get().ClearActiveContext(
			Context
		);
	}
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

void RmlUISystem::Update(
	float DeltaSeconds
)
{
	if (!bInitialized || !Context)
		return;

	UpdateClientSize();

	const bool bWantsInput =
		bAppSelectVisible ||
		bAppMainVisible ||
		bCharacterEditorVisible ||
		bLiveEditorVisible ||
		IsDebuggerVisible();

	if (bWantsInput)
	{
		RmlRuntime::Get().SetActiveContext(
			Context
		);
	}
	else
	{
		RmlRuntime::Get().ClearActiveContext(
			Context
		);
	}

	if (
		bLoadingScreenVisible &&
		LoadingScreenDocument
	)
	{
		if (DeltaSeconds <= 0.0f)
			DeltaSeconds = 0.016f;

		const float Speed = 8.0f;

		const float Alpha =
			R3D_CLAMP(
				DeltaSeconds * Speed,
				0.0f,
				1.0f
			);

		LoadingProgressVisual +=
			(
				LoadingProgressTarget -
				LoadingProgressVisual
			) * Alpha;

		if (
			fabsf(
				LoadingProgressTarget -
				LoadingProgressVisual
			) < 0.001f
		)
		{
			LoadingProgressVisual =
				LoadingProgressTarget;
		}

		ApplyLoadingScreenProgress(
			LoadingProgressVisual
		);
	}

	if (
		bLiveEditorVisible &&
		LiveEditorDocument
	)
	{
		UpdateLiveEditorHighlight();
	}

	Context->Update();
}

void RmlUISystem::Render()
{
	if (!bInitialized || !Context)
		return;

	if (RmlRuntime::Get().IsUsingDX11())
	{
		static bool bLoggedDX11RenderSkip = false;
		if (!bLoggedDX11RenderSkip)
		{
			r3dOutToLog(
				"[RmlUI] DX9 Render() skipped: use RenderDX11 for DX11 runtime\n"
			);
			bLoggedDX11RenderSkip = true;
		}

		return;
	}

	const bool bHasVisibleDocument =
		(
			bAppSelectVisible &&
			AppSelectDocument
		) ||
		(
			bLoadingScreenVisible &&
			LoadingScreenDocument
		) ||
		(
			bAppMainVisible &&
			AppMainDocument
		) ||
		(
			bCharacterEditorVisible &&
			CharacterEditorDocument
		) ||
		(
			bLiveEditorVisible &&
			LiveEditorDocument
		) ||
		IsDebuggerVisible();

	if (!bHasVisibleDocument)
		return;

	RmlRuntime::Get().RenderContext(
		Context,
		ClientWidth,
		ClientHeight
	);
}

void RmlUISystem::RenderDX11(
	ID3D11RenderTargetView* RenderTarget,
	ID3D11DepthStencilView* DepthStencil,
	int Width,
	int Height
)
{
	if (!bInitialized || !Context || !RenderTarget)
		return;

	const bool bHasVisibleDocument =
		(
			bAppSelectVisible &&
			AppSelectDocument
		) ||
		(
			bLoadingScreenVisible &&
			LoadingScreenDocument
		) ||
		(
			bAppMainVisible &&
			AppMainDocument
		) ||
		(
			bCharacterEditorVisible &&
			CharacterEditorDocument
		) ||
		(
			bLiveEditorVisible &&
			LiveEditorDocument
		) ||
		IsDebuggerVisible();

	if (!bHasVisibleDocument)
		return;

	RmlRuntime::Get().RenderContextDX11(
		Context,
		RenderTarget,
		DepthStencil,
		Width,
		Height
	);
}

void RmlUISystem::OnDeviceLost()
{
	RmlRuntime::Get().OnDeviceLost();
}

void RmlUISystem::OnDeviceReset()
{
	UpdateClientSize();

	RmlRuntime::Get().OnDeviceReset(
		ClientWidth,
		ClientHeight
	);
}

void RmlUISystem::OnDeviceResetDX11(
	int Width,
	int Height
)
{
	ClientWidth = std::max(1, Width);
	ClientHeight = std::max(1, Height);

	if (Context)
		Context->SetDimensions(
			Rml::Vector2i(
				ClientWidth,
				ClientHeight
			)
		);

	RmlRuntime::Get().OnDeviceResetDX11(
		ClientWidth,
		ClientHeight
	);
}

void RmlUISystem::SetAppSelectCallback(FAppSelectCallback Callback)
{
	AppSelectCallback = std::move(Callback);
}

bool RmlUISystem::IsInitialized() const
{
	return bInitialized;
}

void RmlUISystem::ToggleDebugger()
{
	SetDebuggerVisible(
		!IsDebuggerVisible()
	);
}

void RmlUISystem::SetDebuggerVisible(
	bool bVisible
)
{
	if (
		!bInitialized ||
		!Context ||
		!bDebuggerInitialized
	)
	{
		return;
	}

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	Runtime.SetDebuggerVisible(
		Context,
		bVisible
	);

	if (bVisible)
	{
		Runtime.SetActiveContext(
			Context
		);
	}
	else if (
		!bAppSelectVisible &&
		!bAppMainVisible &&
		!bCharacterEditorVisible &&
		!bLiveEditorVisible
	)
	{
		Runtime.ClearActiveContext(
			Context
		);
	}

	OutputDebugStringA(
		bVisible
		? "[RmlUI] Debugger shown\n"
		: "[RmlUI] Debugger hidden\n"
	);
}

bool RmlUISystem::IsDebuggerVisible() const
{
	if (
		!bInitialized ||
		!Context ||
		!bDebuggerInitialized
	)
	{
		return false;
	}

	return RmlRuntime::Get().IsDebuggerVisible(
		Context
	);
}

void RmlUISystem::ReloadStyleSheets()
{
	if (!bInitialized || !Context)
		return;

	if (AppSelectDocument)
		AppSelectDocument->ReloadStyleSheet();

	if (LoadingScreenDocument)
		LoadingScreenDocument->ReloadStyleSheet();

	if (AppMainDocument)
		AppMainDocument->ReloadStyleSheet();

	if (CharacterEditorDocument)
		CharacterEditorDocument->ReloadStyleSheet();

	OutputDebugStringA("[RmlUI] Stylesheets reloaded\n");
}

void RmlUISystem::ReloadVisibleDocuments()
{
	if (!bInitialized || !Context)
		return;

	SelectLiveElement(nullptr);

	const bool bWasAppSelectVisible = bAppSelectVisible && AppSelectDocument;
	const bool bWasLoadingScreenVisible = bLoadingScreenVisible && LoadingScreenDocument;
	const bool bWasAppMainVisible = bAppMainVisible && AppMainDocument;
	const bool bWasCharacterEditorVisible = bCharacterEditorVisible && CharacterEditorDocument;
	const bool bWasDebuggerVisible = IsDebuggerVisible();

	if (AppSelectDocument)
	{
		DetachAppSelectEvents();
		Context->UnloadDocument(AppSelectDocument);
		AppSelectDocument = nullptr;
		bAppSelectVisible = false;
	}

	if (LoadingScreenDocument)
	{
		Context->UnloadDocument(LoadingScreenDocument);
		LoadingScreenDocument = nullptr;
		bLoadingScreenVisible = false;
	}

	if (AppMainDocument)
	{
		DetachAppMainEvents();
		Context->UnloadDocument(AppMainDocument);
		AppMainDocument = nullptr;
		bAppMainVisible = false;
	}

	if (CharacterEditorDocument)
	{
		DetachCharacterEvents();
		Context->UnloadDocument(CharacterEditorDocument);
		CharacterEditorDocument = nullptr;
		bCharacterEditorVisible = false;
	}

	if (bWasAppSelectVisible)
		ShowAppSelect();

	if (bWasLoadingScreenVisible)
		ShowLoadingScreen();

	if (bWasAppMainVisible)
		ShowAppMain();

	if (bWasCharacterEditorVisible)
		ShowCharacterEditor();

	if (bWasLoadingScreenVisible)
		ApplyLoadingScreenProgress(LoadingProgressVisual);

	if (
		bWasDebuggerVisible &&
		bDebuggerInitialized
	)
	{
		SetDebuggerVisible(true);
	}

	OutputDebugStringA("[RmlUI] Visible documents reloaded\n");
}

bool RmlUISystem::IsLiveEditorVisible() const
{
	return bInitialized && bLiveEditorVisible && LiveEditorDocument != nullptr;
}

void RmlUISystem::SelectLiveElementAt(int X, int Y)
{
	if (!Context)
		return;

	Rml::Element* Element = Context->GetElementAtPoint(
		Rml::Vector2f((float)X, (float)Y),
		LiveEditorDocument
	);

	while (Element && Element->GetOwnerDocument() == LiveEditorDocument)
		Element = Element->GetParentNode();

	if (Element && Element->GetOwnerDocument() == LiveEditorDocument)
		Element = nullptr;

	SelectLiveElement(Element);
}

void RmlUISystem::SelectLiveElement(Rml::Element* Element)
{
	if (Element && Element->GetOwnerDocument() == LiveEditorDocument)
		Element = nullptr;

	SelectedLiveElement = Element;
	RefreshLiveEditorFields();
	UpdateLiveEditorHighlight();
}

static Rml::String RmlPropertyToString(Rml::Element* Element, const char* Name)
{
	if (!Element || !Name)
		return Rml::String();

	const Rml::Property* Property = Element->GetProperty(Name);
	return Property ? Property->ToString() : Rml::String();
}

void RmlUISystem::RefreshLiveEditorFields()
{
	if (!LiveEditorDocument)
		return;

	if (!SelectedLiveElement)
	{
		SetLiveEditorControlValue("live_selected", "No element selected");
		SetLiveEditorControlValue("live_id", "");
		SetLiveEditorControlValue("live_class", "");
		SetLiveEditorControlValue("live_text", "");
		SetLiveEditorControlValue("live_display", "");
		SetLiveEditorControlValue("live_width", "");
		SetLiveEditorControlValue("live_height", "");
		SetLiveEditorControlValue("live_left", "");
		SetLiveEditorControlValue("live_top", "");
		SetLiveEditorControlValue("live_bg", "");
		SetLiveEditorControlValue("live_color", "");
		SetLiveEditorStatus("Pick an element with Ctrl+Alt+Click.");
		return;
	}

	Rml::String Label = SelectedLiveElement->GetAddress(false, true);
	SetLiveEditorControlValue("live_selected", Label);
	SetLiveEditorControlValue("live_id", SelectedLiveElement->GetId());
	SetLiveEditorControlValue("live_class", SelectedLiveElement->GetClassNames());
	SetLiveEditorControlValue("live_text", SelectedLiveElement->GetInnerRML());
	SetLiveEditorControlValue("live_display", RmlPropertyToString(SelectedLiveElement, "display"));
	SetLiveEditorControlValue("live_width", RmlPropertyToString(SelectedLiveElement, "width"));
	SetLiveEditorControlValue("live_height", RmlPropertyToString(SelectedLiveElement, "height"));
	SetLiveEditorControlValue("live_left", RmlPropertyToString(SelectedLiveElement, "left"));
	SetLiveEditorControlValue("live_top", RmlPropertyToString(SelectedLiveElement, "top"));
	SetLiveEditorControlValue("live_bg", RmlPropertyToString(SelectedLiveElement, "background-color"));
	SetLiveEditorControlValue("live_color", RmlPropertyToString(SelectedLiveElement, "color"));
	SetLiveEditorStatus("Editing selected element in memory.");
}

void RmlUISystem::UpdateLiveEditorHighlight()
{
	if (!LiveEditorDocument)
		return;

	Rml::Element* Highlight = LiveEditorDocument->GetElementById("live_editor_highlight");
	if (!Highlight)
		return;

	if (!SelectedLiveElement || !SelectedLiveElement->IsVisible(true))
	{
		Highlight->SetProperty("display", "none");
		return;
	}

	char Text[64] = {};

	Highlight->SetProperty("display", "block");

	sprintf_s(Text, "%.0fpx", SelectedLiveElement->GetAbsoluteLeft());
	Highlight->SetProperty("left", Text);

	sprintf_s(Text, "%.0fpx", SelectedLiveElement->GetAbsoluteTop());
	Highlight->SetProperty("top", Text);

	sprintf_s(Text, "%.0fpx", SelectedLiveElement->GetOffsetWidth());
	Highlight->SetProperty("width", Text);

	sprintf_s(Text, "%.0fpx", SelectedLiveElement->GetOffsetHeight());
	Highlight->SetProperty("height", Text);
}

static void RmlApplyEditorProperty(Rml::Element* Element, const char* Name, const Rml::String& Value)
{
	if (!Element || !Name)
		return;

	if (Value.empty())
		Element->RemoveProperty(Name);
	else
		Element->SetProperty(Name, Value);
}

void RmlUISystem::ApplyLiveEditorChanges()
{
	if (!SelectedLiveElement)
	{
		SetLiveEditorStatus("No selected element. Use Ctrl+Alt+Click first.");
		return;
	}

	SelectedLiveElement->SetId(GetLiveEditorControlValue("live_id"));
	SelectedLiveElement->SetClassNames(GetLiveEditorControlValue("live_class"));
	SelectedLiveElement->SetInnerRML(GetLiveEditorControlValue("live_text"));

	RmlApplyEditorProperty(SelectedLiveElement, "display", GetLiveEditorControlValue("live_display"));
	RmlApplyEditorProperty(SelectedLiveElement, "width", GetLiveEditorControlValue("live_width"));
	RmlApplyEditorProperty(SelectedLiveElement, "height", GetLiveEditorControlValue("live_height"));
	RmlApplyEditorProperty(SelectedLiveElement, "left", GetLiveEditorControlValue("live_left"));
	RmlApplyEditorProperty(SelectedLiveElement, "top", GetLiveEditorControlValue("live_top"));
	RmlApplyEditorProperty(SelectedLiveElement, "background-color", GetLiveEditorControlValue("live_bg"));
	RmlApplyEditorProperty(SelectedLiveElement, "color", GetLiveEditorControlValue("live_color"));

	if (SelectedLiveElement->GetOwnerDocument())
		SelectedLiveElement->GetOwnerDocument()->UpdateDocument();

	UpdateLiveEditorHighlight();
	SetLiveEditorStatus("Applied to runtime DOM. Copy changes to .rml/.rcss to keep them.");
}

Rml::String RmlUISystem::GetLiveEditorControlValue(const char* ElementId) const
{
	if (!LiveEditorDocument || !ElementId)
		return Rml::String();

	Rml::Element* Element = LiveEditorDocument->GetElementById(ElementId);
	if (!Element)
		return Rml::String();

	Rml::ElementFormControl* Control =
		rmlui_dynamic_cast<Rml::ElementFormControl*>(Element);

	if (Control)
		return Control->GetValue();

	return Element->GetInnerRML();
}

void RmlUISystem::SetLiveEditorControlValue(const char* ElementId, const Rml::String& Value)
{
	if (!LiveEditorDocument || !ElementId)
		return;

	Rml::Element* Element = LiveEditorDocument->GetElementById(ElementId);
	if (!Element)
		return;

	Rml::ElementFormControl* Control =
		rmlui_dynamic_cast<Rml::ElementFormControl*>(Element);

	if (Control)
		Control->SetValue(Value);
	else
		Element->SetInnerRML(EscapeRmlText(Value));
}

void RmlUISystem::SetLiveEditorStatus(const char* Text)
{
	if (!LiveEditorDocument)
		return;

	SetElementText(LiveEditorDocument, "live_status", Text ? Text : "");
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

	if (
		!bInitialized ||
		!Context ||
		(
			!bAppSelectVisible &&
			!bAppMainVisible &&
			!bCharacterEditorVisible &&
			!bLiveEditorVisible &&
			!IsDebuggerVisible()
		)
	)
	{
		return false;
	}

	if (
		!RmlRuntime::Get().IsActiveContext(
			Context
		)
	)
	{
		return false;
	}

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
		if (
			Message == WM_LBUTTONDOWN &&
			bLiveEditorVisible &&
			(GetKeyState(VK_CONTROL) & 0x8000) &&
			(GetKeyState(VK_MENU) & 0x8000)
		)
		{
			const int X = GET_X_LPARAM(LParam);
			const int Y = GET_Y_LPARAM(LParam);

			SelectLiveElementAt(X, Y);
			return true;
		}

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
		if (WParam == VK_F10)
		{
			ToggleDebugger();
			return true;
		}

		if (WParam == VK_F5)
		{
			if (GetKeyState(VK_CONTROL) & 0x8000)
				ReloadStyleSheets();
			else
				ReloadVisibleDocuments();

			return true;
		}

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
	SetAppMainTab(0);

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

		RmlRuntime::Get().SetActiveContext(
			Context
		);
	}
}

void RmlUISystem::HideAppMain()
{
	if (AppMainDocument)
		AppMainDocument->Hide();

	bAppMainVisible = false;

	if (
		!bAppSelectVisible &&
		!bCharacterEditorVisible &&
		!bLiveEditorVisible &&
		!IsDebuggerVisible()
	)
	{
		RmlRuntime::Get().ClearActiveContext(
			Context
		);
	}
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
		"btn_appmain_splat_size",
		"btn_appmain_scroll_up",
		"btn_appmain_scroll_down",
		"appmain_map_list"
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
		"btn_appmain_back",
		"btn_appmain_exit",
		"btn_appmain_terrain_toggle",
		"btn_appmain_terrain2_toggle",
		"btn_appmain_terrain_size",
		"btn_appmain_splat_size",
		"btn_appmain_scroll_up",
		"btn_appmain_scroll_down",
		"appmain_map_list"
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

	if (TabIndex < 0 || TabIndex > 2)
		TabIndex = 0;

	Rml::Element* LiveMapsButton =
		AppMainDocument->GetElementById(
			"btn_appmain_live_maps"
		);

	Rml::Element* EditorMapsButton =
		AppMainDocument->GetElementById(
			"btn_appmain_editor_maps"
		);

	Rml::Element* CreateMapButton =
		AppMainDocument->GetElementById(
			"btn_appmain_create_map"
		);

	if (LiveMapsButton)
		LiveMapsButton->SetClass(
			"selected",
			TabIndex == 0
		);

	if (EditorMapsButton)
		EditorMapsButton->SetClass(
			"selected",
			TabIndex == 1
		);

	if (CreateMapButton)
		CreateMapButton->SetClass(
			"selected",
			TabIndex == 2
		);

	Rml::Element* MapsPanel =
		AppMainDocument->GetElementById(
			"appmain_maps_panel"
		);

	Rml::Element* CreatePanel =
		AppMainDocument->GetElementById(
			"appmain_create_panel"
		);

	if (MapsPanel)
	{
		MapsPanel->SetProperty(
			"display",
			TabIndex == 2 ? "none" : "block"
		);
	}

	if (CreatePanel)
	{
		CreatePanel->SetProperty(
			"display",
			TabIndex == 2 ? "block" : "none"
		);
	}

	if (TabIndex == 0)
	{
		SetElementText(
			AppMainDocument,
			"appmain_section_title",
			"LIVE MAPS"
		);
	}
	else if (TabIndex == 1)
	{
		SetElementText(
			AppMainDocument,
			"appmain_section_title",
			"EDITOR MAPS"
		);
	}
	else
	{
		SetElementText(
			AppMainDocument,
			"appmain_section_title",
			"CREATE MAP"
		);
	}
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

void RmlUISystem::SetAppMainScrollInfo(int FirstIndex, int VisibleCount, int TotalCount)
{
	if (!LoadAppMain())
		return;

	Rml::Element* Thumb = AppMainDocument->GetElementById("appmain_scroll_thumb");
	Rml::Element* Track = AppMainDocument->GetElementById("appmain_scroll_track");
	Rml::Element* Up = AppMainDocument->GetElementById("btn_appmain_scroll_up");
	Rml::Element* Down = AppMainDocument->GetElementById("btn_appmain_scroll_down");

	if (!Thumb || !Track)
		return;

	if (VisibleCount < 1)
		VisibleCount = 1;

	if (TotalCount < 0)
		TotalCount = 0;

	if (FirstIndex < 0)
		FirstIndex = 0;

	const int TrackHeight = 190;

	if (TotalCount <= VisibleCount)
	{
		Thumb->SetProperty("top", "0px");
		Thumb->SetProperty("height", "190px");

		if (Up)
			Up->SetProperty("display", "none");

		if (Down)
			Down->SetProperty("display", "none");

		Track->SetProperty("display", "block");
		return;
	}

	if (Up)
		Up->SetProperty("display", "block");

	if (Down)
		Down->SetProperty("display", "block");

	Track->SetProperty("display", "block");

	int ThumbHeight = (TrackHeight * VisibleCount) / TotalCount;

	if (ThumbHeight < 28)
		ThumbHeight = 28;

	if (ThumbHeight > TrackHeight)
		ThumbHeight = TrackHeight;

	const int MaxFirst = TotalCount - VisibleCount;
	const int MaxTop = TrackHeight - ThumbHeight;

	int ThumbTop = 0;

	if (MaxFirst > 0)
		ThumbTop = (FirstIndex * MaxTop) / MaxFirst;

	char Text[32] = {};

	sprintf_s(Text, "%dpx", ThumbTop);
	Thumb->SetProperty("top", Text);

	sprintf_s(Text, "%dpx", ThumbHeight);
	Thumb->SetProperty("height", Text);
}

void RmlUISystem::SetCharacterCallback(
	FCharacterCallback Callback
)
{
	CharacterCallback = std::move(Callback);
}

bool RmlUISystem::LoadCharacterEditor()
{
	if (!bInitialized || !Context)
		return false;

	if (CharacterEditorDocument)
		return true;

	CharacterEditorDocument =
		Context->LoadDocument(
			"Rml/Studio/Editor_Character.rml"
		);

	if (!CharacterEditorDocument)
	{
		OutputDebugStringA(
			"[RmlUI] Failed to load "
			"Data/Rml/Studio/Editor_Character.rml\n"
		);

		return false;
	}

	AttachCharacterEvents();

	CharacterEditorDocument->Hide();
	bCharacterEditorVisible = false;

	OutputDebugStringA(
		"[RmlUI] Character editor loaded\n"
	);

	return true;
}

void RmlUISystem::ShowCharacterEditor()
{
	if (!LoadCharacterEditor())
		return;

	CharacterEditorDocument->Show();
	bCharacterEditorVisible = true;

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlUISystem::HideCharacterEditor()
{
	if (CharacterEditorDocument)
		CharacterEditorDocument->Hide();

	bCharacterEditorVisible = false;

	if (
		!bAppSelectVisible &&
		!bAppMainVisible &&
		!bLiveEditorVisible &&
		!IsDebuggerVisible()
	)
	{
		RmlRuntime::Get().ClearActiveContext(
			Context
		);
	}
}

bool RmlUISystem::IsCharacterEditorReady() const
{
	return
		bInitialized &&
		CharacterEditorDocument != nullptr;
}

bool RmlUISystem::IsCharacterEditorVisible() const
{
	return
		IsCharacterEditorReady() &&
		bCharacterEditorVisible;
}

void RmlUISystem::AttachCharacterEvents()
{
	if (
		!CharacterEditorDocument ||
		!CharacterClickListener
	)
	{
		return;
	}

	CharacterEditorDocument->AddEventListener(
		"click",
		CharacterClickListener.get()
	);
}

void RmlUISystem::DetachCharacterEvents()
{
	if (
		!CharacterEditorDocument ||
		!CharacterClickListener
	)
	{
		return;
	}

	CharacterEditorDocument->RemoveEventListener(
		"click",
		CharacterClickListener.get()
	);
}

void RmlUISystem::SetCharacterMode(
	int ModeIndex
)
{
	if (!CharacterEditorDocument)
		return;

	Rml::Element* StatesButton =
		CharacterEditorDocument->GetElementById(
			"btn_char_mode_states"
		);

	Rml::Element* AnimationsButton =
		CharacterEditorDocument->GetElementById(
			"btn_char_mode_all_anims"
		);

	Rml::Element* StatesPanel =
		CharacterEditorDocument->GetElementById(
			"character_states_panel"
		);

	Rml::Element* AnimationsPanel =
		CharacterEditorDocument->GetElementById(
			"character_all_anims_panel"
		);

	if (StatesButton)
	{
		StatesButton->SetClass(
			"selected",
			ModeIndex == 0
		);
	}

	if (AnimationsButton)
	{
		AnimationsButton->SetClass(
			"selected",
			ModeIndex == 1
		);
	}

	if (StatesPanel)
	{
		StatesPanel->SetProperty(
			"display",
			ModeIndex == 0
				? "block"
				: "none"
		);
	}

	if (AnimationsPanel)
	{
		AnimationsPanel->SetProperty(
			"display",
			ModeIndex == 1
				? "block"
				: "none"
		);
	}
}

void RmlUISystem::SetCharacterSelectedState(
	int StateIndex
)
{
	if (!CharacterEditorDocument)
		return;

	for (int Index = 0; Index < 9; ++Index)
	{
		char ElementId[64]{};

		sprintf_s(
			ElementId,
			"btn_char_state_%d",
			Index
		);

		Rml::Element* Element =
			CharacterEditorDocument->GetElementById(
				ElementId
			);

		if (Element)
		{
			Element->SetClass(
				"selected",
				Index == StateIndex
			);
		}
	}
}

void RmlUISystem::SetCharacterSelectedDirection(
	int DirectionIndex
)
{
	if (!CharacterEditorDocument)
		return;

	for (int Index = 0; Index < 9; ++Index)
	{
		char ElementId[64]{};

		sprintf_s(
			ElementId,
			"btn_char_direction_%d",
			Index
		);

		Rml::Element* Element =
			CharacterEditorDocument->GetElementById(
				ElementId
			);

		if (Element)
		{
			Element->SetClass(
				"selected",
				Index == DirectionIndex
			);
		}
	}
}

void RmlUISystem::SetCharacterToggle(
	const char* ButtonId,
	const char* ValueId,
	bool bEnabled
)
{
	if (!CharacterEditorDocument)
		return;

	Rml::Element* Button =
		CharacterEditorDocument->GetElementById(
			ButtonId
		);

	if (Button)
	{
		Button->SetClass(
			"enabled",
			bEnabled
		);
	}

	SetCharacterText(
		ValueId,
		bEnabled ? "ON" : "OFF"
	);
}

void RmlUISystem::SetCharacterText(
	const char* ElementId,
	const char* Text
)
{
	SetElementText(
		CharacterEditorDocument,
		ElementId,
		Text ? Text : ""
	);
}

void RmlUISystem::SetCharacterVisible(
	const char* ElementId,
	bool bVisible
)
{
	if (!CharacterEditorDocument)
		return;

	Rml::Element* Element =
		CharacterEditorDocument->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetProperty(
		"display",
		bVisible ? "block" : "none"
	);
}

void RmlUISystem::SetCharacterAnimationList(
	const char** Names,
	int Count,
	int SelectedIndex
)
{
	if (!CharacterEditorDocument)
		return;

	Rml::Element* Container =
		CharacterEditorDocument->GetElementById(
			"character_animation_list"
		);

	if (!Container)
		return;

	Rml::String Markup;

	if (!Names || Count <= 0)
	{
		Markup =
			"<div class=\"empty_text\">"
			"NO ANIMATIONS LOADED"
			"</div>";

		Container->SetInnerRML(Markup);
		return;
	}

	for (int Index = 0; Index < Count; ++Index)
	{
		Markup += "<button id=\"char_anim_";
		Markup += std::to_string(Index);
		Markup += "\" class=\"list_button";

		if (Index == SelectedIndex)
			Markup += " selected";

		Markup += "\">";

		Markup += EscapeRmlText(
			Names[Index] ? Names[Index] : ""
		);

		Markup += "</button>";
	}

	Container->SetInnerRML(Markup);
}

void RmlUISystem::SetCharacterAnimationStack(
	const char** Names,
	const char** Data,
	int Count
)
{
	if (!CharacterEditorDocument)
		return;

	Rml::Element* Container =
		CharacterEditorDocument->GetElementById(
			"character_anim_stack_list"
		);

	if (!Container)
		return;

	Rml::String Markup;

	if (
		!Names ||
		!Data ||
		Count <= 0
	)
	{
		Markup =
			"<div class=\"empty_text\">"
			"NO ACTIVE TRACKS"
			"</div>";

		Container->SetInnerRML(Markup);
		return;
	}

	for (int Index = 0; Index < Count; ++Index)
	{
		Markup +=
			"<div class=\"stack_item\">";

		Markup +=
			"<div class=\"stack_item_name\">";

		Markup += EscapeRmlText(
			Names[Index] ? Names[Index] : ""
		);

		Markup += "</div>";

		Markup +=
			"<div class=\"stack_item_data\">";

		Markup += EscapeRmlText(
			Data[Index] ? Data[Index] : ""
		);

		Markup += "</div>";
		Markup += "</div>";
	}

	Container->SetInnerRML(Markup);
}

void RmlUISystem::SetCharacterAnimationInfo(
	float Length,
	int Frames,
	int Tracks,
	float FrameRate,
	const char* AnimationName,
	const char* AnimationFile
)
{
	char Text[128]{};

	sprintf_s(
		Text,
		"%.4f SEC",
		Length
	);

	SetCharacterText(
		"character_anim_length",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Frames
	);

	SetCharacterText(
		"character_anim_frames",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Tracks
	);

	SetCharacterText(
		"character_anim_tracks",
		Text
	);

	sprintf_s(
		Text,
		"%.4f",
		FrameRate
	);

	SetCharacterText(
		"character_anim_framerate",
		Text
	);

	SetCharacterText(
		"character_anim_name",
		AnimationName ? AnimationName : "-"
	);

	SetCharacterText(
		"character_anim_file",
		AnimationFile ? AnimationFile : "-"
	);
}

float RmlUISystem::GetCharacterInputValue(
	const char* ElementId,
	float DefaultValue
) const
{
	if (!CharacterEditorDocument || !ElementId)
		return DefaultValue;

	Rml::Element* Element =
		CharacterEditorDocument->GetElementById(ElementId);

	if (!Element)
		return DefaultValue;

	Rml::ElementFormControlInput* Input =
		rmlui_dynamic_cast<Rml::ElementFormControlInput*>(
			Element
		);

	if (!Input)
		return DefaultValue;

	const Rml::String Value = Input->GetValue();

	if (Value.empty())
		return DefaultValue;

	char* EndPointer = nullptr;

	const float Result =
		strtof(
			Value.c_str(),
			&EndPointer
		);

	if (EndPointer == Value.c_str())
		return DefaultValue;

	return Result;
}

void RmlUISystem::SetCharacterInputValue(
	const char* ElementId,
	const char* ValueElementId,
	float Value,
	const char* Format
)
{
	if (!CharacterEditorDocument)
		return;

	if (ElementId)
	{
		Rml::Element* Element =
			CharacterEditorDocument->GetElementById(
				ElementId
			);

		if (Element)
		{
			Rml::ElementFormControlInput* Input =
				rmlui_dynamic_cast<
					Rml::ElementFormControlInput*
				>(
					Element
				);

			if (Input)
			{
				char InputText[64]{};

				sprintf_s(
					InputText,
					"%.4f",
					Value
				);

				Input->SetValue(
					Rml::String(InputText)
				);
			}
		}
	}

	if (ValueElementId)
	{
		char ValueText[64]{};

		sprintf_s(
			ValueText,
			Format ? Format : "%.2f",
			Value
		);

		SetCharacterText(
			ValueElementId,
			ValueText
		);
	}
}

void RmlUISystem::SetCharacterInputRange(
	const char* ElementId,
	float Minimum,
	float Maximum,
	float Step
)
{
	if (!CharacterEditorDocument || !ElementId)
		return;

	Rml::Element* Element =
		CharacterEditorDocument->GetElementById(ElementId);

	if (!Element)
	{
		r3dOutToLog(
			"[RmlUI] Character range element missing: %s\n",
			ElementId
		);

		return;
	}

	Rml::ElementFormControlInput* Input =
		rmlui_dynamic_cast<Rml::ElementFormControlInput*>(
			Element
		);

	if (!Input)
	{
		r3dOutToLog(
			"[RmlUI] Character element is not input: %s\n",
			ElementId
		);

		return;
	}
	
	Element->SetAttribute("min", Minimum);
	Element->SetAttribute("max", Maximum);
	Element->SetAttribute("step", Step);
}

void RmlUISystem::SetCharacterEquipmentCategory(
	int CategoryIndex
)
{
	if (!CharacterEditorDocument)
		return;

	for (int Index = 0; Index < 8; ++Index)
	{
		char ElementId[96]{};

		sprintf_s(
			ElementId,
			"btn_char_equipment_category_%d",
			Index
		);

		Rml::Element* Element =
			CharacterEditorDocument->GetElementById(
				ElementId
			);

		if (Element)
		{
			Element->SetClass(
				"selected",
				Index == CategoryIndex
			);
		}
	}

	static const char* CategoryNames[] =
	{
		"HEAD ARMOR",
		"ARMOR",
		"HEAD",
		"BODY",
		"LEGS",
		"HERO",
		"SECONDARY WEAPON",
		"PRIMARY WEAPON"
	};

	if (
		CategoryIndex >= 0 &&
		CategoryIndex < 8
	)
	{
		SetCharacterText(
			"equipment_current",
			CategoryNames[CategoryIndex]
		);
	}
}

void RmlUISystem::SetCharacterEquipmentSelected(
	const char* ItemName
)
{
	SetCharacterText(
		"equipment_selected_value",
		ItemName && ItemName[0]
			? ItemName
			: "EMPTY"
	);
}

void RmlUISystem::SetCharacterEquipmentList(
	const char** ItemNames,
	int ItemCount,
	int SelectedIndex
)
{
	if (!CharacterEditorDocument)
		return;

	Rml::Element* Container =
		CharacterEditorDocument->GetElementById(
			"character_equipment_list"
		);

	if (!Container)
		return;

	if (!ItemNames || ItemCount <= 0)
	{
		Container->SetInnerRML(
			"<div class=\"empty_text\">"
			"NO ITEMS"
			"</div>"
		);

		return;
	}

	Rml::String Markup;

	for (int Index = 0; Index < ItemCount; ++Index)
	{
		Markup +=
			"<button id=\"char_equipment_item_";

		Markup += std::to_string(Index);

		Markup +=
			"\" class=\"equipment_item";

		if (Index == SelectedIndex)
			Markup += " selected";

		Markup += "\">";

		Markup += EscapeRmlText(
			ItemNames[Index]
				? ItemNames[Index]
				: "UNKNOWN"
		);

		Markup += "</button>";
	}

	Container->SetInnerRML(Markup);
}
