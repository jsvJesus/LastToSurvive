#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndShopSizing.h"

#include <algorithm>

namespace
{
	constexpr int ShopCategoryPlaceable =
		28;

	constexpr int ShopCategoryMedicine =
		31;

	constexpr int ShopCategoryUsable =
		32;

	int ClampInt(
		int Value,
		int MinValue,
		int MaxValue
	)
	{
		return std::max(
			MinValue,
			std::min(
				MaxValue,
				Value
			)
		);
	}
}

namespace RmlFrontEndShopSizing
{
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

		Size.Width =
			ClampInt(
				Size.Width,
				1,
				ShopGridColumns
			);

		Size.Height =
			ClampInt(
				Size.Height,
				1,
				6
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

	int GetShopIconWidthPx(
		const FShopGridSize& Size
	)
	{
		return Size.Width *
			ShopGridCellSize;
	}

	int GetShopIconHeightPx(
		const FShopGridSize& Size
	)
	{
		return Size.Height *
			ShopGridCellSize;
	}
}