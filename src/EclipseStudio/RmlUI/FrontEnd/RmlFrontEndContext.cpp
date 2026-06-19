#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndContext.h"
#include "../RmlRuntime.h"
#include "RmlFrontEndCharacterPreview.h"

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
#include <cctype>
#include <windowsx.h>

#include "r3dDebug.h"

namespace
{
	const char* CharacterButtonPrefix =
		"char_slot_";

	const size_t CharacterButtonPrefixLength =
		strlen(CharacterButtonPrefix);

	const char* ForbiddenCharacterNameSymbols =
		"!@#$%^&*()-=+_<>,./?'\":;|{}[]";

	constexpr int DefaultHeroItemID =
		20201;

	constexpr int AppearanceVariantCount =
		4;

	std::string TrimAscii(
		const Rml::String& Value
	)
	{
		std::string Result =
			Value;

		size_t Begin = 0;

		while (
			Begin < Result.length() &&
			std::isspace(
				static_cast<unsigned char>(
					Result[Begin]
				)
			)
		)
		{
			++Begin;
		}

		size_t End =
			Result.length();

		while (
			End > Begin &&
			std::isspace(
				static_cast<unsigned char>(
					Result[End - 1]
				)
			)
		)
		{
			--End;
		}

		return Result.substr(
			Begin,
			End - Begin
		);
	}

	const char* SkillNodeButtonPrefix =
	"skill_node_";

	const size_t SkillNodeButtonPrefixLength =
		strlen(SkillNodeButtonPrefix);

	enum class EFrontendSkillState
	{
		Learned = 0,
		Available,
		Locked
	};

	struct FFrontendSkillNode
	{
		const char* ElementId;
		int BackendSkillId;

		const char* CategoryId;
		const char* CategoryName;

		const char* DisplayName;
		const char* Description;

		const char* RankText;
		const char* LevelText;
		const char* BonusText;

		int Cost;
		int RequiredLevel;

		EFrontendSkillState State;

		const char* RequirementA;
		const char* RequirementB;
	};

	const FFrontendSkillNode FrontendSkillNodes[] =
	{
		{
			"skill_node_endurance_1",
			101,
			"survival",
			"SURVIVAL SKILL",
			"ENDURANCE",
			"Increases stamina capacity and improves movement endurance during long raids.",
			"I",
			"1 / 5",
			"+2% STAMINA",
			0,
			1,
			EFrontendSkillState::Learned,
			"Base survival training completed",
			"Survivor Level 1 reached"
		},
		{
			"skill_node_vitality_1",
			102,
			"survival",
			"SURVIVAL SKILL",
			"VITALITY",
			"Increases maximum health and improves survivability during long raids.",
			"II",
			"0 / 5",
			"+2% MAX HEALTH",
			1,
			2,
			EFrontendSkillState::Available,
			"Endurance Level 1 learned",
			"Survivor Level 2 reached"
		},
		{
			"skill_node_resistance_1",
			103,
			"survival",
			"SURVIVAL SKILL",
			"RESISTANCE",
			"Improves resistance against toxic damage, bleeding and infection effects.",
			"III",
			"0 / 5",
			"+2% RESISTANCE",
			2,
			5,
			EFrontendSkillState::Locked,
			"Vitality Level 1 learned",
			"Survivor Level 5 reached"
		},

		{
			"skill_node_recoil_1",
			201,
			"combat",
			"COMBAT SKILL",
			"RECOIL CONTROL",
			"Improves weapon stability while firing automatic and semi-automatic weapons.",
			"I",
			"1 / 5",
			"-2% RECOIL",
			0,
			1,
			EFrontendSkillState::Learned,
			"Base combat training completed",
			"Survivor Level 1 reached"
		},
		{
			"skill_node_reload_1",
			202,
			"combat",
			"COMBAT SKILL",
			"FAST RELOAD",
			"Reduces reload time for rifles, handguns and shotguns.",
			"II",
			"0 / 5",
			"+2% RELOAD SPEED",
			1,
			2,
			EFrontendSkillState::Available,
			"Recoil Control Level 1 learned",
			"Survivor Level 2 reached"
		},
		{
			"skill_node_marksman_1",
			203,
			"combat",
			"COMBAT SKILL",
			"MARKSMAN",
			"Improves weapon accuracy and long range handling.",
			"III",
			"0 / 5",
			"+2% ACCURACY",
			2,
			5,
			EFrontendSkillState::Locked,
			"Fast Reload Level 1 learned",
			"Survivor Level 5 reached"
		},

		{
			"skill_node_scavenger_1",
			301,
			"support",
			"SUPPORT SKILL",
			"SCAVENGER",
			"Improves looting efficiency and survival resource awareness.",
			"I",
			"0 / 5",
			"+2% LOOT BONUS",
			1,
			1,
			EFrontendSkillState::Available,
			"Base support training available",
			"Survivor Level 1 reached"
		},
		{
			"skill_node_medtech_1",
			302,
			"support",
			"SUPPORT SKILL",
			"MED TECH",
			"Improves medical item efficiency and treatment speed.",
			"II",
			"0 / 5",
			"+2% MEDICAL EFFECT",
			1,
			3,
			EFrontendSkillState::Locked,
			"Scavenger Level 1 learned",
			"Survivor Level 3 reached"
		},
		{
			"skill_node_repair_1",
			303,
			"support",
			"SUPPORT SKILL",
			"FIELD REPAIR",
			"Improves field repair and item maintenance.",
			"III",
			"0 / 5",
			"+2% REPAIR QUALITY",
			2,
			5,
			EFrontendSkillState::Locked,
			"Med Tech Level 1 learned",
			"Survivor Level 5 reached"
		},

		{
			"skill_node_crafting_1",
			401,
			"crafting",
			"CRAFTING SKILL",
			"BASIC CRAFT",
			"Unlocks basic crafting improvements and simple recipe efficiency.",
			"I",
			"0 / 5",
			"+2% CRAFT SPEED",
			1,
			1,
			EFrontendSkillState::Available,
			"Base crafting available",
			"Survivor Level 1 reached"
		},
		{
			"skill_node_trader_1",
			402,
			"crafting",
			"CRAFTING SKILL",
			"TRADER",
			"Improves trade efficiency and marketplace knowledge.",
			"II",
			"0 / 5",
			"+2% TRADE BONUS",
			1,
			3,
			EFrontendSkillState::Locked,
			"Basic Craft Level 1 learned",
			"Survivor Level 3 reached"
		},
		{
			"skill_node_engineer_1",
			403,
			"crafting",
			"CRAFTING SKILL",
			"ENGINEER",
			"Improves engineering, advanced crafting and technical work.",
			"III",
			"0 / 5",
			"+2% ENGINEERING",
			2,
			5,
			EFrontendSkillState::Locked,
			"Trader Level 1 learned",
			"Survivor Level 5 reached"
		}
	};

	const size_t FrontendSkillNodeCount =
		sizeof(FrontendSkillNodes) /
		sizeof(FrontendSkillNodes[0]);

	const FFrontendSkillNode* FindFrontendSkillNode(
		const Rml::String& ElementId
	)
	{
		for (
			size_t Index = 0;
			Index < FrontendSkillNodeCount;
			++Index
		)
		{
			if (
				ElementId ==
				FrontendSkillNodes[Index].ElementId
			)
			{
				return &FrontendSkillNodes[Index];
			}
		}

		return nullptr;
	}

	const char* GetFrontendSkillStateText(
		EFrontendSkillState State
	)
	{
		switch (State)
		{
		case EFrontendSkillState::Learned:
			return "LEARNED";

		case EFrontendSkillState::Available:
			return "AVAILABLE";

		default:
			return "LOCKED";
		}
	}

	const char* GetFrontendSkillStateClass(
		EFrontendSkillState State
	)
	{
		switch (State)
		{
		case EFrontendSkillState::Learned:
			return "learned";

		case EFrontendSkillState::Available:
			return "available";

		default:
			return "locked";
		}
	}

	const char* ShopItemButtonPrefix =
	"shop_item_";

	const size_t ShopItemButtonPrefixLength =
		strlen(ShopItemButtonPrefix);

	struct FFrontendShopItem
	{
		const char* ElementId;
		int BackendItemId;

		const char* CategoryId;
		const char* CategoryName;

		const char* DisplayName;
		const char* TypeName;
		const char* Description;

		const char* IconText;
		const char* BadgeText;

		const char* GcPriceText;
		const char* GdPriceText;

		int Damage;
		int Range;
		int Recoil;
		int Weight;
	};

	const FFrontendShopItem FrontendShopItems[] =
	{
		{
			"shop_item_0",
			101001,
			"featured",
			"FEATURED ITEM",
			"AK-74M",
			"ASSAULT RIFLE",
			"Reliable military assault rifle with solid damage, controllable recoil and good field durability.",
			"AK",
			"FEATURED ITEM",
			"195",
			"15 000",
			72,
			64,
			48,
			38
		},
		{
			"shop_item_1",
			20015,
			"gear",
			"BODY ARMOR",
			"Custom Guerilla",
			"BODY ARMOR",
			"Custom designed guerilla field gear and body armor for fast moving survival operations.",
			"AR",
			"NEW ITEM",
			"115",
			"8 500",
			25,
			30,
			15,
			40
		},
		{
			"shop_item_2",
			20006,
			"gear",
			"HELMET",
			"K. Style Helmet",
			"HELMET",
			"Compact tactical helmet with basic head protection for hostile environments.",
			"HL",
			"LEVEL LOCKED",
			"200",
			"LOCKED",
			30,
			20,
			10,
			30
		},
		{
			"shop_item_3",
			301003,
			"backpacks",
			"BACKPACK",
			"Medium Backpack",
			"BACKPACK",
			"Medium storage backpack for carrying additional weapons, supplies and loot.",
			"BP",
			"SALE ITEM",
			"90",
			"6 000",
			0,
			0,
			0,
			55
		},
		{
			"shop_item_4",
			400004,
			"consumables",
			"CONSUMABLE",
			"Medkit",
			"CONSUMABLE",
			"Medical survival kit used to restore health during raids and emergency encounters.",
			"MD",
			"MEDICAL",
			"25",
			"1 500",
			0,
			0,
			0,
			12
		},
		{
			"shop_item_5",
			500005,
			"crates",
			"LOOT CRATE",
			"Survivor Crate",
			"LOOT CRATE",
			"Randomized survivor supply crate. Contents are controlled by backend rules.",
			"CR",
			"RANDOM BOX",
			"75",
			"RANDOM",
			0,
			0,
			0,
			20
		}
	};

