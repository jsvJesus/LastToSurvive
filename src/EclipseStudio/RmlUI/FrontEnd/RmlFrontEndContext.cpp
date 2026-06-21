#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndContext.h"
#include "../RmlRuntime.h"
#include "RmlFrontEndCharacterPreview.h"

#include "cvar.h"
#include "GameCode/UserProfile.h"
#include "backend/WOBackendAPI.h"
#include "ObjectsCode/weapons/WeaponArmory.h"

#include "RmlFrontEndShop.h"
#include "RmlFrontEndSkills.h"

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
#include <vector>
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

	constexpr int ShopCategoryPlaceable =
		28;

	constexpr int ShopCategoryMedicine =
		31;

	constexpr int ShopCategoryUsable =
		32;

	constexpr int ShopGridColumns =
		12;

	constexpr int ShopGridCellSize =
		64;

	struct FShopGridSize
	{
		int Width = 1;
		int Height = 1;
	};

	bool IsOneOfItemIds(
		uint32_t ItemId,
		const uint32_t* ItemIds,
		size_t ItemCount
	)
	{
		for (size_t Index = 0; Index < ItemCount; ++Index)
		{
			if (ItemIds[Index] == ItemId)
				return true;
		}

		return false;
	}

	bool IsShopMedicineItem(
		uint32_t ItemId
	)
	{
		static const uint32_t MedicineItemIds[] =
		{
			101256,
			101261,
			101262,
			101300,
			101301,
			101302,
			101303,
			101304
		};

		return IsOneOfItemIds(
			ItemId,
			MedicineItemIds,
			sizeof(MedicineItemIds) /
				sizeof(MedicineItemIds[0])
		);
	}

	bool IsShopHandUsableItem(
		uint32_t ItemId
	)
	{
		static const uint32_t UsableItemIds[] =
		{
			101315,
			101319,
			101323
		};

		return IsOneOfItemIds(
			ItemId,
			UsableItemIds,
			sizeof(UsableItemIds) /
				sizeof(UsableItemIds[0])
		);
	}

	bool IsShopPlaceableItem(
		uint32_t ItemId
	)
	{
		static const uint32_t PlaceableItemIds[] =
		{
			101305,
			101316,
			101317,
			101318,
			101324,
			101348,
			101352,
			101353,
			101354,
			101355,
			101356,
			101357,
			101358,
			101359,
			101360,
			101361,
			101362,
			101363,
			101364,
			101365,
			101366,
			101367,
			101368,
			101369,
			101370,
			101371,
			101372,
			101373,
			101374,
			101375,
			101376,
			101377,
			101378,
			101379,
			101380
		};

		return IsOneOfItemIds(
			ItemId,
			PlaceableItemIds,
			sizeof(PlaceableItemIds) /
				sizeof(PlaceableItemIds[0])
		);
	}

	int GetShopRuntimeCategory(
		const BaseItemConfig* Config
	)
	{
		if (!Config)
			return storecat_INVALID;

		const int Category =
			static_cast<int>(
				Config->category
			);

		if (Config->m_StoreCategoryOverride > 0)
			return Config->m_StoreCategoryOverride;

		if (Category == storecat_UsableItem)
		{
			if (IsShopMedicineItem(Config->m_itemID))
				return ShopCategoryMedicine;

			if (IsShopPlaceableItem(Config->m_itemID))
				return ShopCategoryPlaceable;

			if (IsShopHandUsableItem(Config->m_itemID))
				return ShopCategoryUsable;

			return ShopCategoryUsable;
		}

		return Category;
	}

	bool IsShopRuntimeCategoryAllowed(
		int Category
	)
	{
		return
			Category != storecat_INVALID &&
			Category != storecat_Account &&
			Category != storecat_Boost &&
			Category != storecat_LootBox &&
			Category != storecat_HeroPackage;
	}

	const char* GetShopCategoryLabel(
		int Category
	)
	{
		switch (Category)
		{
			case storecat_Account:
				return "ACCOUNT";
			case storecat_Boost:
				return "BOOST";
			case storecat_LootBox:
				return "CRATE";
			case storecat_Armor:
				return "ARMOR";
			case storecat_Backpack:
				return "BACKPACK";
			case storecat_Helmet:
				return "HELMET";
			case storecat_HeroPackage:
				return "SURVIVOR";
			case storecat_FPSAttachment:
				return "ATTACHMENT";
			case storecat_ASR:
				return "ASSAULT RIFLE";
			case storecat_SNP:
				return "SNIPER RIFLE";
			case storecat_SHTG:
				return "SHOTGUN";
			case storecat_MG:
				return "MACHINE GUN";
			case storecat_HG:
				return "HANDGUN";
			case storecat_SMG:
				return "SUBMACHINE GUN";
			case storecat_GRENADE:
				return "EXPLOSIVE";
			case storecat_UsableItem:
				return "PLACEABLE";
			case storecat_MELEE:
				return "MELEE";
			case storecat_Food:
				return "FOOD";
			case ShopCategoryMedicine:
				return "MEDICINE";
			case ShopCategoryUsable:
				return "USABLE";
			case storecat_Water:
				return "WATER";
			default:
				return "ITEM";
		}
	}

	bool IsShopCategoryMatch(
		const Rml::String& CategoryId,
		int Category
	)
	{
		if (
			CategoryId.empty() ||
			CategoryId == "shop_category_featured"
		)
		{
			return IsShopRuntimeCategoryAllowed(
				Category
			);
		}

		if (CategoryId == "shop_category_weapons")
		{
			return
				Category == storecat_ASR ||
				Category == storecat_SNP ||
				Category == storecat_SHTG ||
				Category == storecat_MG ||
				Category == storecat_HG ||
				Category == storecat_SMG ||
				Category == storecat_GRENADE ||
				Category == storecat_MELEE;
		}

		if (CategoryId == "shop_category_body_armor")
			return Category == storecat_Armor;

		if (CategoryId == "shop_category_helmets")
			return Category == storecat_Helmet;

		if (CategoryId == "shop_category_backpacks")
			return Category == storecat_Backpack;

		if (CategoryId == "shop_category_attachments")
			return Category == storecat_FPSAttachment;

		if (CategoryId == "shop_category_placeable")
			return Category == ShopCategoryPlaceable;

		if (CategoryId == "shop_category_food")
			return Category == storecat_Food;

		if (CategoryId == "shop_category_medicine")
			return Category == ShopCategoryMedicine;

		if (CategoryId == "shop_category_usable")
			return Category == ShopCategoryUsable;

		if (CategoryId == "shop_category_water")
			return Category == storecat_Water;

		return true;
	}

	FShopGridSize GetDefaultShopGridSize(
		int Category,
		uint32_t ItemId
	)
	{
		switch (Category)
		{
			case storecat_ASR:
			case storecat_SNP:
			case storecat_SHTG:
			case storecat_MG:
			case storecat_SMG:
				return { 4, 2 };
			case storecat_Armor:
				return { 2, 3 };
			case storecat_Backpack:
			case storecat_Helmet:
			case storecat_HG:
			case storecat_MELEE:
			case ShopCategoryPlaceable:
				return { 2, 2 };
			case storecat_FPSAttachment:
				return ItemId % 2
					? FShopGridSize { 1, 2 }
					: FShopGridSize { 2, 1 };
			case storecat_GRENADE:
			case storecat_Food:
			case ShopCategoryMedicine:
			case ShopCategoryUsable:
			case storecat_Water:
				return { 1, 1 };
			default:
				return { 1, 1 };
		}
	}

	FShopGridSize GetShopGridSize(
		const BaseItemConfig* Config,
		int Category
	)
	{
		FShopGridSize Size =
			GetDefaultShopGridSize(
				Category,
				Config
					? Config->m_itemID
					: 0
			);

		if (
			Config &&
			Config->m_StoreSlotWidth > 0 &&
			Config->m_StoreSlotHeight > 0
		)
		{
			Size.Width =
				Config->m_StoreSlotWidth;

			Size.Height =
				Config->m_StoreSlotHeight;
		}

		Size.Width = std::max(
			1,
			std::min(
				ShopGridColumns,
				Size.Width
			)
		);

		Size.Height = std::max(
			1,
			std::min(
				6,
				Size.Height
			)
		);

		return Size;
	}

	const char* GetShopItemSizeClass(
		const FShopGridSize& Size
	)
	{
		if (Size.Width == 4 && Size.Height == 2)
			return "item_size_4x2";

		if (Size.Width == 2 && Size.Height == 3)
			return "item_size_2x3";

		if (Size.Width == 2 && Size.Height == 2)
			return "item_size_2x2";

		if (Size.Width == 2 && Size.Height == 1)
			return "item_size_2x1";

		if (Size.Width == 1 && Size.Height == 2)
			return "item_size_1x2";

		return "item_size_1x1";
	}

	int GetShopQuantity(
		uint32_t ItemId
	)
	{
		int Quantity = 1;

		if (g_pWeaponArmory)
		{
			const WeaponConfig* Weapon =
				g_pWeaponArmory->getWeaponConfig(
					ItemId
				);

			if (Weapon)
				Quantity = Weapon->m_ShopStackSize;

			const FoodConfig* Food =
				g_pWeaponArmory->getFoodConfig(
					ItemId
				);

			if (Food)
				Quantity = Food->m_ShopStackSize;
		}

		return std::max(
			1,
			Quantity
		);
	}

	std::string GetShopIconPath(
	const BaseItemConfig* Config
)
	{
		if (
			Config &&
			Config->m_StoreIcon &&
			Config->m_StoreIcon[0]
		)
		{
			std::string Path =
				Config->m_StoreIcon;

			std::replace(
				Path.begin(),
				Path.end(),
				'\\',
				'/'
			);

			const std::string DataPrefix =
				"$Data/";

			if (
				Path.compare(
					0,
					DataPrefix.length(),
					DataPrefix
				) == 0
			)
			{
				Path.erase(
					0,
					DataPrefix.length()
				);
			}
			else if (
				Path.compare(
					0,
					5,
					"Data/"
				) == 0
			)
			{
				Path.erase(
					0,
					5
				);
			}

			if (Path.find('/') == std::string::npos)
			{
				Path =
					std::string(
						"Weapons/StoreIcons/"
					) +
					Path;
			}

			const size_t SlashPosition =
				Path.find_last_of('/');

			const size_t DotPosition =
				Path.find_last_of('.');

			const bool bHasExtension =
				DotPosition != std::string::npos &&
				(
					SlashPosition == std::string::npos ||
					DotPosition > SlashPosition
				);

			if (!bHasExtension)
			{
				Path += ".dds";
			}

			/*
			 * Shop.rml лежит тут:
			 * Data/Rml/FrontEnd/Shop.rml
			 *
			 * А store icons лежат тут:
			 * Data/Weapons/StoreIcons/*.dds
			 *
			 * Поэтому из Rml/FrontEnd нужно выйти на два уровня назад.
			 */
			if (
				Path.compare(
					0,
					6,
					"../../"
				) != 0
			)
			{
				Path =
					std::string("../../") +
					Path;
			}

			return Path;
		}

		return "../../Weapons/no_icon.dds";
	}

	uint32_t GetShopLowestPrice(
		const wiStoreItem& StoreItem
	)
	{
		if (
			StoreItem.pricePerm > 0 &&
			StoreItem.gd_pricePerm > 0
		)
		{
			return std::min(
				StoreItem.pricePerm,
				StoreItem.gd_pricePerm
			);
		}

		if (StoreItem.pricePerm > 0)
			return StoreItem.pricePerm;

		return StoreItem.gd_pricePerm;
	}

	bool IsShopItemOwned(
		uint32_t ItemId
	)
	{
		const wiUserProfile& Profile =
			gUserProfile.ProfileData;

		for (
			uint32_t Index = 0;
			Index < Profile.NumItems;
			++Index
		)
		{
			if (
				Profile.Inventory[Index].itemID ==
				ItemId
			)
			{
				return true;
			}
		}

		return false;
	}

	bool IsShopFeaturedItem(
		const wiStoreItem& StoreItem,
		const BaseItemConfig* Config
	)
	{
		return
			StoreItem.isNew ||
			(Config && Config->m_StoreFeatured);
	}

	bool DoesShopSortFilterMatch(
		const Rml::String& SortId,
		const wiStoreItem& StoreItem,
		const BaseItemConfig* Config
	)
	{
		if (SortId == "shop_tab_new")
			return StoreItem.isNew;

		if (SortId == "shop_tab_owned")
			return IsShopItemOwned(
				StoreItem.itemID
			);

		return true;
	}

	std::string FormatShopPrice(
		uint32_t Price
	)
	{
		if (Price == 0)
			return "-";

		char Buffer[64]{};

		sprintf_s(
			Buffer,
			"%u",
			Price
		);

		std::string Raw =
			Buffer;

		std::string Result;
		int Counter = 0;

		for (
			int Index =
				static_cast<int>(Raw.size()) - 1;
			Index >= 0;
			--Index
		)
		{
			if (Counter == 3)
			{
				Result.insert(
					Result.begin(),
					' '
				);
				Counter = 0;
			}

			Result.insert(
				Result.begin(),
				Raw[
					static_cast<size_t>(Index)
				]
			);

			++Counter;
		}

		return Result;
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

	if (Runtime.IsUsingDX11())
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Init] DX9 frontend skipped: runtime uses DX11\n"
		);

		Hwnd = nullptr;
		return false;
	}

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
	SkillsScreen = std::make_unique<RmlFrontEndSkills>();
	ShopScreen = std::make_unique<RmlFrontEndShop>();

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
	SelectedShopStoreIndex = -1;
	SelectedShopBuyIndex = 4;
	SelectedShopCategoryId = "shop_category_featured";
	SelectedShopCurrencyId = "shop_currency_gc";
	SelectedShopSortId = "shop_tab_hot";
	ShopVisibleItemIndices.clear();

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

	if (!SkillsScreen)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Skills screen object is missing\n"
		);

		return false;
	}

	if (!SkillsScreen->Load(Context))
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/Skills.rml\n"
		);

		return false;
	}

	SkillsDocument =
		SkillsScreen->GetDocument();

	if (!ShopScreen)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Shop screen object is missing\n"
		);

		return false;
	}

	if (!ShopScreen->Load(Context))
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd] Failed to load "
			"Data/Rml/FrontEnd/Shop.rml\n"
		);

		return false;
	}

	ShopDocument =
		ShopScreen->GetDocument();

	SkillsScreen->SetCallbacks(
		FRmlFrontEndSkillsCallbacks
		{
			[this]()
			{
				ShowMainMenu();
			},

			[this]()
			{
				ShowShop();
			},

			[this]()
			{
				BuildSkills();
			},

			[this]()
			{
				RequestLearnSelectedSkill();
			},

			[this]()
			{
				SetSkillsStatus(
					"Skill reset is not connected to backend yet."
				);
			},

			[this](const Rml::String& Id)
			{
				SelectSkillNode(
					Id
				);
			},

			[this](const Rml::String& Id)
			{
				(void)Id;

				SetSkillsStatus(
					"Skill category selected."
				);
			},

			[this](const Rml::String& Text)
			{
				SetSkillsStatus(
					Text
				);
			}
		}
	);

	ShopScreen->SetCallbacks(
		FRmlFrontEndShopCallbacks
		{
			[this]()
			{
				ShowMainMenu();
			},

			[this]()
			{
				ShowSkills();
			},

			[this]()
			{
				BuildShop();
			},

			[this]()
			{
				RequestBuySelectedShopItem();
			},

			[this]()
			{
				const int ApiCode =
					gUserProfile.ApiGetShopData();

				BuildShop();

				SetShopStatus(
					ApiCode == 0
						? "Shop data refreshed."
						: "Shop refresh failed."
				);
			},

			[this](const Rml::String& Id)
			{
				SelectShopItem(
					Id
				);
			},

			[this](const Rml::String& Id)
			{
				SelectShopCategory(
					Id
				);
			},

			[this](const Rml::String& Id)
			{
				SelectShopCurrency(
					Id
				);
			},

			[this](const Rml::String& Id)
			{
				if (Id == "cycle")
				{
					if (SelectedShopSortId == "shop_tab_hot")
						SelectedShopSortId = "shop_tab_new";
					else if (SelectedShopSortId == "shop_tab_new")
						SelectedShopSortId = "shop_tab_sale";
					else if (SelectedShopSortId == "shop_tab_sale")
						SelectedShopSortId = "shop_tab_owned";
					else
						SelectedShopSortId = "shop_tab_hot";
				}
				else
				{
					SelectedShopSortId =
						Id;
				}

				BuildShop();

				SetShopStatus(
					"Shop sort changed."
				);
			},

			[this](const Rml::String& Text)
			{
				SetShopStatus(
					Text
				);
			}
		}
	);

	LoginDocument->Hide();
	MainMenuDocument->Hide();
	CharacterCreateDocument->Hide();

	if (SkillsScreen)
		SkillsScreen->Hide();

	if (ShopScreen)
		ShopScreen->Hide();

	return true;
}

