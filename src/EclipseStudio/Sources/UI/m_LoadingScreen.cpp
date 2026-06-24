#include "r3dPCH.h"
#include "r3d.h"

#include "m_LoadingScreen.h"
#include "GameCommon.h"
#include "GameCode\UserProfile.h"

#include "Multiplayer\MasterServerLogic.h"

#include "LangMngr.h"

#include "r3dDeviceQueue.h"
#include "../../RmlUI/RmlUISystem.h"

static const char* DEFAULT_LOADING_TEXTURE =
	"Data\\Menu\\Screen.png";

static volatile LONG gProgressValue = 0;
static LoadingScreen* gLoadingScreen = NULL;
static RmlUISystem g_LoadingRmlUI;

extern bool ProcessStudioPendingResize(
	RmlUISystem* ActiveRmlUI
);

static float LoadingProgressFromLong()
{
	return (float)gProgressValue / 10000.0f;
}

static LONG LoadingProgressToLong(float Progress)
{
	Progress = R3D_MAX(
		R3D_MIN(
			Progress,
			1.0f
		),
		0.0f
	);

	return (LONG)(Progress * 10000.0f);
}

static void CopyWideString(
	wchar_t*& Destination,
	const wchar_t* Source
)
{
	SAFE_DELETE_ARRAY(Destination);

	if (!Source)
		return;

	Destination =
		new wchar_t[wcslen(Source) + 1];

	r3dscpy(
		Destination,
		Source
	);
}

static bool EnsureLoadingRmlUI()
{
	if (g_LoadingRmlUI.IsInitialized())
		return true;

	if (
		!r3dRenderer ||
		!r3dRenderer->pd3ddev ||
		!win::hWnd
	)
	{
		return false;
	}

	if (
		!g_LoadingRmlUI.Init(
			win::hWnd,
			r3dRenderer->pd3ddev,
			false
		)
	)
	{
		return false;
	}

	if (!g_LoadingRmlUI.LoadLoadingScreen())
	{
		g_LoadingRmlUI.Shutdown();
		return false;
	}

	return true;
}

LoadingScreen::LoadingScreen(
	const char* movieName
)
	: m_pBackgroundTex(NULL)
	, m_RenderingDisabled(false)
	, m_MapName(NULL)
	, m_MapDesc(NULL)
	, m_TipOfTheDay(NULL)
{
	(void)movieName;
}

LoadingScreen::~LoadingScreen()
{
	if (m_pBackgroundTex)
	{
		r3dRenderer->DeleteTexture(
			m_pBackgroundTex
		);
	}

	m_pBackgroundTex = NULL;

	SAFE_DELETE_ARRAY(m_MapName);
	SAFE_DELETE_ARRAY(m_MapDesc);
	SAFE_DELETE_ARRAY(m_TipOfTheDay);
}

bool LoadingScreen::Initialize()
{
	if (EnsureLoadingRmlUI())
	{
		g_LoadingRmlUI.ShowLoadingScreen();
		g_LoadingRmlUI.SetLoadingScreenProgress(
			LoadingProgressFromLong()
		);

		ApplyStoredLoadingText();
	}

	return true;
}

void LoadingScreen::ApplyStoredLoadingText()
{
	if (!EnsureLoadingRmlUI())
		return;

	g_LoadingRmlUI.ShowLoadingScreen();

	g_LoadingRmlUI.SetLoadingScreenData(
		m_MapName ? m_MapName : L"Loading",
		m_MapDesc ? m_MapDesc : L"",
		m_TipOfTheDay
	);

	g_LoadingRmlUI.SetLoadingScreenProgress(
		LoadingProgressFromLong()
	);
}

void ClearFullScreen_Menu();

