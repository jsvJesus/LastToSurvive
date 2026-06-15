#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndContext.h"
#include "../RmlRuntime.h"

#include "cvar.h"
#include "GameCode/UserProfile.h"
#include "backend/WOBackendAPI.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Traits.h>

#include <process.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "r3dDebug.h"

namespace
{
	const char* CharacterButtonPrefix =
		"char_slot_";

	const size_t CharacterButtonPrefixLength =
		strlen(CharacterButtonPrefix);
}

RmlFrontEndContext::FClickListener::FClickListener(
	RmlFrontEndContext* InOwner
)
	: Owner(InOwner)
{
}

void RmlFrontEndContext::FClickListener::ProcessEvent(
	Rml::Event& Event
)
{
	if (!Owner)
		return;

	Rml::Element* Element =
		Event.GetTargetElement();

	Owner->HandleClick(
		Element
	);
}

void RmlFrontEndContext::FClickListener::OnDetach(
	Rml::Element* Element
)
{
	(void)Element;
}

RmlFrontEndContext::RmlFrontEndContext()
{
}

RmlFrontEndContext::~RmlFrontEndContext()
{
	Shutdown();
}

bool RmlFrontEndContext::Init(
	HWND WindowHandle,
	IDirect3DDevice9* Device
)
{
	if (bInitialized)
		return true;

	if (!WindowHandle || !Device)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Init] Invalid window or device\n"
		);

		return false;
	}

	Hwnd = WindowHandle;

	RefreshDimensions();

	RmlRuntime& Runtime =
		RmlRuntime::Get();

	if (!Runtime.Acquire(
		WindowHandle,
		Device
	))
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Init] Shared runtime failed\n"
		);

		Hwnd = nullptr;
		return false;
	}

	bRuntimeAcquired = true;

	Context = Runtime.CreateContext(
		"GameFrontEnd",
		Rml::Vector2i(
			Width,
			Height
		)
	);

	if (!Context)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Init] Context creation failed\n"
		);

		Shutdown();
		return false;
	}

	Context->EnableMouseCursor(true);

	ClickListener =
		std::make_unique<FClickListener>(
			this
		);

	if (!LoadDocuments())
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Init] Documents failed to load\n"
		);

		Shutdown();
		return false;
	}

	AttachEvents();

	bInitialized = true;

#ifndef FINAL_BUILD
	if (d_login && d_login->GetString())
	{
		SetInputValue(
			LoginDocument,
			"login_username",
			d_login->GetString()
		);
	}

	if (d_password && d_password->GetString())
	{
		SetInputValue(
			LoginDocument,
			"login_password",
			d_password->GetString()
		);
	}
#endif

	Runtime.SetActiveContext(
		Context
	);

	ShowLogin();

	r3dOutToLog(
		"[RmlUI][FrontEnd][Init] Login frontend ready\n"
	);

	return true;
}

void RmlFrontEndContext::Shutdown()
{
	StopAsyncOperation();

	if (Context)
	{
		RmlRuntime::Get().ClearActiveContext(
			Context
		);
	}

	DetachEvents();
	UnloadDocuments();

	ClickListener.reset();

	if (Context)
	{
		RmlRuntime::Get().DestroyContext(
			Context
		);
	}

	if (bRuntimeAcquired)
	{
		RmlRuntime::Get().Release();
		bRuntimeAcquired = false;
	}

	Hwnd = nullptr;

	Width = 1;
	Height = 1;

	SelectedCharacterIndex = -1;

	CurrentScreen = EScreen::Login;
	PendingResult = ERmlFrontEndResult::None;

	bInitialized = false;
	bProfileLoaded = false;

	LoginUser[0] = 0;
	LoginPassword[0] = 0;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Shutdown] Complete\n"
	);
}

