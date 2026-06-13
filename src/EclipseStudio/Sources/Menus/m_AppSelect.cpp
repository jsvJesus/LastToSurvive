#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "../../RmlUI/RmlUISystem.h"

#include "m_AppSelect.h"
#include "..\UI\UIMenu.h"
#include "..\DiscordPresence.h"

int	AppSelectMode = 100;

extern void RegisterMsgProc(bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam));
extern void UnregisterMsgProc(bool (*proc)(UINT uMsg, WPARAM wParam, LPARAM lParam));

static RmlUISystem g_AppSelectRmlUI;
extern bool ProcessStudioPendingResize(
	RmlUISystem* ActiveRmlUI
);
static int g_AppSelectRmlResult = -1;
static bool g_AppSelectRmlInputEnabled = false;

static bool AppSelect_RmlMsgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!g_AppSelectRmlInputEnabled)
		return false;

	LRESULT result = 0;

	if (g_AppSelectRmlUI.ProcessWin32Message(win::hWnd, uMsg, wParam, lParam, &result))
		return true;

	return false;
}

Menu_AppSelect::Menu_AppSelect()
{
}

Menu_AppSelect::~Menu_AppSelect()
{
}



void Menu_AppSelect::Draw()
{

	return;
}


extern bool g_bExit;

void ClearFullScreen_Menu()
{
	r3dRenderer->pd3ddev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE );
	r3dRenderer->SetViewport( 0.f, 0.f, (float)r3dRenderer->d3dpp.BackBufferWidth, (float)r3dRenderer->d3dpp.BackBufferHeight );
	D3D_V( r3dRenderer->pd3ddev->Clear( 0, NULL, D3DCLEAR_TARGET, 0, r3dRenderer->GetClearZValue(), 0 ) );
	r3dRenderer->pd3ddev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE );
}


int Menu_AppSelect::DoModal()
{
	DiscordPresence_SetMenu();

	AppSelectMode = 100;
	g_AppSelectRmlResult = -1;
	g_AppSelectRmlInputEnabled = false;

	Desktop().SetViewSize(r3dRenderer->ScreenW, r3dRenderer->ScreenH);

	bool bUseRmlUI = false;
	bool bRmlMsgProcRegistered = false;

	if (r3dRenderer && r3dRenderer->pd3ddev && win::hWnd)
	{
		if (g_AppSelectRmlUI.Init(win::hWnd, r3dRenderer->pd3ddev))
		{
			g_AppSelectRmlUI.SetAppSelectCallback([](const char* ModeId)
			{
				if (!ModeId)
					return;

				if (strcmp(ModeId, "game_public") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bStartGamePublic;
				}
				else if (strcmp(ModeId, "game_dev") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bStartGameSVN;
				}
				else if (strcmp(ModeId, "level_editor") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bStartLevelEditor;
				}
				else if (strcmp(ModeId, "particle_editor") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bStartParticleEditor;
				}
				else if (strcmp(ModeId, "physics_editor") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bStartPhysicsEditor;
				}
				else if (strcmp(ModeId, "character_editor") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bStartCharacterEditor;
				}
				else if (strcmp(ModeId, "exit") == 0)
				{
					g_AppSelectRmlResult = Menu_AppSelect::bQuit;
				}
			});

			if (g_AppSelectRmlUI.IsAppSelectReady())
			{
				g_AppSelectRmlUI.ShowAppSelect();

				RegisterMsgProc(AppSelect_RmlMsgProc);
				bRmlMsgProcRegistered = true;
				g_AppSelectRmlInputEnabled = true;

				bUseRmlUI = true;

				r3dOutToLog("[RmlUI] AppSelect enabled\n");
			}
			else
			{
				r3dOutToLog("[RmlUI] AppSelect document not ready, fallback to old imgui AppSelect\n");
			}
		}
		else
		{
			r3dOutToLog("[RmlUI] Init failed, fallback to old imgui AppSelect\n");
		}
	}
	else
	{
		r3dOutToLog("[RmlUI] Invalid renderer/device/window, fallback to old imgui AppSelect\n");
	}

	int finalResult = 0;

	while (1)
	{
		if (g_bExit)
		{
			finalResult = 0;
			break;
		}

		ProcessStudioPendingResize(
			bUseRmlUI
			? &g_AppSelectRmlUI
			: nullptr
		);

		r3dStartFrame();

		mUpdate();
		DiscordPresence_Tick();

		if (!bUseRmlUI)
			imgui_Update();

		mDrawStart();

		ClearFullScreen_Menu();

		r3dRenderer->SetRenderingMode(R3D_BLEND_ALPHA | R3D_BLEND_NZ);
		r3dSetFiltering(R3D_POINT);
		r3dRenderer->SetMipMapBias(-6.0f, -1);

		if (bUseRmlUI)
		{
			g_AppSelectRmlUI.Update(r3dGetFrameTime());
			g_AppSelectRmlUI.Render();
		}
		else
		{
			switch (AppSelectMode)
			{
			case 100:
				{
					const static char* BNames1[] =
					{
						"Update DB",
						"Game (Public Server)",
						"Game (DEV Server)"
					};

					for (int i = 0; i < R3D_ARRAYSIZE(BNames1); i++)
					{
						if (imgui_Button(
							r3dRenderer->ScreenW / 2 - (210 * R3D_ARRAYSIZE(BNames1)) / 2 + 210 * i,
							r3dRenderer->ScreenH / 2 - 30,
							200,
							30,
							BNames1[i],
							0))
						{
							released_id = bUpdateDB + i;
						}
					}

					const static char* BNames[] =
					{
						"Level Editor",
						"Particle Editor",
						"Physics Editor",
						"Character Editor"
					};

					for (int i = 0; i < R3D_ARRAYSIZE(BNames); i++)
					{
						if (imgui_Button(
							r3dRenderer->ScreenW / 2 - (210 * R3D_ARRAYSIZE(BNames)) / 2 + 210 * i,
							r3dRenderer->ScreenH / 2 + 30,
							200,
							30,
							BNames[i],
							0))
						{
							released_id = bStartLevelEditor + i;
						}
					}
				}
				break;
			}
		}

		r3dRenderer->pd3ddev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		r3dRenderer->SetRenderingMode(R3D_BLEND_NOALPHA | R3D_BLEND_NZ);

		mDrawEnd();
		r3dEndFrame();

		if (bUseRmlUI)
		{
			if (g_AppSelectRmlResult != -1)
			{
				finalResult = g_AppSelectRmlResult == Menu_AppSelect::bQuit ? 0 : g_AppSelectRmlResult;
				break;
			}
		}
		else
		{
			switch (released_id)
			{
			case -1:
				break;

			default:
				finalResult = released_id;
				goto AppSelect_ExitLoop;
			}
		}
	}

AppSelect_ExitLoop:

	g_AppSelectRmlInputEnabled = false;

	if (bRmlMsgProcRegistered)
	{
		UnregisterMsgProc(AppSelect_RmlMsgProc);
		bRmlMsgProcRegistered = false;
	}

	if (g_AppSelectRmlUI.IsInitialized())
	{
		g_AppSelectRmlUI.HideAppSelect();
		g_AppSelectRmlUI.Shutdown();
	}

	return finalResult;
}
