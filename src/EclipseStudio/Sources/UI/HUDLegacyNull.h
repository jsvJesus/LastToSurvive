#pragma once

#include "r3d.h"
#include "HUDLegacyTypes.h"

class HUDDisplay
{
public:
	HUDDisplay() {}
	~HUDDisplay() {}

	bool Init() { return true; }
	bool Unload() { return true; }

	int Update() { return 0; }
	int Draw() { return 0; }

	void setBloodAlpha(float alpha) { (void)alpha; }

	void setVisibility(float percent) { (void)percent; }
	void setHearing(float percent) { (void)percent; }

	void setLifeParams(int food, int water, int health, int toxicity, int stamina)
	{
		(void)food;
		(void)water;
		(void)health;
		(void)toxicity;
		(void)stamina;
	}

	void setWeaponInfo(int ammo, int clips, int firemode)
	{
		(void)ammo;
		(void)clips;
		(void)firemode;
	}

	void showWeaponInfo(int state)
	{
		(void)state;
	}

	void setSlotInfo(int slotID, const char* name, int quantity, const char* icon)
	{
		(void)slotID;
		(void)name;
		(void)quantity;
		(void)icon;
	}

	void updateSlotInfo(int slotID, int quantity)
	{
		(void)slotID;
		(void)quantity;
	}

	void showSlots(bool state)
	{
		(void)state;
	}

	void setActiveSlot(int slotID)
	{
		(void)slotID;
	}

	void setActivatedSlot(int slotID)
	{
		(void)slotID;
	}

	void showMessage(const wchar_t* text)
	{
		(void)text;
	}

	void showChat(bool showChat, bool force = false)
	{
		(void)showChat;
		(void)force;
	}

	void showChatInput()
	{
	}

	void addChatMessage(int tabIndex, const char* user, const char* text, uint32_t flags)
	{
		(void)tabIndex;
		(void)user;
		(void)text;
		(void)flags;
	}

	bool isChatInputActive() const
	{
		return false;
	}

	bool isChatVisible() const
	{
		return false;
	}

	void setChatTransparency(float alpha)
	{
		(void)alpha;
	}

	void setChatChannel(int index)
	{
		(void)index;
	}

	void enableClanChannel()
	{
	}

	void clearPlayersList()
	{
	}

	void addPlayerToList(
		int num,
		const char* name,
		int reputation,
		bool isLegend,
		bool isDev,
		bool isPunisher,
		bool isInvitePending
	)
	{
		(void)num;
		(void)name;
		(void)reputation;
		(void)isLegend;
		(void)isDev;
		(void)isPunisher;
		(void)isInvitePending;
	}

	void showPlayersList(int flag)
	{
		(void)flag;
	}

	int isPlayersListVisible() const
	{
		return 0;
	}

	bool canShowWriteNote() const
	{
		return false;
	}

	void showWriteNote(int slotIDFrom)
	{
		(void)slotIDFrom;
	}

	void showReadNote(const char* msg)
	{
		(void)msg;
	}

	void showRangeFinderUI(bool set)
	{
		(void)set;
	}

	void showYouAreDead(const char* killedBy)
	{
		(void)killedBy;
	}

	void showSafeZoneWarning(bool flag)
	{
		(void)flag;
	}

	void addCharTag(const char* name, int reputation, bool isSameClan, HUDNullValue& result)
	{
		(void)name;
		(void)reputation;
		(void)isSameClan;
		result.SetUndefined();
	}

	void moveUserIcon(
		HUDNullValue& icon,
		const r3dPoint3D& pos,
		bool alwaysShow,
		bool force_invisible = false,
		bool pos_in_screen_space = false
	)
	{
		(void)icon;
		(void)pos;
		(void)alwaysShow;
		(void)force_invisible;
		(void)pos_in_screen_space;
	}

	void setCharTagTextVisible(HUDNullValue& icon, bool isShowName, bool isSameGroup)
	{
		(void)icon;
		(void)isShowName;
		(void)isSameGroup;
	}

	void removeUserIcon(HUDNullValue& icon)
	{
		icon.SetUndefined();
	}
};

