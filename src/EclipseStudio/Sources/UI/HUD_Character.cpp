#include "r3dPCH.h"
#include "r3d.h"
#include "d3dfont.h"
#include "d3dfont.h"

#include "ObjectsCode/AI/AI_Player.h"
#include "ObjectsCode/AI/AI_PlayerAnim.h"
#include "UI/Hud_Character.h"
#include "../../RmlUI/RmlUISystem.h"

#include "../rendering/Deffered/CommonPostFX.h"
#include "../rendering/Deffered/PostFXChief.h"
#include "ObjectsCode/weapons/WeaponArmory.h"

extern bool g_bEditMode;

extern void RegisterMsgProc(
	bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam)
);

extern void UnregisterMsgProc(
	bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam)
);

extern bool ProcessStudioPendingResize(
	RmlUISystem* ActiveRmlUI
);

static RmlUISystem g_CharacterRmlUI;

static bool g_CharacterRmlInputEnabled = false;
static bool g_CharacterRmlMsgRegistered = false;

static bool CharacterRml_MsgProc(
	UINT Message,
	WPARAM WParam,
	LPARAM LParam
)
{
	if (!g_CharacterRmlInputEnabled)
		return false;

	if (!g_CharacterRmlUI.IsInitialized())
		return false;

	if (
		!g_CharacterRmlUI.IsCharacterEditorVisible()
	)
	{
		return false;
	}

	LRESULT Result = 0;

	return g_CharacterRmlUI.ProcessWin32Message(
		win::hWnd,
		Message,
		WParam,
		LParam,
		&Result
	);
}

//////////////////////////////////////////////////////////////////////////
CharacterHUD::CharacterHUD()
	: FPS_Acceleration(0, 0, 0)
	, FPS_vViewOrig(0, 0, 0)
	, FPS_ViewAngle(0, 0, 0)
	, FPS_vVision(0, 0, 0)
	, FPS_vRight(0, 0, 0)
	, FPS_vUp(0, 0, 0)
	, FPS_vForw(0, 0, 0)
	, cameraPosition(0, 0, 0)
	, currentDist(2.0f)
	, m_Player(nullptr)
	, paused(true)
	, blendLooped(true)
	, curTime(0.0f)
	, srcTime(0.0f)
	, dstTime(0.0f)
	, bCharacterRmlReady(false)
	, bCharacterRmlInitAttempted(false)
	, bCharacterControlsInitialized(false)
	, bPlayerStatesMode(true)
	, bShowSkeleton(false)
	, bShowAnimStack(true)
	, bShowEquipment(false)
	, bUiIdleMode(false)
	, SelectedPlayerState(0)
	, SelectedMoveDirection(0)
	, SelectedAnimation(-1)
	, CachedAnimationCount(-1)
	, CachedSelectedAnimation(-2)
	, SelectedEquipmentCategory(0)
	, SelectedEquipmentItem(0)
	, CachedEquipmentCategory(-1)
	, CachedEquipmentItem(-1)
{
}
//////////////////////////////////////////////////////////////////////////

static void SetCharacterLoadoutItem(
	wiInventoryItem& Item,
	uint32_t ItemId
)
{
	Item.Reset();

	if (ItemId == 0)
		return;

	Item.itemID = ItemId;
	Item.quantity = 1;
	Item.Var1 = -1;
	Item.Var2 = -1;
}

int CharacterHUD::FindCurrentCharacterEquipmentIndex() const
{
	if (!m_Player)
		return -1;

	uint32_t CurrentValue = 0;

	const wiCharDataFull& Loadout =
		m_Player->CurLoadout;

	switch (SelectedEquipmentCategory)
	{
	case 0:
		CurrentValue =
			Loadout.Items[
				wiCharDataFull::CHAR_LOADOUT_HEADGEAR
			].itemID;
		break;

	case 1:
		CurrentValue =
			Loadout.Items[
				wiCharDataFull::CHAR_LOADOUT_ARMOR
			].itemID;
		break;

	case 2:
		CurrentValue =
			static_cast<uint32_t>(
				Loadout.HeadIdx
			);
		break;

	case 3:
		CurrentValue =
			static_cast<uint32_t>(
				Loadout.BodyIdx
			);
		break;

	case 4:
		CurrentValue =
			static_cast<uint32_t>(
				Loadout.LegsIdx
			);
		break;

	case 5:
		CurrentValue =
			Loadout.HeroItemID;
		break;

	case 6:
		CurrentValue =
			Loadout.Items[
				wiCharDataFull::CHAR_LOADOUT_WEAPON2
			].itemID;
		break;

	case 7:
		CurrentValue =
			Loadout.Items[
				wiCharDataFull::CHAR_LOADOUT_WEAPON1
			].itemID;
		break;

	default:
		return -1;
	}

	for (
		size_t Index = 0;
		Index < CharacterEquipmentValues.size();
		++Index
	)
	{
		if (
			CharacterEquipmentValues[Index] ==
			CurrentValue
		)
		{
			return static_cast<int>(Index);
		}
	}

	return -1;
}

