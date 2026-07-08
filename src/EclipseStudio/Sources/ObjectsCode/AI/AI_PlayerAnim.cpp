#include "r3dpch.h"
#include "r3d.h"

#include "GameCommon.h"
#include "ObjectsCode/weapons/WeaponArmory.h"
#include "ObjectsCode/weapons/Gear.h"
#include "ObjectsCode/weapons/Weapon.h"

#include "AI_Player.h"
#include "AI_PlayerAnim.h"

//////////////////////////////////////////////////////////////////////////
	r3dAnimPool*	g_CharactersAnimationsPool = NULL;

static bool IsEmptyAnimToken(const char* token)
{
	return token == NULL || token[0] == 0 || stricmp(token, "none") == 0;
}

static const char* AnimToken(const char* token)
{
	return IsEmptyAnimToken(token) ? "" : token;
}

static const char* XmlAnimAttr(pugi::xml_node node, const char* name, const char* fallback)
{
	const char* value = node.attribute(name).value();
	return value && value[0] ? value : fallback;
}

static int ParseCsvLine(char* line, const char** tokens, int maxTokens)
{
	bool inQuotes = false;
	char* out = line;
	char* tokenStart = out;
	int count = 0;

	if(maxTokens > 0)
		tokens[count++] = tokenStart;

	for(char* in = line; *in; ++in)
	{
		char c = *in;
		if(c == '"')
		{
			inQuotes = !inQuotes;
			continue;
		}
		if(c == ',' && !inQuotes)
		{
			*out++ = 0;
			if(count < maxTokens)
				tokens[count++] = out;
			continue;
		}
		if((c == '\r' || c == '\n') && !inQuotes)
			break;

		*out++ = c;
	}

	*out = 0;
	return count;
}
	
CUberData::CUberData()
{
	r3d_assert(g_CharactersAnimationsPool == NULL);
	g_CharactersAnimationsPool = &animPool_;

	LoadSkeleton();
	LoadAnimations();
	LoadWeaponTable();
}

CUberData::~CUberData()
{
	g_CharactersAnimationsPool = NULL;
	SAFE_DELETE(bindSkeleton_);
}

int CUberData::GetMoveDirFromAcceleration(const r3dPoint3D& accel)
{
	// detect direction from acceleration
	int MoveDir = CUberData::ANIMDIR_Stand;

	     if(accel.x > 0 && accel.z > 0) MoveDir = CUberData::ANIMDIR_StrRight;
	else if(accel.x < 0 && accel.z > 0) MoveDir = CUberData::ANIMDIR_StrLeft;
	else if(               accel.z > 0) MoveDir = CUberData::ANIMDIR_Str;
	else if(accel.x > 0 && accel.z < 0) MoveDir = CUberData::ANIMDIR_BackRight;
	else if(accel.x < 0 && accel.z < 0) MoveDir = CUberData::ANIMDIR_BackLeft;
	else if(               accel.z < 0) MoveDir = CUberData::ANIMDIR_Back;
	else if(accel.x > 0               ) MoveDir = CUberData::ANIMDIR_Right;
	else if(accel.x < 0               ) MoveDir = CUberData::ANIMDIR_Left;
	
	return MoveDir;
}

int CUberData::AddAnimation(const char* name, const char* anim_fname)
{
	static const char* animDir = GLOBAL_ANIM_FOLDER;

	char buf[MAX_PATH];
	sprintf(buf, "%s\\%s.anm", animDir, anim_fname ? anim_fname : name);
	int aid = animPool_.Add(name, buf);
	if(aid == -1) 
		r3dError("can't add %s anim", name);
  
	return aid;
}

void CUberData::AddAnimationWithFPS(const char* name, int& aid, int& fps_aid)
{
	static const char* animDir = GLOBAL_ANIM_FOLDER;

	char buf[MAX_PATH];
	sprintf(buf, "%s\\%s.anm", animDir, name);
	aid = animPool_.Add(name, buf);
	if(aid == -1) 
		r3dError("can't add %s anim", name);

	sprintf(buf, "%s\\FPS_%s.anm", animDir, name);
	if(r3dFileExists(buf))
	{
		char fps_name[128];
		sprintf(fps_name, "FPS_%s", name);
		fps_aid = animPool_.Add(fps_name, buf);
	}
	else
	{
		fps_aid = aid;
	}
}

int CUberData::TryToAddAnimation(const char* name, const char* anim_fname)
{
	static const char* animDir = GLOBAL_ANIM_FOLDER;

	char buf[MAX_PATH];
	sprintf(buf, "%s\\%s.anm", animDir, anim_fname ? anim_fname : name);
	if(!r3dFileExists(buf))
		return -1;
	int aid = animPool_.Add(name, buf);
	return aid;
}

// recursively enable all bones in animation
static void enableAnimBone(const r3dSkeleton* skel, int bone, r3dAnimData* ad, int enable)
{
	const char* name = skel->GetBoneName(bone);
	ad->EnableTrack(name, enable);
  
	for(int i=0, e = skel->GetNumBones(); i < e; i++) 
	{
		if(skel->GetParentBoneId(i) == bone)
			enableAnimBone(skel, i, ad, enable);
	}
}

void enableAnimBones(const char* boneName, const r3dSkeleton* skel, r3dAnimData* ad, int enable)
{
	ad->BipedSetEnabled(!enable);
  
	int bone = skel->GetBoneID(boneName);
	r3d_assert(bone != -1);
  
	enableAnimBone(skel, bone, ad, enable);
}

void CUberData::LoadLowerAnimations()
{
	// no loops and prefixes, copy-paste FTW

	int* i;
	i = aid_.crouch;
	i[0] = AddAnimation("idle_crouch_1");
	i[1] = AddAnimation("run_crouch_F");
	i[2] = AddAnimation("run_crouch_FL");
	i[3] = AddAnimation("run_crouch_FR");
	i[4] = AddAnimation("run_crouch_L");
	i[5] = AddAnimation("run_crouch_R");
	i[6] = AddAnimation("run_crouch_B");
	i[7] = AddAnimation("run_crouch_BL");
	i[8] = AddAnimation("run_crouch_BR");

	i = aid_.walk;
	i[0] = AddAnimation("walk_stand_F");
	i[1] = AddAnimation("walk_stand_F");
	i[2] = AddAnimation("walk_stand_FL");
	i[3] = AddAnimation("walk_stand_FR");
	i[4] = AddAnimation("walk_stand_L");
	i[5] = AddAnimation("walk_stand_R");
	i[6] = AddAnimation("walk_stand_B");
	i[7] = AddAnimation("walk_stand_BL");
	i[8] = AddAnimation("walk_stand_BR");

	i = aid_.prone;
	i[0] = AddAnimation("aimIdle_prone_SUP_GLX");
	i[1] = AddAnimation("run_prone_F");
	i[2] = AddAnimation("run_prone_L");
	i[3] = AddAnimation("run_prone_R");
	i[4] = AddAnimation("run_prone_L");
	i[5] = AddAnimation("run_prone_R");
	i[6] = AddAnimation("run_prone_B");
	i[7] = AddAnimation("run_prone_B");
	i[8] = AddAnimation("run_prone_B");

	aid_.prone_up_weapon = AddAnimation("idleProne_to_idleStand_W");
	aid_.prone_down_weapon = AddAnimation("idleStand_to_idleProne_W");
	aid_.prone_up_noweapon = AddAnimation("idleProne_to_idleStand");
	aid_.prone_down_noweapon = AddAnimation("idleStand_to_idleProne");

	i = aid_.run;
	i[1] = AddAnimation("run_stand_F");
	i[2] = AddAnimation("run_stand_FL");
	i[3] = AddAnimation("run_stand_FR");
	i[4] = AddAnimation("run_stand_B");
	i[5] = AddAnimation("run_stand_F");
	i[6] = AddAnimation("run_stand_B");
	i[7] = AddAnimation("run_stand_BL");
	i[8] = AddAnimation("run_stand_BR");

	i = aid_.sprint;
	i[1] = AddAnimation("sprint_stand_F");
	i[2] = AddAnimation("sprint_stand_FL");
	i[3] = AddAnimation("sprint_stand_FR");
	
	i = aid_.turnins;
	i[0] = AddAnimation("walk_stand_BL");
	i[1] = AddAnimation("Crouch_Str");
	i[2] = AddAnimation("walk_stand_F");
}

void CUberData::LoadWeaponAnim(int (&wid)[AIDX_COUNT], int (&wid_fps)[AIDX_COUNT], const char* names[AIDX_COUNT], const char* fpsNames[AIDX_COUNT])
{
	for(int i=0; i<AIDX_COUNT; i++)
	{
		wid[i] = -1;
		wid_fps[i] = -1;
		
		if(IsEmptyAnimToken(names[i]))
			continue;
			
		// we need to create dummy upper body anim for idle and stand
		char aname[128];
		sprintf(aname, "%s", names[i]);
		if(i == AIDX_IdleUpper || i == AIDX_StandUpper)
			strcat(aname, "_Upper");
		wid[i] = TryToAddAnimation(aname, names[i]);
		
		if(i >= AIDX_IdleUpper && wid[i]!=-1)
		{
			// those animations is upper body
			r3dAnimData* ad = animPool_.Get(wid[i]);
			enableAnimBones(blendStartBones_[i].c_str(), bindSkeleton_, ad, true);
		}

		if(fpsNames)
		{
			if(IsEmptyAnimToken(fpsNames[i]))
				wid_fps[i] = wid[i];
			else
				wid_fps[i] = TryToAddAnimation(fpsNames[i], fpsNames[i]);
		}
		else
		{
			// load fps anims
			sprintf(aname, "%s", names[i]); // FPS_%s
			wid_fps[i] = TryToAddAnimation(aname);
		}

		if(wid_fps[i] == -1)
			wid_fps[i] = wid[i];

		if(i >= AIDX_IdleUpper && wid_fps[i]!=-1)
		{
			// those animations is upper body
			r3dAnimData* ad = animPool_.Get(wid_fps[i]);
			if(i == AIDX_ProneBlend) // Patrick's request
				enableAnimBones("Bip01_Spine3", bindSkeleton_, ad, true); // Bip01_Spine2
			else
				enableAnimBones(blendStartBones_[i].c_str(), bindSkeleton_, ad, true);
		}
	}
}

void CUberData::LoadGrenadeAnim()
{
	const static char* grenadeAnims[12] = {
		"Crouch_Pull_EXP_M26",		// 0
		"Crouch_Hold_EXP_M26",		// 1
		"Crouch_Release_EXP_M26",	// 2
		"Run_Blend_Pull_EXP_M26",	// 3
		"Run_Blend_Hold_EXP_M26",	// 4
		"Run_Blend_Release_EXP_M26",	// 5
		"Stand_Pull_EXP_M26",		// 6
		"Stand_Hold_EXP_M26",		// 7
		"Stand_Release_EXP_M26",		// 8
		"Walk_Grenade_Throw_01_A_Pullback",	// 9
		"Walk_Aim_Grenade_Hold_01",		// 10
		"Walk_Grenade_Throw_01_B_Release",	// 11
	};
	
	for(int i=0; i<R3D_ARRAYSIZE(grenadeAnims); i++)
	{
		AddAnimationWithFPS(grenadeAnims[i], aid_.grenades_tps[i], aid_.grenades_fps[i]);
		
		r3dAnimData* ad = animPool_.Get(aid_.grenades_tps[i]);
		enableAnimBones("Bip01_Spine", bindSkeleton_, ad, true);
		
		if(aid_.grenades_tps[i] != aid_.grenades_fps[i])
		{
			ad = animPool_.Get(aid_.grenades_fps[i]);
			enableAnimBones("Bip01_Spine", bindSkeleton_, ad, true);
		}
	}
}

