#include "r3dPCH.h"
#include "r3d.h"

#include "RmlFrontEndCharacterPreview.h"

#include "../RmlRuntime.h"

#include "GameCommon.h"
#include "GameLevel.h"

#include "GameCode/UserProfile.h"

#include "multiplayer/ClientGameLogic.h"

#include "UI/FrontEndShared.h"

#include "ObjectsCode/ai/AI_Player.h"
#include "ObjectsCode/ai/AI_PlayerAnim.h"

#include "rendering/Deffered/CommonPostFX.h"
#include "rendering/Deffered/PostFXChief.h"

extern void InitGame_Start();
extern void InitGame_Finish();
extern void DestroyGame();

extern void DoLoadGame(
	const char* LevelFolder,
	int MaxPlayers,
	bool UnloadPrevious,
	bool IsMenuLevel
);
extern r3dScreenBuffer* g_RmlCharacterPortraitRT;

RmlFrontEndCharacterPreview::
RmlFrontEndCharacterPreview()
{
}

RmlFrontEndCharacterPreview::
~RmlFrontEndCharacterPreview()
{
	Shutdown();
}

bool RmlFrontEndCharacterPreview::Initialize(
	const wiCharDataFull& Character
)
{
	if (bInitialized)
	{
		SetCharacter(
			Character
		);

		return true;
	}

	if (
		!r3dRenderer ||
		!r3dRenderer->pd3ddev ||
		!g_pPostFXChief
	)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Preview] "
			"Renderer or PostFX is not ready\n"
		);

		return false;
	}

	RmlRuntime::Get().
		SetCharacterPreviewTexture(
			nullptr
		);

	try
	{
		r3dGameLevel::SetHomeDir(
			"WZ_FrontEndLighting"
		);

		gClientLogic().Reset();

		InitGame_Start();

		DoLoadGame(
			r3dGameLevel::GetHomeDir(),
			4,
			true,
			true
		);

		Player =
			static_cast<obj_Player*>(
				srv_CreateGameObject(
					"obj_Player",
					"RmlFrontEndPlayer",
					r3dPoint3D(
						0.0f,
						0.0f,
						0.0f
					)
				)
			);

		if (!Player)
		{
			r3dOutToLog(
				"[RmlUI][FrontEnd][Preview] "
				"Unable to create obj_Player\n"
			);

			DestroyGame();
			return false;
		}

		Player->PlayerState =
			PLAYER_IDLE;

		Player->bDead = 0;

		Player->CurLoadout =
			Character;

		Player->m_disablePhysSkeleton =
			true;

		Player->m_fPlayerRotationTarget =
			0.0f;

		Player->m_fPlayerRotation =
			0.0f;

		Player->NetworkLocal =
			true;

		Player->OnCreate();

		Player->NetworkLocal =
			false;

		Player->UpdateLoadoutSlot(
			Character
		);

		Player->uberAnim_->IsInUI =
			true;

		Player->uberAnim_->
			AnimPlayerState = -1;

		Player->uberAnim_->
			anim.StopAll();

		Player->SyncAnimation(
			true
		);

		InitGame_Finish();

		if (!CreatePortraitTarget())
		{
			r3dOutToLog(
				"[RmlUI][FrontEnd][Preview] "
				"Unable to create portrait RenderTarget\n"
			);

			GameWorld().DeleteObject(
				Player
			);

			Player =
				nullptr;

			DestroyGame();

			return false;
		}

		bInitialized = true;

		r3dOutToLog(
			"[RmlUI][FrontEnd][Preview] "
			"Character preview initialized. "
			"HeroItemID=%u\n",
			Character.HeroItemID
		);

		return true;
	}
	catch (const char* Error)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Preview] "
			"Initialization failed: %s\n",
			Error
				? Error
				: "<unknown>"
		);
	}
	catch (...)
	{
		r3dOutToLog(
			"[RmlUI][FrontEnd][Preview] "
			"Initialization failed with unknown exception\n"
		);
	}

	if (Player)
	{
		GameWorld().DeleteObject(
			Player
		);

		Player = nullptr;
	}

	DestroyGame();

	bInitialized = false;
	return false;
}