void CharacterHUD::RebuildCharacterEquipmentList()
{
	CharacterEquipmentValues.clear();
	CharacterEquipmentNames.clear();

	if (!m_Player || !g_pWeaponArmory)
		return;

	const wiCharDataFull& Loadout =
		m_Player->CurLoadout;

	/*
		HEAD / BODY / LEGS являются индексами
		вариантов текущего героя.
	*/
	if (
		SelectedEquipmentCategory >= 2 &&
		SelectedEquipmentCategory <= 4
	)
	{
		const HeroConfig* Hero =
			g_pWeaponArmory->getHeroConfig(
				Loadout.HeroItemID
			);

		if (!Hero)
		{
			g_CharacterRmlUI.SetCharacterEquipmentList(
				nullptr,
				0,
				-1
			);

			g_CharacterRmlUI.SetCharacterEquipmentSelected(
				"NO HERO SELECTED"
			);

			return;
		}

		size_t Count = 0;
		const char* Prefix = "";

		switch (SelectedEquipmentCategory)
		{
		case 2:
			Count = Hero->getNumHeads();
			Prefix = "HEAD";
			break;

		case 3:
			Count = Hero->getNumBodys();
			Prefix = "BODY";
			break;

		case 4:
			Count = Hero->getNumLegs();
			Prefix = "LEGS";
			break;
		}

		for (size_t Index = 0; Index < Count; ++Index)
		{
			char Name[64]{};

			sprintf_s(
				Name,
				"%s %u",
				Prefix,
				static_cast<unsigned int>(
					Index + 1
				)
			);

			CharacterEquipmentValues.push_back(
				static_cast<uint32_t>(Index)
			);

			CharacterEquipmentNames.push_back(Name);
		}
	}
	else
	{
		/*
			Для брони, шлема и оружия разрешаем
			снять текущий предмет.
		Для HERO пустой вариант не добавляем.
		*/
		if (SelectedEquipmentCategory != 5)
		{
			CharacterEquipmentValues.push_back(0);
			CharacterEquipmentNames.push_back("EMPTY");
		}

		g_pWeaponArmory->startItemSearch();

		while (g_pWeaponArmory->searchNextItem())
		{
			const uint32_t ItemId =
				g_pWeaponArmory->
					getCurrentSearchItemID();

			const BaseItemConfig* Config =
				g_pWeaponArmory->getConfig(
					ItemId
				);

			if (!Config)
				continue;

			bool bAddItem = false;

			switch (SelectedEquipmentCategory)
			{
			case 0:
				bAddItem =
					Config->category ==
					storecat_Helmet;
				break;

			case 1:
				bAddItem =
					Config->category ==
					storecat_Armor;
				break;

			case 5:
				{
					const HeroConfig* Hero =
						g_pWeaponArmory->getHeroConfig(
							ItemId
						);

					bAddItem =
						Config->category ==
							storecat_HeroPackage &&
						Hero != nullptr &&
						Hero->getNumHeads() > 0 &&
						Hero->getNumBodys() > 0 &&
						Hero->getNumLegs() > 0;

					break;
				}

			case 6:
			case 7:
				bAddItem =
					g_pWeaponArmory->
						getWeaponConfig(ItemId) != nullptr;
				break;
			}

			if (!bAddItem)
				continue;

			CharacterEquipmentValues.push_back(
				ItemId
			);

			CharacterEquipmentNames.push_back(
				Config->m_StoreName
					? Config->m_StoreName
					: "UNKNOWN"
			);
		}
	}

	std::vector<const char*> NamePointers;

	NamePointers.reserve(
		CharacterEquipmentNames.size()
	);

	for (
		const std::string& Name :
		CharacterEquipmentNames
	)
	{
		NamePointers.push_back(
			Name.c_str()
		);
	}

	SelectedEquipmentItem =
		FindCurrentCharacterEquipmentIndex();

	g_CharacterRmlUI.SetCharacterEquipmentList(
		NamePointers.empty()
			? nullptr
			: NamePointers.data(),
		static_cast<int>(
			NamePointers.size()
		),
		SelectedEquipmentItem
	);

	if (
		SelectedEquipmentItem >= 0 &&
		SelectedEquipmentItem <
			static_cast<int>(
				CharacterEquipmentNames.size()
			)
	)
	{
		g_CharacterRmlUI.SetCharacterEquipmentSelected(
			CharacterEquipmentNames[
				SelectedEquipmentItem
			].c_str()
		);
	}
	else
	{
		g_CharacterRmlUI.SetCharacterEquipmentSelected(
			"EMPTY"
		);
	}

	CachedEquipmentCategory =
		SelectedEquipmentCategory;

	CachedEquipmentItem =
		SelectedEquipmentItem;
}

void CharacterHUD::ApplyCharacterEquipmentItem(
	int ListIndex
)
{
	if (!m_Player)
		return;

	if (
		ListIndex < 0 ||
		ListIndex >=
			static_cast<int>(
				CharacterEquipmentValues.size()
			)
	)
	{
		return;
	}

	const uint32_t Value =
		CharacterEquipmentValues[ListIndex];

	wiCharDataFull NewLoadout =
		m_Player->CurLoadout;

	switch (SelectedEquipmentCategory)
	{
	case 0:
		SetCharacterLoadoutItem(
			NewLoadout.Items[
				wiCharDataFull::CHAR_LOADOUT_HEADGEAR
			],
			Value
		);
		break;

	case 1:
		SetCharacterLoadoutItem(
			NewLoadout.Items[
				wiCharDataFull::CHAR_LOADOUT_ARMOR
			],
			Value
		);
		break;

	case 2:
		NewLoadout.HeadIdx =
			static_cast<int>(Value);
		break;

	case 3:
		NewLoadout.BodyIdx =
			static_cast<int>(Value);
		break;

	case 4:
		NewLoadout.LegsIdx =
			static_cast<int>(Value);
		break;

	case 5:
		{
			const HeroConfig* Hero =
				g_pWeaponArmory->getHeroConfig(Value);

			if (
				!Hero ||
				Hero->getNumHeads() == 0 ||
				Hero->getNumBodys() == 0 ||
				Hero->getNumLegs() == 0
			)
			{
				r3dOutToLog(
					"[CharacterEditor] Invalid hero: %u\n",
					Value
				);

				return;
			}

			NewLoadout.HeroItemID = Value;

			if (
				NewLoadout.HeadIdx < 0 ||
				NewLoadout.HeadIdx >=
					static_cast<int>(
						Hero->getNumHeads()
					)
			)
			{
				NewLoadout.HeadIdx = 0;
			}

			if (
				NewLoadout.BodyIdx < 0 ||
				NewLoadout.BodyIdx >=
					static_cast<int>(
						Hero->getNumBodys()
					)
			)
			{
				NewLoadout.BodyIdx = 0;
			}

			if (
				NewLoadout.LegsIdx < 0 ||
				NewLoadout.LegsIdx >=
					static_cast<int>(
						Hero->getNumLegs()
					)
			)
			{
				NewLoadout.LegsIdx = 0;
			}

			break;
		}

	case 6:
		SetCharacterLoadoutItem(
			NewLoadout.Items[
				wiCharDataFull::CHAR_LOADOUT_WEAPON2
			],
			Value
		);
		break;

	case 7:
		SetCharacterLoadoutItem(
			NewLoadout.Items[
				wiCharDataFull::CHAR_LOADOUT_WEAPON1
			],
			Value
		);
		break;

	default:
		return;
	}

	/*
		Это единственное место, где применяем loadout.
		Не вызывать каждый кадр.
	*/
	m_Player->UpdateLoadoutSlot(
		NewLoadout
	);

	SelectedEquipmentItem = ListIndex;

	CachedEquipmentCategory = -1;
	CachedEquipmentItem = -1;

	RebuildCharacterEquipmentList();
}

//////////////////////////////////////////////////////////////////////////

void CharacterHUD::InitPure()
{
	cameraPosition.Assign(0, 1, 5);
	FPS_Position.Assign(0, 20, -20);

	FPS_Acceleration.Assign(0, 0, 0);
	FPS_vViewOrig.Assign(0, -25, 0);
	FPS_ViewAngle.Assign(0, 0, 0);
	FPS_vVision.Assign(0, 0, 1);

	m_Player = nullptr;

	bCharacterRmlReady = false;
	bCharacterRmlInitAttempted = false;
	bCharacterControlsInitialized = false;
}

