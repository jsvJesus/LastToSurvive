#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "UI/UIimEdit2.h"

#include "m_Main.h"

#include "GameLevel.h"
#include "../DiscordPresence.h"
#include "../../RmlUI/RmlUISystem.h"

#include "TrueNature2/Terrain2.h"

Menu_Main::Menu_Main()
{
}

Menu_Main::~Menu_Main()
{
}

void Menu_Main::Draw()
{
  
  return;
}


				  static int __CreateTerrain = 0;
				  static int __CreateTerrain2 = 0;
				  static int __CreateMesh = 0;
				  static int __TerrainSize = 0;
				  static float __TerrainSizeCell = 1.0f;
				  static int __TerrainSMapSize = 0;
				  static float __TerrainSizeHeight = 100.0f;
				  static float __TerrainStepY = 1.0f;
				  static char HMapName[64];
				  static char HMeshName[64];

char LevelEditName[256];
bool g_AppMainBackToAppSelect = false;
static const int APPMAIN_VISIBLE_MAPS = 6;
static int g_MainRmlMapScroll = 0;

void SaveLevelData( char* Str );

static void* ____DummyObjectConstructor()
{
	return NULL;
}

bool gNewLevelCreated = false ;

bool CreateNewLevel()
{
#ifndef FINAL_BUILD
	char Path[256];
	char Str[256];

	gNewLevelCreated = true ;

	sprintf(Path, "Levels\\%s", LevelEditName );
	if ( mkdir(Path) == -1 )
	{
		if ( errno == EEXIST )
		{
			r3dOutToLog( "Map with name \"%s\" already exist.\n", LevelEditName );
			return false;
		}
	}

	if (__CreateTerrain)
	{
		r3d_assert(g_pPhysicsWorld == 0);
		g_pPhysicsWorld = new PhysXWorld();
		g_pPhysicsWorld->Init();

		r3dGameLevel::SetHomeDir( LevelEditName);

		if( __CreateTerrain2 )
		{
			r3dTerrain2::CreationParams params ;

			params.CellCountX = int( pow( 2.0f, 8.0f + __TerrainSize ) ) ;
			params.CellCountZ = params.CellCountX ;
			params.SplatSizeX = int( pow( 2.0f, 8.0f + __TerrainSMapSize ) ) ;
			params.SplatSizeZ = params.SplatSizeX ;

			params.CellSize = __TerrainSizeCell ;
			params.LevelDir = r3dString( "Levels\\" ) + LevelEditName + "\\" ;;
			
			r3dTerrain2 createTerra ;

			createTerra.SaveEmpty( params ) ;
		}
		else
		{
			sprintf(Str, "Levels\\%s\\Terrain", LevelEditName );
			mkdir(Str);

			int TerraWidth = int( pow(2.0f, 8.0f + __TerrainSize) );
			int SplatWidth = int(pow(2.0f, 8+__TerrainSMapSize));

			sprintf(Str, "Levels\\%s\\Terrain\\Mat-Splat.dds", LevelEditName );
			r3dTexture *TempTex = r3dRenderer->AllocateTexture();
			TempTex->Create(SplatWidth, SplatWidth, D3DFMT_A8R8G8B8, 1);
			TempTex->Save(Str);
			r3dRenderer->DeleteTexture(TempTex);

			g_pUndoHistory = new UndoHistory;
			
			extern r3dITerrain* Terrain ;
			extern r3dTerrain* Terrain1 ;
			Terrain1 = new CQuadTerrain;
			Terrain = Terrain1 ;
			
			Terrain1->Height = Terrain1->Width = float( TerraWidth );
			Terrain1->SetCellSize ( __TerrainSizeCell );
			Terrain1->__HeightmapBlend = 0.25f;
			Terrain1->__TerraLOD1 = 1;
			Terrain1->__TerraLOD2 = 2;
			Terrain1->__TerraLOD3 = 24;
			Terrain1->m_HeightmapSize = __TerrainSizeHeight;
			
			Terrain1->MatSplatTex[ 0 ] = r3dRenderer->LoadTexture( Str );
			Terrain1->NumMats = 1;
			r3dTerrain::Layer_t layer;
			for (int i = 0; i < 4 ; i++)
			{
				layer.pMapDiffuse = r3dRenderer->LoadTexture( "Data\\Shaders\\Texture\\MissingTexture.dds" );
				layer.pMapNormal = r3dRenderer->LoadTexture( "Data\\TerrainData\\Materials\\bump_sim.dds" );
				Terrain1->m_dMatLayers[ 0 ][ i ] = layer;
			}
			
			//Terrain->InitData();

			r3d_assert(_CrtCheckMemory());

			Terrain1->CreateDefaultPhysicsData();

			r3d_assert(_CrtCheckMemory());
			Terrain1->HeightFieldData.Resize( uint32_t(Terrain1->Width*Terrain1->Height) );

			r3d_assert(_CrtCheckMemory());
			for(int x=0; x<Terrain1->Width; ++x)
				for(int y=0; y<Terrain1->Height; ++y)
					Terrain1->HeightFieldData[ x*(int)Terrain1->Width + y ] = __TerrainSizeHeight;

			r3d_assert(_CrtCheckMemory());
			Terrain1->UpdatePhysHeightField();
			r3d_assert(_CrtCheckMemory());

			Terrain1->m_pColorData = new uint32_t[ Terrain1->HeightFieldData.Count() ];
			r3d_assert(_CrtCheckMemory());

			memset( Terrain1->m_pColorData, 0xff, sizeof( uint32_t) * uint32_t(Terrain1->Width) * uint32_t(Terrain1->Height) );

			r3d_assert(_CrtCheckMemory());

			Terrain1->m_tBaseLayer.pMapDiffuse = r3dRenderer->LoadTexture( "Data\\Shaders\\Texture\\MissingTexture.dds" );
			Terrain1->m_tBaseLayer.pMapNormal = r3dRenderer->LoadTexture( "Data\\TerrainData\\Materials\\bump_sim.dds" );
			Terrain1->m_tBaseLayer.fScale = 50.0f;
			r3d_assert(_CrtCheckMemory());

			Terrain1->RecalcParams();
			r3d_assert(_CrtCheckMemory());

			Terrain1->SaveData( Path, false );
			Terrain1->Unload();

			Terrain1->UpdateDesc() ;			
		}

		GameWorld().Init(1,1);
		obj_Terrain* pObjTerrain = new obj_Terrain();
		obj_Terrain& objTerrain = *pObjTerrain;
		objTerrain.DrawOrder      = OBJ_DRAWORDER_FIRST;
		objTerrain.ObjFlags      |= OBJFLAG_SkipCastRay;
		objTerrain.FileName = "terra1";
		AClass classData( NULL, "obj_Terrain", "Type", ____DummyObjectConstructor );
		classData.Name = "obj_Terrain";
		objTerrain.Class = &classData;
		GameWorld().AddObject( pObjTerrain );
		SaveLevelData( Str );
		GameWorld().Destroy();

		g_pPhysicsWorld->Destroy();
		SAFE_DELETE(g_pPhysicsWorld);

		delete g_pUndoHistory;
		g_pUndoHistory = NULL;
	}

#endif
	return true;
}