void CUberData::LoadJumpAnim()
{
	aid_.jumps[ 0] = AddAnimation("Jump_ASR_Idle_S");
	aid_.jumps[ 1] = AddAnimation("Jump_Hand_Run_A");
	aid_.jumps[ 2] = AddAnimation("Jump_Hand_Idle_L");
	aid_.jumps[ 3] = AddAnimation("Jump_Hand_Run_S");
	aid_.jumps[ 4] = AddAnimation("Jump_Hand_Run_A");
	aid_.jumps[ 5] = AddAnimation("Jump_Hand_Run_L");
	aid_.jumps[ 6] = AddAnimation("Jump_Hand_Run_S");
	aid_.jumps[ 7] = AddAnimation("Jump_Hand_Run_A");
	aid_.jumps[ 8] = AddAnimation("Jump_Hand_Run_L");
	aid_.jumps[ 9] = AddAnimation("Jump_Hand_Sprint_S");
	aid_.jumps[10] = AddAnimation("Jump_Hand_Sprint_A");
	aid_.jumps[11] = AddAnimation("Jump_Hand_Sprint_L");

	aid_.jumpsASR[ 0] = AddAnimation("Jump_ASR_Idle_S");
	aid_.jumpsASR[ 1] = AddAnimation("Jump_ASR_Idle_A");
	aid_.jumpsASR[ 2] = AddAnimation("Jump_ASR_Idle_L");
	aid_.jumpsASR[ 3] = AddAnimation("Jump_ASR_Run_S");
	aid_.jumpsASR[ 4] = AddAnimation("Jump_ASR_Run_A");
	aid_.jumpsASR[ 5] = AddAnimation("Jump_ASR_Run_L");
	aid_.jumpsASR[ 6] = AddAnimation("Jump_ASR_Run_S");
	aid_.jumpsASR[ 7] = AddAnimation("Jump_ASR_Run_A");
	aid_.jumpsASR[ 8] = AddAnimation("Jump_ASR_Run_L");
	aid_.jumpsASR[ 9] = AddAnimation("Jump_ASR_Sprint_S");
	aid_.jumpsASR[10] = AddAnimation("Jump_ASR_Sprint_A");
	aid_.jumpsASR[11] = AddAnimation("Jump_ASR_Sprint_L");

	aid_.jumpsDeployable[0] = AddAnimation("Jump_Hand_Idle_S");
	aid_.jumpsDeployable[ 1] = AddAnimation("Jump_Deployable_Run_A");
	aid_.jumpsDeployable[ 2] = AddAnimation("Jump_Hand_Idle_L");
	aid_.jumpsDeployable[ 3] = AddAnimation("Jump_Deployable_Run_S");
	aid_.jumpsDeployable[ 4] = AddAnimation("Jump_Deployable_Run_A");
	aid_.jumpsDeployable[ 5] = AddAnimation("Jump_Deployable_Run_L");
	aid_.jumpsDeployable[ 6] = AddAnimation("Jump_Deployable_Run_S");
	aid_.jumpsDeployable[ 7] = AddAnimation("Jump_Deployable_Run_A");
	aid_.jumpsDeployable[ 8] = AddAnimation("Jump_Deployable_Run_L");
	aid_.jumpsDeployable[ 9] = AddAnimation("Jump_Deployable_Sprint_S");
	aid_.jumpsDeployable[10] = AddAnimation("Jump_Deployable_Sprint_A");
	aid_.jumpsDeployable[11] = AddAnimation("Jump_Deployable_Sprint_L");

	aid_.jumpsFallingDown[0] = AddAnimation("Jump_Hand_Run_F");
	aid_.jumpsFallingDown[1] = AddAnimation("Jump_Hand_Sprint_F");
	aid_.jumpsFallingDown[2] = AddAnimation("Jump_Hand_Idle_L");
}

void CUberData::LoadDeathAnim()
{
	aid_.deaths[11] = AddAnimation("Death_02_t1");
}

void CUberData::LoadUpperBlendStartBones()
{
	// init defaults
	for(int i=0; i<AIDX_COUNT; i++) {
		blendStartBones_[i] = "Bip01"; // Bip01_Spine2
	}

	const char* xml_file = PLAYER_UPPER_BLEND_FILE;
	r3dFile* f = r3d_open(xml_file, "rb");
	if(!f) {
		r3dError("Failed to open: %s\n", xml_file);
		return;
	}

	char* fileBuffer = new char[f->size + 1];
	fread(fileBuffer, f->size, 1, f);
	fileBuffer[f->size] = 0;

	pugi::xml_document xmlFile;
	pugi::xml_parse_result parseResult = xmlFile.load_buffer_inplace(fileBuffer, f->size);
	if(!parseResult)
		r3dError("Failed to parse XML %s, error: %s", xml_file, parseResult.description());

	pugi::xml_node xmlBlends = xmlFile.child("UpperBlendStart");
	blendStartBones_[AIDX_IdleUpper]	= XmlAnimAttr(xmlBlends, "AIDX_StandIdle", "Bip01_Spine2");
	blendStartBones_[AIDX_StandUpper]	= XmlAnimAttr(xmlBlends, "AIDX_StandIdleAim", "Bip01_Spine2");
	blendStartBones_[AIDX_CrouchBlend]	= xmlBlends.attribute("AIDX_CrouchBlend").value();
	blendStartBones_[AIDX_CrouchAim]	= xmlBlends.attribute("AIDX_CrouchAim").value();
	blendStartBones_[AIDX_WalkBlend]	= xmlBlends.attribute("AIDX_WalkBlend").value();
	blendStartBones_[AIDX_WalkAim]		= xmlBlends.attribute("AIDX_WalkAim").value();
	blendStartBones_[AIDX_RunBlend]		= xmlBlends.attribute("AIDX_RunBlend").value();
	blendStartBones_[AIDX_SprintBlend]	= xmlBlends.attribute("AIDX_SprintBlend").value();
	blendStartBones_[AIDX_ReloadWalk]	= xmlBlends.attribute("AIDX_ReloadWalk").value();
	blendStartBones_[AIDX_ReloadIdle]	= xmlBlends.attribute("AIDX_ReloadIdle").value();
	blendStartBones_[AIDX_ReloadCrouch]	= xmlBlends.attribute("AIDX_ReloadCrouch").value();
	blendStartBones_[AIDX_ShootWalk]	= xmlBlends.attribute("AIDX_ShootWalk").value();
	blendStartBones_[AIDX_ShootAim]		= xmlBlends.attribute("AIDX_ShootAim").value();
	blendStartBones_[AIDX_ShootCrouch]	= xmlBlends.attribute("AIDX_ShootCrouch").value();
	blendStartBones_[AIDX_ProneBlend]	= xmlBlends.attribute("AIDX_ProneBlend").value();
	blendStartBones_[AIDX_ProneAim]		= xmlBlends.attribute("AIDX_ProneAim").value();
	blendStartBones_[AIDX_ReloadProne]	= xmlBlends.attribute("AIDX_ReloadProne").value();
	blendStartBones_[AIDX_ShootProne]	= xmlBlends.attribute("AIDX_ShootProne").value();
	blendStartBones_[AIDX_IdleProne]	= xmlBlends.attribute("AIDX_ProneIdle").value();

		
	delete[] fileBuffer;
	fclose(f);
}


void CUberData::LoadAnimations()
{
	// reset all animation indices to -1
	memset(&aid_, 0xFF, sizeof(aid_));

	// load config
	LoadUpperBlendStartBones();
	
	// add zero index default anim
	AddAnimation("default", "idle_stand_MEL_Hands");

	LoadLowerAnimations();
	LoadGrenadeAnim();
	LoadJumpAnim();
	LoadDeathAnim();
	
	aid_.UI_IdleNoWeapon = AddAnimation("idle_stand_MEL_Hands");

	aid_.attmMenuRiseWeapon[0] = AddAnimation("FPS_attach_Raise_ASR_ASh12");
	aid_.attmMenuRiseWeapon[1] = AddAnimation("FPS_Attach_Raise_SNP_ARS");
	aid_.attmMenuRiseWeapon[2] = AddAnimation("FPS_Attach_Raise_SHG_SP12");
	aid_.attmMenuRiseWeapon[3] = AddAnimation("FPS_Attach_Raise_LMG");
	aid_.attmMenuRiseWeapon[4] = AddAnimation("FPS_Attach_Idle_HG_Auto");
	aid_.attmMenuRiseWeapon[5] = AddAnimation("FPS_Attach_Raise_SMG_HSE");

	aid_.attmMenuIdleWeapon[0] = AddAnimation("FPS_attach_Idle_ASR_ASh12");
	aid_.attmMenuIdleWeapon[1] = AddAnimation("FPS_Attach_Idle_SNP_ARS");
	aid_.attmMenuIdleWeapon[2] = AddAnimation("FPS_Attach_Idle_SHG_SP12");
	aid_.attmMenuIdleWeapon[3] = AddAnimation("FPS_Attach_Idle_LMG");
	aid_.attmMenuIdleWeapon[4] = AddAnimation("FPS_Attach_Idle_HG_Auto");
	aid_.attmMenuIdleWeapon[5] = AddAnimation("FPS_Attach_Idle_SMG_HSE");

	// add default weapon anim
	const char* names1[] = {
		"idle_stand_1",
		"idle_stand_1",
		"idle_stand_MEL_Hands",
		"idle_stand_MEL_Hands",
		"FPS_Crouch_Blend_MEL_Hands",
		"Crouch_Blend_MEL_Hands",
		"FPS_Attach_Idle_SMG_Bizon",
		"Run_Blend_MEL_Hands",
		"Run_Blend_MEL_Hands",
		"Run_Blend_MEL_Hands",
		"Sprint_Blend_MEL_Hands",
		"reload_standIdle_ASR_AK12",
		"reload_standIdle_ASR_AK12",
		"reload_standIdle_ASR_AK12",
		"shoot_stand_ASR_SHRAM",
		"shoot_stand_ASR_SHRAM",
		"shoot_stand_ASR_SHRAM",
		"FPS_Attach_Idle_SMG_Bizon",
		"FPS_Attach_Idle_SMG_Bizon",
		"reload_standIdle_ASR_AK12",
		"FPS_Attach_Idle_SMG_Bizon",
		"Prone_MEL_Hands",
	};
	
	COMPILE_ASSERT( R3D_ARRAYSIZE(names1) == AIDX_COUNT ) ;

	LoadWeaponAnim(wpn1, wpn1_fps, names1);
}

void CUberData::LoadSkeleton()
{
	const char* skel_fname = PLAYER_BIND_SKELETON_FILE;
	r3dSkeleton* skel = new r3dSkeleton();
	skel->LoadBinary(skel_fname);

	bindSkeleton_ = skel;

	return;
}

static int numWeaponAnimBugs = 0;
static void setWeaponAnimByFNAME(const char* FNAME, int* wid, int* wid_fps)
{
	// mimic store icon name
	char FNAME2[256];
	sprintf(FNAME2, "%s.dds", FNAME);

	// note, there is multiple entries with same FNAME, so set them all
	int numFounds = 0;
	g_pWeaponArmory->startItemSearch();
	while(g_pWeaponArmory->searchNextItem())
	{
		uint32_t itemID = g_pWeaponArmory->getCurrentSearchItemID();
		WeaponConfig* config = (WeaponConfig*)g_pWeaponArmory->getWeaponConfig(itemID);
		if(config)
		{
			const char* name = strrchr(config->m_StoreIcon, '/') + 1;
			if(stricmp(name, FNAME2) != 0)
				continue;

			// fill animation ids for that weapon
			if(config->m_animationIds) {
				r3dError("duplicate weapon anim data for %s\n", FNAME);
			}
			config->m_animationIds = new int[CUberData::AIDX_COUNT];
			memcpy(config->m_animationIds, wid, sizeof(int[CUberData::AIDX_COUNT]));

			if(config->m_animationIds_FPS) {
				r3dError("duplicate FPS weapon anim data for %s\n", FNAME);
			}
			config->m_animationIds_FPS = new int[CUberData::AIDX_COUNT];
			memcpy(config->m_animationIds_FPS, wid_fps, sizeof(int[CUberData::AIDX_COUNT]));

			numFounds++;
		}
	}
	
	if(!numFounds) 
	{
		if(numWeaponAnimBugs == 0)
			r3dPurgeArtBugs();
			
		r3dArtBug("AnimCSV: %s is not in game, remove it\n", FNAME);
		numWeaponAnimBugs++;
	}
}

static void checkIfAllWeaponsHaveAnimation()
{
	g_pWeaponArmory->startItemSearch();
	while(g_pWeaponArmory->searchNextItem())
	{
		uint32_t itemID = g_pWeaponArmory->getCurrentSearchItemID();
		const WeaponConfig* config = g_pWeaponArmory->getWeaponConfig(itemID);
		if(config)
		{
			if(config->category == storecat_UsableItem)
				continue;

			const char* name = strrchr(config->m_StoreIcon, '/') + 1;

			if(!config->m_animationIds)
			{
				numWeaponAnimBugs++;
				r3dArtBug("AnimCSV: weapon %s does not have animation data\n", name);
			}

		}
	}

	if(numWeaponAnimBugs)
		r3dShowArtBugs();
}

static const WeaponConfig* findTPSAnimFallback()
{
	const WeaponConfig* firstWithAnimations = NULL;

	g_pWeaponArmory->startItemSearch();
	while(g_pWeaponArmory->searchNextItem())
	{
		uint32_t itemID = g_pWeaponArmory->getCurrentSearchItemID();
		const WeaponConfig* config = g_pWeaponArmory->getWeaponConfig(itemID);
		if(!config || !config->m_animationIds)
			continue;

		if(config->FNAME && strcmp(config->FNAME, "MEL_UNARMED") == 0)
			return config;

		if(!firstWithAnimations)
			firstWithAnimations = config;
	}

	return firstWithAnimations;
}