void CharacterHUD::CreateCharacter()
{
	if (m_Player)
		return;

	m_Player = static_cast<obj_Player*>(
		srv_CreateGameObject(
			"obj_Player",
			"RespawnPlayer",
			r3dPoint3D(0, 0, 0)
		)
	);

	if (!m_Player)
	{
		r3dOutToLog(
			"[CharacterEditor] Failed to create player\n"
		);

		return;
	}

	m_Player->NetworkLocal = true;
	m_Player->PlayerState = PLAYER_IDLE;
	m_Player->PlayerMoveDir = CUberData::ANIMDIR_Stand;
	m_Player->bDead = 0;
	m_Player->m_enableRendering = true;

	m_Player->OnCreate();

	EnsureDefaultCharacterLoadout();
}

void CharacterHUD::DestroyPure()
{
	ShutdownCharacterRmlUI();
}

extern DWORD gMainThreadID;
void CharacterHUD::InitCharacterRmlUI()
{
	if (bCharacterRmlReady)
		return;

	if (GetCurrentThreadId() != gMainThreadID)
	{
		r3dOutToLog(
			"[RmlUI] Character init postponed: "
			"not running on main thread\n"
		);

		return;
	}

	bCharacterRmlReady = false;
	bCharacterControlsInitialized = false;

	if (
		!r3dRenderer ||
		!r3dRenderer->pd3ddev ||
		!win::hWnd
	)
	{
		r3dOutToLog(
			"[RmlUI] Character editor: "
			"renderer/device/window unavailable\n"
		);

		return;
	}

	if (
		!g_CharacterRmlUI.Init(
			win::hWnd,
			r3dRenderer->pd3ddev,
			false
		)
	)
	{
		r3dOutToLog(
			"[RmlUI] Character editor init failed\n"
		);

		return;
	}

	g_CharacterRmlUI.SetCharacterCallback(
		[this](
			const char* Action,
			const char* Value
		)
		{
			HandleCharacterRmlAction(
				Action,
				Value
			);
		}
	);

	if (!g_CharacterRmlUI.LoadCharacterEditor())
	{
		r3dOutToLog(
			"[RmlUI] Character document load failed, "
			"using legacy UI\n"
		);

		g_CharacterRmlUI.Shutdown();
		return;
	}

	g_CharacterRmlUI.ShowCharacterEditor();

	/*
		Даём RmlUi создать и обновить внутренние элементы
		input type="range" до первого обращения к ним.
	*/

	if (!g_CharacterRmlMsgRegistered)
	{
		RegisterMsgProc(
			CharacterRml_MsgProc
		);

		g_CharacterRmlMsgRegistered = true;
	}

	g_CharacterRmlInputEnabled = true;
	bCharacterRmlReady = true;

	r3dOutToLog(
		"[RmlUI] Character editor enabled\n"
	);
}

void CharacterHUD::ShutdownCharacterRmlUI()
{
	g_CharacterRmlInputEnabled = false;

	if (g_CharacterRmlMsgRegistered)
	{
		UnregisterMsgProc(
			CharacterRml_MsgProc
		);

		g_CharacterRmlMsgRegistered = false;
	}

	if (g_CharacterRmlUI.IsInitialized())
	{
		g_CharacterRmlUI.HideCharacterEditor();
		g_CharacterRmlUI.Shutdown();
	}

	bCharacterRmlReady = false;
	bCharacterControlsInitialized = false;
}

void CharacterHUD::StartCharacterInAir()
{
	if (!m_Player || !m_Player->uberAnim_)
		return;

	int AnimationIndex =
		m_Player->uberAnim_->data_->
			GetJumpAnimId(
				m_Player->uberAnim_->
					AnimPlayerState,
				1
			);

	m_Player->uberAnim_->anim.Stop(
		m_Player->uberAnim_->jumpTrackID
	);

	m_Player->uberAnim_->jumpTrackID =
		m_Player->uberAnim_->anim.StartAnimation(
			AnimationIndex,
			ANIMFLAG_Looped,
			1.0f,
			1.0f,
			0.0f
		);

	m_Player->uberAnim_->jumpState = -1;

	m_Player->uberAnim_->SwitchToState(
		m_Player->uberAnim_->AnimPlayerState,
		m_Player->uberAnim_->AnimMoveDir
	);
}

void CharacterHUD::EnsureDefaultCharacterLoadout()
{
	if (!m_Player || !g_pWeaponArmory)
		return;

	const HeroConfig* CurrentHero =
		g_pWeaponArmory->getHeroConfig(
			m_Player->CurLoadout.HeroItemID
		);

	const bool HasValidCurrentHero =
		CurrentHero &&
		CurrentHero->getNumHeads() > 0 &&
		CurrentHero->getNumBodys() > 0 &&
		CurrentHero->getNumLegs() > 0;

	if (HasValidCurrentHero)
	{
		m_Player->m_enableRendering = true;
		return;
	}

	uint32_t DefaultHeroId = 0;

	g_pWeaponArmory->startItemSearch();

	while (g_pWeaponArmory->searchNextItem())
	{
		const uint32_t ItemId =
			g_pWeaponArmory->getCurrentSearchItemID();

		const HeroConfig* Hero =
			g_pWeaponArmory->getHeroConfig(ItemId);

		if (!Hero)
			continue;

		if (
			Hero->getNumHeads() == 0 ||
			Hero->getNumBodys() == 0 ||
			Hero->getNumLegs() == 0
		)
		{
			continue;
		}

		DefaultHeroId = ItemId;
		break;
	}

	if (DefaultHeroId == 0)
	{
		r3dOutToLog(
			"[CharacterEditor] No valid hero found\n"
		);

		return;
	}

	wiCharDataFull Loadout =
		m_Player->CurLoadout;

	Loadout.HeroItemID = DefaultHeroId;
	Loadout.HeadIdx = 0;
	Loadout.BodyIdx = 0;
	Loadout.LegsIdx = 0;

	m_Player->UpdateLoadoutSlot(Loadout);

	m_Player->m_enableRendering = true;
	m_Player->setSkipOcclusionCheck(true);

	m_Player->ObjFlags |=
		OBJFLAG_ForceSceneBoxBBox |
		OBJFLAG_AlwaysDraw;

	r3dOutToLog(
		"[CharacterEditor] Default hero selected: %u\n",
		DefaultHeroId
	);
}