void RmlFrontEndCharacterPreview::Shutdown()
{
	FinishPreparedFrame();

	RmlRuntime::Get().
		SetCharacterPreviewTexture(
			nullptr
		);

	RmlRuntime::Get().
		SetCharacterPortraitTexture(
			nullptr
		);

	ReleasePortraitTarget();

	if (Player)
	{
		GameWorld().DeleteObject(
			Player
		);

		Player =
			nullptr;
	}

	if (bInitialized)
	{
		DestroyGame();
	}

	bInitialized =
		false;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Preview] "
		"Shutdown complete\n"
	);
}

void RmlFrontEndCharacterPreview::
ApplyFullBodyCamera()
{
	if (!Player)
		return;

	const r3dPoint3D CharacterSize =
		Player->GetBBoxLocal().Size;

	const float Distance =
		GetOptimalDist(
			CharacterSize,
			22.5f
		);

	const r3dPoint3D CameraPosition(
		0.0f,
		CharacterSize.y,
		Distance
	);

	const r3dPoint3D CameraTarget(
		0.0f,
		1.0f,
		0.0f
	);

	gCam =
		CameraPosition;

	gCam.vPointTo =
		(
			CameraTarget -
			CameraPosition
		).NormalizeTo();

	gCam.FOV =
		45.0f;

	gCam.SetPlanes(
		0.01f,
		200.0f
	);

	r3dRenderer->SetCamera(
		gCam,
		true
	);
}

void RmlFrontEndCharacterPreview::
ApplyPortraitCamera()
{
	if (!Player)
		return;

	const r3dPoint3D CharacterSize =
		Player->GetBBoxLocal().Size;

	const r3dPoint3D PlayerPosition =
		Player->GetPosition();

	/*
	 * Точка примерно между грудью и лицом.
	 */
	const float PortraitTargetY =
		PlayerPosition.y +
		CharacterSize.y * 0.78f;

	float PortraitDistance =
		CharacterSize.y * 0.65f;

	if (PortraitDistance < 0.9f)
	{
		PortraitDistance =
			0.9f;
	}

	const r3dPoint3D CameraPosition(
		0.0f,
		PortraitTargetY +
			CharacterSize.y * 0.015f,
		PortraitDistance
	);

	const r3dPoint3D CameraTarget(
		0.0f,
		PortraitTargetY,
		0.0f
	);

	gCam =
		CameraPosition;

	gCam.vPointTo =
		(
			CameraTarget -
			CameraPosition
		).NormalizeTo();

	gCam.FOV =
		34.0f;

	gCam.SetPlanes(
		0.01f,
		50.0f
	);

	r3dRenderer->SetCamera(
		gCam,
		true
	);
}

bool RmlFrontEndCharacterPreview::
CreatePortraitTarget()
{
	if (g_RmlCharacterPortraitRT)
		return true;

	g_RmlCharacterPortraitRT =
		r3dScreenBuffer::CreateClass(
			"RmlCharacterPortraitRT",
			512.0f,
			512.0f,
			D3DFMT_A8R8G8B8,
			r3dScreenBuffer::Z_NO_Z
		);

	if (
		!g_RmlCharacterPortraitRT ||
		g_RmlCharacterPortraitRT->
			IsNull()
	)
	{
		delete g_RmlCharacterPortraitRT;

		g_RmlCharacterPortraitRT =
			nullptr;

		return false;
	}

	r3dOutToLog(
		"[RmlUI][FrontEnd][Preview] "
		"Portrait RenderTarget created: 512x512\n"
	);

	return true;
}

void RmlFrontEndCharacterPreview::
ReleasePortraitTarget()
{
	if (!g_RmlCharacterPortraitRT)
		return;

	delete g_RmlCharacterPortraitRT;

	g_RmlCharacterPortraitRT =
		nullptr;

	r3dOutToLog(
		"[RmlUI][FrontEnd][Preview] "
		"Portrait RenderTarget released\n"
	);
}