extern void RegisterMsgProc(bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam));
extern void UnregisterMsgProc(bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam));

static RmlUISystem g_MainRmlUI;
extern bool ProcessStudioPendingResize(
	RmlUISystem* ActiveRmlUI
);

static bool g_MainRmlInputEnabled = false;
static int g_MainRmlResult = -1;
static int g_MainRmlTab = 0;

static char g_MainRmlMaps[256][256];
static int g_MainRmlMapCount = 0;

static bool MainRml_MsgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!g_MainRmlInputEnabled)
		return false;

	if (!g_MainRmlUI.IsInitialized())
		return false;

	if (!g_MainRmlUI.IsAppMainVisible())
		return false;

	LRESULT Result = 0;
	return g_MainRmlUI.ProcessWin32Message(win::hWnd, uMsg, wParam, lParam, &Result);
}

static bool MainRml_IsValidDirectory(const WIN32_FIND_DATAA& Data)
{
	if (!(Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		return false;

	if (strcmp(Data.cFileName, ".") == 0)
		return false;

	if (strcmp(Data.cFileName, "..") == 0)
		return false;

	return true;
}

static void MainRml_UpdateVisibleMapList()
{
	if (g_MainRmlMapScroll < 0)
		g_MainRmlMapScroll = 0;

	const int MaxScroll = R3D_MAX(0, g_MainRmlMapCount - APPMAIN_VISIBLE_MAPS);

	if (g_MainRmlMapScroll > MaxScroll)
		g_MainRmlMapScroll = MaxScroll;

	const char* Names[APPMAIN_VISIBLE_MAPS] = {};
	int VisibleCount = 0;

	for (int i = 0; i < APPMAIN_VISIBLE_MAPS; ++i)
	{
		const int RealIndex = g_MainRmlMapScroll + i;

		if (RealIndex >= g_MainRmlMapCount)
			break;

		Names[VisibleCount] = g_MainRmlMaps[RealIndex];
		VisibleCount++;
	}

	g_MainRmlUI.SetAppMainTab(g_MainRmlTab);
	g_MainRmlUI.SetAppMainMaps(Names, VisibleCount);
	g_MainRmlUI.SetAppMainScrollInfo(g_MainRmlMapScroll, APPMAIN_VISIBLE_MAPS, g_MainRmlMapCount);
	g_MainRmlUI.SetAppMainSelectedLevel(LevelEditName);
}

static void MainRml_RebuildMapList()
{
	g_MainRmlMapCount = 0;

	const char* Mask = g_MainRmlTab == 0 ? "Levels\\*" : "Levels\\WorkInProgress\\*";

	WIN32_FIND_DATAA Data;
	HANDLE Find = FindFirstFileA(Mask, &Data);

	if (Find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!MainRml_IsValidDirectory(Data))
				continue;

			if (g_MainRmlTab == 0 && stricmp(Data.cFileName, "WorkInProgress") == 0)
				continue;

			if (g_MainRmlMapCount >= 256)
				break;

			r3dscpy(g_MainRmlMaps[g_MainRmlMapCount], Data.cFileName);
			g_MainRmlMapCount++;
		}
		while (FindNextFileA(Find, &Data));

		FindClose(Find);
	}

	g_MainRmlMapScroll = 0;
	MainRml_UpdateVisibleMapList();
}