void CharacterHUD::HandleCharacterRmlAction(
	const char* Action,
	const char* Value
)
{
	if (!Action)
		return;

	CreateCharacter();

	if (!m_Player || !m_Player->uberAnim_)
		return;

	r3dAnimation& Animation =
		m_Player->uberAnim_->anim;

	if (strcmp(Action, "mode") == 0)
	{
		const int ModeIndex =
			Value ? atoi(Value) : 0;

		bPlayerStatesMode =
			ModeIndex == 0;

		g_CharacterRmlUI.SetCharacterMode(
			ModeIndex
		);
	}
	else if (strcmp(Action, "state") == 0)
	{
		const int StateIndex =
			Value ? atoi(Value) : 0;

		if (
			StateIndex >= 0 &&
			StateIndex < 9
		)
		{
			SelectedPlayerState = StateIndex;

			g_CharacterRmlUI.SetCharacterSelectedState(
				SelectedPlayerState
			);

			m_Player->uberAnim_->
				AnimPlayerState = -1;

			m_Player->PlayerState =
				SelectedPlayerState;
		}
	}
	else if (strcmp(Action, "direction") == 0)
	{
		const int DirectionIndex =
			Value ? atoi(Value) : 0;

		if (
			DirectionIndex >= 0 &&
			DirectionIndex < 9
		)
		{
			SelectedMoveDirection =
				DirectionIndex;

			g_CharacterRmlUI.
				SetCharacterSelectedDirection(
					SelectedMoveDirection
				);

			m_Player->PlayerMoveDir =
				SelectedMoveDirection;
		}
	}
	else if (strcmp(Action, "ui_idle") == 0)
	{
		bUiIdleMode = !bUiIdleMode;

		m_Player->uberAnim_->IsInUI =
			bUiIdleMode ? 1 : 0;

		m_Player->uberAnim_->
			AnimPlayerState = -1;
	}
	else if (strcmp(Action, "reload") == 0)
	{
		m_Player->uberAnim_->
			scaleReloadAnimTime = false;

		m_Player->uberAnim_->
			StartReloadAnim();
	}
	else if (strcmp(Action, "shoot") == 0)
	{
		m_Player->uberAnim_->
			StartShootAnim();
	}
	else if (strcmp(Action, "jump") == 0)
	{
		m_Player->StartJump();
	}
	else if (strcmp(Action, "jump_anim") == 0)
	{
		m_Player->uberAnim_->StartJump();
	}
	else if (strcmp(Action, "in_air") == 0)
	{
		StartCharacterInAir();
	}
	else if (strcmp(Action, "animation") == 0)
	{
		const int AnimationIndex =
			Value ? atoi(Value) : -1;

		if (
			Animation.GetAnimPool() &&
			AnimationIndex >= 0 &&
			AnimationIndex <
				static_cast<int>(
					Animation.GetAnimPool()->
						Anims.size()
				)
		)
		{
			SelectedAnimation =
				AnimationIndex;

			curTime = 0.0f;

			const char* AnimationName =
				Animation.GetAnimPool()->
					Anims[AnimationIndex]->
					GetAnimName();

			Animation.StartAnimation(
				AnimationName,
				ANIMFLAG_Looped |
				ANIMFLAG_RemoveOtherNow,
				0.0f,
				1.0f,
				0.0f
			);
		}
	}
	else if (strcmp(Action, "play_pause") == 0)
	{
		paused = !paused;
	}
	else if (strcmp(Action, "stop") == 0)
	{
		Animation.StopAll();
		paused = true;
		curTime = 0.0f;
	}
	else if (strcmp(Action, "default_speed") == 0)
	{
		if (!Animation.AnimTracks.empty())
		{
			Animation.AnimTracks[0].
				SetSpeed(1.0f);

			g_CharacterRmlUI.
				SetCharacterInputValue(
					"char_current_speed",
					"char_current_speed_value",
					1.0f,
					"%.2f"
				);
		}
	}
	else if (strcmp(Action, "show_skeleton") == 0)
	{
		bShowSkeleton = !bShowSkeleton;
	}
	else if (strcmp(Action, "show_anim_stack") == 0)
	{
		bShowAnimStack = !bShowAnimStack;
	}
	else if (strcmp(Action, "show_equipment") == 0)
	{
		bShowEquipment = !bShowEquipment;
	}
	else if (
		strcmp(
			Action,
			"equipment_category"
		) == 0
	)
	{
		SelectedEquipmentCategory =
			Value ? atoi(Value) : 0;

		SelectedEquipmentCategory =
			R3D_CLAMP(
				SelectedEquipmentCategory,
				0,
				7
			);

		g_CharacterRmlUI.
			SetCharacterEquipmentCategory(
				SelectedEquipmentCategory
			);

		CachedEquipmentCategory = -1;
		CachedEquipmentItem = -1;

		RebuildCharacterEquipmentList();
	}
	else if (
		strcmp(
			Action,
			"equipment_item"
		) == 0
	)
	{
		const int ListIndex =
			Value ? atoi(Value) : -1;

		ApplyCharacterEquipmentItem(
			ListIndex
		);
	}
}

void CharacterHUD::InitializeCharacterControls()
{
	if (
		bCharacterControlsInitialized ||
		!m_Player ||
		!m_Player->uberAnim_
	)
	{
		return;
	}

	g_CharacterRmlUI.SetCharacterInputValue(
		"char_jump_speed",
		"char_jump_speed_value",
		m_Player->uberAnim_->jumpAnimSpeed,
		"%.2f"
	);

	g_CharacterRmlUI.SetCharacterInputValue(
		"char_jump_delay",
		"char_jump_delay_value",
		m_Player->uberAnim_->jumpStartTime,
		"%.2f"
	);

	bUiIdleMode =
		m_Player->uberAnim_->IsInUI != 0;

	bCharacterControlsInitialized = true;
}

