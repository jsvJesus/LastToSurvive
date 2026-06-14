#pragma once

#include "UI\hud_base.h"
#include "GameObjects\GameObj.h"
#include "GameObjects\ObjManag.h"

typedef std::vector<std::string> stringlist_t;

class CharacterHUD : public BaseHUD
{
public:
	r3dVector FPS_Acceleration;
	r3dVector FPS_vViewOrig;
	r3dVector FPS_ViewAngle;
	r3dVector FPS_vVision;
	r3dVector FPS_vRight;
	r3dVector FPS_vUp;
	r3dVector FPS_vForw;

	r3dVector cameraPosition;

	float currentDist;

	class obj_Player* m_Player;

public:
	CharacterHUD();
	~CharacterHUD()
	{
	}

	virtual void SetCameraDir(
		r3dPoint3D vPos
	);

	virtual r3dPoint3D GetCameraDir() const;

	virtual void Process();
	virtual void Draw();

	void DrawAllAnims(float& Y);
	void DrawPlayerStates(float& Y);

	virtual void OnProcess();

	void ProcessPick(
		bool bSimple = false
	);

	void HandleCharacterRmlAction(
		const char* Action,
		const char* Value
	);

protected:
	virtual void InitPure();
	virtual void DestroyPure();

	virtual void SetCameraPure(
		r3dCamera& Cam
	);

	virtual void OnHudUnselected();

private:
	void CreateCharacter();
	void DrawLegacyUI();

	r3dPoint2D DrawCurrentAnimInfo(
		float BaseY
	);

	void DrawSkeleton(
		r3dSkeleton& Skeleton
	);

	void StartDefaultAnim();

	void InitCharacterRmlUI();
	void ShutdownCharacterRmlUI();

	void InitializeCharacterControls();
	void UpdateCharacterControls();
	void UpdateCharacterRmlDocument();

	void StartCharacterInAir();

private:
	bool paused;
	bool blendLooped;

	float curTime;
	float srcTime;
	float dstTime;

	bool bCharacterRmlReady;
	bool bCharacterRmlInitAttempted;
	bool bCharacterControlsInitialized;
	void EnsureDefaultCharacterLoadout();

	bool bPlayerStatesMode;

	bool bShowSkeleton;
	bool bShowAnimStack;
	bool bShowEquipment;
	bool bUiIdleMode;

	int SelectedPlayerState;
	int SelectedMoveDirection;
	int SelectedAnimation;

	int CachedAnimationCount;
	int CachedSelectedAnimation;

	int SelectedEquipmentCategory;
	int SelectedEquipmentItem;

	int CachedEquipmentCategory;
	int CachedEquipmentItem;

	std::vector<uint32_t> CharacterEquipmentValues;
	std::vector<std::string> CharacterEquipmentNames;

	void RebuildCharacterEquipmentList();
	void ApplyCharacterEquipmentItem(int ListIndex);

	int FindCurrentCharacterEquipmentIndex() const;
};