int LoadingScreen::Update()
{
	R3D_ENSURE_MAIN_THREAD();

	r3dMouse::Show();

	if (
		!m_RenderingDisabled &&
		EnsureLoadingRmlUI()
	)
	{
		ProcessStudioPendingResize(
			&g_LoadingRmlUI
		);

		g_LoadingRmlUI.Update(
			r3dGetFrameTime()
		);

		return 0;
	}

	r3dStartFrame();

	if (r3dRenderer->DeviceAvailable)
	{
		r3dRenderer->StartRender(1);
		r3dRenderer->StartFrame();

		r3dRenderer->SetRenderingMode(
			R3D_BLEND_ALPHA |
			R3D_BLEND_NZ
		);

		ClearFullScreen_Menu();

		if (m_pBackgroundTex)
		{
			float x;
			float y;
			float w;
			float h;

			r3dRenderer->GetBackBufferViewport(
				&x,
				&y,
				&w,
				&h
			);

			D3DVIEWPORT9 oldVp;
			D3DVIEWPORT9 newVp;

			r3dRenderer->DoGetViewport(
				&oldVp
			);

			newVp = oldVp;
			newVp.X = 0;
			newVp.Y = 0;
			newVp.Width =
				r3dRenderer->d3dpp.BackBufferWidth;
			newVp.Height =
				r3dRenderer->d3dpp.BackBufferHeight;

			r3dRenderer->SetViewport(
				(float)newVp.X,
				(float)newVp.Y,
				(float)newVp.Width,
				(float)newVp.Height
			);

			DWORD oldScissor = 0;

			r3dRenderer->pd3ddev->GetRenderState(
				D3DRS_SCISSORTESTENABLE,
				&oldScissor
			);

			r3dRenderer->pd3ddev->SetRenderState(
				D3DRS_SCISSORTESTENABLE,
				FALSE
			);

			r3dDrawBox2D(
				x,
				y,
				w,
				h,
				r3dColor24::white,
				m_pBackgroundTex
			);

			r3dRenderer->SetViewport(
				(float)oldVp.X,
				(float)oldVp.Y,
				(float)oldVp.Width,
				(float)oldVp.Height
			);

			r3dRenderer->pd3ddev->SetRenderState(
				D3DRS_SCISSORTESTENABLE,
				oldScissor
			);
		}

		if (
			!m_RenderingDisabled &&
			EnsureLoadingRmlUI()
		)
		{
			g_LoadingRmlUI.Update(
				r3dGetFrameTime()
			);

			g_LoadingRmlUI.Render();
		}

		r3dRenderer->Flush();
		r3dRenderer->EndFrame();
	}

	r3dRenderer->EndRender(true);
	r3dEndFrame();

	return 0;
}

void LoadingScreen::SetLoadingTexture(
	const char* ImagePath
)
{
	R3D_ENSURE_MAIN_THREAD();

	if (m_pBackgroundTex)
	{
		r3dRenderer->DeleteTexture(
			m_pBackgroundTex
		);

		m_pBackgroundTex = NULL;
	}

	const char* TexturePath =
		(
			ImagePath &&
			r3d_access(ImagePath, 0) == 0
		)
			? ImagePath
			: DEFAULT_LOADING_TEXTURE;

	m_pBackgroundTex =
		r3dRenderer->LoadTexture(
			TexturePath
		);

	r3d_assert(
		m_pBackgroundTex
	);
}

void LoadingScreen::SetData(
	const char* ImagePath,
	const wchar_t* Name,
	const wchar_t* Message,
	const wchar_t* tip_of_the_day
)
{
	R3D_ENSURE_MAIN_THREAD();

	CopyWideString(
		m_MapName,
		Name
	);

	CopyWideString(
		m_MapDesc,
		Message
	);

	CopyWideString(
		m_TipOfTheDay,
		tip_of_the_day
	);

	if (!m_RenderingDisabled)
	{
		SetLoadingTexture(
			ImagePath
		);
	}

	ApplyStoredLoadingText();

	r3d_assert(
		_CrtCheckMemory()
	);
}

void LoadingScreen::SetProgress(
	float progress
)
{
	R3D_ENSURE_MAIN_THREAD();

	if (EnsureLoadingRmlUI())
	{
		g_LoadingRmlUI.SetLoadingScreenProgress(
			progress
		);
	}
}

void StartLoadingScreen()
{
	r3d_assert(
		!gLoadingScreen
	);

	gLoadingScreen =
		new LoadingScreen(
			""
		);

	gLoadingScreen->Initialize();
	gLoadingScreen->SetRenderingDisabled(
		false
	);
}

void DisableLoadingRendering()
{
	if (gLoadingScreen)
	{
		gLoadingScreen->SetRenderingDisabled(
			true
		);
	}
}

void StopLoadingScreen()
{
	r3d_assert(
		gLoadingScreen
	);

	delete gLoadingScreen;
	gLoadingScreen = NULL;

	if (g_LoadingRmlUI.IsInitialized())
	{
		g_LoadingRmlUI.HideLoadingScreen();
		g_LoadingRmlUI.Shutdown();
	}
}

void SetLoadingTexture(
	const char* ImagePath
)
{
	if (gLoadingScreen)
	{
		gLoadingScreen->SetLoadingTexture(
			ImagePath
		);
	}
}

void SetLoadingProgress(
	float progress
)
{
	InterlockedExchange(
		&gProgressValue,
		LoadingProgressToLong(
			progress
		)
	);
}

void AdvanceLoadingProgress(
	float add
)
{
	const float NewValue =
		LoadingProgressFromLong() +
		add;

	InterlockedExchange(
		&gProgressValue,
		LoadingProgressToLong(
			NewValue
		)
	);
}

float GetLoadingProgress()
{
	return LoadingProgressFromLong();
}

void SetLoadingPhase(
	const char* Phase
)
{
	(void)Phase;
}

