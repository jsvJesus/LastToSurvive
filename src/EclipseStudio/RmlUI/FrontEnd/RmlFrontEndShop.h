#pragma once

#include "RmlFrontEndScreen.h"

#include <functional>

struct FRmlFrontEndShopCallbacks
{
    std::function<void()> ShowMainMenu;
    std::function<void()> BuildShop;
    std::function<void()> RequestBuySelectedShopItem;
    std::function<void()> RefreshShopFromBackend;

    std::function<void(const Rml::String&)> SelectShopItem;
    std::function<void(const Rml::String&)> SelectShopCategory;
    std::function<void(const Rml::String&)> SelectShopCurrency;
    std::function<void(const Rml::String&)> SelectShopSort;
    std::function<void(const Rml::String&)> SetShopStatus;
};

class RmlFrontEndShop final :
    public RmlFrontEndScreen
{
public:
    RmlFrontEndShop();
    ~RmlFrontEndShop() override;

    bool Load(
        Rml::Context* InContext
    );

    void SetCallbacks(
        const FRmlFrontEndShopCallbacks& InCallbacks
    );

    void Refresh();

protected:
    bool HandleClickId(
        const Rml::String& Id
    ) override;

private:
    bool IsShopItemId(
        const Rml::String& Id
    ) const;

    bool IsShopCategoryId(
        const Rml::String& Id
    ) const;

    bool IsShopCurrencyId(
        const Rml::String& Id
    ) const;

    bool IsShopSortId(
        const Rml::String& Id
    ) const;

private:
    FRmlFrontEndShopCallbacks Callbacks;
};