static void fixUsableItemTPSAnim()
{
	// ok, here is idea. 
	// to avoid copying animation files for TPS version of usable items, reuse any valid weapon animation set.
	// New item databases may not contain the old hardcoded consumable IDs.
	const WeaponConfig* wpn2 = findTPSAnimFallback();
	if(!wpn2)
	{
		r3dOutToLog("fixTPSUsableItemAnim: no animation fallback, skipped\n");
		return;
	}
	
	g_pWeaponArmory->startItemSearch();
	while(g_pWeaponArmory->searchNextItem())
	{
		uint32_t itemID = g_pWeaponArmory->getCurrentSearchItemID();
		const WeaponConfig* config = g_pWeaponArmory->getWeaponConfig(itemID);
		if(config && config->category != storecat_UsableItem && config->m_animationIds != NULL)
		{
			for(int j=0; j<CUberData::AIDX_COUNT; j++)
			{
				if(config->m_animationIds[j] == -1)
					config->m_animationIds[j] = wpn2->m_animationIds[j];
			}
		}
	}
}

void CUberData::LoadWeaponTable()
{
#ifndef FINAL_BUILD
	r3dOutToLog("LoadWeaponTable\n"); CLOG_INDENT;
#endif
   	float t1 = r3dGetTime();

	const char* alist_file = PLAYER_ANIMATION_LIST_FILE;
	r3dFile* f = r3d_open(alist_file, "rb");
	if(!f)
		r3dError("failed to open %s\n", alist_file);

	// skip first line = there is header descriptions
	char buf[16384] = "";
	fgets(buf, sizeof(buf), f);
	
	while(!feof(f))
	{
		buf[0] = 0;
		if(fgets(buf, sizeof(buf), f) == NULL)
			break;
		int slen = strlen(buf);
		if(slen < 2) 
			continue;
		if(buf[slen-1] == 0xA) { buf[slen-1] = 0; slen--; }
		if(buf[slen-1] == 0xD) { buf[slen-1] = 0; slen--; }
			
		// parse .CSV string
		const int expectedTokens = 86;
		const char* t[96];
		for(int i=0; i<expectedTokens; i++)
		  t[i] = "";
		
		int n = ParseCsvLine(buf, t, R3D_ARRAYSIZE(t));
		
		if(n == 0 || IsEmptyAnimToken(t[0])) {
			// empty line
			continue;
		}
#ifndef FINAL_BUILD		
		if(n != expectedTokens) {
			r3dOutToLog("Bad number of arguments: %d - %s\n", n, t[0]);
		}
		else
		{
			(void)0;
		}
		
		//r3dOutToLog("Loading %s\n", t[0]);
#endif
		
		// create anim list table
		const char* names[] = {
		  AnimToken(t[2]), //AIDX_UIIdle
		  AnimToken(t[2]), //AIDX_IdleLower,
		  AnimToken(t[2]), //AIDX_StandLower,
		  AnimToken(t[2]), //AIDX_IdleUpper,
		  AnimToken(t[8]), //AIDX_StandUpper,
		  AnimToken(t[4]), //AIDX_CrouchBlend,
		  AnimToken(t[12]), //AIDX_CrouchAim,
		  AnimToken(t[4]), //AIDX_WalkBlend,
		  AnimToken(t[8]), //AIDX_WalkAim,
		  AnimToken(t[32]), //AIDX_RunBlend,
		  AnimToken(t[34]), //AIDX_SprintBlend,
		  AnimToken(t[18]), //AIDX_ReloadWalk,
		  AnimToken(t[16]), //AIDX_ReloadIdle,
		  AnimToken(t[20]), //AIDX_ReloadCrouch,
		  AnimToken(t[24]), //AIDX_ShootWalk,
		  AnimToken(t[24]), //AIDX_ShootAim,
		  AnimToken(t[28]), //AIDX_ShootCrouch
		  AnimToken(t[38]), //AIDX_ProneBlend
		  AnimToken(t[6]), //AIDX_ProneAim
		  AnimToken(t[22]), //AIDX_ProneReload
		  AnimToken(t[30]), //AIDX_ProneShoot
		  AnimToken(t[6]), //AIDX_ProneIdle
		};
		const char* fpsNames[] = {
		  AnimToken(t[3]), //AIDX_UIIdle
		  AnimToken(t[3]), //AIDX_IdleLower,
		  AnimToken(t[3]), //AIDX_StandLower,
		  AnimToken(t[3]), //AIDX_IdleUpper,
		  AnimToken(t[3]), //AIDX_StandUpper,
		  AnimToken(t[5]), //AIDX_CrouchBlend,
		  AnimToken(t[3]), //AIDX_CrouchAim,
		  AnimToken(t[5]), //AIDX_WalkBlend,
		  AnimToken(t[3]), //AIDX_WalkAim,
		  AnimToken(t[33]), //AIDX_RunBlend,
		  AnimToken(t[35]), //AIDX_SprintBlend,
		  AnimToken(t[19]), //AIDX_ReloadWalk,
		  AnimToken(t[17]), //AIDX_ReloadIdle,
		  AnimToken(t[21]), //AIDX_ReloadCrouch,
		  AnimToken(t[27]), //AIDX_ShootWalk,
		  AnimToken(t[25]), //AIDX_ShootAim,
		  AnimToken(t[29]), //AIDX_ShootCrouch
		  AnimToken(t[39]), //AIDX_ProneBlend
		  AnimToken(t[15]), //AIDX_ProneAim
		  AnimToken(t[23]), //AIDX_ProneReload
		  AnimToken(t[31]), //AIDX_ProneShoot
		  AnimToken(t[7]), //AIDX_ProneIdle
		};
		COMPILE_ASSERT( R3D_ARRAYSIZE(names) == AIDX_COUNT ) ;
		COMPILE_ASSERT( R3D_ARRAYSIZE(fpsNames) == AIDX_COUNT ) ;
		
		int wid[AIDX_COUNT];
		int wid_fps[AIDX_COUNT];
		LoadWeaponAnim(wid, wid_fps, names, fpsNames);

		setWeaponAnimByFNAME(t[0], wid, wid_fps);
   	}
   	
#ifndef FINAL_BUILD
   	checkIfAllWeaponsHaveAnimation();

	r3dOutToLog("Loaded weapon animations: %f sec, %d anims\n", r3dGetTime() - t1, animPool_.Anims.size());
#endif

	fixUsableItemTPSAnim();
		
	fclose(f);
}

int CUberData::GetGrenadeAnimId(bool IsFPS, int PlayerState, int GrenadeState)
{
	// GrenadeState
	//  0 - pullback
	//  1 - hold
	//  2 - release

	int idx = 6;
	switch(PlayerState) 
	{
		default:
		case PLAYER_IDLE:
		case PLAYER_IDLEAIM:
			idx = 6;
			break;
		case PLAYER_MOVE_CROUCH:
		case PLAYER_MOVE_CROUCH_AIM:
			idx = 0;
			break;
		case PLAYER_MOVE_WALK_AIM:
			idx = 9;
			break;
		case PLAYER_MOVE_RUN:
			idx = 3;
			break;
	}

	if(IsFPS)
		return aid_.grenades_fps[idx + GrenadeState];
	else
		return aid_.grenades_tps[idx + GrenadeState];
}

int CUberData::GetJumpAnimId(int PlayerState, int JumpState, bool FallingDown)
{
	// JumpState
	//  0 - start
	//  1 - air
	//  2 - landing

	if(FallingDown)
		return aid_.jumpsFallingDown[JumpState];

	int idx = 6;
	switch(PlayerState) 
	{
		default:
		case PLAYER_IDLE:
		case PLAYER_IDLEAIM:
		case PLAYER_MOVE_CROUCH:
		case PLAYER_MOVE_CROUCH_AIM:
			idx = 0;
			break;
		case PLAYER_MOVE_WALK_AIM:
			idx = 3;
			break;
		case PLAYER_MOVE_RUN:
			idx = 6;
			break;
		case PLAYER_MOVE_SPRINT:
			idx = 9;
			break;
	}
	
	return aid_.jumps[idx + JumpState];
}

int CUberData::GetJumpAnimIdASR(int PlayerState, int JumpState, bool FallingDown)
{
	// JumpState
	//  0 - start
	//  1 - air
	//  2 - landing

	if(FallingDown)
		return aid_.jumpsFallingDownASR[JumpState];

	int idx = 6;
	switch(PlayerState)
	{
		default:
		case PLAYER_IDLE:
		case PLAYER_IDLEAIM:
		case PLAYER_MOVE_CROUCH:
		case PLAYER_MOVE_CROUCH_AIM:
			idx = 0;
			break;
		case PLAYER_MOVE_WALK_AIM:
			idx = 3;
			break;
		case PLAYER_MOVE_RUN:
			idx = 6;
			break;
		case PLAYER_MOVE_SPRINT:
			idx = 9;
			break;
	}

	return aid_.jumpsASR[idx + JumpState];
}

int CUberData::GetJumpAnimUsableItems(int PlayerState, int JumpState, bool FallingDown)
{
	// JumpState
	//  0 - start
	//  1 - air
	//  2 - landing

	if (FallingDown)
	{
		return aid_.jumpsFallingDownASR[JumpState];
	}

	int idx = 6;
	switch(PlayerState) //AnimMoveDir
	{
	default:
	case PLAYER_IDLE:
	case PLAYER_IDLEAIM:
	case PLAYER_MOVE_CROUCH:
	case PLAYER_MOVE_CROUCH_AIM:
		idx = 0;
		break;
	case PLAYER_MOVE_WALK_AIM:
		idx = 3;
		break;
	case PLAYER_MOVE_RUN:
		idx = 6;
		break;
	case PLAYER_MOVE_SPRINT:
		idx = 9;
		break;
	}
	return aid_.jumpsDeployable[idx + JumpState];
}

////////////////////////////////////////////////////////////////////////////
CUberEquip::CUberEquip(obj_Player* plr) : 
player(plr)
{
}

CUberEquip::~CUberEquip()
{
	for(int i=0; i<SLOT_Max; ++i)
	{
		// do not delete model, gear itself will handle that
		SAFE_DELETE(slots_[i].gear);
		slots_[i].wpn = NULL;
		slots_[i].mesh = NULL;
	}
	player = NULL;
}

void CUberEquip::SetSlot(ESlot slotId, r3dMesh* mesh)
{
	slot_s& sl = slots_[slotId];
	r3d_assert(sl.wpn == NULL);
	r3d_assert(sl.gear == NULL);

	sl.mesh = mesh;
}

void CUberEquip::SetSlot(ESlot slotId, Gear* gear)
{
	slot_s& sl = slots_[slotId];
	r3d_assert(sl.wpn == NULL); // must be a gear slot
	r3d_assert(sl.mesh == NULL);

	SAFE_DELETE(sl.gear); // release old one
	sl.gear = gear;
}

void CUberEquip::SetSlot(ESlot slotId, Weapon* wpn)
{
	slot_s& sl = slots_[slotId];
	r3d_assert(sl.gear == NULL); // must not be a gear slot!
	r3d_assert(sl.mesh == NULL);

	sl.wpn = wpn;
}

r3dMesh* CUberEquip::getSlotMesh(ESlot slotId)
{
	if(slots_[slotId].gear)
		return slots_[slotId].gear->getModel(g_camera_mode->GetInt()==2 && player->NetworkLocal);
	else if(slots_[slotId].wpn)
		return slots_[slotId].wpn->getModel(true, g_camera_mode->GetInt()==2 && player->NetworkLocal);
	else if(slots_[slotId].mesh)
		return slots_[slotId].mesh;

	return NULL;
}

static bool ShouldHideHairSlot(const CUberEquip::slot_s* slots)
{
	for(int i = 0; i < SLOT_Max; ++i)
	{
		if(slots[i].gear && slots[i].gear->getCategory() == storecat_Helmet)
			return true;
	}

	return false;
}

static r3dSkeleton* GetReadyWeaponSkeleton(Weapon* wpn, bool first_person)
{
	if(!wpn)
		return NULL;

	r3dMesh* msh = wpn->getModel(true, first_person);
	if(!msh || !msh->IsSkeletal())
		return NULL;

	if(!first_person)
	{
		wpn->getConfig()->ensureSkeleton();
		return wpn->getConfig()->getSkeleton();
	}

	wpn->checkForSkeleton();
	if(!wpn->getConfig()->getSkeleton() || !wpn->getAnimation())
		return NULL;

	r3dSkeleton* wpnSkel = wpn->getAnimation()->pSkeleton;
	if(!wpnSkel)
		return NULL;

	wpn->getAnimation()->Recalc();
	return wpnSkel;
}

static bool GetWeaponAttachmentWorldTM(const r3dSkeleton* wpnSkeleton, int attachmentIndex, D3DXMATRIX* out, const D3DXMATRIX& weaponWorld)
{
	if(!wpnSkeleton || attachmentIndex < 0 || attachmentIndex >= WPN_ATTM_MAX)
		return false;

	const int boneId = wpnSkeleton->GetBoneID(WeaponAttachmentBoneNames[attachmentIndex]);
	if(boneId == -1)
		return false;

	wpnSkeleton->GetBoneWorldTM(boneId, out, weaponWorld);
	return true;
}

