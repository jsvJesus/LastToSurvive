#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndShopIconBaker.h"
#include "RmlFrontEndShopSizing.h"

#include "GameCode/UserProfile.h"
#include "ObjectsCode/weapons/WeaponArmory.h"

#include <windows.h>
#include <cstdio>
#include <cstring>

namespace
{
	const char* GetSafeString(
		const char* Text
	)
	{
		return Text
			? Text
			: "";
	}

	void WriteJsonString(
		FILE* File,
		const char* Text
	)
	{
		fputc(
			'"',
			File
		);

		if (Text)
		{
			for (
				const unsigned char* Cursor =
					reinterpret_cast<const unsigned char*>(
						Text
					);
				*Cursor;
				++Cursor
			)
			{
				const unsigned char Character =
					*Cursor;

				switch (Character)
				{
					case '\\':
						fputs(
							"\\\\",
							File
						);
						break;

					case '"':
						fputs(
							"\\\"",
							File
						);
						break;

					case '\n':
						fputs(
							"\\n",
							File
						);
						break;

					case '\r':
						fputs(
							"\\r",
							File
						);
						break;

					case '\t':
						fputs(
							"\\t",
							File
						);
						break;

					default:
						fputc(
							Character,
							File
						);
						break;
				}
			}
		}

		fputc(
			'"',
			File
		);
	}

	const ModelItemConfig* GetModelConfigForItem(
		uint32_t ItemId,
		const BaseItemConfig* Config
	)
	{
		if (
			!g_pWeaponArmory ||
			!Config
		)
		{
			return NULL;
		}

		const WeaponConfig* Weapon =
			g_pWeaponArmory->getWeaponConfig(
				ItemId
			);

		if (Weapon)
			return Weapon;

		const WeaponAttachmentConfig* Attachment =
			g_pWeaponArmory->getAttachmentConfig(
				ItemId
			);

		if (Attachment)
			return Attachment;

		const GearConfig* Gear =
			g_pWeaponArmory->getGearConfig(
				ItemId
			);

		if (Gear)
			return Gear;

		const BackpackConfig* Backpack =
			g_pWeaponArmory->getBackpackConfig(
				ItemId
			);

		if (Backpack)
			return Backpack;

		const FoodConfig* Food =
			g_pWeaponArmory->getFoodConfig(
				ItemId
			);

		if (Food)
			return Food;

		if (
			Config->category != storecat_LootBox
		)
		{
			const ModelItemConfig* Item =
				g_pWeaponArmory->getItemConfig(
					ItemId
				);

			if (Item)
				return Item;
		}

		return NULL;
	}

	int GetBakeRuntimeCategory(
		const BaseItemConfig* Config
	)
	{
		if (!Config)
			return storecat_INVALID;

		if (
			Config->m_StoreCategoryOverride > 0
		)
		{
			return Config->m_StoreCategoryOverride;
		}

		return static_cast<int>(
			Config->category
		);
	}
}

bool RmlFrontEndShopIconBaker::WriteBakeList(
	const char* OutputFileName
)
{
	if (
		!OutputFileName ||
		!OutputFileName[0]
	)
	{
		r3dOutToLog(
			"[ShopIconBaker] Empty output filename.\n"
		);

		return false;
	}

	if (!g_pWeaponArmory)
	{
		r3dOutToLog(
			"[ShopIconBaker] Weapon armory is not initialized.\n"
		);

		return false;
	}

	CreateDirectoryA(
		"Data\\Weapons\\GeneratedShopIcons",
		NULL
	);

	FILE* File = NULL;

	fopen_s(
		&File,
		OutputFileName,
		"wb"
	);

	if (!File)
	{
		r3dOutToLog(
			"[ShopIconBaker] Failed to open output file: %s\n",
			OutputFileName
		);

		return false;
	}

	fprintf(
		File,
		"{\n"
	);

	fprintf(
		File,
		"  \"cellSize\": %d,\n",
		RmlFrontEndShopSizing::ShopGridCellSize
	);

	fprintf(
		File,
		"  \"columns\": %d,\n",
		RmlFrontEndShopSizing::ShopGridColumns
	);

	fprintf(
		File,
		"  \"items\": [\n"
	);

	int WrittenCount =
		0;

	g_pWeaponArmory->startItemSearch();

	while (
		g_pWeaponArmory->searchNextItem()
	)
	{
		const uint32_t ItemId =
			g_pWeaponArmory->getCurrentSearchItemID();

		const BaseItemConfig* Config =
			g_pWeaponArmory->getConfig(
				ItemId
			);

		if (!Config)
			continue;

		const int RuntimeCategory =
			GetBakeRuntimeCategory(
				Config
			);

		const RmlFrontEndShopSizing::FShopGridSize GridSize =
			RmlFrontEndShopSizing::GetShopGridSize(
				Config,
				RuntimeCategory
			);

		const int IconWidth =
			RmlFrontEndShopSizing::GetShopIconWidthPx(
				GridSize
			);

		const int IconHeight =
			RmlFrontEndShopSizing::GetShopIconHeightPx(
				GridSize
			);

		const ModelItemConfig* ModelConfig =
			GetModelConfigForItem(
				ItemId,
				Config
			);

		if (WrittenCount > 0)
		{
			fprintf(
				File,
				",\n"
			);
		}

		fprintf(
			File,
			"    {\n"
		);

		fprintf(
			File,
			"      \"itemID\": %u,\n",
			ItemId
		);

		fprintf(
			File,
			"      \"category\": %d,\n",
			RuntimeCategory
		);

		fprintf(
			File,
			"      \"gridW\": %d,\n",
			GridSize.Width
		);

		fprintf(
			File,
			"      \"gridH\": %d,\n",
			GridSize.Height
		);

		fprintf(
			File,
			"      \"width\": %d,\n",
			IconWidth
		);

		fprintf(
			File,
			"      \"height\": %d,\n",
			IconHeight
		);

		fprintf(
			File,
			"      \"name\": "
		);

		WriteJsonString(
			File,
			GetSafeString(
				Config->m_StoreName
			)
		);

		fprintf(
			File,
			",\n"
		);

		fprintf(
			File,
			"      \"storeIcon\": "
		);

		WriteJsonString(
			File,
			GetSafeString(
				Config->m_StoreIcon
			)
		);

		fprintf(
			File,
			",\n"
		);

		fprintf(
			File,
			"      \"model\": "
		);

		WriteJsonString(
			File,
			ModelConfig
				? GetSafeString(
					ModelConfig->m_ModelPath
				)
				: ""
		);

		fprintf(
			File,
			",\n"
		);

		fprintf(
			File,
			"      \"output\": "
		);

		char OutputIcon[256]{};

		sprintf_s(
			OutputIcon,
			sizeof(OutputIcon),
			"Data/Weapons/GeneratedShopIcons/%u.png",
			ItemId
		);

		WriteJsonString(
			File,
			OutputIcon
		);

		fprintf(
			File,
			"\n"
		);

		fprintf(
			File,
			"    }"
		);

		++WrittenCount;
	}

	fprintf(
		File,
		"\n  ]\n"
	);

	fprintf(
		File,
		"}\n"
	);

	fclose(
		File
	);

	r3dOutToLog(
		"[ShopIconBaker] Bake list written: %s Items=%d\n",
		OutputFileName,
		WrittenCount
	);

	return true;
}