void CharacterHUD::UpdateCharacterControls()
{
	if (
		!bCharacterRmlReady ||
		!m_Player ||
		!m_Player->uberAnim_
	)
	{
		return;
	}

	InitializeCharacterControls();

	m_Player->uberAnim_->jumpAnimSpeed =
		g_CharacterRmlUI.
			GetCharacterInputValue(
				"char_jump_speed",
				m_Player->uberAnim_->
					jumpAnimSpeed
			);

	m_Player->uberAnim_->jumpStartTime =
		g_CharacterRmlUI.
			GetCharacterInputValue(
				"char_jump_delay",
				m_Player->uberAnim_->
					jumpStartTime
			);

	g_CharacterRmlUI.SetCharacterInputValue(
		"char_jump_speed",
		"char_jump_speed_value",
		m_Player->uberAnim_->jumpAnimSpeed,
		"%.2f"
	);

	g_CharacterRmlUI.SetCharacterInputValue(
		"char_jump_delay",
		"char_jump_delay_value",
		m_Player->uberAnim_->jumpStartTime,
		"%.2f"
	);

	r3dAnimation& Animation =
		m_Player->uberAnim_->anim;

	const bool bHaveAnimation =
		!Animation.AnimTracks.empty();

	g_CharacterRmlUI.SetCharacterVisible(
		"character_frame_controls",
		bHaveAnimation && paused
	);

	g_CharacterRmlUI.SetCharacterVisible(
		"character_speed_controls",
		bHaveAnimation && !paused
	);

	if (!bHaveAnimation)
		return;

	r3dAnimation::r3dAnimInfo& Info =
		Animation.AnimTracks[0];

	if (paused)
	{
		Info.dwStatus |= ANIMSTATUS_Paused;

		const float MaximumFrame =
			R3D_MAX(
				0.0f,
				static_cast<float>(
					Info.GetAnim()->
						GetNumFrames()
				) - 1.0f
			);

		g_CharacterRmlUI.SetCharacterInputRange(
			"char_frame",
			0.0f,
			MaximumFrame,
			1.0f
		);

		curTime =
			g_CharacterRmlUI.
				GetCharacterInputValue(
					"char_frame",
					curTime
				);

		curTime = R3D_CLAMP(
			curTime,
			0.0f,
			MaximumFrame
		);

		Info.fCurFrame = curTime;

		g_CharacterRmlUI.SetCharacterInputValue(
			"char_frame",
			"char_frame_value",
			curTime,
			"%.0f"
		);
	}
	else
	{
		Info.dwStatus &=
			~ANIMSTATUS_Paused;

		float Speed =
			g_CharacterRmlUI.
				GetCharacterInputValue(
					"char_current_speed",
					Info.GetSpeed()
				);

		Speed = R3D_CLAMP(
			Speed,
			0.0f,
			4.0f
		);

		Info.SetSpeed(Speed);

		g_CharacterRmlUI.SetCharacterInputValue(
			"char_current_speed",
			"char_current_speed_value",
			Speed,
			"%.2f"
		);
	}
}

void CharacterHUD::UpdateCharacterRmlDocument()
{
	if (
		!bCharacterRmlReady ||
		!m_Player ||
		!m_Player->uberAnim_
	)
	{
		return;
	}

	g_CharacterRmlUI.SetCharacterMode(
		bPlayerStatesMode ? 0 : 1
	);

	g_CharacterRmlUI.SetCharacterSelectedState(
		SelectedPlayerState
	);

	g_CharacterRmlUI.SetCharacterSelectedDirection(
		SelectedMoveDirection
	);

	if (
		bShowEquipment &&
		(
			CachedEquipmentCategory !=
				SelectedEquipmentCategory ||
			CachedEquipmentItem !=
				SelectedEquipmentItem
		)
	)
	{
		RebuildCharacterEquipmentList();
	}

	g_CharacterRmlUI.SetCharacterToggle(
		"btn_char_ui_idle",
		"char_ui_idle_value",
		bUiIdleMode
	);

	g_CharacterRmlUI.SetCharacterEquipmentCategory(
		SelectedEquipmentCategory
	);

	g_CharacterRmlUI.SetCharacterToggle(
		"btn_char_show_skeleton",
		"char_show_skeleton_value",
		bShowSkeleton
	);

	g_CharacterRmlUI.SetCharacterToggle(
		"btn_char_show_anim_stack",
		"char_show_anim_stack_value",
		bShowAnimStack
	);

	g_CharacterRmlUI.SetCharacterToggle(
		"btn_char_show_equipment",
		"char_show_equipment_value",
		bShowEquipment
	);

	g_CharacterRmlUI.SetCharacterVisible(
		"characters_left_panel",
		!bShowEquipment
	);

	g_CharacterRmlUI.SetCharacterVisible(
		"character_equipment_panel",
		bShowEquipment
	);

	g_CharacterRmlUI.SetCharacterText(
		"btn_char_play_pause",
		paused ? "PLAY" : "PAUSE"
	);

	r3dAnimation& Animation =
		m_Player->uberAnim_->anim;

	if (Animation.GetAnimPool())
	{
		const int AnimationCount =
			static_cast<int>(
				Animation.GetAnimPool()->
					Anims.size()
			);

		if (
			AnimationCount !=
				CachedAnimationCount ||
			SelectedAnimation !=
				CachedSelectedAnimation
		)
		{
			std::vector<const char*>
				AnimationNames;

			AnimationNames.reserve(
				AnimationCount
			);

			for (
				int Index = 0;
				Index < AnimationCount;
				++Index
			)
			{
				AnimationNames.push_back(
					Animation.GetAnimPool()->
						Anims[Index]->
						GetAnimName()
				);
			}

			g_CharacterRmlUI.
				SetCharacterAnimationList(
					AnimationNames.empty()
						? nullptr
						: AnimationNames.data(),
					AnimationCount,
					SelectedAnimation
				);

			CachedAnimationCount =
				AnimationCount;

			CachedSelectedAnimation =
				SelectedAnimation;
		}
	}

	float Length = 0.0f;
	int Frames = 0;
	int Tracks = 0;
	float FrameRate = 0.0f;

	const char* AnimationName = "-";
	const char* AnimationFile = "-";

	if (
		!Animation.AnimTracks.empty() &&
		Animation.AnimTracks[0].pAnim
	)
	{
		const r3dAnimData* Data =
			Animation.AnimTracks[0].pAnim;

		Frames = Data->GetNumFrames();
		Tracks = Data->GetNumTracks();
		FrameRate = Data->GetFrameRate();

		if (FrameRate > 0.0f)
		{
			Length =
				static_cast<float>(Frames) /
				FrameRate;
		}

		AnimationName =
			Data->GetAnimName();

		AnimationFile =
			Data->GetAnimFileName();
	}

	g_CharacterRmlUI.SetCharacterAnimationInfo(
		Length,
		Frames,
		Tracks,
		FrameRate,
		AnimationName,
		AnimationFile
	);

	if (!bShowAnimStack)
	{
		g_CharacterRmlUI.
			SetCharacterAnimationStack(
				nullptr,
				nullptr,
				0
			);

		return;
	}

	std::vector<std::string> StackNames;
	std::vector<std::string> StackData;

	StackNames.reserve(
		Animation.AnimTracks.size()
	);

	StackData.reserve(
		Animation.AnimTracks.size()
	);

	for (
		size_t Index = 0;
		Index < Animation.AnimTracks.size();
		++Index
	)
	{
		const r3dAnimation::r3dAnimInfo& Info =
			Animation.AnimTracks[Index];

		const char* Name =
			Info.pAnim
				? Info.pAnim->GetAnimName()
				: "UNKNOWN";

		StackNames.push_back(Name);

		char Status[128]{};

		if (Info.dwStatus & ANIMSTATUS_Playing)
			strcat_s(Status, "PLAY ");

		if (Info.dwStatus & ANIMSTATUS_Paused)
			strcat_s(Status, "PAUSE ");

		if (Info.dwStatus & ANIMSTATUS_Finished)
			strcat_s(Status, "FINISH ");

		if (Info.dwStatus & ANIMSTATUS_Fading)
			strcat_s(Status, "FADE ");

		if (Info.dwStatus & ANIMSTATUS_Expiring)
			strcat_s(Status, "EXPIRE ");

		char DataText[256]{};

		sprintf_s(
			DataText,
			"INFLUENCE %.2f | FRAME %.1f | %04X | %s",
			Info.fInfluence,
			Info.fCurFrame,
			Info.dwStatus,
			Status
		);

		StackData.push_back(DataText);
	}

	std::vector<const char*> StackNamePointers;
	std::vector<const char*> StackDataPointers;

	StackNamePointers.reserve(
		StackNames.size()
	);

	StackDataPointers.reserve(
		StackData.size()
	);

	for (
		size_t Index = 0;
		Index < StackNames.size();
		++Index
	)
	{
		StackNamePointers.push_back(
			StackNames[Index].c_str()
		);

		StackDataPointers.push_back(
			StackData[Index].c_str()
		);
	}

	g_CharacterRmlUI.SetCharacterAnimationStack(
		StackNamePointers.empty()
			? nullptr
			: StackNamePointers.data(),
		StackDataPointers.empty()
			? nullptr
			: StackDataPointers.data(),
		static_cast<int>(
			StackNamePointers.size()
		)
	);
}