static bool IsNewCharacterSkeleton(const r3dSkeleton* skel)
{
	const char* skelFile = skel ? skel->GetFileName() : NULL;
	return skelFile &&
		(stristr(skelFile, "ProperScale_AndBiped_new") ||
		stristr(skelFile, "CharactersNew"));
}

static void ApplyLegacyWeaponRotation(D3DXMATRIX* world)
{
	D3DXMATRIX mr1;
	D3DXMatrixRotationYawPitchRoll(&mr1, 0, R3D_PI/2, 0);
	*world = mr1 * *world;
}

D3DXMATRIX CUberEquip::getWeaponBone(const r3dSkeleton* skel, const D3DXMATRIX& offset)
{
	if(!skel)
		return offset;

	if(IsNewCharacterSkeleton(skel))
	{
		D3DXMATRIX mr1, world; 
		D3DXMatrixRotationYawPitchRoll(&mr1, 0, R3D_PI / 2, 0);

		D3DXMATRIX RotateMatrix, rt2;
		D3DXMatrixRotationYawPitchRoll(&RotateMatrix, 0, R3D_DEG2RAD(-90), 0);
		D3DXMatrixMultiply(&rt2, &mr1, &RotateMatrix);

		skel->GetBoneWorldTM("PrimaryWeaponBone", &world, offset);
		world = rt2 * world;
		return world;
	}

	struct WeaponBoneCandidate
	{
		const char* name;
		bool useLegacyRotation;
	};

	const WeaponBoneCandidate candidates[] =
	{
		{ "PrimaryWeaponBone", true },
		{ "PrimaryWeapon", false },
		{ "Weapon1", false },
		{ "Bone_Weapon", false },
		{ "Bip01_R_Hand", true },
	};

	D3DXMATRIX world;
	D3DXMatrixIdentity(&world);

	for(int i = 0; i < _countof(candidates); ++i)
	{
		const int boneId = skel->GetBoneID(candidates[i].name);
		if(boneId == -1)
			continue;

		skel->GetBoneWorldTM(boneId, &world, offset);

		if(candidates[i].useLegacyRotation)
		{
			ApplyLegacyWeaponRotation(&world);
		}

		return world;
	}

	return offset;
}

r3dPoint3D CUberEquip::getBonePos(int BoneID, const r3dSkeleton* skel, const D3DXMATRIX& offset)
{
	D3DXMATRIX mr1, world;
	D3DXMatrixRotationYawPitchRoll(&mr1, 0, R3D_PI/2, 0);
	skel->GetBoneWorldTM(BoneID, &world, offset);
	world = mr1 * world;
	return r3dPoint3D(world._41, world._42, world._43);
}

int	CUberEquip::IsLoaded()
{
	bool isFirstPerson = g_camera_mode->GetInt()==2 && player && player->NetworkLocal;
	for( int i = 0 ; i < SLOT_Max; i ++ )
	{
		if( slots_[ i ].gear )
		{
			if(slots_[ i ].gear->getModel(isFirstPerson) && slots_[ i ].gear->getModel(isFirstPerson)->IsDrawable()==false )
				return false ;
		}
		else if(slots_[i].mesh)
			if(!slots_[i].mesh->IsDrawable())
				return false;
	}

	return true ;
}

void CUberEquip::DrawSlot(ESlot slotId, const D3DXMATRIX& world, DrawType dt, bool skin, bool draw_firstperson, const r3dSkeleton* wpnSkeleton)
{
	if(slotId == SLOT_WeaponSide)
		return;
	if(slotId == SLOT_Hair && ShouldHideHairSlot(slots_))
		return;
	if(draw_firstperson)
	{
		if(slotId == SLOT_Armor || slotId == SLOT_LowerBody || slotId == SLOT_Head || slotId == SLOT_Hair || slotId == SLOT_Feet || slotId == SLOT_Helmet || slotId == SLOT_Backpack)
			return;
	}

	bool isFirstPerson = g_camera_mode->GetInt()==2 && player && player->NetworkLocal;
	r3dMesh* mesh = NULL;
	if(slots_[slotId].gear)
		mesh = slots_[slotId].gear->getModel(isFirstPerson&&draw_firstperson);
	else if(slots_[slotId].wpn)
		mesh = slots_[slotId].wpn->getModel(true, isFirstPerson&&draw_firstperson);
	else if(slots_[slotId].mesh)
		mesh = slots_[slotId].mesh;

	if(mesh == NULL)
		return;
	if(!mesh->IsDrawable())
		return;

	DrawSlotMesh(mesh, world, dt, skin);

	if(slots_[slotId].wpn && wpnSkeleton)
	{
		Weapon* wpn = slots_[slotId].wpn;
		for(int i=0; i<WPN_ATTM_MAX; ++i)
		{
			bool HaveClip = wpn->getPlayerItem().Var2 != 0;

			if (i==WPN_ATTM_CLIP && !HaveClip)
				continue;

			mesh = wpn->getWeaponAttachmentMesh((WeaponAttachmentTypeEnum)i, player->m_isAiming && (i==WPN_ATTM_UPPER_RAIL) && g_camera_mode->GetInt()==2 && player->NetworkLocal);
			if(mesh && mesh->IsDrawable())
			{
				//wpn->getPlayerItem()
				D3DXMATRIX attmWorld;
				wpnSkeleton->GetBoneWorldTM(WeaponAttachmentBoneNames[i], &attmWorld, world);
				DrawSlotMesh(mesh, attmWorld, dt, false);
			}
		}
	}
}

void CUberEquip::DrawSlotMesh(r3dMesh* mesh, const D3DXMATRIX& world, DrawType dt, bool skin)
{
	if(skin)
	{
		r3dMeshSetVSConsts_Localized(world, NULL, NULL);
	}
	else
	{
		mesh->SetVSConsts_Localized(world);

		D3DXVECTOR4 scale(mesh->unpackScale.x, mesh->unpackScale.y, mesh->unpackScale.z, 0.f) ;
		D3D_V(r3dRenderer->pd3ddev->SetVertexShaderConstantF(24, (float*)&scale, 1)) ;
	}

	switch(dt)
	{
	case DT_DEFERRED:
		{
			r3dBoundBox worldBBox = mesh->localBBox;
			worldBBox.Transform(reinterpret_cast<const r3dMatrix *>(&world));
			// Vertex lights for forward transparent renderer.
			for (int i = 0; i < mesh->NumMatChunks; i++)
			{
				SetLightsIfTransparent(mesh->MatChunks[i].Mat, worldBBox);
			}

			mesh->DrawMeshDeferred(r3dColor::white, 0);
			break ;
		}

	case DT_DEPTH:
		if(mesh->IsSkeletal())
			r3dRenderer->SetVertexShader(VS_SKIN_DEPTH_ID) ;
		else
			r3dRenderer->SetVertexShader(VS_DEPTH_ID) ;

		// NOTE : no break on purpose

	case DT_AURA:
		mesh->DrawMeshWithoutMaterials();
		break ;

	case DT_SHADOWS:
		mesh->DrawMeshShadows();
		break ;
	}
}

void CUberEquip::AppendSlotMeshRenderables( RenderArray& renderArray, r3dMesh* mesh, const D3DXMATRIX& world, const r3dSkeleton* skeleton )
{
	if( !mesh || !mesh->IsDrawable() )
		return;
}

void CUberEquip::AppendSlotMeshShadowRenderables(
	RenderArray& renderArray,
	r3dMesh* mesh,
	const D3DXMATRIX& world,
	const r3dSkeleton* skeleton
)
{
	if (!mesh || !mesh->IsDrawable())
		return;
}

void CUberEquip::AppendSlotShadowRenderables(
	RenderArray& renderArray,
	ESlot slotId,
	const D3DXMATRIX& world,
	bool skin,
	bool draw_firstperson,
	const r3dSkeleton* slotSkeleton,
	const r3dSkeleton* wpnSkeleton
)
{
	if (slotId == SLOT_WeaponSide)
		return;

	if (slotId == SLOT_Hair && ShouldHideHairSlot(slots_))
		return;

	if (draw_firstperson)
	{
		if (
			slotId == SLOT_Armor ||
			slotId == SLOT_LowerBody ||
			slotId == SLOT_Head ||
			slotId == SLOT_Hair ||
			slotId == SLOT_Feet ||
			slotId == SLOT_Helmet ||
			slotId == SLOT_Backpack
		)
		{
			return;
		}
	}

	const bool isFirstPerson =
		g_camera_mode->GetInt() == 2 &&
		player &&
		player->NetworkLocal;

	r3dMesh* mesh = NULL;

	if (slots_[slotId].gear)
	{
		mesh = slots_[slotId].gear->getModel(isFirstPerson && draw_firstperson);
	}
	else if (slots_[slotId].wpn)
	{
		mesh = slots_[slotId].wpn->getModel(true, isFirstPerson && draw_firstperson);
	}
	else if (slots_[slotId].mesh)
	{
		mesh = slots_[slotId].mesh;
	}

	AppendSlotMeshShadowRenderables(
		renderArray,
		mesh,
		world,
		skin ? slotSkeleton : NULL
	);

	if (slots_[slotId].wpn && wpnSkeleton)
	{
		Weapon* wpn = slots_[slotId].wpn;

		for (int i = 0; i < WPN_ATTM_MAX; ++i)
		{
			const bool useAimAttachment =
				player &&
				player->m_isAiming &&
				i == WPN_ATTM_UPPER_RAIL &&
				g_camera_mode->GetInt() == 2 &&
				player->NetworkLocal;
			mesh = wpn->getWeaponAttachmentMesh(
				(WeaponAttachmentTypeEnum)i,
				useAimAttachment
			);

			if (mesh && mesh->IsDrawable())
			{
				D3DXMATRIX attmWorld;
				if (GetWeaponAttachmentWorldTM(wpnSkeleton, i, &attmWorld, world))
				{
					AppendSlotMeshShadowRenderables(
						renderArray,
						mesh,
						attmWorld,
						NULL
					);
				}
			}
		}
	}
}

void CUberEquip::AppendSlotRenderables( RenderArray& renderArray, ESlot slotId, const D3DXMATRIX& world, bool skin, bool draw_firstperson, const r3dSkeleton* slotSkeleton, const r3dSkeleton* wpnSkeleton )
{
	if(slotId == SLOT_WeaponSide)
		return;
	if(slotId == SLOT_Hair && ShouldHideHairSlot(slots_))
		return;
	if(draw_firstperson)
	{
		if(slotId == SLOT_Armor || slotId == SLOT_LowerBody || slotId == SLOT_Head || slotId == SLOT_Hair || slotId == SLOT_Feet || slotId == SLOT_Helmet || slotId == SLOT_Backpack)
			return;
	}

	bool isFirstPerson = g_camera_mode->GetInt()==2 && player && player->NetworkLocal;
	r3dMesh* mesh = NULL;
	if(slots_[slotId].gear)
		mesh = slots_[slotId].gear->getModel(isFirstPerson&&draw_firstperson);
	else if(slots_[slotId].wpn)
		mesh = slots_[slotId].wpn->getModel(true, isFirstPerson&&draw_firstperson);
	else if(slots_[slotId].mesh)
		mesh = slots_[slotId].mesh;

	AppendSlotMeshRenderables(renderArray, mesh, world, skin ? slotSkeleton : NULL);

	if(slots_[slotId].wpn && wpnSkeleton)
	{
		Weapon* wpn = slots_[slotId].wpn;
		for(int i=0; i<WPN_ATTM_MAX; ++i)
		{
			const bool useAimAttachment =
				player &&
				player->m_isAiming &&
				i == WPN_ATTM_UPPER_RAIL &&
				g_camera_mode->GetInt() == 2 &&
				player->NetworkLocal;
			mesh = wpn->getWeaponAttachmentMesh((WeaponAttachmentTypeEnum)i, useAimAttachment);
			if(mesh && mesh->IsDrawable())
			{
				D3DXMATRIX attmWorld;
				if(GetWeaponAttachmentWorldTM(wpnSkeleton, i, &attmWorld, world))
					AppendSlotMeshRenderables(renderArray, mesh, attmWorld, NULL);
			}
		}
	}
}