int DoLoadingScreen(
	volatile LONG* Loading,
	const wchar_t* LevelName,
	const wchar_t* LevelDescription,
	const char* LevelFolder,
	float TimeOut,
	int gameMode
)
{
	(void)gameMode;

	r3d_assert(
		gLoadingScreen
	);

	char sFullPath[512];

	sprintf(
		sFullPath,
		"%s\\%s",
		LevelFolder,
		"LoadingScreen.dds"
	);

	if (r3d_access(sFullPath, 0) != 0)
	{
		const int Selected =
			rand() % 3;

		sprintf(
			sFullPath,
			"%s\\LoadingScreen%d.dds",
			LevelFolder,
			Selected
		);
	}

	if (r3d_access(sFullPath, 0) != 0)
	{
		r3dscpy(
			sFullPath,
			DEFAULT_LOADING_TEXTURE
		);
	}

	char TipKey[32];

	sprintf(
		TipKey,
		"TipOfTheDay%d",
		int(
			floorf(
				u_GetRandom(
					0.0f,
					12.99f
				)
			)
		)
	);

	gLoadingScreen->SetData(
		sFullPath,
		LevelName,
		LevelDescription,
		gLangMngr.getString(
			TipKey
		)
	);

	const bool CheckTimeOut =
		TimeOut != 0.0f;

	const float EndWait =
		r3dGetTime() +
		TimeOut;

	while (*Loading)
	{
		if (
			CheckTimeOut &&
			r3dGetTime() > EndWait
		)
		{
			return 0;
		}

		r3dProcessWindowMessages();

		if (r3dRenderer->DeviceAvailable)
		{
			const float TimeStart =
				r3dGetTime();

			float MaxRenderTime =
				0.033f;

			for (
				;
				r3dGetTime() - TimeStart < 0.033f;
			)
			{
				ProcessDeviceQueue(
					TimeStart,
					MaxRenderTime
				);
			}
		}

		gLoadingScreen->SetProgress(
			LoadingProgressFromLong()
		);

		gLoadingScreen->Update();
	}

	return 1;
}

bool IsNeedExit();

int DoConnectScreen(
	volatile LONG* Loading,
	const wchar_t* Message,
	float TimeOut
)
{
	r3d_assert(
		gLoadingScreen
	);

	gLoadingScreen->SetData(
		"Data\\Menu\\ConnectScreen.dds",
		gLangMngr.getString(
			"Connecting"
		),
		Message,
		NULL
	);

	const bool CheckTimeOut =
		TimeOut != 0.0f;

	const float EndWait =
		r3dGetTime() +
		TimeOut;

	while (*Loading)
	{
		r3dProcessWindowMessages();

		if (IsNeedExit())
			return 0;

		if (
			CheckTimeOut &&
			r3dGetTime() > EndWait
		)
		{
			return 0;
		}

		gLoadingScreen->SetProgress(
			CheckTimeOut
				? 1.0f -
					(
						EndWait -
						r3dGetTime()
					) /
					TimeOut
				: LoadingProgressFromLong()
		);

		gLoadingScreen->Update();

		Sleep(33);
	}

	return 1;
}

template <typename T>
int DoConnectScreen(
	T* Logic,
	bool (T::*CheckFunc)(),
	const wchar_t* Message,
	float TimeOut
)
{
	r3d_assert(
		gLoadingScreen
	);

	gLoadingScreen->SetData(
		"Data\\Menu\\ConnectScreen.dds",
		gLangMngr.getString(
			"Connecting"
		),
		Message,
		NULL
	);

	const bool CheckTimeOut =
		TimeOut != 0.0f;

	const float StartWait =
		r3dGetTime();

	const float EndWait =
		StartWait +
		TimeOut;

	for (;;)
	{
		extern void tempDoMsgLoop();

		tempDoMsgLoop();

		if ((Logic->*CheckFunc)())
			break;

		if (IsNeedExit())
			return 0;

		if (
			CheckTimeOut &&
			r3dGetTime() > EndWait
		)
		{
			return 0;
		}

		if (r3dGetTime() > StartWait + 1.0f)
		{
			gLoadingScreen->SetProgress(
				CheckTimeOut
					? 1.0f -
						(
							EndWait -
							r3dGetTime()
						) /
						TimeOut
					: LoadingProgressFromLong()
			);

			gLoadingScreen->Update();
		}

		Sleep(33);
	}

	return 1;
}

template int DoConnectScreen(
	ClientGameLogic* Logic,
	bool (ClientGameLogic::*CheckFunc)(),
	const wchar_t* Message,
	float TimeOut
);

template int DoConnectScreen(
	MasterServerLogic* Logic,
	bool (MasterServerLogic::*CheckFunc)(),
	const wchar_t* Message,
	float TimeOut
);
