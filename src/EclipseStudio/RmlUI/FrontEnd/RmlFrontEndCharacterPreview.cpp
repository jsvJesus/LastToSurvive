#include "r3dPCH.h"
#include "r3d.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "RmlFrontEndCharacterPreview.h"

#include "../RmlRuntime.h"

#include "GameCommon.h"
#include "GameLevel.h"

#include "GameCode/UserProfile.h"

#include "multiplayer/ClientGameLogic.h"

#include "UI/FrontEndShared.h"

#include "ObjectsCode/ai/AI_Player.h"
#include "ObjectsCode/ai/AI_PlayerAnim.h"

#include "gameobjects/ObjManag.h"

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

namespace
{
	struct FRmlPreviewObjectDrawBackup
	{
		GameObject* Object;
		int ObjFlags;
	};

	void HideWorldObjectsForRmlCharacterPreview(
		obj_Player* PreviewPlayer,
		std::vector<FRmlPreviewObjectDrawBackup>& Backups
	)
	{
		if (!PreviewPlayer)
			return;

		ObjectIterator It =
			GameWorld().GetFirstOfAllObjects();

		while (It.current)
		{
			GameObject* Object =
				It.current;

			if (
				Object &&
				Object != PreviewPlayer
			)
			{
				FRmlPreviewObjectDrawBackup Backup;
				Backup.Object =
					Object;
				Backup.ObjFlags =
					Object->ObjFlags;

				Backups.push_back(
					Backup
				);

				Object->ObjFlags |=
					OBJFLAG_SkipDraw;
			}

			It =
				GameWorld().GetNextOfAllObjects(
					It
				);
		}
	}

	void RestoreWorldObjectsForRmlCharacterPreview(
		std::vector<FRmlPreviewObjectDrawBackup>& Backups
	)
	{
		for (
			size_t Index = 0;
			Index < Backups.size();
			++Index
		)
		{
			if (Backups[Index].Object)
			{
				Backups[Index].Object->ObjFlags =
					Backups[Index].ObjFlags;
			}
		}

		Backups.clear();
	}
}

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
	if (!Player || !r3dRenderer)
		return;

	const r3dPoint3D CharacterSize =
		Player->GetBBoxLocal().Size;

	const r3dPoint3D PlayerPosition =
		Player->GetPosition();

	float Distance =
		GetOptimalDist(
			CharacterSize,
			24.0f
		);

	Distance *=
		ViewDistanceScale;

	const float TargetX =
		PlayerPosition.x +
		ViewHorizontalOffset;

	const float TargetY =
		PlayerPosition.y +
		CharacterSize.y * 0.50f +
		ViewVerticalOffset;

	const float TargetZ =
		PlayerPosition.z;

	const r3dPoint3D CameraTarget(
		TargetX,
		TargetY,
		TargetZ
	);

	const r3dPoint3D CameraPosition(
		TargetX,
		TargetY +
			CharacterSize.y * 0.035f,
		TargetZ +
			Distance
	);

	gCam =
		CameraPosition;

	gCam.vPointTo =
		(
			CameraTarget -
			CameraPosition
		).NormalizeTo();

	gCam.FOV =
		42.0f;

	gCam.SetPlanes(
		0.01f,
		250.0f
	);

	r3dRenderer->SetCamera(
		gCam,
		true
	);
}