void CUberEquip::Draw(const r3dSkeleton* skel, const D3DXMATRIX& CharMat, bool draw_weapon, DrawType dt, bool first_person)
{
	//todo: call extern void r3dMeshSetWorldMatrix(const D3DXMATRIX& world)
	// instead of mesh->SetWorldMatrix

    // in first person mode we need to render player and gun into different Z range
	if(dt == DT_AURA)
	{
		float expandConst[ 4 ] = { r_aura_extrude->GetFloat(), 0.f, 0.f, 0.f } ;
		D3D_V(r3dRenderer->pd3ddev->SetVertexShaderConstantF(23, expandConst, 1)) ;
	}

	skel->SetShaderConstants();

	for(int i=0; i<=SLOT_Backpack; i++)
	{
		DrawSlot((ESlot)i, CharMat, dt, true, first_person, NULL);
	}

	if(dt != DT_AURA)
	{
		D3DXMATRIX world = getWeaponBone(skel, CharMat);
		if(!first_person)
		{
			if(slots_[SLOT_WeaponBackRight].wpn)
			{
				skel->GetBoneWorldTM("Weapon_BackRight", &world, CharMat);
				DrawSlot(SLOT_WeaponBackRight, world, dt, false, first_person, NULL);
			}
			if(slots_[SLOT_Weapon_BackRPG].wpn)
			{
				skel->GetBoneWorldTM("Weapon_BackRPG", &world, CharMat);
				DrawSlot(SLOT_Weapon_BackRPG, world, dt, false, first_person, NULL);
			}
			if(slots_[SLOT_WeaponSide].wpn)
			{
				skel->GetBoneWorldTM("Weapon_Side", &world, CharMat);
				DrawSlot(SLOT_WeaponSide, world, dt, false, first_person, NULL);
			}
		}

		if(draw_weapon)
		{
			world = getWeaponBone(skel, CharMat);
			bool skinned = false;
			r3dSkeleton* wpnSkel = NULL;
			Weapon* wpn = slots_[SLOT_Weapon].wpn;
			if(wpn)
			{
				wpnSkel = GetReadyWeaponSkeleton(wpn, first_person);
				if(wpnSkel)
				{
					wpnSkel->SetShaderConstants();
					skinned = true;
				}
			}
			DrawSlot(SLOT_Weapon, world, dt, skinned, first_person, wpnSkel);
		}
	}
}

void CUberEquip::AppendDeferredRenderables( RenderArray& renderArray, const r3dSkeleton* skel, const D3DXMATRIX& CharMat, bool draw_weapon, bool first_person )
{
	if( !skel )
		return;

	for(int i=0; i<=SLOT_Backpack; i++)
	{
		AppendSlotRenderables(renderArray, (ESlot)i, CharMat, true, first_person, skel, NULL);
	}

	D3DXMATRIX world = getWeaponBone(skel, CharMat);
	if(!first_person)
	{
		if(slots_[SLOT_WeaponBackRight].wpn)
		{
			skel->GetBoneWorldTM("Weapon_BackRight", &world, CharMat);
			AppendSlotRenderables(renderArray, SLOT_WeaponBackRight, world, false, first_person, NULL, NULL);
		}
		if(slots_[SLOT_Weapon_BackRPG].wpn)
		{
			skel->GetBoneWorldTM("Weapon_BackRPG", &world, CharMat);
			AppendSlotRenderables(renderArray, SLOT_Weapon_BackRPG, world, false, first_person, NULL, NULL);
		}
		if(slots_[SLOT_WeaponSide].wpn)
		{
			skel->GetBoneWorldTM("Weapon_Side", &world, CharMat);
			AppendSlotRenderables(renderArray, SLOT_WeaponSide, world, false, first_person, NULL, NULL);
		}
	}

	if(draw_weapon)
	{
		world = getWeaponBone(skel, CharMat);
		bool skinned = false;
		r3dSkeleton* wpnSkel = NULL;
		Weapon* wpn = slots_[SLOT_Weapon].wpn;
		if(wpn)
		{
			wpnSkel = GetReadyWeaponSkeleton(wpn, first_person);
			if(wpnSkel)
			{
				skinned = true;
			}
		}
		AppendSlotRenderables(renderArray, SLOT_Weapon, world, skinned, first_person, wpnSkel, wpnSkel);
	}
}

void CUberEquip::AppendShadowRenderables(
	RenderArray& renderArray,
	const r3dSkeleton* skel,
	const D3DXMATRIX& CharMat,
	bool draw_weapon,
	bool first_person
)
{
	if (!skel)
		return;

	for (int i = 0; i <= SLOT_Backpack; ++i)
	{
		AppendSlotShadowRenderables(
			renderArray,
			(ESlot)i,
			CharMat,
			true,
			first_person,
			skel,
			NULL
		);
	}

	D3DXMATRIX world = getWeaponBone(skel, CharMat);

	if (!first_person)
	{
		if (slots_[SLOT_WeaponBackRight].wpn)
		{
			skel->GetBoneWorldTM("Weapon_BackRight", &world, CharMat);

			AppendSlotShadowRenderables(
				renderArray,
				SLOT_WeaponBackRight,
				world,
				false,
				first_person,
				NULL,
				NULL
			);
		}

		if (slots_[SLOT_Weapon_BackRPG].wpn)
		{
			skel->GetBoneWorldTM("Weapon_BackRPG", &world, CharMat);

			AppendSlotShadowRenderables(
				renderArray,
				SLOT_Weapon_BackRPG,
				world,
				false,
				first_person,
				NULL,
				NULL
			);
		}

		if (slots_[SLOT_WeaponSide].wpn)
		{
			skel->GetBoneWorldTM("Weapon_Side", &world, CharMat);

			AppendSlotShadowRenderables(
				renderArray,
				SLOT_WeaponSide,
				world,
				false,
				first_person,
				NULL,
				NULL
			);
		}
	}

	if (draw_weapon)
	{
		world = getWeaponBone(skel, CharMat);

		bool skinned = false;
		r3dSkeleton* wpnSkel = NULL;

		Weapon* wpn = slots_[SLOT_Weapon].wpn;

		if (wpn)
		{
			wpnSkel = GetReadyWeaponSkeleton(wpn, first_person);
			if (wpnSkel)
			{
				skinned = true;
			}
		}

		AppendSlotShadowRenderables(
			renderArray,
			SLOT_Weapon,
			world,
			skinned,
			first_person,
			wpnSkel,
			wpnSkel
		);
	}
}

CUberAnim::CUberAnim(obj_Player* in_player, CUberData* in_data)
{
	player = in_player;
	data_     = in_data;
	r3d_assert(data_->animPool_.Anims.size() > 0);

	extern void _player_AdjustBoneCallback(uintptr_t dwData, int boneId, D3DXMATRIX &mp, D3DXMATRIX &anim);
	anim.Init(data_->bindSkeleton_, &data_->animPool_, _player_AdjustBoneCallback, reinterpret_cast<uintptr_t>(in_player));

	reloadAnimTrackID	= INVALID_TRACK_ID;
	recoilAnimTrackID	= INVALID_TRACK_ID;
	turnInPlaceTrackID	= INVALID_TRACK_ID;
	grenadePinPullTrackID	= INVALID_TRACK_ID;
	grenadeThrowTrackID	= INVALID_TRACK_ID;
	//bombPlantingTrackID	= INVALID_TRACK_ID;
	shootAnimTrackID = INVALID_TRACK_ID;
	
	jumpAnimSpeed           = 1.9f;
	jumpStartTime           = 0.3f;
	jumpStartTimeByState[0] = 0.3f; // idle
	jumpStartTimeByState[1] = 0.05f; // not idle
	jumpTrackID             = INVALID_TRACK_ID;
	jumpMoveTrackID         = INVALID_TRACK_ID;
	jumpMoveTrackID2		= INVALID_TRACK_ID;
	jumpState               = -1;
	jumpAirTime             = 0;
	//jumpWeInAir             = false;
	//FallingDown             = false;
	
	scaleReloadAnimTime     = 1;

	FillAnimStatesSpeed();

	AnimPlayerState = -1;
	AnimMoveDir     = -1;
	CurrentWeapon       = 0;
	IsInUI          = false;
}

CUberAnim::~CUberAnim()
{
}

bool CUberAnim::IsFPSMode()
{
	if (player)
	{
		return g_camera_mode->GetInt() == 2 && player->NetworkLocal;
	}
	return false;
}

void CUberAnim::FillAnimStatesSpeed()
{
	// set animation speed for all states - so movement speed will be synched with animation speed
	for(int i=0; i<PLAYER_NUM_STATES; i++) {
		AnimSpeedStates[i] = 1.0f;
	};
	AnimSpeedRunFwd = 1.0f; 
	
#if 1 // animation speed overrides
	AnimSpeedStates[PLAYER_MOVE_CROUCH]     = 1.0f;
	AnimSpeedStates[PLAYER_MOVE_CROUCH_AIM] = 1.0f;
	AnimSpeedStates[PLAYER_MOVE_WALK_AIM]   = 1.0f;
	AnimSpeedStates[PLAYER_MOVE_RUN]        = 1.05f; // MUST BE 3.0f for correct feet placement;
	AnimSpeedStates[PLAYER_MOVE_SPRINT]     = 1.1f;
	AnimSpeedRunFwd                         = 1.05f; // MUST be 2.0 for correct feet placement

#endif
}

int CUberAnim::GetBoneID(const char* Name) const
{
	return anim.pSkeleton->GetBoneID(Name);
}