void CharacterHUD :: SetCameraDir (r3dPoint3D vPos )
{
	FPS_vVision = vPos;
	FPS_vVision.Normalize();
}

r3dPoint3D CharacterHUD :: GetCameraDir () const
{
	return FPS_vVision;
}

void CharacterHUD :: SetCameraPure ( r3dCamera &Cam)
{
 	r3dPoint3D CamPos = cameraPosition;
 	r3dPoint3D ViewPos = CamPos + FPS_vVision*10.0f;
 
 	extern float GameFOV;
 	Cam.FOV = GameFOV;
 	Cam.SetPosition( CamPos );
 	Cam.PointTo(ViewPos);
}  

r3dPoint2D CharacterHUD::DrawCurrentAnimInfo(float BaseY)
{
	r3dAnimation& a = m_Player->uberAnim_->anim;

	int W = 290;
	float X = r3dRenderer->ScreenW - W;
	float Y = BaseY;
	char buffer[1024] = "";
	
	if(a.AnimTracks.size() > 0)
	{
		const r3dAnimData* ad = a.AnimTracks[0].pAnim;
		sprintf(buffer, "Length: %-02.4f sec\nNum Frames: %d\nNum Tracks: %d\nFrameRate: %-02.4f\nAnim Name: %s\nAnim File Name: %s", 
			ad->GetNumFrames() / ad->GetFrameRate(), 
			ad->GetNumFrames(), 
			ad->GetNumTracks(), 
			ad->GetFrameRate(), 
			ad->GetAnimName(), 
			ad->GetAnimFileName());
	}		

	Y += imgui_Static(X, Y, buffer, W, true, 100);
	return r3dPoint2D(X, Y);
}

void CharacterHUD::DrawSkeleton(r3dSkeleton& skel)
{
	skel.DrawSkeleton(gCam, r3dPoint3D(0,0,0));
}

void CharacterHUD::StartDefaultAnim()
{
	r3dAnimation& a = m_Player->uberAnim_->anim;
	a.StartAnimation(0, ANIMFLAG_Looped | ANIMFLAG_RemoveOtherNow, 0.0f, 1.0f, 0.0f);
}

//////////////////////////////////////////////////////////////////////////
void CharacterHUD::Draw()
{
	assert(bInited);
	
	if (!bInited)
		return;
	
	if (!bCharacterRmlReady && !bCharacterRmlInitAttempted)
	{
		bCharacterRmlInitAttempted = true;

		InitCharacterRmlUI();

		if (!bCharacterRmlReady)
		{
			r3dOutToLog(
				"[RmlUI] Character editor init failed, "
				"legacy UI will be used\n"
			);
		}
	}

	CreateCharacter();

	if (
		!bCharacterRmlReady ||
		!g_CharacterRmlUI.
			IsCharacterEditorVisible()
	)
	{
		DrawLegacyUI();
		return;
	}

	if (!m_Player || !m_Player->uberAnim_)
		return;

	r3dAnimation& Animation =
		m_Player->uberAnim_->anim;

	if (
		bShowSkeleton &&
		Animation.GetCurrentSkeleton()
	)
	{
		DrawSkeleton(
			*Animation.GetCurrentSkeleton()
		);
	}

	g_CharacterRmlUI.Update(
	r3dGetFrameTime()
	);

	UpdateCharacterControls();
	UpdateCharacterRmlDocument();

	r3dRenderer->pd3ddev->SetRenderState(
		D3DRS_ALPHATESTENABLE,
		FALSE
	);

	r3dRenderer->SetMaterial(nullptr);

	r3dRenderer->SetRenderingMode(
		R3D_BLEND_ALPHA |
		R3D_BLEND_NZ
	);

	g_CharacterRmlUI.Render();

	r3dRenderer->pd3ddev->SetRenderState(
		D3DRS_ALPHATESTENABLE,
		FALSE
	);

	r3dRenderer->pd3ddev->SetRenderState(
		D3DRS_ALPHAREF,
		1
	);

	r3dRenderer->SetRenderingMode(
		R3D_BLEND_ALPHA |
		R3D_BLEND_NZ
	);
}