bool RmlFrontEndContext::LoadDocuments()
{
	if (!Context)
		return false;

	LoginDocument =
		Context->LoadDocument(
			"Rml/FrontEnd/Login.rml"
		);

	if (!LoginDocument)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/Login.rml\n"
		);

		return false;
	}

	MainMenuDocument =
		Context->LoadDocument(
			"Rml/FrontEnd/MainMenu.rml"
		);

	if (!MainMenuDocument)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/MainMenu.rml\n"
		);

		return false;
	}

	LoginDocument->Hide();
	MainMenuDocument->Hide();

	return true;
}

void RmlFrontEndContext::UnloadDocuments()
{
	if (!Context)
		return;

	if (LoginDocument)
	{
		Context->UnloadDocument(
			LoginDocument
		);

		LoginDocument = nullptr;
	}

	if (MainMenuDocument)
	{
		Context->UnloadDocument(
			MainMenuDocument
		);

		MainMenuDocument = nullptr;
	}
}

void RmlFrontEndContext::AttachEvents()
{
	if (!ClickListener)
		return;

	if (LoginDocument)
	{
		LoginDocument->AddEventListener(
			"click",
			ClickListener.get()
		);
	}

	if (MainMenuDocument)
	{
		MainMenuDocument->AddEventListener(
			"click",
			ClickListener.get()
		);
	}
}

void RmlFrontEndContext::DetachEvents()
{
	if (!ClickListener)
		return;

	if (LoginDocument)
	{
		LoginDocument->RemoveEventListener(
			"click",
			ClickListener.get()
		);
	}

	if (MainMenuDocument)
	{
		MainMenuDocument->RemoveEventListener(
			"click",
			ClickListener.get()
		);
	}
}

void RmlFrontEndContext::Update()
{
	if (!bInitialized || !Context)
		return;

	RefreshDimensions();
	PollAsyncOperation();

	RmlRuntime::Get().SetActiveContext(
		Context
	);

	Context->Update();
}

void RmlFrontEndContext::Render()
{
	if (!bInitialized || !Context)
		return;

	RmlRuntime::Get().RenderContext(
		Context,
		Width,
		Height
	);
}

bool RmlFrontEndContext::ProcessWin32Message(
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

	if (
		Message == WM_KEYDOWN &&
		WParam == VK_RETURN &&
		CurrentScreen == EScreen::Login &&
		!IsBusy()
	)
	{
		RequestLogin();
		return true;
	}

	return RmlRuntime::Get().ProcessWin32Message(
		Context,
		WindowHandle,
		Message,
		WParam,
		LParam,
		OutResult
	);
}

bool RmlFrontEndContext::IsInitialized() const
{
	return bInitialized;
}

ERmlFrontEndResult RmlFrontEndContext::ConsumeResult()
{
	const ERmlFrontEndResult Result =
		PendingResult;

	PendingResult =
		ERmlFrontEndResult::None;

	return Result;
}