void CUberAnim::SwitchToState(int PlayerState, int MoveDir)
{
	r3d_assert(MoveDir >= 0 && MoveDir < CUberData::ANIMDIR_COUNT);

	const CUberData::animIndices_s& aid = data_->aid_;
	const int* wids = IsFPSMode()?data_->wpn1_fps:data_->wpn1;
	if(CurrentWeapon && CurrentWeapon->getConfig())
	{
		if(IsFPSMode() && CurrentWeapon->getWeaponAnimID_FPS())
			wids = CurrentWeapon->getWeaponAnimID_FPS();
		else if(CurrentWeapon->getConfig()->m_animationIds)
			wids = CurrentWeapon->getConfig()->m_animationIds;
	}
	
	// special case for UI_Idle anims
	if(IsInUI)
	{
		if(CurrentWeapon == NULL)
		{
			int aid;
			
			if(anim.AnimTracks.size() > 0)
				anim.StartAnimation(aid, ANIMFLAG_RemoveOtherFade | ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
			else
				anim.StartAnimation(aid, ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
		}
		else
		{
			if(anim.AnimTracks.size() > 0)
				anim.StartAnimation(wids[CUberData::AIDX_IdleLower], ANIMFLAG_RemoveOtherFade | ANIMFLAG_Looped, 0.0f, 1.0f, 0.2f);
			else
				anim.StartAnimation(wids[CUberData::AIDX_IdleLower], ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
		}
		return;
	}

	// new animation idx
	int a1 = -1;
	int a2 = -1;
	// transition animation idx
	int t1 = -1;
	int t2 = -1;
	// lower/upper current frame numbers if we're switching to same anim
	float f1 = -1;
	float f2 = -1;
	// lower/upper current frame numbers if we're switching to synchronized anim
	float f3 = -1;
	float f4 = -1;

	if(player && player->m_isAiming && player->NetworkLocal && IsFPSMode())
		PlayerState = PLAYER_IDLEAIM;
	
	switch(PlayerState)
	{
		default:
			r3dOutToLog("Unknown PlayerState=%d\n", PlayerState);
			r3d_assert(0);
			break;

		case PLAYER_DIE:
			return;
			
		case PLAYER_IDLE:
			a1 = wids[CUberData::AIDX_IdleLower];
			a2 = wids[CUberData::AIDX_IdleUpper];

			if (!IsFPSMode() && jumpState > -1)
				a2 = -1;

			if(!IsFPSMode() && FallingDown == true)
				a2 = -1;
			break;
		case PLAYER_IDLEAIM:
			a1 = wids[CUberData::AIDX_StandLower];
			a2 = wids[CUberData::AIDX_StandUpper];
			break;
		case PLAYER_MOVE_CROUCH:
			a1 = aid.crouch[MoveDir];
			a2 = wids[CUberData::AIDX_CrouchBlend];
			break;
		case PLAYER_MOVE_CROUCH_AIM:
			a1 = aid.crouch[MoveDir];
			a2 = wids[CUberData::AIDX_CrouchAim];
			break;
		case PLAYER_MOVE_PRONE:
			a1 = aid.prone[MoveDir];
			a2 = wids[CUberData::AIDX_ProneBlend];
			if(!CurrentWeapon)
				a2 = -1;
			break;
		case PLAYER_PRONE_AIM:
			a1 = aid.prone[MoveDir];
			a2 = wids[CUberData::AIDX_ProneAim];
			break;
		case PLAYER_PRONE_UP:
			if(CurrentWeapon)
			{
				a1 = aid.prone_up_weapon;
				a2 = wids[CUberData::AIDX_ProneBlend];
			}
			else
			{
				a1 = aid.prone_up_noweapon;
				a2 = -1;
			}
			break;
		case PLAYER_PRONE_DOWN:
			if(CurrentWeapon)
			{
				a1 = aid.prone_down_weapon;
				a2 = wids[CUberData::AIDX_ProneBlend];
			}
			else
			{
				a1 = aid.prone_down_noweapon;
				a2 = -1;
			}
			break;
		case PLAYER_PRONE_IDLE:
			a1 = aid.prone[MoveDir];
			a2 = wids[CUberData::AIDX_IdleProne];
			break;
		case PLAYER_MOVE_WALK_AIM:
			a1 = aid.walk[MoveDir];
			a2 = wids[CUberData::AIDX_WalkAim];
			//if(!IsFPSMode() && jumpState > -1)
				//a2 = -1;
			break;
		case PLAYER_MOVE_RUN:
			a1 = aid.run[MoveDir];
			a2 = wids[CUberData::AIDX_RunBlend];
			if(!IsFPSMode() && jumpState > -1)
				a2 = -1;
			if(!IsFPSMode() && FallingDown == true)
				a2 = -1;
		
			//ANIM_HACK: because we can move in air, we can switch from to any moving direction in this pose
			if(a1 == -1)
				a1 = aid.run[CUberData::ANIMDIR_Str];
			break;
		case PLAYER_MOVE_SPRINT:
			a1 = aid.sprint[MoveDir];
			a2 = wids[CUberData::AIDX_SprintBlend];
			if(!IsFPSMode() && jumpState > -1)
				a2 = -1;
			if(!IsFPSMode() && FallingDown == true)
				a2 = -1;
		
			//ANIM_HACK: because we can move in air, we can switch from to any moving direction in this pose
			if(a1 == -1)
				a1 = aid.sprint[CUberData::ANIMDIR_Str];
			break;
	}

	//r3d_assert(a1 >= 0);
	//r3d_assert(a2 >= 0);
	if(a1 == -1 && !IsFPSMode()) {	
		r3dOutToLog("no animation for state %d, dir: %d\n", PlayerState, MoveDir);
		a2 = -1;
	}

	// there was no animations, start new
	if(anim.AnimTracks.size() == 0) 
	{
		if(!IsFPSMode()) // do not play lower body anim in FPS mode
			anim.StartAnimation(a1, ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
		anim.StartAnimation(a2, ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
			
		return;
	}

	// set animation speed based on state
	float fAnimSpeed = AnimSpeedStates[PlayerState];
	if(PlayerState == PLAYER_MOVE_RUN && MoveDir == CUberData::ANIMDIR_Str)
		fAnimSpeed = AnimSpeedRunFwd;
	

	float inf1 = 0.0f; // start influence
	float inf2 = 1.0f; // end influence
	float inf3 = 0.1f; // time to blend

	std::vector<r3dAnimation::r3dAnimInfo>::iterator it;
	
	// create two stacks for lower & upper animations
	std::vector<r3dAnimation::r3dAnimInfo> lower;
	std::vector<r3dAnimation::r3dAnimInfo> upper;
	std::vector<r3dAnimation::r3dAnimInfo> top;
	r3dAnimation::r3dAnimInfo              jumpAnim;
	
	for(it=anim.AnimTracks.begin(); it!=anim.AnimTracks.end(); ++it) 
	{
		const r3dAnimation::r3dAnimInfo& ai = *it;
		if(ai.iTrackId == grenadePinPullTrackID || ai.iTrackId == grenadeThrowTrackID)
			top.push_back(ai);
//		else if(ai.iTrackId == bombPlantingTrackID)
//			top.push_back(ai);
		if(ai.iTrackId == reloadAnimTrackID)
			top.push_back(ai);
		else if(ai.iTrackId == shootAnimTrackID)
			top.push_back(ai);
		else if(ai.iTrackId == jumpTrackID)
			jumpAnim = ai;
		else if(ai.pAnim->pTracks[0].bEnabled)
			lower.push_back(ai);
		else
			upper.push_back(ai);
	}
	
	// expire all previous animations but be on lookout for same animation
	for(it = lower.begin(); it!=lower.end(); ++it)
	{
		r3dAnimation::r3dAnimInfo& ai = *it;
		if((ai.dwStatus & ANIMSTATUS_Expiring))
			continue;
		if(
			ai.pAnim->iAnimId == a1 || 
			( (PlayerState == PLAYER_PRONE_UP || PlayerState == PLAYER_PRONE_DOWN) && (ai.pAnim->iAnimId == aid.prone_down_weapon || ai.pAnim->iAnimId == aid.prone_up_weapon || ai.pAnim->iAnimId == aid.prone_down_noweapon || ai.pAnim->iAnimId == aid.prone_up_noweapon) ) 
		  )
		{
			// same lower body anim
			a1 = -1;
			f1 = ai.fCurFrame;

			// update speed for aim/walk states
			ai.fSpeed = fAnimSpeed;
			continue;
		}
		
		// different anim, but they are synchronized, so save the frame
		if( ( ( AnimMoveDir == CUberData::ANIMDIR_Str && ( MoveDir == CUberData::ANIMDIR_StrLeft || MoveDir == CUberData::ANIMDIR_StrRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_StrLeft && ( MoveDir == CUberData::ANIMDIR_Str || MoveDir == CUberData::ANIMDIR_StrRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_StrRight && ( MoveDir == CUberData::ANIMDIR_Str || MoveDir == CUberData::ANIMDIR_StrLeft ) )
			  ) ||
			  ( ( AnimMoveDir == CUberData::ANIMDIR_Back && ( MoveDir == CUberData::ANIMDIR_BackLeft || MoveDir == CUberData::ANIMDIR_BackRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_BackLeft && ( MoveDir == CUberData::ANIMDIR_Back || MoveDir == CUberData::ANIMDIR_BackRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_BackRight && ( MoveDir == CUberData::ANIMDIR_Back || MoveDir == CUberData::ANIMDIR_BackLeft ) )
			  )
			)
		{
			f3 = ai.fCurFrame;
		}
		
		ai.dwStatus    |= ANIMSTATUS_Expiring;
		ai.fExpireTime  = inf3;
	}

	for(it = upper.begin(); it!=upper.end(); ++it)
	{
		r3dAnimation::r3dAnimInfo& ai = *it;
		if((ai.dwStatus & ANIMSTATUS_Expiring))
			continue;
		if(ai.pAnim->iAnimId == a2) 
		{
			// same upper body
			a2 = -1;
			f2 = ai.fCurFrame;

			// update speed for aim/walk states
			ai.fSpeed = fAnimSpeed;
			continue;
		}
		
		if (( ( ( AnimMoveDir == CUberData::ANIMDIR_Str && ( MoveDir == CUberData::ANIMDIR_StrLeft || MoveDir == CUberData::ANIMDIR_StrRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_StrLeft && ( MoveDir == CUberData::ANIMDIR_Str || MoveDir == CUberData::ANIMDIR_StrRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_StrRight && ( MoveDir == CUberData::ANIMDIR_Str || MoveDir == CUberData::ANIMDIR_StrLeft ) )
			  ) ||
			  ( ( AnimMoveDir == CUberData::ANIMDIR_Back && ( MoveDir == CUberData::ANIMDIR_BackLeft || MoveDir == CUberData::ANIMDIR_BackRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_BackLeft && ( MoveDir == CUberData::ANIMDIR_Back || MoveDir == CUberData::ANIMDIR_BackRight ) ) ||
				( AnimMoveDir == CUberData::ANIMDIR_BackRight && ( MoveDir == CUberData::ANIMDIR_Back || MoveDir == CUberData::ANIMDIR_BackLeft ) )
			  ))
			)
		{
			f4 = ai.fCurFrame;
		}
		ai.dwStatus    |= ANIMSTATUS_Expiring;
		ai.fExpireTime  = inf3;
	}
	
	//ANIM_HACK: randomize IDLE animation frame
	float fIdleAnimFrame = 0;
	if(PlayerState == PLAYER_IDLE)
		fIdleAnimFrame = u_GetRandom(0.0f, 999.0f);
		
	// reassemble animation stack and add new ones
	anim.AnimTracks.clear();
		
	//
	// 1. add lower anims
	//
	for(it = lower.begin(); it!=lower.end(); ++it)	
	{
		anim.AnimTracks.push_back(*it);
	}
	
	// start new lower anim
	if(a1 >= 0 && !IsFPSMode()) // do not play lower body anim in FPS mode
	{
		uint32_t animFlag = ANIMFLAG_Looped;
		if(PlayerState==PLAYER_PRONE_DOWN || PlayerState == PLAYER_PRONE_UP)
			animFlag = ANIMFLAG_PauseOnEnd;
		anim.StartAnimation(a1, animFlag, inf1, inf2, inf3);
		r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[anim.AnimTracks.size() - 1];

		// sync changed lower body anim with saved upper body frame
		if(f2 >= 0)
			ai.fCurFrame = f2;

		// sync changed lower body anim with new lower body anim
		if(f3 >= 0)
			ai.fCurFrame = f3;
			
		if(PlayerState == PLAYER_IDLE)
			ai.fCurFrame = fIdleAnimFrame;
			
		ai.fSpeed = fAnimSpeed;
	}

	// if jump present, it must be last of lower anims
	if(jumpAnim.pAnim) 
	{
		anim.AnimTracks.push_back(jumpAnim);
	}

	// Transitions must be last of lower anims.  Since there is no transition to jump atm, jump will not be affected.
	if( t1 != -1 )
	{
		uint32_t animFlag = 0;
		anim.StartAnimation(t1, animFlag, inf1, inf2, inf3);
		r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[anim.AnimTracks.size() - 1];
		ai.fCurFrame = 0;
		ai.fSpeed = fAnimSpeed;
	}

	//
	// 2. add uppers
	//
	for(it = upper.begin(); it!=upper.end(); ++it) 
	{
		anim.AnimTracks.push_back(*it);
	}

	if(a2 >= 0) 
	{
		anim.StartAnimation(a2, ANIMFLAG_Looped, inf1, inf2, inf3);
		r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[anim.AnimTracks.size() - 1];

		// sync changed upper body anim with saved lower body frame
		if(f1 >= 0)
			ai.fCurFrame = f1;

		// sync changed upper body anim with new upper body anim
		if(f4 >= 0)
			ai.fCurFrame = f4;
			
		if(PlayerState == PLAYER_IDLE)
			ai.fCurFrame = fIdleAnimFrame;
			
		ai.fSpeed = fAnimSpeed;
	}
	

	//ANIM_HACK freeze/unfree crouch upper body at frame 0 in crouch standing poses
	// twisted logic here that because of our animation, upper walking crouch anim does not sync with lower body stand crouch
	if(!IsFPSMode()) // do not play lower body crouch in FPS mode
	{
		if(PlayerState == PLAYER_MOVE_CROUCH || PlayerState == PLAYER_MOVE_CROUCH_AIM)
		{
			// need to scan all animations, because switching from crouch_aim move to stand does not restart upper body anim
			for(size_t i = 0, size = anim.AnimTracks.size(); i < size; i++)
			{
				r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[i];
				if(ai.pAnim->iAnimId == wids[CUberData::AIDX_CrouchAim] || ai.pAnim->iAnimId == wids[CUberData::AIDX_CrouchBlend])
				{
					if(MoveDir == CUberData::ANIMDIR_Stand)
					{
						ai.fCurFrame  = 0;
						ai.dwStatus  |= ANIMSTATUS_Paused;
					}
					else
					{
						ai.dwStatus  &= ~ANIMSTATUS_Paused;
					}
				}
			}
		}

		if((PlayerState == PLAYER_MOVE_WALK_AIM || PlayerState == PLAYER_IDLEAIM))
		{
			// need to scan all animations, because switching from crouch_aim move to stand does not restart upper body anim
			for(size_t i = 0, size = anim.AnimTracks.size(); i < size; i++)
			{
				r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[i];
				if(ai.pAnim->iAnimId == wids[CUberData::AIDX_WalkAim] || ai.pAnim->iAnimId == wids[CUberData::AIDX_StandUpper])
				{
					ai.fCurFrame = 1.0f;
					ai.dwStatus  |= ANIMSTATUS_Paused;
				}
			}
		}
	}

	/*if(!IsFPSMode())
	{
		if(PlayerState == PLAYER_MOVE_WALK_AIM || PlayerState == PLAYER_IDLEAIM)
		{
			for(size_t i = 0, size = anim.AnimTracks.size(); i < size; i++)
			{
				r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[i];
				if(ai.pAnim->iAnimId == wids[CUberData::AIDX_WalkAim] || ai.pAnim->iAnimId == wids[CUberData::AIDX_StandUpper])
				{
					ai.fCurFrame = 1.0f;
					ai.dwStatus |= ANIMSTATUS_Paused;
				}
			}
		}
	}*/

	//
	// 3. add all top anims
	//
	for(it = top.begin(); it!=top.end(); ++it) {
		anim.AnimTracks.push_back(*it);
	}
	
	// Transitions must be last of upper anims.  Since there is no transition to jump atm, jump will not be affected.
	if( t2 != -1 )
	{
		uint32_t animFlag = 0;
		anim.StartAnimation(t2, animFlag, inf1, inf2, inf3);
		r3dAnimation::r3dAnimInfo& ai = anim.AnimTracks[anim.AnimTracks.size() - 1];
		ai.fCurFrame = 0;
		ai.fSpeed = fAnimSpeed;
	}
		
	return;
}

static bool isNeedToSkipWeaponSwitch(const Weapon* wpn)
{
	if(!wpn)
		return false;

	int cat = wpn->getCategory();
	return(cat == storecat_UsableItem || cat == storecat_GRENADE);
}

void CUberAnim::SyncAnimation(int PlayerState, int MoveDir, bool force, const Weapon* weap, bool isInAttmMenu)
{
	if(weap)
	{
		// switch state if we firing in idle mode
		if(PlayerState == PLAYER_IDLE && ((r3dGetTime() < weap->getLastTimeFired() + 5.0f) && !IsFPSMode())) // 5sec delay before returning back to idle
			PlayerState = PLAYER_IDLEAIM;

		// switch state if we firing in idle mode
		if(PlayerState == PLAYER_PRONE_IDLE && ((r3dGetTime() < weap->getLastTimeFired() + 5.0f) && !IsFPSMode())) // 5sec delay before returning back to idle
			PlayerState = PLAYER_PRONE_AIM;

// 		// if throwing grenades from idle, switch to idleaim
 		if(PlayerState == PLAYER_IDLE && (grenadePinPullTrackID != INVALID_TRACK_ID))
 			PlayerState = PLAYER_IDLEAIM;
		
		// if weapon was changed, recreate animation indices
		if(CurrentWeapon != weap)
		{
			// disable animation switch to/from grenades because of weird animatino transitions
			if(IsFPSMode() || IsInUI)
			{
				if(isNeedToSkipWeaponSwitch(weap) || isNeedToSkipWeaponSwitch(CurrentWeapon))
					anim.AnimTracks.clear();
			}

			CurrentWeapon = weap;
			force     = true;
		}
	}
	else
	{
		if(IsInUI && (CurrentWeapon != weap || (weap == NULL && anim.AnimTracks.size() == 0)))
		{
			CurrentWeapon = NULL;
			anim.StartAnimation(data_->aid_.UI_IdleNoWeapon, ANIMFLAG_RemoveOtherFade | ANIMFLAG_Looped, 1.0f, 1.0f, 0.0f);
			return;
		}

		CurrentWeapon = NULL;
	}

	if(isInAttmMenu && CurrentWeapon && IsFPSMode())
	{
		bool playingRiseAnim = false;
		bool finishedRiseAnim = true;
		bool playingIdleAnim = false;

		int animIdx = 0;
		switch(CurrentWeapon->getCategory())
		{
		case storecat_ASR:
			animIdx = 0;
			break;
		case storecat_SNP:
			animIdx = 1;
			break;
		case storecat_SHTG:
			animIdx = 2;
			break;
		case storecat_MG:
			animIdx = 3;
			break;
		case storecat_HG:
			animIdx = 4;
			break;
		case storecat_SMG:
			animIdx = 5;
			break;
		default:
			r3d_assert(false);
			break;
		}

		std::vector<r3dAnimation::r3dAnimInfo>::iterator it;
		for(it=anim.AnimTracks.begin(); it!=anim.AnimTracks.end(); ++it) 
		{
			const r3dAnimation::r3dAnimInfo& ai = *it;
			if(ai.pAnim->iAnimId == data_->aid_.attmMenuRiseWeapon[animIdx])
			{
				playingRiseAnim = true;
				if((ai.dwStatus & ANIMSTATUS_Finished))
					finishedRiseAnim = true;
				break;
			}
			else if(ai.pAnim->iAnimId == data_->aid_.attmMenuIdleWeapon[animIdx])
			{
				playingIdleAnim = true;
				break;
			}
		}
		
		if(!playingIdleAnim && !playingRiseAnim) // just entered menu, play rise anim
			anim.StartAnimation(data_->aid_.attmMenuRiseWeapon[animIdx], ANIMFLAG_RemoveOtherFade | ANIMFLAG_PauseOnEnd, 0.0f, 1.0f, 0.1f);
		else if(finishedRiseAnim && !playingIdleAnim)
			anim.StartAnimation(data_->aid_.attmMenuIdleWeapon[animIdx], ANIMFLAG_RemoveOtherNow | ANIMFLAG_Looped, 0.0f, 1.0f, 0.1f);
		else // playing idle anim
		{
			// do nothing :)
		}

		AnimPlayerState = -1; // reset, so that after attm menu we will return back to proper animation
		return;
	}


	// switch anim state
	if(AnimPlayerState == PlayerState && AnimMoveDir == MoveDir && !force)
		return;
	AnimPlayerState = PlayerState;
	AnimMoveDir     = MoveDir;

	SwitchToState(PlayerState, MoveDir);
	//r3dOutToLog("%s anim -> %d, s:%d, a:%d\n", Name.c_str(), aid, PlayerState, anim.AnimTracks.size());
}

void CUberAnim::StartRecoilAnim()
{
	if(IsFPSMode())
	{
		/*
		int animType = m_WeaponArray[m_SelectedWeapon]->getAnimType();
		if(animType == WPN_ANIM_GRENADE)
		return;

		r3dAnimation::r3dAnimInfo* animInfo = uberAnim.GetTrack(animState_.recoilAnimTrackID);
		if(animInfo && (animInfo->GetStatus()&ANIMSTATUS_Playing))
		return; // let it finish before starting a new one

		r3d_assert(m_WeaponArray[m_SelectedWeapon]);

		anim.Stop( recoilAnimTrackID );
		int aid = uchar->GetRecoilAnimId( animType );
		recoilAnimTrackID = anim.StartAnimation(aid, 0, 0.0f, 1.0f, 0.1f);
		*/	
	}
}


void CUberAnim::StartTurnInPlaceAnim()
{
	if(IsFPSMode()) // no need for turn in place in FPS mode
		return;

	anim.Stop(turnInPlaceTrackID);

	int aid = data_->aid_.turnins[0];
	if(AnimPlayerState == PLAYER_MOVE_CROUCH || AnimPlayerState == PLAYER_MOVE_CROUCH_AIM)
		aid = data_->aid_.turnins[1];
		
	turnInPlaceTrackID = anim.StartAnimation(aid, 0, 0.0f, 1.0f, 0.1f);

	// we play turn-in-place anim only at idle/crouch position. so right now we must have at least 3 tracks
	if(anim.AnimTracks.size() >= 3) {
		// move turn-in to 2nd lower body anim place
		r3dAnimation::r3dAnimInfo ai = anim.AnimTracks.back(); 
		anim.AnimTracks.pop_back();
		anim.AnimTracks.insert(anim.AnimTracks.begin() + 1, ai);
	}
}


void CUberAnim::StopTurnInPlaceAnim()
{
	if(IsFPSMode()) // no need for turn in place in FPS mode
		return;

	if(turnInPlaceTrackID != INVALID_TRACK_ID)
	{
		anim.FadeOut(turnInPlaceTrackID, 0.1f);
		turnInPlaceTrackID = INVALID_TRACK_ID;
	}
}

void CUberAnim::UpdateTurnInPlaceAnim()
{
	if(IsFPSMode()) // no need for turn in place in FPS mode
		return;

	if(anim.GetTrack(turnInPlaceTrackID) == NULL)
	{
		turnInPlaceTrackID = CUberAnim::INVALID_TRACK_ID;
	}
}

float CUberAnim::GetGrenadeLaunchFrame()
{
	if(!CurrentWeapon)
		return 0.0f;
		
	const static float GrenadeLaunchFrames[][2] = 
	{
		 {4, 2}, //PLAYER_IDLE,
		 {4, 2}, //PLAYER_IDLEAIM,
		 {4, 2}, //PLAYER_MOVE_CROUCH,
		 {4, 2}, //PLAYER_MOVE_CROUCH_AIM,
		 {4, 2}, //PLAYER_MOVE_WALK_AIM,
		 {4, 2}, //PLAYER_MOVE_RUN,
		 {4, 2}, //PLAYER_MOVE_SPRINT,
		 {0, 0}, //PLAYER_MOVE_PRONE,
		 {0, 0}, //PLAYER_PRONE_AIM,
		 {0, 0}, //PLAYER_PRONE_UP,
		 {0, 0}, //PLAYER_PRONE_DOWN,
		 {0, 0}, //PLAYER_PRONE_IDLE,
		 {0, 0}, //PLAYER_DIE,
	};
	COMPILE_ASSERT( R3D_ARRAYSIZE(GrenadeLaunchFrames) == PLAYER_NUM_STATES ) ;

//	int type = CurrentWeapon->getConfig()->getGrenadeAnimType();
	int fps  = IsFPSMode() ? 1 : 0;
	
	// grenade
//	if(type == WeaponConfig::GRENADE_ANIM_Normal)
		return GrenadeLaunchFrames[AnimPlayerState][fps];
	
	// mines
// 	const static float MineLaunchFrames[WeaponConfig::GRENADE_ANIM_LASTTYPE][2] = {
// 	  {0,  0}, //GRENADE_ANIM_Normal,
// 	  {37, 37}, //GRENADE_ANIM_Claymore
// 	  {37, 37}, //GRENADE_ANIM_VS50
// 	  {37, 37}, //GRENADE_ANIM_V69
// 	};
// 	r3d_assert(type < R3D_ARRAYSIZE(MineLaunchFrames));
// 	return MineLaunchFrames[type][fps];
}

void CUberAnim::StartGrenadePinPullAnimation()
{
	// skip pinpull anim for mines
// 	if(CurrentWeapon && CurrentWeapon->getCategory() == storecat_GRENADES)
// 	{
// 		if(CurrentWeapon->getConfig()->getGrenadeAnimType() != WeaponConfig::GRENADE_ANIM_Normal)
// 		{
// 			grenadePinPullTrackID = 0xF000000;	// make some temporary track id
// 			return;
// 		}
// 	}

	int grIdx = data_->GetGrenadeAnimId(IsFPSMode(), AnimPlayerState, 0);
	
	anim.Stop(grenadePinPullTrackID);
	grenadePinPullTrackID = anim.StartAnimation(grIdx, ANIMFLAG_PauseOnEnd, 0.0f, 1.0f, 0.1f);
}

bool CUberAnim::IsGrenadePinPullActive()
{
	return grenadePinPullTrackID != INVALID_TRACK_ID ;
}

bool CUberAnim::IsGrenadePinPullFinished()
{
	r3dAnimation::r3dAnimInfo* ai = anim.GetTrack(grenadePinPullTrackID);
	if(!ai)
		return true;
	return (ai->GetStatus() & ANIMSTATUS_Finished) ? true : false;
}

void CUberAnim::StartGrenadeThrowAnimation()
{
	StopGrenadeAnimations();

	// note, no fading in, start with full influence
	int grIdx = data_->GetGrenadeAnimId(IsFPSMode(), AnimPlayerState, 2);
	grenadeThrowTrackID = anim.StartAnimation(grIdx, 0, 1.0f, 1.0f, 0.0f);
}

void CUberAnim::StopGrenadeAnimations()
{
	if(grenadePinPullTrackID != INVALID_TRACK_ID)
	{
		anim.Stop(grenadePinPullTrackID);
		grenadePinPullTrackID = INVALID_TRACK_ID;
	}

	if(grenadeThrowTrackID != INVALID_TRACK_ID)
	{
		anim.Stop(grenadeThrowTrackID);
		grenadeThrowTrackID = INVALID_TRACK_ID;
	}
}

bool CUberAnim::IsGrenadeLaunched()
{
	if(CurrentWeapon && grenadeThrowTrackID != INVALID_TRACK_ID)
	{
		r3dAnimation::r3dAnimInfo* ai = anim.GetTrack(grenadeThrowTrackID);
		if(ai && (int)ai->fCurFrame > GetGrenadeLaunchFrame())
			return true;
	}
		
	return false;
}

int CUberAnim::GetGrenadeAnimState()
{
	const r3dAnimation::r3dAnimInfo* ai1 = anim.GetTrack(grenadePinPullTrackID);
	if(ai1)
		return 1;

	const r3dAnimation::r3dAnimInfo* ai2 = anim.GetTrack(grenadeThrowTrackID);
	if(ai2)
		return 2;
		
	return 0;
}

void CUberAnim::StartReloadAnim()
{
	// check if we already have reloading animation
	r3dAnimation::r3dAnimInfo* animInfo = anim.GetTrack(reloadAnimTrackID);
	if(animInfo && (animInfo->GetStatus()&ANIMSTATUS_Playing)) {
		return;
	}

	if(!CurrentWeapon) {
		return;
	}
		
	// no reload for grenades or mines.
	if(CurrentWeapon->getConfig()->m_AnimType == WPN_ANIM_GRENADE || CurrentWeapon->getConfig()->m_AnimType == WPN_ANIM_MINE ) {
		return;
	}

	const int* wids = IsFPSMode()?data_->wpn1_fps:data_->wpn1;
	if(CurrentWeapon)
	{
		if(IsFPSMode() && CurrentWeapon->getWeaponAnimID_FPS())
			wids = CurrentWeapon->getWeaponAnimID_FPS();
		else if(CurrentWeapon->getConfig()->m_animationIds)
			wids = CurrentWeapon->getConfig()->m_animationIds;
	}
	
	// we have different upper reload blends for different states
	int reloadIdx;
	switch(AnimPlayerState)
	{
		default:
			reloadIdx = CUberData::AIDX_ReloadWalk;
			break;
		case PLAYER_IDLE:
			reloadIdx = CUberData::AIDX_ReloadIdle;
			break;
		case PLAYER_MOVE_CROUCH:
		case PLAYER_MOVE_CROUCH_AIM:
			reloadIdx = CUberData::AIDX_ReloadCrouch;
			break;
		case PLAYER_MOVE_PRONE:
		case PLAYER_PRONE_AIM:
		case PLAYER_PRONE_IDLE:
			reloadIdx = CUberData::AIDX_ReloadProne;
			break;
	}

	anim.Stop(reloadAnimTrackID);
	reloadAnimTrackID = anim.StartAnimation(wids[reloadIdx], 0, 0.0f, 1.0f, 0.1f);

	// scale reload anim to match weapon reload time
	r3dAnimation::r3dAnimInfo* ai = anim.GetTrack(reloadAnimTrackID);
	if(scaleReloadAnimTime && ai && CurrentWeapon->getReloadTime() > 0)
	{
		float animTime = (float)ai->pAnim->NumFrames / ai->pAnim->fFrameRate;
		float k = animTime / CurrentWeapon->getReloadTime();
		ai->SetSpeed(k);
	}
}

void CUberAnim::StopReloadAnim()
{
	if(reloadAnimTrackID != INVALID_TRACK_ID)
	{
		anim.Stop(reloadAnimTrackID);
		reloadAnimTrackID = INVALID_TRACK_ID;
	}
}

void CUberAnim::StartShootAnim()
{
	if(!CurrentWeapon)
		return;

	if(CurrentWeapon->getCategory() == storecat_GRENADE)
		return;

	// play shoot anim only in FPS mode
	if(!IsFPSMode() && CurrentWeapon->getCategory() != storecat_MELEE) // for melee we need to play fire anim all the time. Not sure why in TPS we don't want to play fire anim
		return;
		
	const int* wids = IsFPSMode()?data_->wpn1_fps:data_->wpn1;
	if(CurrentWeapon)
	{
		if(IsFPSMode() && CurrentWeapon->getWeaponAnimID_FPS())
			wids = CurrentWeapon->getWeaponAnimID_FPS();
		else if(CurrentWeapon->getConfig()->m_animationIds)
			wids = CurrentWeapon->getConfig()->m_animationIds;
	}

	int shootAnimIdx;
	switch(AnimPlayerState)
	{
	default:
		shootAnimIdx = CUberData::AIDX_ShootWalk;
		break;
	case PLAYER_MOVE_CROUCH_AIM:
	case PLAYER_IDLEAIM:
	case PLAYER_MOVE_WALK_AIM:
	case PLAYER_PRONE_AIM:
		shootAnimIdx = CUberData::AIDX_ShootAim;
		break;
	case PLAYER_MOVE_CROUCH:
		shootAnimIdx = CUberData::AIDX_ShootCrouch;
		break;
	case PLAYER_MOVE_PRONE:
	case PLAYER_PRONE_IDLE:
		shootAnimIdx = CUberData::AIDX_ShootProne;
		break;
	}

	// check if we already have this shoot animation
	r3dAnimation::r3dAnimInfo* animInfo = anim.GetTrack(shootAnimTrackID);
	if(animInfo && animInfo->GetAnim())
	{
		if(animInfo->GetAnim()->iAnimId == wids[shootAnimIdx])
		{
			updateShootAnim(false);
			return;
		}
	}
	
	anim.Stop(shootAnimTrackID);
	shootAnimTrackID = anim.StartAnimation(wids[shootAnimIdx], ANIMFLAG_PauseOnEnd, 0.0f, 1.0f, 0.1f);
}

void CUberAnim::updateShootAnim(bool disable_loop)
{
	r3dAnimation::r3dAnimInfo* animInfo = anim.GetTrack(shootAnimTrackID);
	if(animInfo)
	{
		DWORD status = animInfo->GetStatus();
		if(status&ANIMSTATUS_Finished) 
		{
			if(!disable_loop)
			{
				DWORD status = animInfo->GetStatus();
				status &= ~ANIMSTATUS_Finished;
				animInfo->SetStatus(status);
				animInfo->fCurFrame = 0.0f;
				animInfo->fInfluence = 1.0f;
			}
			else
			{
				animInfo->fInfluence = 0.0f;
			}
		}
	}
}

void CUberAnim::StopShootAnim()
{
	if(shootAnimTrackID != INVALID_TRACK_ID)
	{
		anim.Stop(shootAnimTrackID);
		shootAnimTrackID = INVALID_TRACK_ID;
	}
}

void CUberAnim::StartDeathAnim()
{
	StopReloadAnim();
	StopGrenadeAnimations();
	
	anim.StartAnimation(data_->aid_.deaths[11], ANIMFLAG_PauseOnEnd | ANIMFLAG_RemoveOtherFade, 0.0f, 1.0f, 0.1f);
}

void CUberAnim::StartJump()
{
	if(IsFPSMode()) // no need for jump in FPS mode
	{
		jumpStartTime = 0.0f;
		return;
	}

	if(AnimPlayerState == PLAYER_IDLE || AnimPlayerState == PLAYER_IDLEAIM)
		jumpStartTime = jumpStartTimeByState[0];
	else
		jumpStartTime = jumpStartTimeByState[1];

	int idx = data_->GetJumpAnimId(AnimPlayerState, 0);

	if (CurrentWeapon && CurrentWeapon->getCategory() >=storecat_ASR && CurrentWeapon->getCategory()<=storecat_SMG  && CurrentWeapon->getCategory() != storecat_HG)
		idx = data_->GetJumpAnimIdASR(AnimPlayerState, 0);

	if (CurrentWeapon && CurrentWeapon->getCategory() == storecat_UsableItem)
		idx = data_->GetJumpAnimUsableItems(AnimPlayerState, 0);

	r3dAnimData* ad = data_->animPool_.Get(idx);
	jumpTrackID = anim.StartAnimation(idx, ANIMFLAG_PauseOnEnd, 0.0f, 1.0f, 0.1f);
	jumpState   = 0;
	jumpWeInAir = true;
	FallingDown = false;
	jumpPlayerState = AnimPlayerState;

	// resync animation, so jump track will be relocated to top of lower bodys anim
	SwitchToState(AnimPlayerState, AnimMoveDir);
}

void CUberAnim::UpdateJump(bool bOnGround, float fHeightAboveGround)
{
	if(IsFPSMode()) // no need for jump in FPS mode
		return;

	// check if we're in air
	if(!jumpWeInAir && !bOnGround)
		jumpWeInAir = true;

	//bool FallingDown = false;
	if(!jumpWeInAir && !bOnGround && jumpState == -1 && fHeightAboveGround>4.5f)
	{
		if(AnimPlayerState == PLAYER_IDLE || AnimPlayerState == PLAYER_IDLEAIM)
			jumpStartTime = jumpStartTimeByState[0];
		else
			jumpStartTime = jumpStartTimeByState[1];

		int idx = data_->GetJumpAnimId(jumpPlayerState, 0,true);

		if (CurrentWeapon && CurrentWeapon->getCategory() >=storecat_ASR && CurrentWeapon->getCategory()<=storecat_SMG  && CurrentWeapon->getCategory() != storecat_HG)
			idx = data_->GetJumpAnimIdASR(AnimPlayerState, 0,true);

		if (CurrentWeapon && CurrentWeapon->getCategory() == storecat_UsableItem)
			idx = data_->GetJumpAnimUsableItems(AnimPlayerState, 0,true);

		r3dAnimData* ad = data_->animPool_.Get(idx);
		jumpTrackID = anim.StartAnimation(idx, ANIMFLAG_PauseOnEnd, 0.0f, 1.0f, 0.1f);
		jumpState   = 0;
		FallingDown = true; // false
		jumpPlayerState = AnimPlayerState;
		SwitchToState(AnimPlayerState, AnimMoveDir);
	}
	
	// update air time - used in free fall detection
	if(!bOnGround) {
		jumpAirTime += r3dGetFrameTime();
	} else {
		jumpAirTime = 0;
	}

	// started to jump
	if(jumpState == 0) 
	{
		const r3dAnimation::r3dAnimInfo* ai = anim.GetTrack(jumpTrackID);
		if(!ai || (ai->dwStatus & ANIMSTATUS_Finished)) 
		{
			// switch to AIR
			if (!jumpWeInAir)
				FallingDown = true;

			int idx = data_->GetJumpAnimId(jumpPlayerState, 1,FallingDown);
			if (CurrentWeapon && CurrentWeapon->getCategory() >=storecat_ASR && CurrentWeapon->getCategory()<=storecat_SMG  && CurrentWeapon->getCategory() != storecat_HG)
				idx = data_->GetJumpAnimIdASR(AnimPlayerState, 1,FallingDown);

			if (CurrentWeapon && CurrentWeapon->getCategory() == storecat_UsableItem)
				idx = data_->GetJumpAnimUsableItems(AnimPlayerState, 1,FallingDown);

			anim.Stop(jumpTrackID);
			jumpTrackID = anim.StartAnimation(idx, ANIMFLAG_PauseOnEnd, 0.0f, 1.0f, 0.1f);
			jumpState   = 1;
			if (!FallingDown)
				jumpWeInAir = true; // after initial jump, set us to in-air no matter what, in case if for some reason we jumped in such a way that we need left the ground, we would be stuck in first jump animation forever
			
			SwitchToState(AnimPlayerState, AnimMoveDir);
		}
		return;
	}
	
	// in air
	if(jumpState == 1)
	{
		// switch to landing only when we actually started to jump
		if(jumpWeInAir && bOnGround || FallingDown && bOnGround)
		{
			int idx = data_->GetJumpAnimId(jumpPlayerState, 2,FallingDown);
			if (CurrentWeapon && CurrentWeapon->getCategory() >=storecat_ASR && CurrentWeapon->getCategory()<=storecat_SMG  && CurrentWeapon->getCategory() != storecat_HG)
				idx = data_->GetJumpAnimIdASR(AnimPlayerState, 2,FallingDown);

			if (CurrentWeapon && CurrentWeapon->getCategory() == storecat_UsableItem)
				idx = data_->GetJumpAnimUsableItems(AnimPlayerState, 2,FallingDown);

			anim.Stop(jumpTrackID);
			jumpTrackID = anim.StartAnimation(idx, ANIMFLAG_PauseOnEnd, 1.0f, 1.0f, 0.0f);
			jumpState   = 2;

			// resync animation, so jump track will be relocated to top of lower bodys anim
			SwitchToState(AnimPlayerState, AnimMoveDir);

			if(AnimPlayerState >= PLAYER_MOVE_CROUCH && AnimPlayerState <= PLAYER_MOVE_SPRINT)
			{
				//ANIM_HACK: animation before jump should be lower body walking anim
				// so set it to frame 1 and pause (because end of landing anim will be start of walk)
				for(size_t i=1; i<anim.AnimTracks.size(); i++) {
					if(anim.AnimTracks[i].iTrackId == jumpTrackID) {
						// lower
						anim.AnimTracks[i-1].fCurFrame = 1.0f;
						anim.AnimTracks[i-1].dwStatus |= ANIMSTATUS_Paused;
						jumpMoveTrackID = anim.AnimTracks[i-1].iTrackId;
						// upper
						if (i+1<anim.AnimTracks.size())
						{
							anim.AnimTracks[i+1].fCurFrame = 1.0f;
							anim.AnimTracks[i+1].dwStatus |= ANIMSTATUS_Paused;
							jumpMoveTrackID2 = anim.AnimTracks[i+1].iTrackId;
						}
					}
				}
			}
		}
		return;
	}
	
	// landing
	if(jumpState == 2)
	{
		const r3dAnimation::r3dAnimInfo* ai = anim.GetTrack(jumpTrackID);

		// if player state was changed in landing, abort it
		if(AnimPlayerState != jumpPlayerState)
		{
			ai = NULL;
		}

		if(!ai || (ai->dwStatus & ANIMSTATUS_Finished)) 
		{
			anim.FadeOut(jumpTrackID, 0.1f);
			jumpTrackID = INVALID_TRACK_ID;
			jumpState   = -1;
			jumpWeInAir = false;
			FallingDown = false;

			// resume walking animation
			r3dAnimation::r3dAnimInfo* ai2 = anim.GetTrack(jumpMoveTrackID);
			if(ai2) {
				ai2->dwStatus &= ~ANIMSTATUS_Paused;
				jumpMoveTrackID = INVALID_TRACK_ID;
			}
			r3dAnimation::r3dAnimInfo* ai3 = anim.GetTrack(jumpMoveTrackID2);
			if(ai3) {
				ai3->dwStatus &= ~ANIMSTATUS_Paused;
				jumpMoveTrackID2 = INVALID_TRACK_ID;
			}
		}
		
		return;
	}
}