void RmlFrontEndCharacterPreview::
ApplyPortraitCamera()
{
	if (!Player || !r3dRenderer)
		return;

	const r3dPoint3D CharacterSize =
		Player->GetBBoxLocal().Size;

	const r3dPoint3D PlayerPosition =
		Player->GetPosition();

	const float TargetY =
		PlayerPosition.y +
		CharacterSize.y * 0.79f;

	float Distance =
		CharacterSize.y * 0.58f;

	Distance =
		std::max(
			Distance,
			0.82f
		);

	const r3dPoint3D CameraTarget(
		PlayerPosition.x,
		TargetY,
		PlayerPosition.z
	);

	const r3dPoint3D CameraPosition(
		PlayerPosition.x,
		TargetY +
			CharacterSize.y * 0.015f,
		PlayerPosition.z +
			Distance
	);

	gCam =
		CameraPosition;

	gCam.vPointTo =
		(
			CameraTarget -
			CameraPosition
		).NormalizeTo();

	gCam.FOV =
		31.0f;

	gCam.SetPlanes(
		0.01f,
		60.0f
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

	Player->SyncAnimation(
		true
	);

	ResetView();

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

void RmlFrontEndCharacterPreview::Rotate(
	float DeltaPixelsX,
	float DeltaPixelsY
)
{
	(void)DeltaPixelsY;

	ViewYawDegrees +=
		DeltaPixelsX * 0.32f;

	if (ViewYawDegrees > 360.0f)
	{
		ViewYawDegrees =
			std::fmod(
				ViewYawDegrees,
				360.0f
			);
	}
	else if (ViewYawDegrees < -360.0f)
	{
		ViewYawDegrees =
			std::fmod(
				ViewYawDegrees,
				360.0f
			);
	}
}

void RmlFrontEndCharacterPreview::Move(
	float DeltaPixelsX,
	float DeltaPixelsY
)
{
	ViewHorizontalOffset +=
		DeltaPixelsX * 0.0022f;

	ViewVerticalOffset -=
		DeltaPixelsY * 0.0022f;

	ViewHorizontalOffset =
		std::clamp(
			ViewHorizontalOffset,
			-0.42f,
			0.42f
		);

	ViewVerticalOffset =
		std::clamp(
			ViewVerticalOffset,
			-0.32f,
			0.32f
		);
}

void RmlFrontEndCharacterPreview::Zoom(
	float WheelSteps
)
{
	ViewDistanceScale -=
		WheelSteps * 0.08f;

	ViewDistanceScale =
		std::clamp(
			ViewDistanceScale,
			0.62f,
			1.55f
		);
}

void RmlFrontEndCharacterPreview::ResetView()
{
	ViewYawDegrees = 0.0f;
	ViewDistanceScale = 1.0f;

	ViewHorizontalOffset = 0.0f;
	ViewVerticalOffset = 0.0f;

	if (Player)
	{
		Player->m_fPlayerRotationTarget =
			ViewYawDegrees;

		Player->m_fPlayerRotation =
			ViewYawDegrees;
	}
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
		ViewYawDegrees;

	Player->m_fPlayerRotation =
		ViewYawDegrees;

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
	 * Рендерим только персонажа.
	 *
	 * WZ_FrontEndLighting нужен как техническая сцена для создания obj_Player,
	 * но сама карта не должна попадать в rml://character-preview.
	 */
	std::vector<FRmlPreviewObjectDrawBackup> HiddenObjects;

	HideWorldObjectsForRmlCharacterPreview(
		Player,
		HiddenObjects
	);

	CurRenderPipeline->PreRender();
	CurRenderPipeline->Render();
	CurRenderPipeline->AppendPostFXes();

	RestoreWorldObjectsForRmlCharacterPreview(
		HiddenObjects
	);

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
		0.30f;

	float PortraitScaleX =
		0.30f;

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

	PortraitScaleX =
		std::clamp(
			PortraitScaleX,
			0.12f,
			0.50f
		);

	PFX_Copy::Settings PortraitCopySettings;

	PortraitCopySettings.TexScaleX =
		PortraitScaleX;

	PortraitCopySettings.TexScaleY =
		PortraitScaleY;

	PortraitCopySettings.TexOffsetX =
		(
			1.0f -
			PortraitScaleX
		) * 0.5f;

	/*
	 * Верхняя центральная область полного кадра.
	 * Голова и плечи находятся по центру портретной рамки.
	 */
	PortraitCopySettings.TexOffsetY =
		0.055f;

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