void RmlFrontEndCharacterPreview::SetCharacter(
	const wiCharDataFull& Character
)
{
	if (!Player)
		return;

	Player->uberAnim_->
		anim.StopAll();

	Player->CurLoadout =
		Character;

	Player->UpdateLoadoutSlot(
		Character
	);

	Player->m_fPlayerRotationTarget =
		0.0f;

	Player->m_fPlayerRotation =
		0.0f;

	Player->SyncAnimation(
		true
	);

	r3dOutToLog(
		"[RmlUI][FrontEnd][Preview] "
		"Loadout updated. HeroItemID=%u, "
		"Head=%d, Body=%d, Legs=%d\n",
		Character.HeroItemID,
		Character.HeadIdx,
		Character.BodyIdx,
		Character.LegsIdx
	);
}

void RmlFrontEndCharacterPreview::PrepareFrame()
{
	if (
		!bInitialized ||
		!Player ||
		!r3dRenderer ||
		!r3dRenderer->DeviceAvailable
	)
	{
		return;
	}

	FinishPreparedFrame();

	PreviousCamera =
		gCam;

	Player->SetPosition(
		r3dPoint3D(
			0.0f,
			0.38f,
			0.0f
		)
	);

	Player->m_fPlayerRotationTarget =
		0.0f;

	Player->m_fPlayerRotation =
		0.0f;

	Player->UpdateTransform();

	ApplyFullBodyCamera();

	GameWorld().StartFrame();
	GameWorld().Update();

	bFramePrepared =
		true;
}

void RmlFrontEndCharacterPreview::RenderFrame()
{
	if (
		!bFramePrepared ||
		!bInitialized ||
		!Player
	)
	{
		return;
	}

	/*
	 * Мир и персонаж рендерятся строго один раз.
	 *
	 * Внутри RenderCharacterToTarget():
	 * 1. создаётся полный центральный RenderTarget;
	 * 2. из него вырезается портрет в отдельный 512x512 RT.
	 */
	ApplyFullBodyCamera();

	RenderCharacterToTarget(
		false
	);

	r3dScreenBuffer* CharacterBuffer =
		g_pPostFXChief->GetBuffer(
			PostFXChief::
				RTT_UI_CHARACTER_32BIT
		);

	r3dScreenBuffer* PortraitBuffer =
		g_pPostFXChief->GetBuffer(
			PostFXChief::
				RTT_UI_CHARACTER_PORTRAIT_32BIT
		);

	RmlRuntime::Get().
		SetCharacterPreviewTexture(
			CharacterBuffer
				? CharacterBuffer->AsTex2D()
				: nullptr
		);

	RmlRuntime::Get().
		SetCharacterPortraitTexture(
			PortraitBuffer
				? PortraitBuffer->AsTex2D()
				: nullptr
		);

	FinishPreparedFrame();
}

