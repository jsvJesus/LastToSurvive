#include "RmlFrontEndShop.h"

#include <cstddef>

namespace
{
	constexpr char ShopItemButtonPrefix[] =
		"shop_item_";

	constexpr std::size_t ShopItemButtonPrefixLength =
		sizeof(ShopItemButtonPrefix) - 1;
}

RmlFrontEndShop::RmlFrontEndShop()
{
}

RmlFrontEndShop::~RmlFrontEndShop()
{
}

bool RmlFrontEndShop::Load(
	Rml::Context* InContext
)
{
	return RmlFrontEndScreen::Load(
		InContext,
		
		"Rml/FrontEnd/Shop.rml"
	);
}

void RmlFrontEndShop::SetCallbacks(
	const FRmlFrontEndShopCallbacks& InCallbacks
)
{
	Callbacks =
		InCallbacks;
}

void RmlFrontEndShop::Refresh()
{
	if (Callbacks.BuildShop)
	{
		Callbacks.BuildShop();
	}
}

bool RmlFrontEndShop::HandleClickId(
	const Rml::String& Id
)
{
	if (Id == "nav_survivor" ||
		Id == "btn_back_to_main" ||
		Id == "btn_shop_back")
	{
		if (Callbacks.ShowMainMenu)
			Callbacks.ShowMainMenu();

		return true;
	}

	if (Id == "nav_skills")
	{
		if (Callbacks.ShowSkills)
			Callbacks.ShowSkills();

		return true;
	}

	if (Id == "nav_shop")
	{
		return true;
	}

	if (Id == "btn_shop_buy_selected")
	{
		if (Callbacks.RequestBuySelectedShopItem)
			Callbacks.RequestBuySelectedShopItem();

		return true;
	}

	if (Id == "btn_shop_preview_item")
	{
		if (Callbacks.SetShopStatus)
		{
			Callbacks.SetShopStatus(
				"Item preview is not connected yet."
			);
		}

		return true;
	}

	if (Id == "btn_shop_refresh")
	{
		if (Callbacks.RefreshShopFromBackend)
			Callbacks.RefreshShopFromBackend();

		return true;
	}

	if (Id == "btn_shop_sort")
	{
		if (Callbacks.SelectShopSort)
			Callbacks.SelectShopSort("cycle");

		return true;
	}

	if (IsShopItemId(Id))
	{
		if (Callbacks.SelectShopItem)
			Callbacks.SelectShopItem(Id);

		return true;
	}

	if (IsShopCategoryId(Id))
	{
		if (Callbacks.SelectShopCategory)
			Callbacks.SelectShopCategory(Id);

		return true;
	}

	if (IsShopCurrencyId(Id))
	{
		if (Callbacks.SelectShopCurrency)
			Callbacks.SelectShopCurrency(Id);

		return true;
	}

	if (IsShopSortId(Id))
	{
		if (Callbacks.SelectShopSort)
			Callbacks.SelectShopSort(Id);

		return true;
	}

	return false;
}

bool RmlFrontEndShop::IsShopItemId(
	const Rml::String& Id
) const
{
	return Id.compare(
		0,
		ShopItemButtonPrefixLength,
		ShopItemButtonPrefix
	) == 0;
}

bool RmlFrontEndShop::IsShopCategoryId(
	const Rml::String& Id
) const
{
	return
		Id == "shop_category_featured" ||
		Id == "shop_category_weapons" ||
		Id == "shop_category_body_armor" ||
		Id == "shop_category_helmets" ||
		Id == "shop_category_backpacks" ||
		Id == "shop_category_attachments" ||
		Id == "shop_category_placeable" ||
		Id == "shop_category_food" ||
		Id == "shop_category_medicine" ||
		Id == "shop_category_usable" ||
		Id == "shop_category_water";
}

bool RmlFrontEndShop::IsShopCurrencyId(
	const Rml::String& Id
) const
{
	return
		Id == "shop_currency_gc" ||
		Id == "shop_currency_gd";
}

bool RmlFrontEndShop::IsShopSortId(
	const Rml::String& Id
) const
{
	return
		Id == "shop_tab_hot" ||
		Id == "shop_tab_new" ||
		Id == "shop_tab_sale" ||
		Id == "shop_tab_owned";
}