void CharacterHUD::DrawLegacyUI()
{
	assert(bInited);

	if ( !bInited ) return;

	CreateCharacter();

	r3dSetFiltering( R3D_POINT );

	r3dRenderer->pd3ddev->SetRenderState( D3DRS_ALPHATESTENABLE, 	FALSE );
	r3dRenderer->pd3ddev->SetRenderState( D3DRS_ALPHAREF,        	1 );

	r3dRenderer->SetMaterial(NULL);
	r3dRenderer->SetRenderingMode(R3D_BLEND_ALPHA);

	imgui_Update();

	r3dAnimation& a = m_Player->uberAnim_->anim;
	float Y = 10.0f;

	static int testBlend = 1;
	imgui_Checkbox(0.0f, Y, "Player States", &testBlend, 1);

	static int equipShow = 0;
	imgui_Checkbox(370.0f, Y, "Show Equipment", &equipShow, 1);

	Y += 30.0f;
	
	if(testBlend)
		DrawPlayerStates(Y);
	else
		DrawAllAnims(Y);


	r3dPoint2D uiPos = DrawCurrentAnimInfo(10.0f);

	static int showSkeleton = 0;
	float ssY = uiPos.y;
	float ssX = uiPos.x;
	imgui_Checkbox(ssX, ssY, 200, 30, "Show Skeleton", &showSkeleton, 1);
	ssY += 40;

	if(showSkeleton)
	{
		DrawSkeleton(*a.GetCurrentSkeleton());
	}

	static int showAnimStack = 1;	
	imgui_Checkbox(ssX, ssY, 200, 30, "Show Anim Stack", &showAnimStack, 1);
	ssY += 40;
	
        for(size_t i=0; i<a.AnimTracks.size(); i++) 
        {
          const r3dAnimation::r3dAnimInfo& ai = a.AnimTracks[i];
          
          char st[256] = "";
          if(ai.dwStatus & ANIMSTATUS_Playing) strcat(st, "Play ");
          if(ai.dwStatus & ANIMSTATUS_Paused) strcat(st, "Pause ");
          if(ai.dwStatus & ANIMSTATUS_Finished) strcat(st, "Finish ");
          if(ai.dwStatus & ANIMSTATUS_Fading) strcat(st, "Fade ");
          if(ai.dwStatus & ANIMSTATUS_Expiring) strcat(st, "Expire ");
          
	  _r3dSystemFont->PrintF(ssX, ssY, r3dColor(255, 255, 255), "%s: %.2f, f:%.1f, %04X %s", 
	    ai.pAnim->pAnimName, ai.fInfluence, ai.fCurFrame, ai.dwStatus, st);

	  ssY += 20;
	}
	
	if(equipShow)
	{
		extern void ProcessCharacterEditor(obj_Player* pl, float left, float top, float height);
		ProcessCharacterEditor(m_Player, 0.0f, r3dRenderer->ScreenH - 275.0f, 250.0f);
	}	
	
	r3dRenderer->pd3ddev->SetRenderState( D3DRS_ALPHATESTENABLE, 	FALSE );
	r3dRenderer->pd3ddev->SetRenderState( D3DRS_ALPHAREF,        	1 );

	r3dRenderer->SetRenderingMode(R3D_BLEND_ALPHA | R3D_BLEND_NZ);
}