class HUDPause
{
public:
	HUDPause()
		: isActive_(false)
		, isInit_(false)
	{
	}

	~HUDPause()
	{
	}

	bool Init()
	{
		isInit_ = true;
		return true;
	}

	bool Unload()
	{
		isInit_ = false;
		isActive_ = false;
		return true;
	}

	bool IsInited() const
	{
		return isInit_;
	}

	void Update()
	{
	}

	void Draw()
	{
	}

	bool isActive() const
	{
		return isActive_;
	}

	void Activate()
	{
		isActive_ = true;
	}

	void Deactivate()
	{
		isActive_ = false;
	}

	void showInventory()
	{
	}

	void showMap()
	{
	}

	void setTime(__int64 utcTime)
	{
		(void)utcTime;
	}

	void reloadBackpackInfo()
	{
	}

	void updateSurvivorTotalWeight()
	{
	}

private:
	bool isActive_;
	bool isInit_;
};

class HUDAttachments
{
public:
	HUDAttachments()
		: isActive_(false)
		, isInit_(false)
	{
	}

	~HUDAttachments()
	{
	}

	bool Init()
	{
		isInit_ = true;
		return true;
	}

	bool Unload()
	{
		isInit_ = false;
		isActive_ = false;
		return true;
	}

	bool IsInited() const
	{
		return isInit_;
	}

	void Update()
	{
	}

	void Draw()
	{
	}

	bool isActive() const
	{
		return isActive_;
	}

	void Activate()
	{
		isActive_ = true;
	}

	void Deactivate()
	{
		isActive_ = false;
	}

private:
	bool isActive_;
	bool isInit_;
};

class HUDActionUI
{
public:
	HUDActionUI()
		: isActive_(false)
		, isInit_(false)
	{
	}

	~HUDActionUI()
	{
	}

	bool Init()
	{
		isInit_ = true;
		return true;
	}

	bool Unload()
	{
		isInit_ = false;
		isActive_ = false;
		return true;
	}

	bool IsInited() const
	{
		return isInit_;
	}

	void Update()
	{
	}

	void Draw()
	{
	}

	void setScreenPos(int x, int y)
	{
		(void)x;
		(void)y;
	}

	bool isActive() const
	{
		return isActive_;
	}

	void Activate()
	{
		isActive_ = true;
	}

	void Deactivate()
	{
		isActive_ = false;
	}

	void setText(const wchar_t* title, const wchar_t* msg, const char* letter)
	{
		(void)title;
		(void)msg;
		(void)letter;
	}

	void enableRegularBlock()
	{
	}

	void enableProgressBlock()
	{
	}

	void setProgress(int value)
	{
		(void)value;
	}

private:
	bool isActive_;
	bool isInit_;
};

class HUDGeneralStore
{
public:
	HUDGeneralStore()
		: isActive_(false)
		, isInit_(false)
	{
	}

	~HUDGeneralStore()
	{
	}

	bool Init()
	{
		isInit_ = true;
		return true;
	}

	bool Unload()
	{
		isInit_ = false;
		isActive_ = false;
		return true;
	}

	bool IsInited() const
	{
		return isInit_;
	}

	void Update()
	{
	}

	void Draw()
	{
	}

	bool isActive() const
	{
		return isActive_;
	}

	void Activate()
	{
		isActive_ = true;
	}

	void Deactivate()
	{
		isActive_ = false;
	}

private:
	bool isActive_;
	bool isInit_;
};

class HUDVault
{
public:
	HUDVault()
		: isActive_(false)
		, isInit_(false)
	{
	}

	~HUDVault()
	{
	}

	bool Init()
	{
		isInit_ = true;
		return true;
	}

	bool Unload()
	{
		isInit_ = false;
		isActive_ = false;
		return true;
	}

	bool IsInited() const
	{
		return isInit_;
	}

	void Update()
	{
	}

	void Draw()
	{
	}

	bool isActive() const
	{
		return isActive_;
	}

	void Activate()
	{
		isActive_ = true;
	}

	void Deactivate()
	{
		isActive_ = false;
	}

private:
	bool isActive_;
	bool isInit_;
};