void RmlFrontEndCharacterPreview::
RenderCharacterToTarget(
	bool bPortrait
)
{
	/*
	 * Параметр оставлен, чтобы пока не менять header.
	 * Отдельного второго render-pass больше нет.
	 */
	(void)bPortrait;

	if (
		!g_pPostFXChief ||
		!CurRenderPipeline
	)
	{
		return;
	}

	/*
	 * Единственный рендер мира в этом кадре.
	 */
	CurRenderPipeline->PreRender();
	CurRenderPipeline->Render();
	CurRenderPipeline->AppendPostFXes();

	/*
	 * Заполняем alpha.
	 */
	PFX_Fill::Settings FillSettings;

	FillSettings.ColorWriteMask =
		D3DCOLORWRITEENABLE_ALPHA;

	gPFX_Fill.PushSettings(
		FillSettings
	);

	g_pPostFXChief->AddFX(
		gPFX_Fill,
		PostFXChief::
			RTT_PINGPONG_LAST,
		PostFXChief::
			RTT_DIFFUSE_32BIT
	);

	/*
	 * Маска персонажа.
	 */
	PFX_StencilToMask::Settings MaskSettings;

	MaskSettings.Value =
		float4(
			0.0f,
			0.0f,
			0.0f,
			1.0f
		);

	gPFX_StencilToMask.PushSettings(
		MaskSettings
	);

	g_pPostFXChief->AddFX(
		gPFX_StencilToMask,
		PostFXChief::
			RTT_PINGPONG_LAST
	);

	/*
	 * Полная сцена для центрального окна.
	 */
	PFX_Copy::Settings FullCopySettings;

	FullCopySettings.TexScaleX =
		1.0f;

	FullCopySettings.TexScaleY =
		1.0f;

	FullCopySettings.TexOffsetX =
		0.0f;

	FullCopySettings.TexOffsetY =
		0.0f;

	FullCopySettings.ForceFiltering =
		false;

	gPFX_Copy.PushSettings(
		FullCopySettings
	);

	g_pPostFXChief->AddFX(
		gPFX_Copy,
		PostFXChief::
			RTT_UI_CHARACTER_32BIT,
		PostFXChief::
			RTT_PINGPONG_LAST
	);

	/*
	 * Отдельный 512x512 портрет.
	 *
	 * Источник — уже готовый центральный RenderTarget.
	 * Мир повторно не рендерится.
	 */
	r3dScreenBuffer* FullCharacterTarget =
		g_pPostFXChief->GetBuffer(
			PostFXChief::
				RTT_UI_CHARACTER_32BIT
		);

	float PortraitScaleY =
		0.20f;

	float PortraitScaleX =
		0.20f;

	/*
	 * Сохраняем квадратные пропорции области:
	 *
	 * pixel width  = SourceWidth  * ScaleX
	 * pixel height = SourceHeight * ScaleY
	 */
	if (
		FullCharacterTarget &&
		FullCharacterTarget->Width > 1.0f &&
		FullCharacterTarget->Height > 1.0f
	)
	{
		PortraitScaleX =
			PortraitScaleY *
			(
				FullCharacterTarget->Height /
				FullCharacterTarget->Width
			);
	}

	if (PortraitScaleX < 0.10f)
		PortraitScaleX = 0.10f;

	if (PortraitScaleX > 0.50f)
		PortraitScaleX = 0.50f;

	PFX_Copy::Settings PortraitCopySettings;

	PortraitCopySettings.TexScaleX =
		PortraitScaleX;

	PortraitCopySettings.TexScaleY =
		PortraitScaleY;

	/*
	 * Персонаж находится по центру кадра.
	 */
	PortraitCopySettings.TexOffsetX =
		(
			1.0f -
			PortraitScaleX
		) * 0.5f;

	/*
	 * Верхняя часть полного кадра:
	 * голова, плечи и часть груди.
	 */
	PortraitCopySettings.TexOffsetY =
		0.0f; // Поднять/Опустить камеру, чем выше значение тем больше опускает или наоборт

	PortraitCopySettings.ForceFiltering =
		true;

	gPFX_Copy.PushSettings(
		PortraitCopySettings
	);

	g_pPostFXChief->AddFX(
		gPFX_Copy,
		PostFXChief::
			RTT_UI_CHARACTER_PORTRAIT_32BIT,
		PostFXChief::
			RTT_UI_CHARACTER_32BIT
	);

	/*
	 * Выполняем оба копирования одним PostFX-проходом.
	 */
	g_pPostFXChief->Execute(
		false,
		true
	);

	r3dRenderer->SetVertexShader();
	r3dRenderer->SetPixelShader();
}

void RmlFrontEndCharacterPreview::
FinishPreparedFrame()
{
	if (!bFramePrepared)
		return;

	GameWorld().EndFrame();

	gCam =
		PreviousCamera;

	if (r3dRenderer)
	{
		r3dRenderer->SetCamera(
			gCam,
			true
		);
	}

	bFramePrepared = false;
}

bool RmlFrontEndCharacterPreview::
IsInitialized() const
{
	return bInitialized;
}