static void MainRml_SelectMapByIndex(int VisibleIndex)
{
	const int RealIndex = g_MainRmlMapScroll + VisibleIndex;

	if (RealIndex < 0 || RealIndex >= g_MainRmlMapCount)
		return;

	if (g_MainRmlTab == 0)
	{
		r3dscpy(LevelEditName, g_MainRmlMaps[RealIndex]);
	}
	else
	{
		sprintf(LevelEditName, "WorkInProgress\\%s", g_MainRmlMaps[RealIndex]);
	}

	g_MainRmlUI.SetAppMainSelectedLevel(LevelEditName);
	DiscordPresence_SetEditor("Level Editor", LevelEditName);
}

static void MainRml_OnAction(const char* Action, const char* Value)
{
	if (!Action)
		return;

	if (strcmp(Action, "tab") == 0)
	{
		g_MainRmlTab = Value ? atoi(Value) : 0;

		if (g_MainRmlTab < 0)
			g_MainRmlTab = 0;

		if (g_MainRmlTab > 2)
			g_MainRmlTab = 2;

		if (g_MainRmlTab == 2)
		{
			r3dscpy(LevelEditName, "NewLevel");
			g_MainRmlUI.SetAppMainSelectedLevel(LevelEditName);
			g_MainRmlUI.SetAppMainTab(g_MainRmlTab);

			g_MainRmlUI.SetAppMainCreateOptions(
				__CreateTerrain != 0,
				__CreateTerrain2 != 0,
				__TerrainSize,
				__TerrainSMapSize,
				__TerrainSizeCell,
				__TerrainSizeHeight
			);
		}
		else
		{
			LevelEditName[0] = 0;
			MainRml_RebuildMapList();
		}
	}
	else if (strcmp(Action, "select") == 0)
	{
		MainRml_SelectMapByIndex(Value ? atoi(Value) : -1);
	}
	else if (strcmp(Action, "load") == 0)
	{
		if (LevelEditName[0])
			g_MainRmlResult = Menu_Main::bEditor;
	}
	else if (strcmp(Action, "create") == 0)
	{
		char NewLevelName[256] = {};

		bool DummyHaveTerrain = false;
		bool DummyTerrainV2 = false;
		int DummyTerrainSizeIndex = 0;
		int DummySplatSizeIndex = 0;
		float CellSize = __TerrainSizeCell;
		float Height = __TerrainSizeHeight;

		g_MainRmlUI.GetAppMainCreateData(
			NewLevelName,
			sizeof(NewLevelName),
			DummyHaveTerrain,
			DummyTerrainV2,
			DummyTerrainSizeIndex,
			DummySplatSizeIndex,
			CellSize,
			Height
		);

		if (!NewLevelName[0])
			r3dscpy(NewLevelName, "NewLevel");

		r3dscpy(LevelEditName, NewLevelName);

		__TerrainSizeCell = CellSize;
		__TerrainSizeHeight = Height;

		if (__TerrainSizeCell <= 0.0f)
			__TerrainSizeCell = 1.0f;

		if (__TerrainSizeHeight < 0.0f)
			__TerrainSizeHeight = 0.0f;

		if (CreateNewLevel())
			g_MainRmlResult = Menu_Main::bEditor;
	}
	else if (strcmp(Action, "back") == 0)
	{
		LevelEditName[0] = 0;

		g_AppMainBackToAppSelect = true;
		g_MainRmlResult = 0;
	}
	else if (strcmp(Action, "exit") == 0)
	{
		extern bool g_bExit;

		LevelEditName[0] = 0;

		g_bExit = true;
		g_MainRmlResult = 0;
	}
	else if (strcmp(Action, "terrain_toggle") == 0)
	{
		__CreateTerrain = __CreateTerrain ? 0 : 1;

		g_MainRmlUI.SetAppMainCreateOptions(
			__CreateTerrain != 0,
			__CreateTerrain2 != 0,
			__TerrainSize,
			__TerrainSMapSize,
			__TerrainSizeCell,
			__TerrainSizeHeight
		);
	}
	else if (strcmp(Action, "terrain2_toggle") == 0)
	{
		__CreateTerrain2 = __CreateTerrain2 ? 0 : 1;

		g_MainRmlUI.SetAppMainCreateOptions(
			__CreateTerrain != 0,
			__CreateTerrain2 != 0,
			__TerrainSize,
			__TerrainSMapSize,
			__TerrainSizeCell,
			__TerrainSizeHeight
		);
	}
	else if (strcmp(Action, "terrain_size") == 0)
	{
		__TerrainSize++;

		if (__TerrainSize > 4)
			__TerrainSize = 0;

		if (__TerrainSMapSize > __TerrainSize)
			__TerrainSMapSize = __TerrainSize;

		g_MainRmlUI.SetAppMainCreateOptions(
			__CreateTerrain != 0,
			__CreateTerrain2 != 0,
			__TerrainSize,
			__TerrainSMapSize,
			__TerrainSizeCell,
			__TerrainSizeHeight
		);
	}
	else if (strcmp(Action, "splat_size") == 0)
	{
		__TerrainSMapSize++;

		if (__TerrainSMapSize > __TerrainSize)
			__TerrainSMapSize = 0;

		g_MainRmlUI.SetAppMainCreateOptions(
			__CreateTerrain != 0,
			__CreateTerrain2 != 0,
			__TerrainSize,
			__TerrainSMapSize,
			__TerrainSizeCell,
			__TerrainSizeHeight
		);
	}
	else if (strcmp(Action, "splat_size") == 0)
	{
		__TerrainSMapSize++;

		if (__TerrainSMapSize > __TerrainSize)
			__TerrainSMapSize = 0;

		g_MainRmlUI.SetAppMainCreateOptions(__CreateTerrain != 0, __CreateTerrain2 != 0, __TerrainSize, __TerrainSMapSize, __TerrainSizeCell, __TerrainSizeHeight);
	}
	else if (strcmp(Action, "scroll_up") == 0)
	{
		g_MainRmlMapScroll--;

		MainRml_UpdateVisibleMapList();
	}
	else if (strcmp(Action, "scroll_down") == 0)
	{
		g_MainRmlMapScroll++;

		MainRml_UpdateVisibleMapList();
	}
}