	const size_t FrontendShopItemCount =
		sizeof(FrontendShopItems) /
		sizeof(FrontendShopItems[0]);

	const FFrontendShopItem* FindFrontendShopItem(
		const Rml::String& ElementId
	)
	{
		for (
			size_t Index = 0;
			Index < FrontendShopItemCount;
			++Index
		)
		{
			if (
				ElementId ==
				FrontendShopItems[Index].ElementId
			)
			{
				return &FrontendShopItems[Index];
			}
		}

		return nullptr;
	}

	std::string FormatPlayedTime(int TotalSeconds)
	{
		if (TotalSeconds < 0)
			TotalSeconds = 0;

		const int Days =
			TotalSeconds / 86400;

		const int Hours =
			(TotalSeconds / 3600) % 24;

		const int Minutes =
			(TotalSeconds / 60) % 60;

		char Text[64]{};

		sprintf_s(
			Text,
			"%dd %02dh %02dm",
			Days,
			Hours,
			Minutes
		);

		return Text;
	}

	struct FFrontendLevelProgress
	{
		int Level = 1;
		int TotalExperience = 0;
		int NextLevelExperience = 100;
		float Percent = 0.0f;
	};

	FFrontendLevelProgress
	CalculateFrontendLevelProgress(
		int TotalExperience
	)
	{
		FFrontendLevelProgress Result;

		Result.TotalExperience =
			std::max(
				0,
				TotalExperience
			);

		/*
		 * Временная frontend-кривая:
		 * каждые 100 XP повышают отображаемый уровень.
		 *
		 * Когда появится отдельная серверная таблица уровней,
		 * менять нужно будет только эту функцию.
		 */
		Result.Level =
			Result.TotalExperience / 100 + 1;

		const int CurrentLevelStart =
			(Result.Level - 1) * 100;

		Result.NextLevelExperience =
			Result.Level * 100;

		const int ExperienceInsideLevel =
			Result.TotalExperience -
			CurrentLevelStart;

		Result.Percent =
			static_cast<float>(
				ExperienceInsideLevel
			);

		Result.Percent =
			std::clamp(
				Result.Percent,
				0.0f,
				100.0f
			);

		return Result;
	}

	std::string FormatGroupedNumber(
		long long Value
	)
	{
		const bool bNegative =
			Value < 0;

		unsigned long long AbsoluteValue =
			bNegative
				? static_cast<unsigned long long>(
					-Value
				)
				: static_cast<unsigned long long>(
					Value
				);

		std::string Result =
			std::to_string(
				AbsoluteValue
			);

		for (
			int Position =
				static_cast<int>(
					Result.length()
				) - 3;
			Position > 0;
			Position -= 3
		)
		{
			Result.insert(
				static_cast<size_t>(
					Position
				),
				" "
			);
		}

		if (bNegative)
		{
			Result.insert(
				Result.begin(),
				'-'
			);
		}

		return Result;
	}

	const char* GetExperienceTitle(
		int Level
	)
	{
		if (Level >= 50)
			return "VETERAN";

		if (Level >= 25)
			return "EXPERIENCED";

		if (Level >= 10)
			return "SEASONED";

		if (Level >= 5)
			return "SURVIVOR";

		return "RECRUIT";
	}

	const char* GetCharacterRole(
		uint32_t HeroItemID
	)
	{
		switch (HeroItemID)
		{
		case 20174:
			return "EX MILITARY";

		default:
			return "SURVIVOR";
		}
	}
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
	ClickListener = std::make_unique<FClickListener>(this);
	CharacterPreview = std::make_unique<RmlFrontEndCharacterPreview>();

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
	CancelPreviewDrag();

	if (CharacterPreview)
	{
		CharacterPreview->Shutdown();
		CharacterPreview.reset();
	}

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

	SelectedSkillElementId = "skill_node_vitality_1";
	SelectedSkillBackendId = 0;

	SelectedShopItemElementId = "shop_item_0";
	SelectedShopBackendItemId = 0;

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

	CharacterCreateDocument =
		Context->LoadDocument(
			"Rml/FrontEnd/CharacterCreate.rml"
		);