void RmlFrontEndContext::ShowLogin()
{
	if (!LoginDocument || !MainMenuDocument)
		return;

	MainMenuDocument->Hide();
	LoginDocument->Show();

	CurrentScreen = EScreen::Login;

	SetLoginControlsEnabled(
		!IsBusy()
	);

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlFrontEndContext::ShowMainMenu()
{
	if (!LoginDocument || !MainMenuDocument)
		return;

	LoginDocument->Hide();
	MainMenuDocument->Show();

	CurrentScreen = EScreen::MainMenu;

	SetMainMenuControlsEnabled(
		!IsBusy()
	);

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlFrontEndContext::ShowLoginMessage(
	const wchar_t* Message
)
{
	PendingResult =
		ERmlFrontEndResult::None;

	bProfileLoaded = false;
	SelectedCharacterIndex = -1;

	ShowLogin();

	SetLoginStatus(
		WideToUtf8(
			Message
				? Message
				: L""
		)
	);
}

void RmlFrontEndContext::ShowMainMenuMessage(
	const wchar_t* Message
)
{
	if (!bProfileLoaded)
	{
		ShowLoginMessage(
			Message
		);

		return;
	}

	ShowMainMenu();

	SetMainMenuStatus(
		WideToUtf8(
			Message
				? Message
				: L""
		)
	);
}

void RmlFrontEndContext::RefreshProfile()
{
	if (
		!gUserProfile.CustomerID ||
		!gUserProfile.SessionID
	)
	{
		ShowLoginMessage(
			L"Your login session is no longer valid."
		);

		return;
	}

	ShowMainMenu();

	SetMainMenuStatus(
		"Refreshing profile..."
	);

	BeginProfileLoad();
}

void RmlFrontEndContext::HandleClick(
	Rml::Element* Element
)
{
	if (!Element)
		return;

	Rml::Element* Current =
		Element;

	while (Current)
	{
		const Rml::String& Id =
			Current->GetId();

		if (Id == "btn_login")
		{
			RequestLogin();
			return;
		}

		if (Id == "btn_login_exit")
		{
			if (!IsBusy())
			{
				PendingResult =
					ERmlFrontEndResult::Exit;
			}

			return;
		}

		if (Id == "btn_quick_join")
		{
			RequestQuickJoin();
			return;
		}

		if (Id == "btn_refresh_profile")
		{
			RefreshProfile();
			return;
		}

		if (Id == "btn_create_character")
		{
			SetMainMenuStatus(
				"Character creation is the next frontend stage."
			);

			return;
		}

		if (Id == "btn_frontend_exit")
		{
			if (!IsBusy())
			{
				PendingResult =
					ERmlFrontEndResult::Exit;
			}

			return;
		}

		if (
			Id.compare(
				0,
				CharacterButtonPrefixLength,
				CharacterButtonPrefix
			) == 0
		)
		{
			const int CharacterIndex =
				atoi(
					Id.c_str() +
					CharacterButtonPrefixLength
				);

			SelectCharacter(
				CharacterIndex
			);

			return;
		}

		if (
			Current == LoginDocument ||
			Current == MainMenuDocument
		)
		{
			break;
		}

		Current =
			Current->GetParentNode();
	}
}

void RmlFrontEndContext::RequestLogin()
{
	if (
		CurrentScreen != EScreen::Login ||
		IsBusy()
	)
	{
		return;
	}

	const Rml::String Username =
		GetInputValue(
			LoginDocument,
			"login_username"
		);

	const Rml::String Password =
		GetInputValue(
			LoginDocument,
			"login_password"
		);

	if (
		Username.length() < 2 ||
		Password.length() < 2
	)
	{
		SetLoginStatus(
			"Enter a valid username and password."
		);

		return;
	}

	strncpy_s(
		LoginUser,
		sizeof(LoginUser),
		Username.c_str(),
		_TRUNCATE
	);

	strncpy_s(
		LoginPassword,
		sizeof(LoginPassword),
		Password.c_str(),
		_TRUNCATE
	);

	SetLoginControlsEnabled(false);

	SetLoginStatus(
		"Connecting to account server..."
	);

	if (!StartAsyncOperation(
		AsyncOperation_Login
	))
	{
		SetLoginControlsEnabled(true);

		SetLoginStatus(
			"Unable to start login operation."
		);
	}
}

void RmlFrontEndContext::BeginProfileLoad()
{
	SetLoginControlsEnabled(false);
	SetMainMenuControlsEnabled(false);

	if (CurrentScreen == EScreen::Login)
	{
		SetLoginStatus(
			"Loading account profile..."
		);
	}
	else
	{
		SetMainMenuStatus(
			"Loading account profile..."
		);
	}

	if (!StartAsyncOperation(
		AsyncOperation_Profile
	))
	{
		if (CurrentScreen == EScreen::Login)
		{
			SetLoginControlsEnabled(true);

			SetLoginStatus(
				"Unable to start profile loading."
			);
		}
		else
		{
			SetMainMenuControlsEnabled(true);

			SetMainMenuStatus(
				"Unable to start profile loading."
			);
		}
	}
}

bool RmlFrontEndContext::StartAsyncOperation(
	EAsyncOperation Operation
)
{
	if (WorkerThread)
		return false;

	InterlockedExchange(
		&AsyncOperation,
		static_cast<LONG>(Operation)
	);

	InterlockedExchange(
		&AsyncResult,
		AsyncResult_Working
	);

	unsigned int ThreadId = 0;

	WorkerThread =
		reinterpret_cast<HANDLE>(
			_beginthreadex(
				nullptr,
				0,
				&AsyncThreadEntry,
				this,
				0,
				&ThreadId
			)
		);

	if (!WorkerThread)
	{
		InterlockedExchange(
			&AsyncOperation,
			AsyncOperation_None
		);

		InterlockedExchange(
			&AsyncResult,
			AsyncResult_Idle
		);

		return false;
	}

	return true;
}

unsigned int WINAPI
RmlFrontEndContext::AsyncThreadEntry(
	void* Parameter
)
{
	r3dThreadAutoInstallCrashHelper CrashHelper;

	RmlFrontEndContext* Owner =
		static_cast<RmlFrontEndContext*>(
			Parameter
		);

	if (!Owner)
		return 0;

	return Owner->RunAsyncOperation();
}

unsigned int RmlFrontEndContext::RunAsyncOperation()
{
	const LONG Operation =
		InterlockedCompareExchange(
			&AsyncOperation,
			0,
			0
		);

	LONG Result =
		AsyncResult_Error;

	if (
		Operation ==
		AsyncOperation_Login
	)
	{
		gUserProfile.CustomerID = 0;
		gUserProfile.SessionID = 0;
		gUserProfile.AccountStatus = 0;

		CWOBackendReq Request(
			"api_Login.aspx"
		);

		Request.AddParam(
			"username",
			LoginUser
		);

		Request.AddParam(
			"password",
			LoginPassword
		);

		if (!Request.Issue())
		{
			r3dOutToLog(
				"[RmlUI][FrontEnd][Login] "
				"Backend request failed: %d\n",
				Request.resultCode_
			);

			Result =
				Request.resultCode_ == 8
					? AsyncResult_Timeout
					: AsyncResult_Error;
		}
		else
		{
			int CustomerId = 0;
			int SessionId = 0;
			int AccountStatus = 0;

			const int Parsed =
				sscanf_s(
					Request.bodyStr_,
					"%d %d %d",
					&CustomerId,
					&SessionId,
					&AccountStatus
				);

			if (Parsed != 3)
			{
				r3dOutToLog(
					"[RmlUI][FrontEnd][Login] "
					"Invalid backend response: %s\n",
					Request.bodyStr_
						? Request.bodyStr_
						: "<null>"
				);

				Result =
					AsyncResult_Error;
			}
			else
			{
				gUserProfile.CustomerID =
					static_cast<DWORD>(
						CustomerId
					);

				/*
				 * SQL SessionID является signed int.
				 * Приведение к DWORD сохраняет те же 32 бита.
				 */
				gUserProfile.SessionID =
					static_cast<DWORD>(
						SessionId
					);

				gUserProfile.AccountStatus =
					AccountStatus;

				r3dOutToLog(
					"[RmlUI][FrontEnd][Login] "
					"CustomerID=%d, SessionID=%d, "
					"AccountStatus=%d\n",
					CustomerId,
					SessionId,
					AccountStatus
				);

				if (CustomerId == 0)
				{
					Result =
						AsyncResult_BadPassword;
				}
				else if (AccountStatus >= 200)
				{
					Result =
						AsyncResult_Frozen;
				}
				else
				{
					Result =
						AsyncResult_Success;
				}
			}
		}
	}
	else if (
		Operation ==
		AsyncOperation_Profile
	)
	{
		const int ProfileResult =
			gUserProfile.GetProfile();

		r3dOutToLog(
			"[RmlUI][FrontEnd][Profile] "
			"GetProfile result=%d\n",
			ProfileResult
		);

		Result =
			ProfileResult == 0
				? AsyncResult_Success
				: AsyncResult_Error;
	}

	InterlockedExchange(
		&AsyncResult,
		Result
	);

	return 0;
}

void RmlFrontEndContext::PollAsyncOperation()
{
	if (!WorkerThread)
		return;

	LONG Result =
		InterlockedCompareExchange(
			&AsyncResult,
			0,
			0
		);

	if (
		Result == AsyncResult_Idle ||
		Result == AsyncResult_Working
	)
	{
		return;
	}

	const LONG Operation =
		InterlockedCompareExchange(
			&AsyncOperation,
			0,
			0
		);

	WaitForSingleObject(
		WorkerThread,
		INFINITE
	);

	CloseHandle(
		WorkerThread
	);

	WorkerThread = nullptr;

	InterlockedExchange(
		&AsyncOperation,
		AsyncOperation_None
	);

	InterlockedExchange(
		&AsyncResult,
		AsyncResult_Idle
	);

	if (Operation == AsyncOperation_Login)
	{
		gUserProfile.CustomerID = 0;
		gUserProfile.SessionID = 0;
		gUserProfile.AccountStatus = 0;

		CWOBackendReq Request(
			"api_Login.aspx"
		);

		Request.AddParam(
			"username",
			LoginUser
		);

		Request.AddParam(
			"password",
			LoginPassword
		);

		if (!Request.Issue())
		{
			r3dOutToLog(
				"[RmlUI][FrontEnd][Login] "
				"Backend request failed: %d\n",
				Request.resultCode_
			);

			Result =
				Request.resultCode_ == 8
					? AsyncResult_Timeout
					: AsyncResult_Error;
		}
		else
		{
			int CustomerId = 0;
			int SessionId = 0;
			int AccountStatus = 0;

			const int Parsed =
				sscanf_s(
					Request.bodyStr_,
					"%d %d %d",
					&CustomerId,
					&SessionId,
					&AccountStatus
				);

			if (Parsed != 3)
			{
				r3dOutToLog(
					"[RmlUI][FrontEnd][Login] "
					"Invalid backend response: %s\n",
					Request.bodyStr_
						? Request.bodyStr_
						: "<null>"
				);

				Result =
					AsyncResult_Error;
			}
			else
			{
				gUserProfile.CustomerID =
					static_cast<DWORD>(
						CustomerId
					);

				/*
				 * SessionID приходит из SQL как signed int.
				 * Приведение к DWORD сохраняет те же 32 бита.
				 * При следующем запросе AddSessionInfo()
				 * снова передаст его как signed int.
				 */
				gUserProfile.SessionID =
					static_cast<DWORD>(
						SessionId
					);

				gUserProfile.AccountStatus =
					AccountStatus;

				r3dOutToLog(
					"[RmlUI][FrontEnd][Login] "
					"CustomerID=%d, SessionID=%d, Status=%d\n",
					CustomerId,
					SessionId,
					AccountStatus
				);

				if (CustomerId == 0)
				{
					Result =
						AsyncResult_BadPassword;
				}
				else if (AccountStatus >= 200)
				{
					Result =
						AsyncResult_Frozen;
				}
				else
				{
					Result =
						AsyncResult_Success;
				}
			}
		}
	}
	else if (
		Operation ==
		AsyncOperation_Profile
	)
	{
		const int ProfileResult =
			gUserProfile.GetProfile();

		r3dOutToLog(
			"[RmlUI][FrontEnd][Profile] "
			"GetProfile result=%d\n",
			ProfileResult
		);

		Result =
			ProfileResult == 0
				? AsyncResult_Success
				: AsyncResult_Error;
	}
}

void RmlFrontEndContext::StopAsyncOperation()
{
	if (WorkerThread)
	{
		WaitForSingleObject(
			WorkerThread,
			INFINITE
		);

		CloseHandle(
			WorkerThread
		);

		WorkerThread = nullptr;
	}

	InterlockedExchange(
		&AsyncOperation,
		AsyncOperation_None
	);

	InterlockedExchange(
		&AsyncResult,
		AsyncResult_Idle
	);
}

void RmlFrontEndContext::HandleLoginResult(
	EAsyncResult Result
)
{
	switch (Result)
	{
	case AsyncResult_Success:
		r3dOutToLog(
			"[RmlUI][FrontEnd][Login] Login successful. CustomerID=%u\n",
			gUserProfile.CustomerID
		);

		BeginProfileLoad();
		break;

	case AsyncResult_Timeout:
		SetLoginControlsEnabled(true);

		SetLoginStatus(
			"Connection timed out. Check the backend server."
		);
		break;

	case AsyncResult_BadPassword:
		gUserProfile.CustomerID = 0;
		gUserProfile.SessionID = 0;
		gUserProfile.AccountStatus = 0;

		SetLoginControlsEnabled(true);

		SetLoginStatus(
			"Incorrect username or password."
		);
		break;

	case AsyncResult_Frozen:
		gUserProfile.CustomerID = 0;
		gUserProfile.SessionID = 0;
		gUserProfile.AccountStatus = 0;

		SetLoginControlsEnabled(true);

		SetLoginStatus(
			"This account is frozen."
		);
		break;

	default:
		gUserProfile.CustomerID = 0;
		gUserProfile.SessionID = 0;
		gUserProfile.AccountStatus = 0;

		SetLoginControlsEnabled(true);

		SetLoginStatus(
			"Account server returned an invalid response."
		);
		break;
	}
}

void RmlFrontEndContext::HandleProfileResult(
	EAsyncResult Result
)
{
	if (Result != AsyncResult_Success)
	{
		if (CurrentScreen == EScreen::MainMenu)
		{
			SetMainMenuControlsEnabled(true);

			SetMainMenuStatus(
				"Unable to refresh account profile."
			);
		}
		else
		{
			SetLoginControlsEnabled(true);

			SetLoginStatus(
				"Login succeeded, but the profile could not be loaded."
			);
		}

		return;
	}

	bProfileLoaded = true;

	BuildMainMenu();
	ShowMainMenu();

	r3dOutToLog(
		"[RmlUI][FrontEnd][Profile] Loaded. Characters=%d\n",
		gUserProfile.ProfileData.NumSlots
	);
}

void RmlFrontEndContext::BuildMainMenu()
{
	if (!MainMenuDocument)
		return;

	char Text[256]{};

	sprintf_s(
		Text,
		"ACCOUNT ID: %u",
		gUserProfile.CustomerID
	);

	SetElementText(
		MainMenuDocument,
		"account_id",
		Text
	);

	sprintf_s(
		Text,
		"CREDITS: %d    GOLD: %d",
		gUserProfile.ProfileData.GameDollars,
		gUserProfile.ProfileData.GamePoints
	);

	SetElementText(
		MainMenuDocument,
		"account_balance",
		Text
	);

	Rml::Element* CharacterList =
		MainMenuDocument->GetElementById(
			"character_list"
		);

	if (!CharacterList)
		return;

	Rml::String Markup;

	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	if (CharacterCount <= 0)
	{
		Markup =
			"<div class=\"empty_character\">"
			"<div class=\"empty_title\">NO CHARACTERS</div>"
			"<div class=\"empty_text\">"
			"Create your first character before joining a server."
			"</div>"
			"</div>";

		SelectedCharacterIndex = -1;

		SetElementText(
			MainMenuDocument,
			"selected_character",
			"NO CHARACTER SELECTED"
		);

		SetMainMenuStatus(
			"Character creation will be added in the next stage."
		);
	}
	else
	{
		if (
			gUserProfile.SelectedCharID < 0 ||
			gUserProfile.SelectedCharID >= CharacterCount
		)
		{
			gUserProfile.SelectedCharID = 0;
		}

		SelectedCharacterIndex =
			gUserProfile.SelectedCharID;

		for (
			int Index = 0;
			Index < CharacterCount;
			++Index
		)
		{
			const wiCharDataFull& Character =
				gUserProfile.ProfileData.ArmorySlots[Index];

			Markup += "<button id=\"char_slot_";
			Markup += std::to_string(Index);
			Markup += "\" class=\"character_slot";

			if (Index == SelectedCharacterIndex)
				Markup += " selected";

			Markup += "\">";

			Markup +=
				"<div class=\"character_name\">";

			Markup +=
				EscapeRmlText(
					Character.Gamertag
				);

			Markup += "</div>";

			Markup +=
				"<div class=\"character_state\">";

			if (Character.Alive == 1)
				Markup += "ALIVE";
			else if (Character.Alive == 3)
				Markup += "NEW CHARACTER";
			else
				Markup += "DEAD";

			Markup += "</div>";

			Markup += "</button>";
		}

		SetMainMenuStatus(
			"Select a character and press QUICK JOIN."
		);
	}

	CharacterList->SetInnerRML(
		Markup
	);

	RefreshCharacterSelection();
	SetMainMenuControlsEnabled(true);
}

void RmlFrontEndContext::SelectCharacter(
	int CharacterIndex
)
{
	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	if (
		CharacterIndex < 0 ||
		CharacterIndex >= CharacterCount
	)
	{
		return;
	}

	SelectedCharacterIndex =
		CharacterIndex;

	gUserProfile.SelectedCharID =
		CharacterIndex;

	RefreshCharacterSelection();

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.ArmorySlots[
			CharacterIndex
		];

	SetElementText(
		MainMenuDocument,
		"selected_character",
		Character.Gamertag
	);

	SetMainMenuStatus(
		"Character selected."
	);
}

void RmlFrontEndContext::RefreshCharacterSelection()
{
	if (!MainMenuDocument)
		return;

	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	for (
		int Index = 0;
		Index < CharacterCount;
		++Index
	)
	{
		char ElementId[64]{};

		sprintf_s(
			ElementId,
			"char_slot_%d",
			Index
		);

		Rml::Element* Element =
			MainMenuDocument->GetElementById(
				ElementId
			);

		if (Element)
		{
			Element->SetClass(
				"selected",
				Index ==
					SelectedCharacterIndex
			);
		}
	}

	if (
		SelectedCharacterIndex >= 0 &&
		SelectedCharacterIndex < CharacterCount
	)
	{
		const wiCharDataFull& Character =
			gUserProfile.ProfileData.ArmorySlots[
				SelectedCharacterIndex
			];

		SetElementText(
			MainMenuDocument,
			"selected_character",
			Character.Gamertag
		);
	}
}

void RmlFrontEndContext::RequestQuickJoin()
{
	if (
		CurrentScreen != EScreen::MainMenu ||
		IsBusy() ||
		!bProfileLoaded
	)
	{
		return;
	}

	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	if (CharacterCount <= 0)
	{
		SetMainMenuStatus(
			"You need to create a character first."
		);

		return;
	}

	if (
		SelectedCharacterIndex < 0 ||
		SelectedCharacterIndex >= CharacterCount
	)
	{
		SetMainMenuStatus(
			"Select a character first."
		);

		return;
	}

	gUserProfile.SelectedCharID =
		SelectedCharacterIndex;

	PendingResult =
		ERmlFrontEndResult::JoinGame;

	SetMainMenuStatus(
		"Searching for a game server..."
	);
}

void RmlFrontEndContext::SetLoginControlsEnabled(
	bool bEnabled
)
{
	SetElementEnabled(
		LoginDocument,
		"login_username",
		bEnabled
	);

	SetElementEnabled(
		LoginDocument,
		"login_password",
		bEnabled
	);

	SetElementEnabled(
		LoginDocument,
		"btn_login",
		bEnabled
	);

	SetElementEnabled(
		LoginDocument,
		"btn_login_exit",
		bEnabled
	);
}

void RmlFrontEndContext::SetMainMenuControlsEnabled(
	bool bEnabled
)
{
	const bool bCanJoin =
		bEnabled &&
		bProfileLoaded &&
		gUserProfile.ProfileData.NumSlots > 0;

	SetElementEnabled(
		MainMenuDocument,
		"btn_quick_join",
		bCanJoin
	);

	SetElementEnabled(
		MainMenuDocument,
		"btn_refresh_profile",
		bEnabled
	);

	SetElementEnabled(
		MainMenuDocument,
		"btn_create_character",
		bEnabled
	);

	SetElementEnabled(
		MainMenuDocument,
		"btn_frontend_exit",
		bEnabled
	);
}

void RmlFrontEndContext::SetElementEnabled(
	Rml::ElementDocument* Document,
	const char* ElementId,
	bool bEnabled
)
{
	if (!Document || !ElementId)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	if (bEnabled)
	{
		Element->RemoveAttribute(
			"disabled"
		);

		Element->SetClass(
			"disabled",
			false
		);
	}
	else
	{
		Element->SetAttribute(
			"disabled",
			"disabled"
		);

		Element->SetClass(
			"disabled",
			true
		);
	}
}

void RmlFrontEndContext::SetElementText(
	Rml::ElementDocument* Document,
	const char* ElementId,
	const Rml::String& Text
)
{
	if (!Document || !ElementId)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetInnerRML(
		EscapeRmlText(Text)
	);
}

Rml::String RmlFrontEndContext::GetInputValue(
	Rml::ElementDocument* Document,
	const char* ElementId
) const
{
	if (!Document || !ElementId)
		return Rml::String();

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return Rml::String();

	Rml::ElementFormControlInput* Input =
		rmlui_dynamic_cast<
			Rml::ElementFormControlInput*
		>(
			Element
		);

	if (!Input)
		return Rml::String();

	return Input->GetValue();
}

void RmlFrontEndContext::SetInputValue(
	Rml::ElementDocument* Document,
	const char* ElementId,
	const Rml::String& Value
)
{
	if (!Document || !ElementId)
		return;

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Rml::ElementFormControlInput* Input =
		rmlui_dynamic_cast<
			Rml::ElementFormControlInput*
		>(
			Element
		);

	if (Input)
		Input->SetValue(Value);
}

void RmlFrontEndContext::SetLoginStatus(
	const Rml::String& Text
)
{
	SetElementText(
		LoginDocument,
		"login_status",
		Text
	);
}

void RmlFrontEndContext::SetMainMenuStatus(
	const Rml::String& Text
)
{
	SetElementText(
		MainMenuDocument,
		"main_menu_status",
		Text
	);
}

bool RmlFrontEndContext::IsBusy() const
{
	return
		InterlockedCompareExchange(
			const_cast<volatile LONG*>(
				&AsyncResult
			),
			0,
			0
		) == AsyncResult_Working;
}

void RmlFrontEndContext::RefreshDimensions()
{
	if (!Hwnd)
	{
		Width = 1;
		Height = 1;
		return;
	}

	RECT ClientRectangle{};

	GetClientRect(
		Hwnd,
		&ClientRectangle
	);

	Width =
		std::max(
			1,
			static_cast<int>(
				ClientRectangle.right -
				ClientRectangle.left
			)
		);

	Height =
		std::max(
			1,
			static_cast<int>(
				ClientRectangle.bottom -
				ClientRectangle.top
			)
		);

	if (Context)
	{
		Context->SetDimensions(
			Rml::Vector2i(
				Width,
				Height
			)
		);
	}
}

Rml::String RmlFrontEndContext::WideToUtf8(
	const wchar_t* Text
)
{
	if (!Text || !Text[0])
		return Rml::String();

	const int Required =
		WideCharToMultiByte(
			CP_UTF8,
			0,
			Text,
			-1,
			nullptr,
			0,
			nullptr,
			nullptr
		);

	if (Required <= 1)
		return Rml::String();

	Rml::String Result;

	Result.resize(
		Required
	);

	WideCharToMultiByte(
		CP_UTF8,
		0,
		Text,
		-1,
		&Result[0],
		Required,
		nullptr,
		nullptr
	);

	Result.resize(
		Required - 1
	);

	return Result;
}

Rml::String RmlFrontEndContext::EscapeRmlText(
	const Rml::String& Text
)
{
	Rml::String Result;

	Result.reserve(
		Text.size()
	);

	for (char Character : Text)
	{
		switch (Character)
		{
		case '&':
			Result += "&amp;";
			break;

		case '<':
			Result += "&lt;";
			break;

		case '>':
			Result += "&gt;";
			break;

		case '"':
			Result += "&quot;";
			break;

		default:
			Result += Character;
			break;
		}
	}

	return Result;
}