void ClearFullScreen_Menu();

extern bool g_bExit;
int Menu_Main::DoModal()
{
 LevelEditName[0] = 0;
 char tempName[256] = {0};
 char discordLevelName[256] = {0};
 DiscordPresence_SetEditor("Level Editor", "No map selected");

	g_MainRmlResult = -1;
	g_MainRmlTab = 0;
	g_MainRmlInputEnabled = false;

	bool bUseRmlUI = false;
	bool bRmlMsgProcRegistered = false;

	bool bRmlInitialized = false;

	if (
		!bRmlInitialized &&
		r3dRenderer &&
		r3dRenderer->pd3ddev &&
		win::hWnd
	)
	{
		bRmlInitialized =
			g_MainRmlUI.Init(
				win::hWnd,
				r3dRenderer->pd3ddev,
				false
			);
	}

	if (bRmlInitialized)
	{
		g_MainRmlUI.SetAppMainCallback(MainRml_OnAction);

		if (g_MainRmlUI.LoadAppMain())
		{
			g_MainRmlUI.ShowAppMain();
			g_MainRmlUI.SetAppMainTab(0);
			g_MainRmlUI.SetAppMainSelectedLevel("");

			MainRml_RebuildMapList();

			RegisterMsgProc(MainRml_MsgProc);
			bRmlMsgProcRegistered = true;
			g_MainRmlInputEnabled = true;

			bUseRmlUI = true;

			r3dOutToLog("[RmlUI] AppMain enabled\n");
		}
		else
		{
			r3dOutToLog("[RmlUI] AppMain load failed, fallback to old imgui m_Main\n");

			g_MainRmlUI.Shutdown();
		}
	}

	auto ShutdownMainRml = [&]()
	{
		g_MainRmlInputEnabled = false;

		if (bRmlMsgProcRegistered)
		{
			UnregisterMsgProc(MainRml_MsgProc);
			bRmlMsgProcRegistered = false;
		}

		if (g_MainRmlUI.IsInitialized())
		{
			g_MainRmlUI.HideAppMain();
			g_MainRmlUI.Shutdown();
		}
	};

	if (bUseRmlUI)
	{
		int FinalResult = 0;

		while (1)
		{
			if (g_bExit)
			{
				FinalResult = 0;
				break;
			}

			ProcessStudioPendingResize(
				&g_MainRmlUI
			);

			r3dStartFrame();
			ClearFullScreen_Menu();

			mUpdate();
			DiscordPresence_Tick();

			mDrawStart();

			r3dRenderer->SetRenderingMode(R3D_BLEND_ALPHA | R3D_BLEND_NZ);
			r3dSetFiltering(R3D_POINT);

			g_MainRmlUI.Update(r3dGetFrameTime());

			g_MainRmlUI.Render();

			mDrawEnd();
			r3dEndFrame();

			if (g_MainRmlResult != -1)
			{
				FinalResult = g_MainRmlResult;
				break;
			}
		}

		g_MainRmlInputEnabled = false;

		if (bRmlMsgProcRegistered)
		{
			UnregisterMsgProc(MainRml_MsgProc);
			bRmlMsgProcRegistered = false;
		}

		if (g_MainRmlUI.IsInitialized())
		{
			g_MainRmlUI.HideAppMain();
			g_MainRmlUI.Shutdown();
		}

		ShutdownMainRml();
		return FinalResult;
	}

  while(1)
  {
	  if(g_bExit)
		  return 0;
    r3dStartFrame();

	ClearFullScreen_Menu();

	mUpdate();
	DiscordPresence_Tick();

	imgui_Update();
	imgui2_Update();

	int ret = 1;

	mDrawStart();
  
	r3dRenderer->SetRenderingMode(R3D_BLEND_ALPHA | R3D_BLEND_NZ);
	r3dSetFiltering( R3D_POINT );

	const static char* list[3] = { "LIVE MAPS", "EDITOR MAPS", "CREATE MAP"};


	static int LevelIndex = 0;

	int lastLevelIndex = LevelIndex;

    imgui_Toolbar(r3dRenderer->ScreenW/2-250, r3dRenderer->ScreenH/2-150-40, 500, 40, &LevelIndex, 0, list, 3);
    r3dDrawBox2D(r3dRenderer->ScreenW/2-250, r3dRenderer->ScreenH/2-150+2, 500, 350, imgui_bkgDlg);

	switch (LevelIndex)
	{
			case	0: // LIVE MAPS
				{
					static float offset = 0.f ;
					imgui_FileList(r3dRenderer->ScreenW/2-240, r3dRenderer->ScreenH/2-140+2, 360, 330, "Levels\\*.", LevelEditName, &offset );

					if (LevelEditName[0] && stricmp(LevelEditName, "workinprogress")!=0)
					{
						if(stricmp(discordLevelName, LevelEditName))
						{
							r3dscpy(discordLevelName, LevelEditName);
							DiscordPresence_SetEditor("Level Editor", LevelEditName);
						}
						if (imgui_Button(r3dRenderer->ScreenW/2+250-150, r3dRenderer->ScreenH/2+200+5,150,30, "Load Level", 0)) 
							released_id = bEditor;
					}
				}
				break;

			case	1: // EDITOR MAPS
				{
					static float offset = 0.f ;
					imgui_FileList(r3dRenderer->ScreenW/2-240, r3dRenderer->ScreenH/2-140+2, 360, 330, "Levels\\WorkInProgress\\*.", tempName, &offset );

					if (tempName[0])
					{
						sprintf(LevelEditName, "WorkInProgress\\%s", tempName);
						if(stricmp(discordLevelName, LevelEditName))
						{
							r3dscpy(discordLevelName, LevelEditName);
							DiscordPresence_SetEditor("Level Editor", LevelEditName);
						}
						if (imgui_Button(r3dRenderer->ScreenW/2+250-150, r3dRenderer->ScreenH/2+200+5,150,30, "Load Level", 0)) 
							released_id = bEditor;
					}
				}
				break;

			case	2:
				{
					if( lastLevelIndex != LevelIndex )
					{
						r3dscpy( LevelEditName, "NewLevel" );
					}

					float SliderX = r3dRenderer->ScreenW/2-240; 
					float SliderY = r3dRenderer->ScreenH/2-140+2;

					const float MAP_NAME_HEIGHT = 24;

					imgui2_StartArea ( "MAP_NAME_AREA", SliderX, SliderY, 360.f, MAP_NAME_HEIGHT );
					imgui2_StringValue ( "MAP NAME", LevelEditName );
					imgui2_EndArea ();

					SliderY += MAP_NAME_HEIGHT;

					SliderY += imgui_Checkbox(SliderX, SliderY, "HAVE TERRAIN",	    &__CreateTerrain, 1);
					if (__CreateTerrain)
					{
						const static char* list[5] = { "256", "512", "1024", "2048", "4096" };
						const static char* list1[3] = { "PLANE", "IMAGE", "NOISE" };

						SliderY += imgui_Checkbox( SliderX, SliderY, "Create Terrain V2", &__CreateTerrain2, 1 ) ;

						SliderY += imgui_Static(SliderX, SliderY, "Terrain Size");
						SliderY += imgui_Value_Slider(SliderX, SliderY, "Cell size in Meters",			&__TerrainSizeCell,	1,100,	"%-02.2f",1);
						imgui_Toolbar(SliderX, SliderY, 360, 35, &__TerrainSize, 0, list, 5);

						SliderY += 36;
						SliderY += imgui_Static(SliderX, SliderY, "Splat map Size");

						__TerrainSMapSize = R3D_MIN( __TerrainSMapSize, __TerrainSize  ) ;

						imgui_Toolbar(SliderX, SliderY, 360, 35, &__TerrainSMapSize, 0, list, __TerrainSize + 1 );

						SliderY += 36;
						SliderY += imgui_Value_Slider(SliderX, SliderY, "Terrain height in Meters",			&__TerrainSizeHeight,	0,1200,	"%-02.2f",1);
					}

				}
				if (imgui_Button(r3dRenderer->ScreenW/2+250-150, r3dRenderer->ScreenH/2+200+5,150,30, "Create Level", 0))
				{
					if ( CreateNewLevel() )
					{
						released_id = bEditor;
					}
				}

				break;
	}


	mDrawEnd();
  
	switch(released_id)
	{
		case bEditor:
				   return bEditor;
	};

    r3dEndFrame();
  }
  
  return 0;
}