	if (!CharacterCreateDocument)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/CharacterCreate.rml\n"
		);

		return false;
	}

	SkillsDocument =
	Context->LoadDocument(
		"Rml/FrontEnd/Skills.rml"
	);

	if (!SkillsDocument)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/Skills.rml\n"
		);

		return false;
	}

	ShopDocument =
	Context->LoadDocument(
		"Rml/FrontEnd/Shop.rml"
	);

	if (!ShopDocument)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/Shop.rml\n"
		);

		return false;
	}

	LoginDocument->Hide();
	MainMenuDocument->Hide();
	CharacterCreateDocument->Hide();
	SkillsDocument->Hide();
	ShopDocument->Hide();

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

	if (CharacterCreateDocument)
	{
		Context->UnloadDocument(
			CharacterCreateDocument
		);

		CharacterCreateDocument = nullptr;
	}

	if (SkillsDocument)
	{
		Context->UnloadDocument(
			SkillsDocument
		);

		SkillsDocument = nullptr;
	}

	if (ShopDocument)
	{
		Context->UnloadDocument(
			ShopDocument
		);

		ShopDocument = nullptr;
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

	if (CharacterCreateDocument)
	{
		CharacterCreateDocument->AddEventListener(
			"click",
			ClickListener.get()
		);
	}

	if (SkillsDocument)
	{
		SkillsDocument->AddEventListener(
			"click",
			ClickListener.get()
		);
	}

	if (ShopDocument)
	{
		ShopDocument->AddEventListener(
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

	if (CharacterCreateDocument)
	{
		CharacterCreateDocument->RemoveEventListener(
			"click",
			ClickListener.get()
		);
	}

	if (SkillsDocument)
	{
		SkillsDocument->RemoveEventListener(
			"click",
			ClickListener.get()
		);
	}

	if (ShopDocument)
	{
		ShopDocument->RemoveEventListener(
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
	if (
		!bInitialized ||
		!Context
	)
	{
		return;
	}

	if (
		CharacterPreview &&
		CurrentScreen == EScreen::MainMenu ||
		CurrentScreen == EScreen::Skills ||
		CurrentScreen == EScreen::Shop
	)
	{
		CharacterPreview->
			RenderFrame();
	}

	RmlRuntime::Get().
		RenderContext(
			Context,
			Width,
			Height
		);
}

void RmlFrontEndContext::PrepareRender()
{
	if (
		!bInitialized ||
		!CharacterPreview
	)
	{
		return;
	}

	if (
		CurrentScreen != EScreen::MainMenu &&
		CurrentScreen != EScreen::Skills &&
		CurrentScreen != EScreen::Shop
	)
	{
		return;
	}

	CharacterPreview->
		PrepareFrame();
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
		!IsBusy()
	)
	{
		if (CurrentScreen == EScreen::Login)
		{
			RequestLogin();
			return true;
		}

		if (
			CurrentScreen ==
			EScreen::CharacterCreate
		)
		{
			RequestCreateCharacter();
			return true;
		}
	}

	if (
		Message == WM_KEYDOWN &&
		WParam == VK_ESCAPE &&
		CurrentScreen ==
			EScreen::CharacterCreate &&
		!IsBusy()
	)
	{
		ShowMainMenu();
		return true;
	}

	if (
		Message == WM_KEYDOWN &&
		WParam == VK_ESCAPE &&
		CurrentScreen == EScreen::Skills &&
		!IsBusy()
	)
	{
		ShowMainMenu();
		return true;
	}

	if (
		Message == WM_KEYDOWN &&
		WParam == VK_ESCAPE &&
		CurrentScreen == EScreen::Shop &&
		!IsBusy()
	)
	{
		ShowMainMenu();
		return true;
	}

	if (
		Message == WM_KEYDOWN &&
		WParam == VK_RETURN &&
		CurrentScreen == EScreen::Skills &&
		!IsBusy()
	)
	{
		RequestLearnSelectedSkill();
		return true;
	}

	const bool bRmlHandled =
		RmlRuntime::Get().
			ProcessWin32Message(
				Context,
				WindowHandle,
				Message,
				WParam,
				LParam,
				OutResult
			);

	if (
	(
		CurrentScreen != EScreen::MainMenu &&
		CurrentScreen != EScreen::Skills &&
		CurrentScreen != EScreen::Shop
	) ||
		!CharacterPreview ||
		!CharacterPreview->
			IsInitialized()
	)
	{
		return bRmlHandled;
	}

	if (
		Message == WM_KEYDOWN &&
		WParam == 'R' &&
		IsPointerOverMainMenuElement(
			"character_preview_stage"
		)
	)
	{
		CharacterPreview->ResetView();
		return true;
	}

	switch (Message)
	{
	case WM_LBUTTONDOWN:
		if (
			IsPointerOverMainMenuElement(
				"character_preview_stage"
			)
		)
		{
			PreviewDragMode =
				EPreviewDragMode::Rotate;

			PreviewDragLastPoint.x =
				GET_X_LPARAM(
					LParam
				);

			PreviewDragLastPoint.y =
				GET_Y_LPARAM(
					LParam
				);

			SetCapture(
				WindowHandle
			);

			return true;
		}
		break;

	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		if (
			IsPointerOverMainMenuElement(
				"character_preview_stage"
			)
		)
		{
			PreviewDragMode =
				EPreviewDragMode::Move;

			PreviewDragLastPoint.x =
				GET_X_LPARAM(
					LParam
				);

			PreviewDragLastPoint.y =
				GET_Y_LPARAM(
					LParam
				);

			SetCapture(
				WindowHandle
			);

			return true;
		}
		break;

	case WM_MOUSEMOVE:
		if (
			PreviewDragMode !=
				EPreviewDragMode::None
		)
		{
			const POINT CurrentPoint
			{
				GET_X_LPARAM(
					LParam
				),
				GET_Y_LPARAM(
					LParam
				)
			};

			const float DeltaX =
				static_cast<float>(
					CurrentPoint.x -
					PreviewDragLastPoint.x
				);

			const float DeltaY =
				static_cast<float>(
					CurrentPoint.y -
					PreviewDragLastPoint.y
				);

			PreviewDragLastPoint =
				CurrentPoint;

			if (
				PreviewDragMode ==
				EPreviewDragMode::Rotate
			)
			{
				if (!(WParam & MK_LBUTTON))
				{
					CancelPreviewDrag();
					break;
				}

				CharacterPreview->Rotate(
					DeltaX,
					DeltaY
				);
			}
			else
			{
				if (
					!(WParam & MK_RBUTTON) &&
					!(WParam & MK_MBUTTON)
				)
				{
					CancelPreviewDrag();
					break;
				}

				CharacterPreview->Move(
					DeltaX,
					DeltaY
				);
			}

			return true;
		}
		break;

	case WM_LBUTTONUP:
		if (
			PreviewDragMode ==
			EPreviewDragMode::Rotate
		)
		{
			CancelPreviewDrag();
			return true;
		}
		break;

	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if (
			PreviewDragMode ==
			EPreviewDragMode::Move
		)
		{
			CancelPreviewDrag();
			return true;
		}
		break;

	case WM_MOUSEWHEEL:
		if (
			IsPointerOverMainMenuElement(
				"character_preview_stage"
			)
		)
		{
			const float WheelSteps =
				static_cast<float>(
					GET_WHEEL_DELTA_WPARAM(
						WParam
					)
				) /
				static_cast<float>(
					WHEEL_DELTA
				);

			CharacterPreview->Zoom(
				WheelSteps
			);

			return true;
		}
		break;

	case WM_CAPTURECHANGED:
	case WM_CANCELMODE:
	case WM_KILLFOCUS:
		CancelPreviewDrag();
		break;

	default:
		break;
	}

	return bRmlHandled;
}

bool RmlFrontEndContext::IsElementOrChildOfId(
	Rml::Element* Element,
	const char* ParentId
) const
{
	if (!Element || !ParentId)
		return false;

	Rml::Element* Current =
		Element;

	while (Current)
	{
		if (
			Current->GetId() ==
			ParentId
		)
		{
			return true;
		}

		if (
			Current ==
			MainMenuDocument
		)
		{
			break;
		}

		Current =
			Current->GetParentNode();
	}

	return false;
}

bool RmlFrontEndContext::
IsPointerOverMainMenuElement(
	const char* ElementId
) const
{
	if (
		!Context ||
		!MainMenuDocument ||
		!ElementId
	)
	{
		return false;
	}

	return IsElementOrChildOfId(
		Context->GetHoverElement(),
		ElementId
	);
}

void RmlFrontEndContext::CancelPreviewDrag()
{
	PreviewDragMode =
		EPreviewDragMode::None;

	if (
		Hwnd &&
		GetCapture() == Hwnd
	)
	{
		ReleaseCapture();
	}
}

void RmlFrontEndContext::SetElementProperty(
	Rml::ElementDocument* Document,
	const char* ElementId,
	const char* PropertyName,
	const Rml::String& Value
)
{
	if (
		!Document ||
		!ElementId ||
		!PropertyName
	)
	{
		return;
	}

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetProperty(
		PropertyName,
		Value
	);
}

void RmlFrontEndContext::SetElementClass(
	Rml::ElementDocument* Document,
	const char* ElementId,
	const char* ClassName,
	bool bEnabled
)
{
	if (
		!Document ||
		!ElementId ||
		!ClassName
	)
	{
		return;
	}

	Rml::Element* Element =
		Document->GetElementById(
			ElementId
		);

	if (!Element)
		return;

	Element->SetClass(
		ClassName,
		bEnabled
	);
}

void RmlFrontEndContext::SetElementPercent(
	Rml::ElementDocument* Document,
	const char* ElementId,
	float Percent
)
{
	Percent =
		std::clamp(
			Percent,
			0.0f,
			100.0f
		);

	char Value[32]{};

	sprintf_s(
		Value,
		"%.2f%%",
		Percent
	);

	SetElementProperty(
		Document,
		ElementId,
		"width",
		Value
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

	if (
		Result ==
			ERmlFrontEndResult::JoinGame &&
		CharacterPreview
	)
	{
		CharacterPreview->Shutdown();
	}

	return Result;
}

void RmlFrontEndContext::ShowLogin()
{
	if (
		!LoginDocument ||
		!MainMenuDocument ||
		!CharacterCreateDocument ||
		SkillsDocument ||
		!ShopDocument
	)
	{
		return;
	}

	MainMenuDocument->Hide();
	CharacterCreateDocument->Hide();
	LoginDocument->Show();
	SkillsDocument->Hide();
	ShopDocument->Hide();

	CurrentScreen =
		EScreen::Login;

	SetLoginControlsEnabled(
		!IsBusy()
	);

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlFrontEndContext::ShowMainMenu()
{
	if (
		!LoginDocument ||
		!MainMenuDocument ||
		!CharacterCreateDocument ||
		!SkillsDocument ||
		!ShopDocument
	)
	{
		return;
	}

	LoginDocument->Hide();
	CharacterCreateDocument->Hide();
	SkillsDocument->Hide();
	ShopDocument->Hide();
	MainMenuDocument->Show();

	CurrentScreen =
		EScreen::MainMenu;

	SetMainMenuControlsEnabled(
		!IsBusy()
	);

	if (bProfileLoaded && gUserProfile.ProfileData.NumSlots > 0)
	{
		EnsureCharacterPreview();
	}

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlFrontEndContext::ShowSkills()
{
	if (
		!LoginDocument ||
		!MainMenuDocument ||
		!CharacterCreateDocument ||
		!SkillsDocument ||
		ShopDocument
	)
	{
		return;
	}

	if (
		!bProfileLoaded ||
		IsBusy()
	)
	{
		SetMainMenuStatus(
			"Profile is not ready for Skills screen."
		);

		return;
	}

	LoginDocument->Hide();
	MainMenuDocument->Hide();
	CharacterCreateDocument->Hide();
	ShopDocument->Hide();
	SkillsDocument->Show();

	CurrentScreen =
		EScreen::Skills;

	BuildSkills();

	SetSkillsControlsEnabled(
		true
	);

	if (
		bProfileLoaded &&
		gUserProfile.ProfileData.NumSlots > 0
	)
	{
		EnsureCharacterPreview();
	}

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlFrontEndContext::ShowShop()
{
	if (
		!LoginDocument ||
		!MainMenuDocument ||
		!CharacterCreateDocument ||
		!SkillsDocument ||
		!ShopDocument
	)
	{
		return;
	}

	if (
		!bProfileLoaded ||
		IsBusy()
	)
	{
		SetMainMenuStatus(
			"Profile is not ready for Shop screen."
		);

		return;
	}

	LoginDocument->Hide();
	MainMenuDocument->Hide();
	CharacterCreateDocument->Hide();
	SkillsDocument->Hide();
	ShopDocument->Show();

	CurrentScreen =
		EScreen::Shop;

	BuildShop();

	SetShopControlsEnabled(
		true
	);

	if (
		bProfileLoaded &&
		gUserProfile.ProfileData.NumSlots > 0
	)
	{
		EnsureCharacterPreview();
	}

	RmlRuntime::Get().SetActiveContext(
		Context
	);
}

void RmlFrontEndContext::ShowCharacterCreate()
{
	if (
		!CharacterCreateDocument ||
		!bProfileLoaded ||
		IsBusy()
	)
	{
		return;
	}

	if (
		gUserProfile.ProfileData.NumSlots >=
		wiUserProfile::MAX_LOADOUT_SLOTS
	)
	{
		SetMainMenuStatus(
			"Maximum character count reached."
		);

		return;
	}

	LoginDocument->Hide();
	MainMenuDocument->Hide();
	SkillsDocument->Hide();
	ShopDocument->Hide();
	CharacterCreateDocument->Show();

	CurrentScreen =
		EScreen::CharacterCreate;

	ResetCharacterCreate();

	SetCharacterCreateControlsEnabled(
		true
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

	if (CharacterPreview)
	{
		CharacterPreview->Shutdown();
	}

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

		if (Id == "btn_rename_character")
		{
			RequestRenameCharacter();
			return;
		}

		if (Id == "btn_refresh_profile")
		{
			RefreshProfile();
			return;
		}

		if (Id == "btn_reset_preview")
		{
			if (CharacterPreview)
			{
				CharacterPreview->
					ResetView();
			}

			SetMainMenuStatus(
				"Character preview reset."
			);

			return;
		}

		if (Id == "nav_survivor")
		{
			if (
				CurrentScreen == EScreen::Skills ||
				CurrentScreen == EScreen::Shop
			)
			{
				ShowMainMenu();
			}
			else
			{
				SetMainMenuStatus(
					"Survivor profile active."
				);
			}

			return;
		}

		if (Id == "nav_shop")
		{
			ShowShop();
			return;
		}

		if (Id == "nav_community")
		{
			SetMainMenuStatus(
				"Community screen is not connected yet."
			);

			return;
		}

		if (Id == "nav_skills")
		{
			ShowSkills();
			return;
		}

		if (Id == "nav_equipment")
		{
			SetMainMenuStatus(
				"Equipment screen is not connected yet."
			);

			return;
		}

		if (Id == "nav_clan")
		{
			SetMainMenuStatus(
				"Clan screen is not connected yet."
			);

			return;
		}

		if (Id == "nav_awards")
		{
			SetMainMenuStatus(
				"Awards screen is not connected yet."
			);

			return;
		}

		if (Id == "btn_global_inventory")
		{
			SetMainMenuStatus(
				"Global Inventory is not connected yet."
			);

			return;
		}

		if (Id == "btn_skill_tree")
		{
			ShowSkills();
			return;
		}

		if (Id == "btn_customize_character")
		{
			SetMainMenuStatus(
				"Character customization is not connected yet."
			);

			return;
		}

		if (Id == "btn_view_rewards")
		{
			SetMainMenuStatus(
				"Rewards screen is not connected yet."
			);

			return;
		}

		if (
			Id == "btn_options" ||
			Id == "btn_settings"
		)
		{
			SetMainMenuStatus(
				"Options screen is not connected yet."
			);

			return;
		}

		if (Id == "btn_social")
		{
			SetMainMenuStatus(
				"Social panel is not connected yet."
			);

			return;
		}

		if (Id == "btn_messages")
		{
			SetMainMenuStatus(
				"Messages panel is not connected yet."
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

		if (Id == "btn_shop_buy_selected")
		{
			SetShopStatus(
				"Shop purchase backend is not connected yet."
			);

			return;
		}

		if (Id == "btn_shop_preview_item")
		{
			SetShopStatus(
				"Item preview is not connected yet."
			);

			return;
		}

		if (Id == "btn_shop_sort")
		{
			SetShopStatus(
				"Shop sorting is visual only."
			);

			return;
		}

		if (Id == "btn_shop_refresh")
		{
			BuildShop();

			SetShopStatus(
				"Shop visual data refreshed."
			);

			return;
		}

		if (
			Id.compare(
				0,
				ShopItemButtonPrefixLength,
				ShopItemButtonPrefix
			) == 0
		)
		{
			SelectShopItem(
				Id
			);

			return;
		}

		if (
			Id == "shop_category_featured" ||
			Id == "shop_category_weapons" ||
			Id == "shop_category_gear" ||
			Id == "shop_category_backpacks" ||
			Id == "shop_category_consumables" ||
			Id == "shop_category_crates"
		)
		{
			SetShopStatus(
				"Shop category selected. Backend filtering is not connected yet."
			);

			return;
		}

		if (
			Id == "shop_tab_hot" ||
			Id == "shop_tab_new" ||
			Id == "shop_tab_sale" ||
			Id == "shop_tab_owned"
		)
		{
			SetShopStatus(
				"Shop tab selected. Backend filtering is not connected yet."
			);

			return;
		}

		if (Id == "btn_learn_selected_skill")
		{
			RequestLearnSelectedSkill();
			return;
		}

		if (Id == "btn_reset_skills")
		{
			SetSkillsStatus(
				"Skill reset is not connected to backend yet."
			);

			return;
		}

		if (
			Id.compare(
				0,
				SkillNodeButtonPrefixLength,
				SkillNodeButtonPrefix
			) == 0
		)
		{
			SelectSkillNode(
				Id
			);

			return;
		}

		if (
			Id == "skill_category_survival" ||
			Id == "skill_category_combat" ||
			Id == "skill_category_support" ||
			Id == "skill_category_crafting"
		)
		{
			SetSkillsStatus(
				"Skill category selected."
			);

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

		if (Current == LoginDocument ||
			Current == MainMenuDocument ||
			Current == CharacterCreateDocument ||
			Current == SkillsDocument ||
			Current == ShopDocument
		)
		{
			break;
		}

		Current =
			Current->GetParentNode();
	}
}

void RmlFrontEndContext::ResetCharacterCreate()
{
	CreateGamertag[0] = 0;

	CreateHeroItemID =
		DefaultHeroItemID;

	CreateHardcore = 0;

	CreateHeadIndex = 0;
	CreateBodyIndex = 0;
	CreateLegsIndex = 0;

	SetInputValue(
		CharacterCreateDocument,
		"create_character_name",
		""
	);

	SetElementText(
		CharacterCreateDocument,
		"create_hero_name",
		"ASIAN MALE"
	);

	SetElementText(
		CharacterCreateDocument,
		"create_hero_item",
		"20201"
	);

	SetElementText(
		CharacterCreateDocument,
		"create_game_mode",
		"NORMAL"
	);

	SetCharacterCreateStatus(
		"Configure your survivor and enter a name."
	);

	RefreshCharacterCreateAppearance();
}

void RmlFrontEndContext::AdjustCharacterAppearance(
	const Rml::String& ControlId
)
{
	if (ControlId == "btn_create_head_prev")
	{
		if (CreateHeadIndex > 0)
			--CreateHeadIndex;
	}
	else if (ControlId == "btn_create_head_next")
	{
		if (
			CreateHeadIndex <
			AppearanceVariantCount - 1
		)
		{
			++CreateHeadIndex;
		}
	}
	else if (ControlId == "btn_create_body_prev")
	{
		if (CreateBodyIndex > 0)
			--CreateBodyIndex;
	}
	else if (ControlId == "btn_create_body_next")
	{
		if (
			CreateBodyIndex <
			AppearanceVariantCount - 1
		)
		{
			++CreateBodyIndex;
		}
	}
	else if (ControlId == "btn_create_legs_prev")
	{
		if (CreateLegsIndex > 0)
			--CreateLegsIndex;
	}
	else if (ControlId == "btn_create_legs_next")
	{
		if (
			CreateLegsIndex <
			AppearanceVariantCount - 1
		)
		{
			++CreateLegsIndex;
		}
	}

	RefreshCharacterCreateAppearance();
}

void RmlFrontEndContext::RefreshCharacterCreateAppearance()
{
	char Text[32]{};

	sprintf_s(
		Text,
		"%d / %d",
		CreateHeadIndex + 1,
		AppearanceVariantCount
	);

	SetElementText(
		CharacterCreateDocument,
		"create_head_value",
		Text
	);

	sprintf_s(
		Text,
		"%d / %d",
		CreateBodyIndex + 1,
		AppearanceVariantCount
	);

	SetElementText(
		CharacterCreateDocument,
		"create_body_value",
		Text
	);

	sprintf_s(
		Text,
		"%d / %d",
		CreateLegsIndex + 1,
		AppearanceVariantCount
	);

	SetElementText(
		CharacterCreateDocument,
		"create_legs_value",
		Text
	);
}

void RmlFrontEndContext::RequestCreateCharacter()
{
	if (
		CurrentScreen != EScreen::CharacterCreate ||
		IsBusy() ||
		!bProfileLoaded
	)
	{
		return;
	}

	if (
		gUserProfile.ProfileData.NumSlots >=
		wiUserProfile::MAX_LOADOUT_SLOTS
	)
	{
		SetCharacterCreateStatus(
			"Maximum character count reached."
		);

		return;
	}

	const std::string Gamertag =
		TrimAscii(
			GetInputValue(
				CharacterCreateDocument,
				"create_character_name"
			)
		);

	if (Gamertag.length() < 4)
	{
		SetCharacterCreateStatus(
			"Character name must contain at least 4 characters."
		);

		return;
	}

	if (Gamertag.length() > 16)
	{
		SetCharacterCreateStatus(
			"Character name cannot exceed 16 characters."
		);

		return;
	}

	if (
		Gamertag.find_first_of(
			ForbiddenCharacterNameSymbols
		) != std::string::npos
	)
	{
		SetCharacterCreateStatus(
			"Character name contains forbidden symbols."
		);

		return;
	}

	strncpy_s(
		CreateGamertag,
		sizeof(CreateGamertag),
		Gamertag.c_str(),
		_TRUNCATE
	);

	SetCharacterCreateControlsEnabled(
		false
	);

	SetCharacterCreateStatus(
		"Creating survivor..."
	);

	if (!StartAsyncOperation(
		AsyncOperation_CreateCharacter
	))
	{
		SetCharacterCreateControlsEnabled(
			true
		);

		SetCharacterCreateStatus(
			"Unable to start character creation."
		);
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

bool IsValidAsciiGamertag(
	const std::string& Value
)
{
	if (
		Value.length() < 4 ||
		Value.length() > 16
	)
	{
		return false;
	}

	for (const unsigned char Character : Value)
	{
		const bool bLetter =
			(
				Character >= 'A' &&
				Character <= 'Z'
			)
			||
			(
				Character >= 'a' &&
				Character <= 'z'
			);

		const bool bDigit =
			Character >= '0' &&
			Character <= '9';

		if (!bLetter && !bDigit)
			return false;
	}

	return true;
}

void RmlFrontEndContext::RequestRenameCharacter()
{
	if (
		CurrentScreen != EScreen::MainMenu ||
		IsBusy() ||
		!bProfileLoaded
	)
	{
		return;
	}

	if (
		gUserProfile.ProfileData.NumSlots <= 0
	)
	{
		SetMainMenuStatus(
			"Account has no permanent survivor."
		);

		return;
	}

	const std::string Gamertag =
		TrimAscii(
			GetInputValue(
				MainMenuDocument,
				"rename_character_name"
			)
		);

	if (!IsValidAsciiGamertag(
		Gamertag
	))
	{
		SetMainMenuStatus(
			"Nickname must contain 4-16 letters or digits."
		);

		return;
	}

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.ArmorySlots[0];

	if (
		_stricmp(
			Character.Gamertag,
			Gamertag.c_str()
		) == 0
	)
	{
		SetMainMenuStatus(
			"This is already your current nickname."
		);

		return;
	}

	strncpy_s(
		RenameGamertag,
		sizeof(RenameGamertag),
		Gamertag.c_str(),
		_TRUNCATE
	);

	SetMainMenuControlsEnabled(
		false
	);

	SetMainMenuStatus(
		"Changing survivor nickname..."
	);

	if (!StartAsyncOperation(
		AsyncOperation_RenameCharacter
	))
	{
		SetMainMenuControlsEnabled(
			true
		);

		SetMainMenuStatus(
			"Unable to start nickname change."
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

	InterlockedExchange(
		&AsyncApiCode,
		0
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
	else if (
		Operation ==
		AsyncOperation_RenameCharacter)
	{
		const int ApiCode =
			gUserProfile.ApiCharRename(
				RenameGamertag
			);

		InterlockedExchange(
			&AsyncApiCode,
			ApiCode
		);

		r3dOutToLog(
			"[RmlUI][FrontEnd][Rename] "
			"ApiCharRename result=%d\n",
			ApiCode
		);

		if (ApiCode == 0)
		{
			Result =
				AsyncResult_Success;
		}
		else if (ApiCode == 8)
		{
			Result =
				AsyncResult_Timeout;
		}
		else
		{
			Result =
				AsyncResult_Error;
		}
	}
	else if (
		Operation ==
		AsyncOperation_CreateCharacter
	)
	{
		const int ApiCode =
			gUserProfile.ApiCharCreate(
				CreateGamertag,
				CreateHardcore,
				CreateHeroItemID,
				CreateHeadIndex,
				CreateBodyIndex,
				CreateLegsIndex
			);

		InterlockedExchange(
			&AsyncApiCode,
			ApiCode
		);

		r3dOutToLog(
			"[RmlUI][FrontEnd][CharacterCreate] "
			"ApiCharCreate result=%d\n",
			ApiCode
		);

		if (ApiCode == 0)
		{
			Result =
				AsyncResult_Success;
		}
		else if (ApiCode == 8)
		{
			Result =
				AsyncResult_Timeout;
		}
		else
		{
			Result =
				AsyncResult_Error;
		}
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

	const LONG Result =
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

	const EAsyncResult CompletedResult =
		static_cast<EAsyncResult>(
			Result
		);

	if (Operation == AsyncOperation_Login)
	{
		HandleLoginResult(
			CompletedResult
		);
	}
	else if (Operation == AsyncOperation_Profile)
	{
		HandleProfileResult(
			CompletedResult
		);
	}
	else if (
		Operation ==
		AsyncOperation_RenameCharacter)
	{
		HandleRenameCharacterResult(
			CompletedResult
		);
	}
	else if (
		Operation ==
		AsyncOperation_CreateCharacter
	)
	{
		HandleCreateCharacterResult(
			CompletedResult
		);
	}
}

void RmlFrontEndContext::HandleCreateCharacterResult(
	EAsyncResult Result
)
{
	const int ApiCode =
		static_cast<int>(
			InterlockedCompareExchange(
				&AsyncApiCode,
				0,
				0
			)
		);

	if (Result == AsyncResult_Success)
	{
		const int CharacterCount =
			gUserProfile.ProfileData.NumSlots;

		if (CharacterCount <= 0)
		{
			SetCharacterCreateControlsEnabled(
				true
			);

			SetCharacterCreateStatus(
				"Character was created, but the profile was not refreshed."
			);

			return;
		}

		SelectedCharacterIndex =
			CharacterCount - 1;

		gUserProfile.SelectedCharID =
			SelectedCharacterIndex;

		bProfileLoaded = true;

		BuildMainMenu();
		ShowMainMenu();

		SetMainMenuStatus(
			"Character created and selected."
		);

		r3dOutToLog(
			"[RmlUI][FrontEnd][CharacterCreate] "
			"Created character index=%d\n",
			SelectedCharacterIndex
		);

		return;
	}

	SetCharacterCreateControlsEnabled(
		true
	);

	if (Result == AsyncResult_Timeout)
	{
		SetCharacterCreateStatus(
			"Character creation request timed out."
		);

		return;
	}

	if (ApiCode == 9)
	{
		SetCharacterCreateStatus(
			"This character name is already in use or was rejected."
		);

		return;
	}

	if (ApiCode == 6)
	{
		SetCharacterCreateStatus(
			"Character creation was rejected. Check the character limit."
		);

		return;
	}

	if (ApiCode == 1)
	{
		ShowLoginMessage(
			L"Your login session is no longer valid."
		);

		return;
	}

	char ErrorText[128]{};

	sprintf_s(
		ErrorText,
		"Character creation failed. Backend code: %d",
		ApiCode
	);

	SetCharacterCreateStatus(
		ErrorText
	);
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

	InterlockedExchange(
		&AsyncApiCode,
		0
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

void RmlFrontEndContext::HandleRenameCharacterResult(
	EAsyncResult Result
)
{
	const int ApiCode =
		static_cast<int>(
			InterlockedCompareExchange(
				&AsyncApiCode,
				0,
				0
			)
		);

	if (Result == AsyncResult_Success)
	{
		SelectedCharacterIndex = 0;
		gUserProfile.SelectedCharID = 0;

		BuildMainMenu();
		ShowMainMenu();

		SetMainMenuStatus(
			"Survivor nickname changed."
		);

		r3dOutToLog(
			"[RmlUI][FrontEnd][Rename] "
			"Nickname changed to %s\n",
			RenameGamertag
		);

		return;
	}

	SetMainMenuControlsEnabled(
		true
	);

	if (Result == AsyncResult_Timeout)
	{
		SetMainMenuStatus(
			"Nickname change request timed out."
		);

		return;
	}

	if (ApiCode == 9)
	{
		SetMainMenuStatus(
			"Nickname is invalid or already in use."
		);

		return;
	}

	if (ApiCode == 6)
	{
		SetMainMenuStatus(
			"Permanent survivor was not found."
		);

		return;
	}

	if (ApiCode == 1)
	{
		ShowLoginMessage(
			L"Your login session is no longer valid."
		);

		return;
	}

	char ErrorText[128]{};

	sprintf_s(
		ErrorText,
		"Nickname change failed. Backend code: %d",
		ApiCode
	);

	SetMainMenuStatus(
		ErrorText
	);
}

void RmlFrontEndContext::HandleProfileResult(
	EAsyncResult Result
)
{
	if (Result != AsyncResult_Success)
	{
		if (CurrentScreen == EScreen::MainMenu)
		{
			SetMainMenuControlsEnabled(
				true
			);

			SetMainMenuStatus(
				"Unable to refresh account profile."
			);
		}
		else
		{
			SetLoginControlsEnabled(
				true
			);

			SetLoginStatus(
				"Login succeeded, but the profile could not be loaded."
			);
		}

		return;
	}

	bProfileLoaded = true;

	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	if (CharacterCount > 0)
	{
		SelectedCharacterIndex = 0;
		gUserProfile.SelectedCharID = 0;
	}
	else
	{
		SelectedCharacterIndex = -1;
		gUserProfile.SelectedCharID = 0;
	}

	BuildMainMenu();
	ShowMainMenu();

	if (CharacterCount <= 0)
	{
		SetMainMenuStatus(
			"Account has no permanent survivor. Check registration data."
		);
	}

	if (CharacterCount > 1)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Profile] "
			"Warning: account contains %d characters. "
			"Only slot 0 is used.\n",
			CharacterCount
		);
	}

	r3dOutToLog(
		"[RmlUI][FrontEnd][Profile] "
		"Loaded. Characters=%d, Selected=0\n",
		CharacterCount
	);
}

void RmlFrontEndContext::BuildMainMenu()
{
	if (!MainMenuDocument)
		return;

	const std::string GcText =
		FormatGroupedNumber(
			gUserProfile.ProfileData.
				GamePoints
		);

	const std::string GdText =
		FormatGroupedNumber(
			gUserProfile.ProfileData.
				GameDollars
		);

	SetElementText(
		MainMenuDocument,
		"balance_gc",
		GcText
	);

	SetElementText(
		MainMenuDocument,
		"balance_gd",
		GdText
	);

	SetElementText(
		MainMenuDocument,
		"balance_ltc",
		"0"
	);

	SetElementText(
		MainMenuDocument,
		"account_name",
		LoginUser[0]
			? LoginUser
			: "SURVIVOR"
	);

	const int CharacterCount =
		gUserProfile.ProfileData.
			NumSlots;

	if (CharacterCount <= 0)
	{
		SelectedCharacterIndex =
			-1;

		gUserProfile.SelectedCharID =
			0;

		SetElementText(
			MainMenuDocument,
			"top_survivor_name",
			"NO SURVIVOR"
		);

		SetElementText(
			MainMenuDocument,
			"top_survivor_role",
			"EMPTY SLOT"
		);

		SetElementText(
			MainMenuDocument,
			"top_level_value",
			"0"
		);

		SetElementText(
			MainMenuDocument,
			"top_level_xp_text",
			"0 / 100"
		);

		SetElementPercent(
			MainMenuDocument,
			"top_level_xp_fill",
			0.0f
		);

		SetElementText(
			MainMenuDocument,
			"survivor_nickname",
			"NO SURVIVOR"
		);

		SetElementText(
			MainMenuDocument,
			"survivor_class",
			"EMPTY SLOT"
		);

		SetElementText(
			MainMenuDocument,
			"survivor_state",
			"UNAVAILABLE"
		);

		SetElementText(
			MainMenuDocument,
			"stat_player_kills",
			"0"
		);

		SetElementText(
			MainMenuDocument,
			"stat_zombie_kills",
			"0"
		);

		SetElementText(
			MainMenuDocument,
			"stat_time_played",
			"0d 00h 00m"
		);

		SetElementText(
			MainMenuDocument,
			"stat_health",
			"0%"
		);

		SetElementText(
			MainMenuDocument,
			"stat_reputation",
			"0"
		);

		SetElementText(
			MainMenuDocument,
			"stat_rank",
			"UNRANKED"
		);

		SetElementText(
			MainMenuDocument,
			"survivor_level_value",
			"0"
		);

		SetElementText(
			MainMenuDocument,
			"survivor_level_xp_text",
			"0 / 100"
		);

		SetElementPercent(
			MainMenuDocument,
			"survivor_level_xp_fill",
			0.0f
		);

		SetElementText(
			MainMenuDocument,
			"reward_rank_title",
			"RECRUIT"
		);

		SetElementText(
			MainMenuDocument,
			"reward_xp_text",
			"0 / 100"
		);

		SetElementPercent(
			MainMenuDocument,
			"reward_xp_fill",
			0.0f
		);

		SetElementClass(
			MainMenuDocument,
			"survivor_state",
			"ready",
			false
		);

		SetElementClass(
			MainMenuDocument,
			"survivor_state",
			"wounded",
			false
		);

		SetElementClass(
			MainMenuDocument,
			"survivor_state",
			"dead",
			true
		);

		SetMainMenuStatus(
			"Account has no permanent survivor."
		);

		SetMainMenuControlsEnabled(
			true
		);

		return;
	}

	if (
		SelectedCharacterIndex < 0 ||
		SelectedCharacterIndex >=
			CharacterCount
	)
	{
		SelectedCharacterIndex =
			0;
	}

	gUserProfile.SelectedCharID =
		SelectedCharacterIndex;

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.
			ArmorySlots[
				SelectedCharacterIndex
			];

	const char* CharacterRole =
		GetCharacterRole(
			Character.HeroItemID
		);

	const FFrontendLevelProgress Level =
		CalculateFrontendLevelProgress(
			Character.Stats.XP
		);

	const std::string ExperienceText =
		FormatGroupedNumber(
			Level.TotalExperience
		) +
		" / " +
		FormatGroupedNumber(
			Level.NextLevelExperience
		);

	char Text[256]{};

	sprintf_s(
		Text,
		"%d",
		Level.Level
	);

	SetElementText(
		MainMenuDocument,
		"top_level_value",
		Text
	);

	SetElementText(
		MainMenuDocument,
		"top_level_xp_text",
		ExperienceText
	);

	SetElementPercent(
		MainMenuDocument,
		"top_level_xp_fill",
		Level.Percent
	);

	SetElementText(
		MainMenuDocument,
		"top_survivor_name",
		Character.Gamertag
	);

	SetElementText(
		MainMenuDocument,
		"top_survivor_role",
		CharacterRole
	);

	SetElementText(
		MainMenuDocument,
		"survivor_nickname",
		Character.Gamertag
	);

	SetElementText(
		MainMenuDocument,
		"survivor_class",
		CharacterRole
	);

	SetElementText(
		MainMenuDocument,
		"selected_character",
		Character.Gamertag
	);

	const bool bDead =
		Character.Alive == 0 ||
		Character.Health <= 0.0f;

	const bool bReady =
		!bDead &&
		Character.Health >= 99.5f;

	const bool bWounded =
		!bDead &&
		!bReady;

	if (bDead)
	{
		SetElementText(
			MainMenuDocument,
			"survivor_state",
			"DEAD"
		);
	}
	else if (bReady)
	{
		SetElementText(
			MainMenuDocument,
			"survivor_state",
			"READY"
		);
	}
	else
	{
		SetElementText(
			MainMenuDocument,
			"survivor_state",
			"WOUNDED"
		);
	}

	SetElementClass(
		MainMenuDocument,
		"survivor_state",
		"ready",
		bReady
	);

	SetElementClass(
		MainMenuDocument,
		"survivor_state",
		"wounded",
		bWounded
	);

	SetElementClass(
		MainMenuDocument,
		"survivor_state",
		"dead",
		bDead
	);

	const int PlayerKills =
		Character.Stats.
			KilledSurvivors +
		Character.Stats.
			KilledBandits;

	sprintf_s(
		Text,
		"%d",
		PlayerKills
	);

	SetElementText(
		MainMenuDocument,
		"stat_player_kills",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Character.Stats.
			KilledZombies
	);

	SetElementText(
		MainMenuDocument,
		"stat_zombie_kills",
		Text
	);

	SetElementText(
		MainMenuDocument,
		"stat_time_played",
		FormatPlayedTime(
			Character.Stats.
				TimePlayed
		)
	);

	const int Health =
		static_cast<int>(
			std::clamp(
				Character.Health,
				0.0f,
				100.0f
			)
		);

	sprintf_s(
		Text,
		"%d%%",
		Health
	);

	SetElementText(
		MainMenuDocument,
		"stat_health",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Character.Stats.
			Reputation
	);

	SetElementText(
		MainMenuDocument,
		"stat_reputation",
		Text
	);

	SetElementText(
		MainMenuDocument,
		"stat_rank",
		"UNRANKED"
	);

	sprintf_s(
		Text,
		"%d",
		Level.Level
	);

	SetElementText(
		MainMenuDocument,
		"survivor_level_value",
		Text
	);

	SetElementText(
		MainMenuDocument,
		"survivor_level_xp_text",
		ExperienceText
	);

	SetElementPercent(
		MainMenuDocument,
		"survivor_level_xp_fill",
		Level.Percent
	);

	SetElementText(
		MainMenuDocument,
		"reward_rank_title",
		GetExperienceTitle(
			Level.Level
		)
	);

	SetElementText(
		MainMenuDocument,
		"reward_xp_text",
		ExperienceText
	);

	SetElementPercent(
		MainMenuDocument,
		"reward_xp_fill",
		Level.Percent
	);

	SetInputValue(
		MainMenuDocument,
		"rename_character_name",
		Character.Gamertag
	);

	SetElementText(
		MainMenuDocument,
		"footer_region",
		"AUTO"
	);

	SetElementText(
		MainMenuDocument,
		"footer_server",
		g_serverip &&
		g_serverip->GetString() &&
		g_serverip->GetString()[0]
			? g_serverip->GetString()
			: "OFFLINE"
	);

	EnsureCharacterPreview();

	SetMainMenuControlsEnabled(
		true
	);

	SetMainMenuStatus(
		"Survivor profile ready."
	);
}

void RmlFrontEndContext::BuildSkills()
{
	if (!SkillsDocument)
		return;

	const std::string GcText =
		FormatGroupedNumber(
			gUserProfile.ProfileData.
				GamePoints
		);

	const std::string GdText =
		FormatGroupedNumber(
			gUserProfile.ProfileData.
				GameDollars
		);

	SetElementText(
		SkillsDocument,
		"balance_gc",
		GcText
	);

	SetElementText(
		SkillsDocument,
		"balance_gd",
		GdText
	);

	SetElementText(
		SkillsDocument,
		"balance_ltc",
		"0"
	);

	SetElementText(
		SkillsDocument,
		"account_name",
		LoginUser[0]
			? LoginUser
			: "ACCOUNT"
	);

	const int CharacterCount =
		gUserProfile.ProfileData.
			NumSlots;

	if (
		CharacterCount <= 0 ||
		SelectedCharacterIndex < 0 ||
		SelectedCharacterIndex >= CharacterCount
	)
	{
		SetElementText(
			SkillsDocument,
			"top_survivor_name",
			"NO SURVIVOR"
		);

		SetElementText(
			SkillsDocument,
			"top_survivor_role",
			"EMPTY SLOT"
		);

		SetElementText(
			SkillsDocument,
			"top_level_value",
			"0"
		);

		SetElementText(
			SkillsDocument,
			"top_level_xp_text",
			"0 / 100"
		);

		SetElementPercent(
			SkillsDocument,
			"top_level_xp_fill",
			0.0f
		);

		SetElementText(
			SkillsDocument,
			"skill_points_value",
			"0"
		);

		SetElementText(
			SkillsDocument,
			"skills_survivor_level_value",
			"0"
		);

		SetElementText(
			SkillsDocument,
			"skills_survivor_name",
			"NO SURVIVOR"
		);

		SetElementText(
			SkillsDocument,
			"skills_survivor_class",
			"EMPTY SLOT"
		);

		SetElementText(
			SkillsDocument,
			"skills_xp_text",
			"0 / 100 XP"
		);

		SetElementPercent(
			SkillsDocument,
			"skills_xp_fill",
			0.0f
		);

		SetElementText(
			SkillsDocument,
			"footer_region",
			"AUTO"
		);

		SetElementText(
			SkillsDocument,
			"footer_server",
			"OFFLINE"
		);

		SetSkillsControlsEnabled(
			false
		);

		SetSkillsStatus(
			"No survivor selected."
		);

		return;
	}

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.
			ArmorySlots[
				SelectedCharacterIndex
			];

	const char* CharacterRole =
		GetCharacterRole(
			Character.HeroItemID
		);

	const FFrontendLevelProgress Level =
		CalculateFrontendLevelProgress(
			Character.Stats.XP
		);

	const std::string ExperienceText =
		FormatGroupedNumber(
			Level.TotalExperience
		) +
		" / " +
		FormatGroupedNumber(
			Level.NextLevelExperience
		);

	char Text[256]{};

	sprintf_s(
		Text,
		"%d",
		Level.Level
	);

	SetElementText(
		SkillsDocument,
		"top_level_value",
		Text
	);

	SetElementText(
		SkillsDocument,
		"top_level_xp_text",
		ExperienceText
	);

	SetElementPercent(
		SkillsDocument,
		"top_level_xp_fill",
		Level.Percent
	);

	SetElementText(
		SkillsDocument,
		"top_survivor_name",
		Character.Gamertag
	);

	SetElementText(
		SkillsDocument,
		"top_survivor_role",
		CharacterRole
	);

	SetElementText(
		SkillsDocument,
		"skills_survivor_name",
		Character.Gamertag
	);

	SetElementText(
		SkillsDocument,
		"skills_survivor_class",
		CharacterRole
	);

	SetElementText(
		SkillsDocument,
		"selected_character",
		Character.Gamertag
	);

	sprintf_s(
		Text,
		"%d",
		Level.Level
	);

	SetElementText(
		SkillsDocument,
		"skills_survivor_level_value",
		Text
	);

	SetElementText(
		SkillsDocument,
		"skills_xp_text",
		ExperienceText + " XP"
	);

	SetElementPercent(
		SkillsDocument,
		"skills_xp_fill",
		Level.Percent
	);

	sprintf_s(
		Text,
		"%d",
		std::max(
			0,
			Character.Stats.SkillXPPool
		)
	);

	SetElementText(
		SkillsDocument,
		"skill_points_value",
		Text
	);

	SetElementText(
		SkillsDocument,
		"skill_points_available",
		Text
	);

	SetElementText(
		SkillsDocument,
		"skill_points_spent",
		"0"
	);

	SetElementText(
		SkillsDocument,
		"footer_region",
		"AUTO"
	);

	SetElementText(
		SkillsDocument,
		"footer_server",
		g_serverip &&
		g_serverip->GetString() &&
		g_serverip->GetString()[0]
			? g_serverip->GetString()
			: "OFFLINE"
	);

	for (
		size_t Index = 0;
		Index < FrontendSkillNodeCount;
		++Index
	)
	{
		const FFrontendSkillNode& Node =
			FrontendSkillNodes[Index];

		SetElementClass(
			SkillsDocument,
			Node.ElementId,
			"learned",
			Node.State ==
				EFrontendSkillState::Learned
		);

		SetElementClass(
			SkillsDocument,
			Node.ElementId,
			"available",
			Node.State ==
				EFrontendSkillState::Available
		);

		SetElementClass(
			SkillsDocument,
			Node.ElementId,
			"locked",
			Node.State ==
				EFrontendSkillState::Locked
		);
	}

	if (
		SelectedSkillElementId.empty() ||
		!FindFrontendSkillNode(
			SelectedSkillElementId
		)
	)
	{
		SelectedSkillElementId =
			"skill_node_vitality_1";
	}

	SelectSkillNode(
		SelectedSkillElementId
	);

	SetSkillsStatus(
		"Skills loaded."
	);
}

void RmlFrontEndContext::BuildShop()
{
	if (!ShopDocument)
		return;

	const std::string GcText =
		FormatGroupedNumber(
			gUserProfile.ProfileData.
				GamePoints
		);

	const std::string GdText =
		FormatGroupedNumber(
			gUserProfile.ProfileData.
				GameDollars
		);

	SetElementText(
		ShopDocument,
		"balance_gc",
		GcText
	);

	SetElementText(
		ShopDocument,
		"balance_gd",
		GdText
	);

	SetElementText(
		ShopDocument,
		"balance_ltc",
		"0"
	);

	SetElementText(
		ShopDocument,
		"account_name",
		LoginUser[0]
			? LoginUser
			: "ACCOUNT"
	);

	const int CharacterCount =
		gUserProfile.ProfileData.
			NumSlots;

	if (
		CharacterCount <= 0 ||
		SelectedCharacterIndex < 0 ||
		SelectedCharacterIndex >= CharacterCount
	)
	{
		SetElementText(
			ShopDocument,
			"top_survivor_name",
			"NO SURVIVOR"
		);

		SetElementText(
			ShopDocument,
			"top_survivor_role",
			"EMPTY SLOT"
		);

		SetElementText(
			ShopDocument,
			"top_level_value",
			"0"
		);

		SetElementText(
			ShopDocument,
			"top_level_xp_text",
			"0 / 100"
		);

		SetElementPercent(
			ShopDocument,
			"top_level_xp_fill",
			0.0f
		);

		SetElementText(
			ShopDocument,
			"footer_region",
			"AUTO"
		);

		SetElementText(
			ShopDocument,
			"footer_server",
			"OFFLINE"
		);

		SetShopControlsEnabled(
			false
		);

		SetShopStatus(
			"No survivor selected."
		);

		return;
	}

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.
			ArmorySlots[
				SelectedCharacterIndex
			];

	const char* CharacterRole =
		GetCharacterRole(
			Character.HeroItemID
		);

	const FFrontendLevelProgress Level =
		CalculateFrontendLevelProgress(
			Character.Stats.XP
		);

	const std::string ExperienceText =
		FormatGroupedNumber(
			Level.TotalExperience
		) +
		" / " +
		FormatGroupedNumber(
			Level.NextLevelExperience
		);

	char Text[256]{};

	sprintf_s(
		Text,
		"%d",
		Level.Level
	);

	SetElementText(
		ShopDocument,
		"top_level_value",
		Text
	);

	SetElementText(
		ShopDocument,
		"top_level_xp_text",
		ExperienceText
	);

	SetElementPercent(
		ShopDocument,
		"top_level_xp_fill",
		Level.Percent
	);

	SetElementText(
		ShopDocument,
		"top_survivor_name",
		Character.Gamertag
	);

	SetElementText(
		ShopDocument,
		"top_survivor_role",
		CharacterRole
	);

	SetElementText(
		ShopDocument,
		"selected_character",
		Character.Gamertag
	);

	SetElementText(
		ShopDocument,
		"footer_region",
		"AUTO"
	);

	SetElementText(
		ShopDocument,
		"footer_server",
		g_serverip &&
		g_serverip->GetString() &&
		g_serverip->GetString()[0]
			? g_serverip->GetString()
			: "OFFLINE"
	);

	if (
		SelectedShopItemElementId.empty() ||
		!FindFrontendShopItem(
			SelectedShopItemElementId
		)
	)
	{
		SelectedShopItemElementId =
			"shop_item_0";
	}

	SelectShopItem(
		SelectedShopItemElementId
	);

	SetShopControlsEnabled(
		true
	);

	SetShopStatus(
		"Shop visual screen loaded."
	);
}

void RmlFrontEndContext::SelectShopItem(
	const Rml::String& ShopItemId
)
{
	if (!ShopDocument)
		return;

	const FFrontendShopItem* Item =
		FindFrontendShopItem(
			ShopItemId
		);

	if (!Item)
		return;

	SelectedShopItemElementId =
		Item->ElementId;

	SelectedShopBackendItemId =
		Item->BackendItemId;

	RefreshShopSelection();

	SetElementText(
		ShopDocument,
		"selected_item_icon",
		Item->IconText
	);

	SetElementText(
		ShopDocument,
		"selected_item_badge",
		Item->BadgeText
	);

	SetElementText(
		ShopDocument,
		"selected_item_name",
		Item->DisplayName
	);

	SetElementText(
		ShopDocument,
		"selected_item_category",
		Item->TypeName
	);

	SetElementText(
		ShopDocument,
		"selected_item_description",
		Item->Description
	);

	SetElementText(
		ShopDocument,
		"selected_item_gc_price",
		Item->GcPriceText
	);

	SetElementText(
		ShopDocument,
		"selected_item_gd_price",
		Item->GdPriceText
	);

	SetElementText(
		ShopDocument,
		"selected_shop_item_id",
		Item->ElementId
	);

	char Text[64]{};

	sprintf_s(
		Text,
		"%d",
		Item->BackendItemId
	);

	SetElementText(
		ShopDocument,
		"selected_shop_backend_item_id",
		Text
	);

	SetElementText(
		ShopDocument,
		"selected_shop_category_id",
		Item->CategoryId
	);

	sprintf_s(
		Text,
		"%d",
		Item->Damage
	);

	SetElementText(
		ShopDocument,
		"selected_item_damage_value",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Item->Range
	);

	SetElementText(
		ShopDocument,
		"selected_item_range_value",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Item->Recoil
	);

	SetElementText(
		ShopDocument,
		"selected_item_recoil_value",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Item->Weight
	);

	SetElementText(
		ShopDocument,
		"selected_item_weight_value",
		Text
	);

	SetShopStatus(
		"Shop item selected."
	);
}

void RmlFrontEndContext::RefreshShopSelection()
{
	if (!ShopDocument)
		return;

	for (
		size_t Index = 0;
		Index < FrontendShopItemCount;
		++Index
	)
	{
		const FFrontendShopItem& Item =
			FrontendShopItems[Index];

		SetElementClass(
			ShopDocument,
			Item.ElementId,
			"selected",
			SelectedShopItemElementId ==
				Item.ElementId
		);
	}
}

void RmlFrontEndContext::SetShopControlsEnabled(
	bool bEnabled
)
{
	const bool bHasCharacter =
		bProfileLoaded &&
		gUserProfile.ProfileData.
			NumSlots > 0;

	const bool bAllow =
		bEnabled &&
		bHasCharacter;

	SetElementEnabled(
		ShopDocument,
		"btn_shop_buy_selected",
		bAllow
	);

	SetElementEnabled(
		ShopDocument,
		"btn_shop_preview_item",
		bAllow
	);

	SetElementEnabled(
		ShopDocument,
		"btn_shop_sort",
		bAllow
	);

	SetElementEnabled(
		ShopDocument,
		"btn_shop_refresh",
		bAllow
	);
}

void RmlFrontEndContext::SetShopStatus(
	const Rml::String& Text
)
{
	SetElementText(
		ShopDocument,
		"shop_details_status",
		Text
	);

	SetElementText(
		ShopDocument,
		"main_menu_status",
		Text
	);
}

void RmlFrontEndContext::SelectSkillNode(
	const Rml::String& SkillNodeId
)
{
	if (!SkillsDocument)
		return;

	const FFrontendSkillNode* Node =
		FindFrontendSkillNode(
			SkillNodeId
		);

	if (!Node)
		return;

	SelectedSkillElementId =
		Node->ElementId;

	SelectedSkillBackendId =
		Node->BackendSkillId;

	RefreshSkillSelection();

	SetElementText(
		SkillsDocument,
		"selected_skill_name",
		Node->DisplayName
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_category",
		Node->CategoryName
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_description",
		Node->Description
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_level",
		Node->LevelText
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_bonus",
		Node->BonusText
	);

	char Text[64]{};

	sprintf_s(
		Text,
		"%d SP",
		Node->Cost
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_cost",
		Text
	);

	sprintf_s(
		Text,
		"%d",
		Node->RequiredLevel
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_required_level",
		Text
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_state",
		GetFrontendSkillStateText(
			Node->State
		)
	);

	SetElementClass(
		SkillsDocument,
		"selected_skill_state",
		"learned",
		Node->State ==
			EFrontendSkillState::Learned
	);

	SetElementClass(
		SkillsDocument,
		"selected_skill_state",
		"available",
		Node->State ==
			EFrontendSkillState::Available
	);

	SetElementClass(
		SkillsDocument,
		"selected_skill_state",
		"locked",
		Node->State ==
			EFrontendSkillState::Locked
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_requirement_1",
		Node->RequirementA
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_requirement_2",
		Node->RequirementB
	);

	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	int SkillPoints = 0;

	if (
		CharacterCount > 0 &&
		SelectedCharacterIndex >= 0 &&
		SelectedCharacterIndex < CharacterCount
	)
	{
		const wiCharDataFull& Character =
			gUserProfile.ProfileData.
				ArmorySlots[
					SelectedCharacterIndex
				];

		SkillPoints =
			std::max(
				0,
				Character.Stats.SkillXPPool
			);
	}

	if (SkillPoints >= Node->Cost)
	{
		SetElementText(
			SkillsDocument,
			"selected_skill_requirement_3",
			"Enough Skill Points"
		);

		SetElementClass(
			SkillsDocument,
			"selected_skill_requirement_3",
			"passed",
			true
		);

		SetElementClass(
			SkillsDocument,
			"selected_skill_requirement_3",
			"failed",
			false
		);
	}
	else
	{
		SetElementText(
			SkillsDocument,
			"selected_skill_requirement_3",
			"Need more Skill Points"
		);

		SetElementClass(
			SkillsDocument,
			"selected_skill_requirement_3",
			"passed",
			false
		);

		SetElementClass(
			SkillsDocument,
			"selected_skill_requirement_3",
			"failed",
			true
		);
	}

	SetElementText(
		SkillsDocument,
		"selected_skill_id",
		Node->ElementId
	);

	sprintf_s(
		Text,
		"%d",
		Node->BackendSkillId
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_backend_id",
		Text
	);

	SetElementText(
		SkillsDocument,
		"selected_skill_category_id",
		Node->CategoryId
	);

	const bool bCanLearn =
		Node->State ==
			EFrontendSkillState::Available &&
		SkillPoints >= Node->Cost;

	SetElementEnabled(
		SkillsDocument,
		"btn_learn_selected_skill",
		bCanLearn
	);

	if (bCanLearn)
	{
		SetSkillsStatus(
			"Selected skill is ready to learn."
		);
	}
	else if (
		Node->State ==
		EFrontendSkillState::Learned
	)
	{
		SetSkillsStatus(
			"Selected skill is already learned."
		);
	}
	else if (
		Node->State ==
		EFrontendSkillState::Locked
	)
	{
		SetSkillsStatus(
			"Selected skill is locked."
		);
	}
	else
	{
		SetSkillsStatus(
			"Not enough Skill Points."
		);
	}
}

void RmlFrontEndContext::RefreshSkillSelection()
{
	if (!SkillsDocument)
		return;

	for (
		size_t Index = 0;
		Index < FrontendSkillNodeCount;
		++Index
	)
	{
		const FFrontendSkillNode& Node =
			FrontendSkillNodes[Index];

		SetElementClass(
			SkillsDocument,
			Node.ElementId,
			"selected",
			SelectedSkillElementId ==
				Node.ElementId
		);
	}
}

void RmlFrontEndContext::RequestLearnSelectedSkill()
{
	if (
		CurrentScreen != EScreen::Skills ||
		IsBusy() ||
		!bProfileLoaded
	)
	{
		return;
	}

	const FFrontendSkillNode* Node =
		FindFrontendSkillNode(
			SelectedSkillElementId
		);

	if (!Node)
	{
		SetSkillsStatus(
			"No skill node selected."
		);

		return;
	}

	if (
		Node->State ==
		EFrontendSkillState::Learned
	)
	{
		SetSkillsStatus(
			"Skill is already learned."
		);

		return;
	}

	if (
		Node->State ==
		EFrontendSkillState::Locked
	)
	{
		SetSkillsStatus(
			"Skill is locked by requirements."
		);

		return;
	}

	const int CharacterCount =
		gUserProfile.ProfileData.NumSlots;

	if (
		CharacterCount <= 0 ||
		SelectedCharacterIndex < 0 ||
		SelectedCharacterIndex >= CharacterCount
	)
	{
		SetSkillsStatus(
			"Select a survivor first."
		);

		return;
	}

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.
			ArmorySlots[
				SelectedCharacterIndex
			];

	const int SkillPoints =
		std::max(
			0,
			Character.Stats.SkillXPPool
		);

	if (SkillPoints < Node->Cost)
	{
		SetSkillsStatus(
			"Not enough Skill Points."
		);

		return;
	}

	r3dOutToLog(
		"[RmlUI][FrontEnd][Skills] "
		"Learn skill requested. Node=%s BackendId=%d\n",
		Node->ElementId,
		Node->BackendSkillId
	);

	SetSkillsStatus(
		"Skill backend API is not connected yet."
	);
}

void RmlFrontEndContext::SetSkillsControlsEnabled(
	bool bEnabled
)
{
	const bool bHasCharacter =
		bProfileLoaded &&
		gUserProfile.ProfileData.
			NumSlots > 0;

	SetElementEnabled(
		SkillsDocument,
		"btn_learn_selected_skill",
		bEnabled &&
		bHasCharacter
	);

	SetElementEnabled(
		SkillsDocument,
		"btn_reset_skills",
		bEnabled &&
		bHasCharacter
	);
}

void RmlFrontEndContext::SetSkillsStatus(
	const Rml::String& Text
)
{
	SetElementText(
		SkillsDocument,
		"skill_details_status",
		Text
	);

	SetElementText(
		SkillsDocument,
		"main_menu_status",
		Text
	);
}

void RmlFrontEndContext::SelectCharacter(
	int CharacterIndex
)
{
	const int CharacterCount =
		gUserProfile.ProfileData.
			NumSlots;

	if (
		CharacterIndex < 0 ||
		CharacterIndex >=
			CharacterCount
	)
	{
		return;
	}

	SelectedCharacterIndex =
		CharacterIndex;

	gUserProfile.SelectedCharID =
		CharacterIndex;

	BuildMainMenu();
	RefreshCharacterSelection();

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

void RmlFrontEndContext::
SetMainMenuControlsEnabled(
	bool bEnabled
)
{
	const bool bHasCharacter =
		bProfileLoaded &&
		gUserProfile.ProfileData.
			NumSlots > 0;

	const char* CharacterControls[] =
	{
		"btn_quick_join",
		"btn_global_inventory",
		"btn_skill_tree",
		"btn_customize_character",
		"rename_character_name",
		"btn_rename_character",
		"btn_view_rewards"
	};

	for (
		const char* ElementId :
		CharacterControls
	)
	{
		SetElementEnabled(
			MainMenuDocument,
			ElementId,
			bEnabled &&
			bHasCharacter
		);
	}

	const char* CommonControls[] =
	{
		"nav_survivor",
		"nav_shop",
		"nav_community",
		"nav_skills",
		"nav_equipment",
		"nav_clan",
		"nav_awards",

		"btn_social",
		"btn_messages",
		"btn_settings",
		"btn_options",
		"btn_frontend_exit",

		"btn_reset_preview"
	};

	for (
		const char* ElementId :
		CommonControls
	)
	{
		SetElementEnabled(
			MainMenuDocument,
			ElementId,
			bEnabled
		);
	}
}

void RmlFrontEndContext::SetCharacterCreateControlsEnabled(
	bool bEnabled
)
{
	SetElementEnabled(
		CharacterCreateDocument,
		"create_character_name",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_head_prev",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_head_next",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_body_prev",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_body_next",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_legs_prev",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_legs_next",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_character_confirm",
		bEnabled
	);

	SetElementEnabled(
		CharacterCreateDocument,
		"btn_create_character_cancel",
		bEnabled
	);
}

void RmlFrontEndContext::SetCharacterCreateStatus(
	const Rml::String& Text
)
{
	SetElementText(
		CharacterCreateDocument,
		"create_character_status",
		Text
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

bool RmlFrontEndContext::
EnsureCharacterPreview()
{
	if (
		!CharacterPreview ||
		!bProfileLoaded ||
		gUserProfile.ProfileData.
			NumSlots <= 0
	)
	{
		return false;
	}

	const int CharacterCount =
		gUserProfile.ProfileData.
			NumSlots;

	if (
		SelectedCharacterIndex < 0 ||
		SelectedCharacterIndex >=
			CharacterCount
	)
	{
		SelectedCharacterIndex =
			0;
	}

	gUserProfile.SelectedCharID =
		SelectedCharacterIndex;

	const wiCharDataFull& Character =
		gUserProfile.ProfileData.
			ArmorySlots[
				SelectedCharacterIndex
			];

	if (
		!CharacterPreview->
			IsInitialized()
	)
	{
		if (
			!CharacterPreview->
				Initialize(
					Character
				)
		)
		{
			SetMainMenuStatus(
				"Unable to initialize character preview."
			);

			return false;
		}
	}
	else
	{
		CharacterPreview->
			SetCharacter(
				Character
			);
	}

	return true;
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