void RmlFrontEndContext::UnloadDocuments()
{
	if (!Context)
		return;

	if (SkillsScreen)
	{
		SkillsScreen->Unload();
		SkillsScreen.reset();
		SkillsDocument = nullptr;
	}

	if (ShopScreen)
	{
		ShopScreen->Unload();
		ShopScreen.reset();
		ShopDocument = nullptr;
	}

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
		CurrentScreen == EScreen::MainMenu
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
		CurrentScreen != EScreen::MainMenu
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
		CurrentScreen != EScreen::MainMenu
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

void RmlFrontEndContext::SetElementAttribute(
	Rml::ElementDocument* Document,
	const char* ElementId,
	const char* AttributeName,
	const Rml::String& Value
)
{
	if (
		!Document ||
		!ElementId ||
		!AttributeName
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

	Element->SetAttribute(
		AttributeName,
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
		!SkillsDocument ||
		!ShopDocument
	)
	{
		return;
	}

	MainMenuDocument->Hide();
	CharacterCreateDocument->Hide();
	
	if (SkillsScreen)
		SkillsScreen->Hide();
	if (ShopScreen)
		ShopScreen->Hide();
	
	LoginDocument->Show();

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
	
	if (SkillsScreen)
		SkillsScreen->Hide();

	if (ShopScreen)
		ShopScreen->Hide();
	
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
		!SkillsScreen ||
		!ShopScreen
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

	ShopScreen->Hide();
	SkillsScreen->Show();

	CurrentScreen =
		EScreen::Skills;

	SkillsScreen->Refresh();

	SetSkillsControlsEnabled(
		true
	);

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
		!SkillsScreen ||
		!ShopScreen
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

	SkillsScreen->Hide();
	ShopScreen->Show();

	CurrentScreen =
		EScreen::Shop;

	ShopScreen->Refresh();

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
	
	if (SkillsScreen)
		SkillsScreen->Hide();

	if (ShopScreen)
		ShopScreen->Hide();
	
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

		if (Id == "btn_change_name")
		{
			SetMainMenuStatus(
				"Change name screen is not connected yet."
			);

			return;
		}

		if (Id == "btn_change_character")
		{
			SetMainMenuStatus(
				"Character selection screen is not connected yet."
			);

			return;
		}

		if (Id == "btn_find_friend")
		{
			SetMainMenuStatus(
				"Friend search screen is not connected yet."
			);

			return;
		}

		if (Id == "btn_rewards")
		{
			SetMainMenuStatus(
				"Awards screen is not connected yet."
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

		if (ProfileResult == 0)
		{
			const int ShopResult =
				gUserProfile.ApiGetShopData();

			r3dOutToLog(
				"[RmlUI][FrontEnd][Profile] "
				"ApiGetShopData result=%d Items=%u\n",
				ShopResult,
				g_NumStoreItems
			);

			Result =
				ShopResult == 0
					? AsyncResult_Success
					: AsyncResult_Error;
		}
		else
		{
			Result =
				AsyncResult_Error;
		}
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

	BuildShopItemList();

	if (ShopVisibleItemIndices.empty())
	{
		SelectedShopStoreIndex = -1;
		SelectedShopBackendItemId = 0;

		SetElementText(
			ShopDocument,
			"selected_item_name",
			"NO ITEMS"
		);

		SetElementText(
			ShopDocument,
			"selected_item_description",
			"No shop items are available for this category."
		);

		SetElementAttribute(
			ShopDocument,
			"selected_item_store_icon",
			"src",
			"Weapons/no_icon.dds"
		);

		SetElementText(
			ShopDocument,
			"selected_item_gc_price",
			"-"
		);

		SetElementText(
			ShopDocument,
			"selected_item_gd_price",
			"-"
		);

		SetShopControlsEnabled(
			false
		);

		SetShopStatus(
			"No shop items available."
		);

		return;
	}

	bool bSelectionVisible = false;

	for (int StoreIndex : ShopVisibleItemIndices)
	{
		if (StoreIndex == SelectedShopStoreIndex)
		{
			bSelectionVisible = true;
			break;
		}
	}

	if (!bSelectionVisible)
	{
		SelectedShopStoreIndex =
			ShopVisibleItemIndices.front();
	}

	SelectedShopItemElementId =
		"shop_item_" +
		std::to_string(
			SelectedShopStoreIndex
		);

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

void RmlFrontEndContext::BuildShopItemList()
{
	ShopVisibleItemIndices.clear();

	if (!ShopDocument)
		return;

	Rml::Element* Container =
		ShopDocument->GetElementById(
			"shop_items_grid"
		);

	if (!Container)
		return;

	for (uint32_t Index = 0; Index < g_NumStoreItems; ++Index)
	{
		const wiStoreItem& StoreItem =
			g_StoreItems[Index];

		if (
			StoreItem.pricePerm == 0 &&
			StoreItem.gd_pricePerm == 0
		)
		{
			continue;
		}

		if (
			StoreItem.itemID >= 301151 &&
			StoreItem.itemID <= 301157
		)
		{
			continue;
		}

		const BaseItemConfig* Config =
			g_pWeaponArmory
				? g_pWeaponArmory->getConfig(
					StoreItem.itemID
				)
				: nullptr;

		if (!Config)
			continue;

		const int RuntimeCategory =
			GetShopRuntimeCategory(
				Config
			);

		if (
			!IsShopRuntimeCategoryAllowed(
				RuntimeCategory
			)
		)
		{
			continue;
		}

		if (
			SelectedShopCategoryId ==
			"shop_category_featured" &&
			!IsShopFeaturedItem(
				StoreItem,
				Config
			)
		)
		{
			continue;
		}

		if (
			!IsShopCategoryMatch(
				SelectedShopCategoryId,
				RuntimeCategory
			)
		)
		{
			continue;
		}

		ShopVisibleItemIndices.push_back(
			static_cast<int>(Index)
		);
	}

	std::sort(
		ShopVisibleItemIndices.begin(),
		ShopVisibleItemIndices.end(),
		[this](
			int LeftIndex,
			int RightIndex
		)
		{
			const wiStoreItem& LeftStore =
				g_StoreItems[LeftIndex];

			const wiStoreItem& RightStore =
				g_StoreItems[RightIndex];

			const BaseItemConfig* LeftConfig =
				g_pWeaponArmory
					? g_pWeaponArmory->getConfig(
						LeftStore.itemID
					)
					: nullptr;

			const BaseItemConfig* RightConfig =
				g_pWeaponArmory
					? g_pWeaponArmory->getConfig(
						RightStore.itemID
					)
					: nullptr;

			const char* LeftName =
				LeftConfig &&
				LeftConfig->m_StoreName
					? LeftConfig->m_StoreName
					: "";

			const char* RightName =
				RightConfig &&
				RightConfig->m_StoreName
					? RightConfig->m_StoreName
					: "";

			if (SelectedShopSortId == "shop_tab_new")
			{
				if (LeftStore.isNew != RightStore.isNew)
					return LeftStore.isNew;
			}
			else if (SelectedShopSortId == "shop_tab_sale")
			{
				const uint32_t LeftPrice =
					GetShopLowestPrice(
						LeftStore
					);

				const uint32_t RightPrice =
					GetShopLowestPrice(
						RightStore
					);

				if (LeftPrice != RightPrice)
					return LeftPrice < RightPrice;
			}
			else if (SelectedShopSortId == "shop_tab_owned")
			{
				const bool bLeftOwned =
					IsShopItemOwned(
						LeftStore.itemID
					);

				const bool bRightOwned =
					IsShopItemOwned(
						RightStore.itemID
					);

				if (bLeftOwned != bRightOwned)
					return bLeftOwned;
			}
			else
			{
				const bool bLeftFeatured =
					IsShopFeaturedItem(
						LeftStore,
						LeftConfig
					);

				const bool bRightFeatured =
					IsShopFeaturedItem(
						RightStore,
						RightConfig
					);

				if (bLeftFeatured != bRightFeatured)
					return bLeftFeatured;
			}

			const int LeftSort =
				LeftConfig
					? LeftConfig->m_StoreSortOrder
					: 0;

			const int RightSort =
				RightConfig
					? RightConfig->m_StoreSortOrder
					: 0;

			if (LeftSort != RightSort)
				return LeftSort < RightSort;

			const int NameCompare =
				_stricmp(
					LeftName,
					RightName
				);

			if (NameCompare != 0)
				return NameCompare < 0;

			return LeftStore.itemID <
				RightStore.itemID;
		}
	);

	Rml::String ItemsMarkup;
	std::vector<unsigned char> GridOccupancy;
	int GridRows = 0;

	auto EnsureRows =
		[&GridOccupancy, &GridRows](
			int RequiredRows
		)
		{
			if (RequiredRows <= GridRows)
				return;

			GridOccupancy.resize(
				static_cast<size_t>(
					RequiredRows *
					ShopGridColumns
				),
				0
			);

			GridRows =
				RequiredRows;
		};

	auto CanPlace =
		[&GridOccupancy, &EnsureRows](
			int Column,
			int Row,
			const FShopGridSize& Size
		)
		{
			if (
				Column < 0 ||
				Column + Size.Width >
					ShopGridColumns
			)
			{
				return false;
			}

			EnsureRows(
				Row + Size.Height
			);

			for (int Y = 0; Y < Size.Height; ++Y)
			{
				for (int X = 0; X < Size.Width; ++X)
				{
					const int Offset =
						(Row + Y) *
						ShopGridColumns +
						Column + X;

					if (
						GridOccupancy[
							static_cast<size_t>(
								Offset
							)
						]
					)
					{
						return false;
					}
				}
			}

			return true;
		};

	auto MarkPlaced =
		[&GridOccupancy](
			int Column,
			int Row,
			const FShopGridSize& Size
		)
		{
			for (int Y = 0; Y < Size.Height; ++Y)
			{
				for (int X = 0; X < Size.Width; ++X)
				{
					const int Offset =
						(Row + Y) *
						ShopGridColumns +
						Column + X;

					GridOccupancy[
						static_cast<size_t>(
							Offset
						)
					] = 1;
				}
			}
		};

	int UsedRows = 0;

	for (int StoreIndex : ShopVisibleItemIndices)
	{
		const uint32_t Index =
			static_cast<uint32_t>(
				StoreIndex
			);

		const wiStoreItem& StoreItem =
			g_StoreItems[Index];

		const BaseItemConfig* Config =
			g_pWeaponArmory
				? g_pWeaponArmory->getConfig(
					StoreItem.itemID
				)
				: nullptr;

		if (!Config)
			continue;

		const int RuntimeCategory =
			GetShopRuntimeCategory(
				Config
			);

		const FShopGridSize GridSize =
			GetShopGridSize(
				Config,
				RuntimeCategory
			);

		int ItemColumn = 0;
		int ItemRow = 0;
		bool bPlaced = false;

		for (
			ItemRow = 0;
			!bPlaced;
			++ItemRow
		)
		{
			for (
				ItemColumn = 0;
				ItemColumn <=
					ShopGridColumns -
					GridSize.Width;
				++ItemColumn
			)
			{
				if (
					CanPlace(
						ItemColumn,
						ItemRow,
						GridSize
					)
				)
				{
					MarkPlaced(
						ItemColumn,
						ItemRow,
						GridSize
					);

					bPlaced = true;
					break;
				}
			}
		}

		--ItemRow;

		UsedRows = std::max(
			UsedRows,
			ItemRow + GridSize.Height
		);

		const bool bSelected =
			static_cast<int>(Index) ==
			SelectedShopStoreIndex;

		const std::string ElementId =
			"shop_item_" +
			std::to_string(Index);

		const char* Name =
			Config->m_StoreName &&
			Config->m_StoreName[0]
				? Config->m_StoreName
				: "UNKNOWN ITEM";

		const char* Category =
			GetShopCategoryLabel(
				RuntimeCategory
			);

		std::string PriceText;

		if (StoreItem.pricePerm > 0)
		{
			PriceText +=
				FormatShopPrice(
					StoreItem.pricePerm
				);
			PriceText += " GC";
		}

		if (StoreItem.gd_pricePerm > 0)
		{
			if (!PriceText.empty())
				PriceText += " / ";

			PriceText +=
				FormatShopPrice(
					StoreItem.gd_pricePerm
				);
			PriceText += " GD";
		}

		const int Quantity =
			GetShopQuantity(
				StoreItem.itemID
			);

		ItemsMarkup += "<button id=\"";
		ItemsMarkup += ElementId;
		ItemsMarkup += "\" class=\"shop_item_card ";
		ItemsMarkup += GetShopItemSizeClass(
			GridSize
		);

		if (bSelected)
			ItemsMarkup += " selected";

		ItemsMarkup += "\" style=\"left: ";
		ItemsMarkup += std::to_string(
			ItemColumn * ShopGridCellSize
		);
		ItemsMarkup += "px; top: ";
		ItemsMarkup += std::to_string(
			ItemRow * ShopGridCellSize
		);
		ItemsMarkup += "px; width: ";
		ItemsMarkup += std::to_string(
			GridSize.Width * ShopGridCellSize
		);
		ItemsMarkup += "px; height: ";
		ItemsMarkup += std::to_string(
			GridSize.Height * ShopGridCellSize
		);
		ItemsMarkup += "px;\">";

		ItemsMarkup += "<div class=\"shop_item_badge\">";
		ItemsMarkup += StoreItem.isNew
			? "NEW"
			: Category;
		ItemsMarkup += "</div>";

		ItemsMarkup += "<img class=\"shop_item_image\" src=\"";
		ItemsMarkup += GetShopIconPath(
			Config
		);
		ItemsMarkup += "\"/>";

		if (Quantity > 1)
		{
			ItemsMarkup += "<div class=\"shop_item_stack\">x";
			ItemsMarkup += std::to_string(
				Quantity
			);
			ItemsMarkup += "</div>";
		}

		ItemsMarkup += "<div class=\"shop_item_name\">";
		ItemsMarkup += EscapeRmlText(
			Name
		);
		ItemsMarkup += "</div>";

		ItemsMarkup += "<div class=\"shop_item_type\">";
		ItemsMarkup += Category;
		ItemsMarkup += "</div>";

		ItemsMarkup += "<div class=\"shop_item_price\">";
		ItemsMarkup += EscapeRmlText(
			PriceText
		);
		ItemsMarkup += "</div>";

		ItemsMarkup += "</button>";
	}

	Rml::String Markup;

	if (ShopVisibleItemIndices.empty())
	{
		Markup =
			"<div class=\"shop_empty_text\">"
			"NO ITEMS IN THIS CATEGORY"
			"</div>";
	}
	else
	{
		const int ContentHeight =
			std::max(
				UsedRows * ShopGridCellSize,
				ShopGridCellSize
			);

		Markup += "<div class=\"shop_grid_spacer\" style=\"width: ";
		Markup += std::to_string(
			ShopGridColumns *
			ShopGridCellSize
		);
		Markup += "px; height: ";
		Markup += std::to_string(
			ContentHeight
		);
		Markup += "px;\"></div>";
		Markup += ItemsMarkup;
	}

	Container->SetInnerRML(
		Markup
	);

	const char* CategoryButtons[] =
	{
		"shop_category_featured",
		"shop_category_weapons",
		"shop_category_body_armor",
		"shop_category_helmets",
		"shop_category_backpacks",
		"shop_category_attachments",
		"shop_category_placeable",
		"shop_category_food",
		"shop_category_medicine",
		"shop_category_usable",
		"shop_category_water"
	};

	for (const char* ElementId : CategoryButtons)
	{
		SetElementClass(
			ShopDocument,
			ElementId,
			"selected",
			SelectedShopCategoryId == ElementId
		);
	}

	const char* SortButtons[] =
	{
		"shop_tab_hot",
		"shop_tab_new",
		"shop_tab_sale",
		"shop_tab_owned"
	};

	for (const char* ElementId : SortButtons)
	{
		SetElementClass(
			ShopDocument,
			ElementId,
			"selected",
			SelectedShopSortId == ElementId
		);
	}
}

void RmlFrontEndContext::SelectShopItem(
	const Rml::String& ShopItemId
)
{
	if (!ShopDocument)
		return;

	if (
		ShopItemId.compare(
			0,
			ShopItemButtonPrefixLength,
			ShopItemButtonPrefix
		) != 0
	)
	{
		return;
	}

	const int StoreIndex =
		atoi(
			ShopItemId.c_str() +
			ShopItemButtonPrefixLength
		);

	if (
		StoreIndex < 0 ||
		static_cast<uint32_t>(StoreIndex) >=
			g_NumStoreItems
	)
	{
		return;
	}

	const wiStoreItem& StoreItem =
		g_StoreItems[StoreIndex];

	const BaseItemConfig* Config =
		g_pWeaponArmory
			? g_pWeaponArmory->getConfig(
				StoreItem.itemID
			)
			: nullptr;

	if (!Config)
		return;

	SelectedShopItemElementId =
		ShopItemId;

	SelectedShopBackendItemId =
		static_cast<int>(
			StoreItem.itemID
		);

	SelectedShopStoreIndex =
		StoreIndex;

	if (
		SelectedShopCurrencyId == "shop_currency_gc" &&
		StoreItem.pricePerm == 0 &&
		StoreItem.gd_pricePerm > 0
	)
	{
		SelectedShopCurrencyId =
			"shop_currency_gd";
	}
	else if (
		SelectedShopCurrencyId == "shop_currency_gd" &&
		StoreItem.gd_pricePerm == 0 &&
		StoreItem.pricePerm > 0
	)
	{
		SelectedShopCurrencyId =
			"shop_currency_gc";
	}

	RefreshShopSelection();

	const char* Name =
		Config->m_StoreName &&
		Config->m_StoreName[0]
			? Config->m_StoreName
			: "UNKNOWN ITEM";

	const char* Description =
		Config->m_Description &&
		Config->m_Description[0]
			? Config->m_Description
			: "No description available.";

	const char* Category =
		GetShopCategoryLabel(
			GetShopRuntimeCategory(
				Config
			)
		);

	char IconText[4]{};
	IconText[0] = Name[0]
		? static_cast<char>(
			toupper(
				static_cast<unsigned char>(
					Name[0]
				)
			)
		)
		: 'I';
	IconText[1] = Name[1]
		? static_cast<char>(
			toupper(
				static_cast<unsigned char>(
					Name[1]
				)
			)
		)
		: 'T';
	IconText[2] = '\0';

	SetElementText(
		ShopDocument,
		"selected_item_icon",
		IconText
	);

	SetElementText(
		ShopDocument,
		"selected_item_badge",
		StoreItem.isNew
			? "NEW ITEM"
			: Category
	);

	SetElementText(
		ShopDocument,
		"selected_item_name",
		Name
	);

	SetElementText(
		ShopDocument,
		"selected_item_preview_name",
		Name
	);

	SetElementText(
		ShopDocument,
		"selected_item_category",
		Category
	);

	SetElementText(
		ShopDocument,
		"selected_item_description",
		Description
	);

	SetElementText(
		ShopDocument,
		"selected_item_gc_price",
		FormatShopPrice(
			StoreItem.pricePerm
		)
	);

	SetElementText(
		ShopDocument,
		"selected_item_gd_price",
		FormatShopPrice(
			StoreItem.gd_pricePerm
		)
	);

	SetElementAttribute(
		ShopDocument,
		"selected_item_store_icon",
		"src",
		GetShopIconPath(
			Config
		)
	);

	SetElementText(
		ShopDocument,
		"selected_shop_item_id",
		ShopItemId
	);

	char Text[64]{};

	sprintf_s(
		Text,
		"%d",
		StoreItem.itemID
	);

	SetElementText(
		ShopDocument,
		"selected_shop_backend_item_id",
		Text
	);

	SetElementText(
		ShopDocument,
		"selected_shop_category_id",
		Category
	);

	const WeaponConfig* Weapon =
		g_pWeaponArmory
			? g_pWeaponArmory->getWeaponConfig(
				StoreItem.itemID
			)
			: nullptr;

	const GearConfig* Gear =
		g_pWeaponArmory
			? g_pWeaponArmory->getGearConfig(
				StoreItem.itemID
			)
			: nullptr;

	if (Weapon)
	{
		sprintf_s(
			Text,
			"%.0f",
			Weapon->m_AmmoDamage
		);
	}
	else if (Gear)
	{
		sprintf_s(
			Text,
			"%.0f%%",
			Gear->m_damagePerc * 100.0f
		);
	}
	else
	{
		sprintf_s(
			Text,
			"%u",
			StoreItem.itemID
		);
	}

	SetElementText(
		ShopDocument,
		"selected_item_damage_value",
		Text
	);

	if (Weapon)
	{
		sprintf_s(
			Text,
			"%.0f",
			Weapon->m_AmmoSpeed
		);
	}
	else if (Gear)
	{
		sprintf_s(
			Text,
			"%.0f",
			Gear->m_damageMax
		);
	}
	else
	{
		sprintf_s(
			Text,
			"%d",
			GetShopQuantity(
				StoreItem.itemID
			)
		);
	}

	SetElementText(
		ShopDocument,
		"selected_item_range_value",
		Text
	);

	if (Weapon)
	{
		sprintf_s(
			Text,
			"%.2f",
			static_cast<double>(
				static_cast<float>(
					Weapon->m_recoil
				)
			)
		);
	}
	else
	{
		sprintf_s(
			Text,
			"%.1f",
			Config->m_Weight
		);
	}

	SetElementText(
		ShopDocument,
		"selected_item_recoil_value",
		Text
	);

	sprintf_s(
		Text,
		"%.1f",
		Config->m_Weight
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

void RmlFrontEndContext::SelectShopCategory(
	const Rml::String& CategoryId
)
{
	SelectedShopCategoryId =
		CategoryId;

	BuildShop();

	SetShopStatus(
		"Shop category selected."
	);
}

void RmlFrontEndContext::SelectShopCurrency(
	const Rml::String& CurrencyId
)
{
	Rml::String ResolvedCurrencyId =
		CurrencyId;

	bool bCanBuyForGc = true;
	bool bCanBuyForGd = true;

	if (
		SelectedShopStoreIndex >= 0 &&
		static_cast<uint32_t>(SelectedShopStoreIndex) <
			g_NumStoreItems
	)
	{
		const wiStoreItem& StoreItem =
			g_StoreItems[
				SelectedShopStoreIndex
			];

		bCanBuyForGc =
			StoreItem.pricePerm > 0;

		bCanBuyForGd =
			StoreItem.gd_pricePerm > 0;

		if (
			ResolvedCurrencyId == "shop_currency_gd" &&
			!bCanBuyForGd &&
			bCanBuyForGc
		)
		{
			ResolvedCurrencyId =
				"shop_currency_gc";
		}
		else if (
			ResolvedCurrencyId == "shop_currency_gc" &&
			!bCanBuyForGc &&
			bCanBuyForGd
		)
		{
			ResolvedCurrencyId =
				"shop_currency_gd";
		}
	}

	SelectedShopCurrencyId =
		ResolvedCurrencyId;

	if (SelectedShopCurrencyId == "shop_currency_gd")
		SelectedShopBuyIndex = 8;
	else
		SelectedShopBuyIndex = 4;

	SetElementEnabled(
		ShopDocument,
		"shop_currency_gc",
		bCanBuyForGc
	);

	SetElementEnabled(
		ShopDocument,
		"shop_currency_gd",
		bCanBuyForGd
	);

	SetElementClass(
		ShopDocument,
		"shop_currency_gc",
		"selected",
		SelectedShopCurrencyId == "shop_currency_gc"
	);

	SetElementClass(
		ShopDocument,
		"shop_currency_gd",
		"selected",
		SelectedShopCurrencyId == "shop_currency_gd"
	);
}

void RmlFrontEndContext::RequestBuySelectedShopItem()
{
	if (
		IsBusy() ||
		SelectedShopBackendItemId <= 0 ||
		SelectedShopStoreIndex < 0 ||
		static_cast<uint32_t>(SelectedShopStoreIndex) >=
			g_NumStoreItems
	)
	{
		return;
	}

	const wiStoreItem& StoreItem =
		g_StoreItems[
			SelectedShopStoreIndex
		];

	if (
		SelectedShopCurrencyId == "shop_currency_gc" &&
		StoreItem.pricePerm <= 0
	)
	{
		SetShopStatus(
			"Selected item cannot be bought for GC."
		);
		return;
	}

	if (
		SelectedShopCurrencyId == "shop_currency_gd" &&
		StoreItem.gd_pricePerm <= 0
	)
	{
		SetShopStatus(
			"Selected item cannot be bought for GD."
		);
		return;
	}

	if (
		SelectedShopCurrencyId == "shop_currency_gc" &&
		gUserProfile.ProfileData.GamePoints <
			static_cast<int>(StoreItem.pricePerm)
	)
	{
		SetShopStatus(
			"Not enough GC."
		);
		return;
	}

	if (
		SelectedShopCurrencyId == "shop_currency_gd" &&
		gUserProfile.ProfileData.GameDollars <
			static_cast<int>(StoreItem.gd_pricePerm)
	)
	{
		SetShopStatus(
			"Not enough GD."
		);
		return;
	}

	__int64 InventoryId = 0;

	const int ApiCode =
		gUserProfile.ApiBuyItem(
			SelectedShopBackendItemId,
			SelectedShopBuyIndex,
			&InventoryId
		);

	if (ApiCode != 0)
	{
		SetShopStatus(
			"Buy item failed."
		);
		return;
	}

	gUserProfile.GetProfile();

	BuildShop();

	SetShopStatus(
		"Item bought."
	);
}

void RmlFrontEndContext::RefreshShopSelection()
{
	if (!ShopDocument)
		return;

	for (int StoreIndex : ShopVisibleItemIndices)
	{
		const std::string ElementId =
			"shop_item_" +
			std::to_string(
				StoreIndex
			);

		SetElementClass(
			ShopDocument,
			ElementId.c_str(),
			"selected",
			StoreIndex ==
				SelectedShopStoreIndex
		);
	}

	SelectShopCurrency(
		SelectedShopCurrencyId
	);
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