void CharacterHUD::DrawPlayerStates(float& Y)
{
	float W = 300.0f;

	static stringlist_t names1;
	if(names1.size() == 0)
	{
		names1.push_back("IDLE");
		names1.push_back("IDLEAIM");
		names1.push_back("MOVE_CROUCH");
		names1.push_back("MOVE_CROUCH_AIM");
		names1.push_back("MOVE_WALK_AIM");
		names1.push_back("MOVE_RUN");
		names1.push_back("MOVE_SPRINT");
		names1.push_back("JUMP_STAND");
		names1.push_back("DIE");
	}

	static stringlist_t names2;
	if(names2.size() == 0)
	{
		// same as CUberData::ANIMDIR_*
		names2.push_back("Stand");
		names2.push_back("Str");
		names2.push_back("StrLeft");
		names2.push_back("StrRight");
		names2.push_back("Left");
		names2.push_back("Right");
		names2.push_back("Back");
		names2.push_back("BackLeft");
		names2.push_back("BackRight");
	}

	static float offset1;
	static int idx1 = 0;
	int current1 = idx1;
	imgui_DrawList(0.0f, Y, W, 170, names1, &offset1, &current1);
	if(idx1 != current1) {
		idx1 = current1;
		m_Player->uberAnim_->AnimPlayerState = -1;
		m_Player->PlayerState = current1;
	}  
	Y += 170 + 20;

	static float offset2;
	static int idx2 = 0;
	int current2 = idx2;
	imgui_DrawList(0.0f, Y, W, 170, names2, &offset2, &current2);
	if(idx2 != current2) {
		idx2 = current2;
		m_Player->PlayerMoveDir = current2;
	}
	Y += 170 + 20;
	
	static int IsInUI = 0;
	Y += imgui_Checkbox(0.0f, Y, (int)W, 30, "Toggle UI Idle Mode", &IsInUI, 0x1);
	if(IsInUI != m_Player->uberAnim_->IsInUI) {
		m_Player->uberAnim_->IsInUI = IsInUI;
		m_Player->uberAnim_->AnimPlayerState = -1;
	}

	if(imgui_Button(0.0f, Y, W, 30.0f, "Reload"))
	{
		m_Player->uberAnim_->scaleReloadAnimTime = false;	// DISABLE reload scaling in editor
		m_Player->uberAnim_->StartReloadAnim();
	}
	Y += 30;
	if(imgui_Button(0.0f, Y, W, 30.0f, "Shoot"))
	{
		m_Player->uberAnim_->StartShootAnim();
	}
	Y += 30;
	Y += 10;

	Y += imgui_Value_Slider(0.0f, Y, "JumpAnimSpeed", &m_Player->uberAnim_->jumpAnimSpeed, 0.5f, 4.0f, "%.2f");
	Y += imgui_Value_Slider(0.0f, Y, "JumpDelay", &m_Player->uberAnim_->jumpStartTime, 0.0f, 1.0f, "%.2f");
	if(imgui_Button(0.0f, Y, W, 30.0f, "Jump"))
	{
		m_Player->StartJump();
	}
	Y += 30;
	if(imgui_Button(0.0f, Y, W, 30.0f, "Jump Only Anim"))
	{
		m_Player->uberAnim_->StartJump();
	}
	Y += 30;
	
	if(imgui_Button(0.0f, Y, W, 30.0f, "In Air"))
	{
		// switch to AIR
		int idx = m_Player->uberAnim_->data_->GetJumpAnimId(m_Player->uberAnim_->AnimPlayerState, 1);
		m_Player->uberAnim_->anim.Stop(m_Player->uberAnim_->jumpTrackID);
		m_Player->uberAnim_->jumpTrackID = m_Player->uberAnim_->anim.StartAnimation(idx, ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
		m_Player->uberAnim_->jumpState   = -1;
	
		// resync animation, so jump track will be relocated to top of lower bodys anim
		m_Player->uberAnim_->SwitchToState(m_Player->uberAnim_->AnimPlayerState, m_Player->uberAnim_->AnimMoveDir);
	}
	Y += 30;
	
	m_Player->UpdateLocalPlayerMovement();
}

void CharacterHUD::DrawAllAnims(float& Y)
{
	float TH = 200.0f;
	float W  = 300.0f;

	r3dAnimation& a = m_Player->uberAnim_->anim;

	stringlist_t names;
	unsigned int count = a.GetAnimPool()->Anims.size();
	names.resize(count);
	for(unsigned int i = 0; i < count; ++i)
	{
		names[i] = a.GetAnimPool()->Anims[i]->GetAnimName();
	}

	static float offset;
	static int idx = -1;
	int current = idx;
	imgui_DrawList(0.0f, Y, W, TH * 2.0f, names, &offset, &current);

	if(current != idx)
	{
		idx = current;
		a.StartAnimation(names[idx].c_str(), ANIMFLAG_Looped | ANIMFLAG_RemoveOtherNow, 0.0f, 1.0f, 0.0f);
	}  
	Y += TH * 2.0f;

	const char* status[2] = {"Play", "Pause"};
	int statusIdx = paused ? 0 : 1;		
	if(imgui_Button(0.0f, Y, 40.0f, 30.0f, status[statusIdx]))
	{
		paused = !paused;
	}

	if(!paused)
	{
		if(imgui_Button(41.0f, Y, 40.0f, 30.0f, "Stop"))
		{
			a.StopAll();
		}
	}		

	Y += 30.0f;
	statusIdx = paused ? 0 : 1;


	if(a.AnimTracks.empty())
		return;
		
	r3dAnimation::r3dAnimInfo& info = a.AnimTracks[0];
	if(paused)
	{
		info.dwStatus |= ANIMSTATUS_Paused;
		info.fCurFrame = curTime;
	}
	else
	{
		info.dwStatus &= ~ANIMSTATUS_Paused;
	}

	if(paused)
	{
		float numLimit = (float)info.GetAnim()->GetNumFrames() - 1;
		Y += imgui_Value_Slider(0.0f, Y, "Frame", &curTime, 0.0f, numLimit, "%-02.0f");
	}
	else
	{
		float s = info.GetSpeed();
		Y += imgui_Value_Slider(0.0f, Y, "Current Speed", &s, 0.0f, 4.0f, "%-02.2f");
		info.SetSpeed(s);

		if(imgui_Button(0.0f, Y, 80.0f, 30.0f, "Default Speed"))
		{
			info.SetSpeed(1.0f);
		}
	}

	Y += 30.0f;	
}


//----------------------------------------------------------------
void CharacterHUD::Process()
{
	if (bCharacterRmlReady)
	{
		ProcessStudioPendingResize(
			&g_CharacterRmlUI
		);
	}
	else
	{
		ProcessStudioPendingResize(
			nullptr
		);
	}

	CreateCharacter();

	FPS_Acceleration.Assign(0, 0, 0);

	float	glb_MouseSens    = 0.5f;	// in range (0.1 - 1.0)
	float  glb_MouseSensAdj = 1.0f;	// in range (0.1 - 1.0)


	OnProcess ();

	extern float imgui_mmx;
	extern float imgui_mmy;

	//
	//  Mouse controls are here
	//
	if( Mouse->IsPressed(r3dMouse::mRightButton) )
	{
		FPS_ViewAngle.x += -imgui_mmx * glb_MouseSensAdj;
		FPS_ViewAngle.y += -imgui_mmy * glb_MouseSensAdj;

		if(FPS_ViewAngle.y > 70)  FPS_ViewAngle.y = 70;
		if(FPS_ViewAngle.y < -70) FPS_ViewAngle.y = -70;
	}

	//r3dPoint3D playerPos(0, 0, 0);
	//m_Player->SetPosition(playerPos);
	//m_Player->m_fPlayerRotationTarget = m_Player->m_fPlayerRotation;
	m_Player->UpdateTransform();

	// walk
	float fSpeed = 2.0f; //_ai_fWalkSpeed;

	if(Keyboard->IsPressed(kbsW)) FPS_Acceleration.Z = fSpeed;
	if(Keyboard->IsPressed(kbsS)) FPS_Acceleration.Z = -fSpeed * 0.7f;
	if(Keyboard->IsPressed(kbsA)) FPS_Acceleration.X = -fSpeed * 0.7f;
	if(Keyboard->IsPressed(kbsD)) FPS_Acceleration.X = fSpeed * 0.7f;
	if(Keyboard->IsPressed(kbsQ)) cameraPosition.Y    += SRV_WORLD_SCALE(1.0f)* r3dGetFrameTime();
	if(Keyboard->IsPressed(kbsE)) cameraPosition.Y    -= SRV_WORLD_SCALE(1.0f)* r3dGetFrameTime();

	float mult = 1;
	if(Keyboard->IsPressed(kbsLeftShift)) mult = 10.0f;

	r3dPoint3D size = m_Player->GetBBoxLocal().Size;
	float minDist = 0.0f; //std::max(size.x, size.z) * 2.5f;
	float maxDist = 999.0f;
	currentDist -= FPS_Acceleration.Z * r3dGetFrameTime() * mult;
	currentDist = R3D_CLAMP(currentDist, minDist, maxDist);

 	cameraPosition = r3dPoint3D(0,0,-1)*currentDist;
// 	cameraPosition.y = size.y * 0.5f;

	D3DXMATRIX mr;
	D3DXMatrixIdentity(&mr);
	D3DXMatrixRotationYawPitchRoll(&mr, R3D_DEG2RAD(-FPS_ViewAngle.x), R3D_DEG2RAD(FPS_ViewAngle.y), 0);

	D3DXMATRIX t0, t1;
	D3DXMatrixTranslation(&t0, -cameraPosition.x, -cameraPosition.y, -cameraPosition.z);
 
	D3DXVECTOR3 pos(0,0,0);
	D3DXVECTOR3 up(0,1,0);
	D3DXVECTOR4 rup;
	D3DXVECTOR4 rpos;
	D3DXVec3Transform(&rpos, &pos, &(t0*mr));
	D3DXVec3Transform(&rup, &up, &(mr));

// 	D3DXMatrixLookAtRH(&t1, &D3DXVECTOR3(rpos.x, rpos.y, rpos.z), &pos, &up);
// 	D3DXVec3Transform(&rpos, &pos, &(t1));

	cameraPosition.x = rpos.x;
	cameraPosition.y = rpos.y + size.y * 0.5f;
	cameraPosition.z = rpos.z;

	SetCameraDir(r3dPoint3D(0,size.y * 0.5f, 0)-cameraPosition);
	gCam.FOV = 60;
	gCam.SetPlanes(currentDist + 0.001f, currentDist + minDist);

	static bool first = true;
	if(first)
	{
		StartDefaultAnim();
		first = false;
	}

	if (
		bPlayerStatesMode &&
		m_Player &&
		m_Player->uberAnim_
	)
	{
		m_Player->UpdateLocalPlayerMovement();
	}
}



void CharacterHUD::ProcessPick( bool bSimple/* = false*/ )
{
}

void CharacterHUD::OnProcess ()
{

}

void CharacterHUD::OnHudUnselected ()
{
}
