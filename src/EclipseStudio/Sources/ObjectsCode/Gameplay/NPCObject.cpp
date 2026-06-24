#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"
#include "NPCObject.h"
#include "../AI/AI_Player.H"

#ifndef WO_SERVER
#endif

static bool NPC_UseDX11WorldPath()
{
#ifndef WO_SERVER
	return false;
#else
	return false;
#endif
}

NPCObject::NPCObject() :
animated_(false),
physXObstacleIndex_(-1)
{
	ObjTypeFlags |= OBJTYPE_NPC;
}

NPCObject::~NPCObject()
{
}

BOOL NPCObject::OnCreate()
{
	m_spawnPos = GetPosition();

	r3d_assert(physXObstacleIndex_ == -1);
	physXObstacleIndex_ = AcquirePlayerObstacle(GetPosition());

	animation_.Init(&skeleton_, &animationPool_);
	SetIdleAnimation();

	return parent::OnCreate();
}

BOOL NPCObject::OnDestroy()
{
	ReleasePlayerObstacle(&physXObstacleIndex_);
	return parent::OnDestroy();
}

void NPCObject::OnPreRender()
{
	animation_.GetCurrentSkeleton()->SetShaderConstants();
}

void NPCObject::SetIdleAnimation()
{
	int aid = -1;
	std::string filename = FileName.c_str();
	if (filename.find_first_of("Civ_") != std::string::npos)
	{
		if (filename.find_first_of("_01") != std::string::npos)
		{
			aid = AddAnimation("Civ_Idle_01");
		}
		else if (filename.find_first_of("_02") != std::string::npos)
		{
			aid = AddAnimation("Civ_Idle_02");
		}
	}

	if (aid != -1)
	{
		// start with randomized frame
		const int tr = animation_.StartAnimation(aid, ANIMFLAG_Looped, 0.0f, 0.0f, 0.0f);
		r3dAnimation::r3dAnimInfo* ai = animation_.GetTrack(tr);
		ai->fCurFrame = u_GetRandom(0, (float)ai->pAnim->NumFrames - 1);

		animated_ = true;
	}
}

void NPCObject::UpdateAnimation()
{
	if (animated_)
	{
		const float timePassed = r3dGetFrameTime();
		D3DXMATRIX mr;
		D3DXMatrixIdentity(&mr);
		animation_.Update(timePassed, r3dPoint3D(0,0,0), mr);
		animation_.Recalc();
	}
}

BOOL NPCObject::Update()
{
	UpdateAnimation();

	return parent::Update();
}

struct NPCDX11DeferredRenderable : MeshDeferredRenderable
{
	void Init()
	{
		DrawFunc = Draw;
	}

	static void Draw(Renderable* RThis, const r3dCamera& Cam)
	{
		NPCDX11DeferredRenderable* This = static_cast<NPCDX11DeferredRenderable*>(RThis);

		r3dApplyPreparedMeshVSConsts(This->Parent->preparedVSConsts);
		This->Parent->OnPreRender();

		MeshDeferredRenderable::Draw(RThis, Cam);
	}

	NPCObject* Parent;
};

struct NPCDX11ShadowRenderable : MeshShadowRenderable
{
	void Init()
	{
		DrawFunc = Draw;
	}

	static void Draw(Renderable* RThis, const r3dCamera& Cam)
	{
		NPCDX11ShadowRenderable* This = static_cast<NPCDX11ShadowRenderable*>(RThis);

		r3dApplyPreparedMeshVSConsts(This->Parent->preparedVSConsts);
		This->Parent->OnPreRender();

		This->SubDrawFunc(RThis, Cam);
	}

	NPCObject* Parent;
};

void NPCObject::AppendRenderables(RenderArray (&render_arrays)[rsCount], const r3dCamera& Cam)
{
	if (!NPC_UseDX11WorldPath() || !animated_)
	{
		parent::AppendRenderables(render_arrays, Cam);
		return;
	}

	const float distSq = (Cam - GetPosition()).LengthSq();
	const int meshLodIndex = ChoseMeshLOD(distSq);

	r3dMesh* mesh = MeshLOD[meshLodIndex];

	if (!mesh || !mesh->IsDrawable())
		return;

	r3dSkeleton* skeleton = animation_.GetCurrentSkeleton();

	if (!skeleton)
	{
		parent::AppendRenderables(render_arrays, Cam);
		return;
	}

	animation_.Recalc();

	const float dist = sqrtf(distSq);
	const int idist = R3D_MIN((int)dist, 0xffff);

	uint32_t prevCount = render_arrays[rsFillGBuffer].Count();

	mesh->AppendRenderablesDeferred(render_arrays[rsFillGBuffer], m_ObjectColor);

	for (uint32_t i = prevCount, e = render_arrays[rsFillGBuffer].Count(); i < e; ++i)
	{
		NPCDX11DeferredRenderable& rend =
			static_cast<NPCDX11DeferredRenderable&>(render_arrays[rsFillGBuffer][i]);

		rend.Init();
		rend.InitDX11(&GetTransformMatrix(), skeleton);
		rend.Parent = this;
		rend.DX11WorldTransform = &GetTransformMatrix();
		rend.DX11Skeleton = skeleton;
		rend.SortValue |= idist;
	}

	uint32_t prevTranspCount = render_arrays[rsDrawTransparents].Count();

	mesh->AppendTransparentRenderables(
		render_arrays[rsDrawTransparents],
		m_ObjectColor,
		dist,
		0
	);

	for (uint32_t i = prevTranspCount, e = render_arrays[rsDrawTransparents].Count(); i < e; ++i)
	{
		NPCDX11DeferredRenderable& rend =
			static_cast<NPCDX11DeferredRenderable&>(render_arrays[rsDrawTransparents][i]);

		rend.Init();
		rend.InitDX11(&GetTransformMatrix(), skeleton);
		rend.Parent = this;
		rend.DX11WorldTransform = &GetTransformMatrix();
		rend.DX11Skeleton = skeleton;
		rend.SortValue |= idist;
	}
}

void NPCObject::AppendShadowRenderables(RenderArray& rarr, const r3dCamera& Cam)
{
	if (!NPC_UseDX11WorldPath() || !animated_)
	{
		parent::AppendShadowRenderables(rarr, Cam);
		return;
	}

	const float distSq = (gCam - GetPosition()).LengthSq();
	const float dist = sqrtf(distSq);
	const UINT32 idist = (UINT32)R3D_MIN(dist * 64.0f, (float)0x3fffffff);

	const int meshLodIndex = ChoseMeshLOD(distSq);
	r3dMesh* mesh = MeshLOD[meshLodIndex];

	if (!mesh || !mesh->IsDrawable())
		return;

	r3dSkeleton* skeleton = animation_.GetCurrentSkeleton();

	if (!skeleton)
	{
		parent::AppendShadowRenderables(rarr, Cam);
		return;
	}

	animation_.Recalc();

	const uint32_t prevCount = rarr.Count();

	mesh->AppendShadowRenderables(rarr);

	for (uint32_t i = prevCount, e = rarr.Count(); i < e; ++i)
	{
		NPCDX11ShadowRenderable& rend =
			static_cast<NPCDX11ShadowRenderable&>(rarr[i]);

		rend.Init();
		rend.InitDX11(&GetTransformMatrix(), skeleton);
		rend.Parent = this;
		rend.SortValue |= idist;
	}
}

bool NPCObject::LoadSkeleton(const std::string& meshFilename)
{
	std::string skeletonFilename = meshFilename.substr(0, meshFilename.length()-3) + "skl";
	if (r3d_access(skeletonFilename.c_str(), 0) == 0)
	{
		skeleton_.LoadBinary(skeletonFilename.c_str());
	}
	return skeleton_.bLoaded ? true : false;
}

int NPCObject::AddAnimation(const std::string& animationName)
{
	static const char* animationDir = GLOBAL_ANIM_FOLDER;

	std::stringstream ss;
	ss << animationDir << "/" << animationName << ".anm";

	int aid = animationPool_.Add(animationName.c_str(), ss.str().c_str());
	if(aid == -1)
	{
		r3dError("can't add %s anim", animationName);
	}
	return aid;
}

BOOL NPCObject::Load(const char* filename)
{
	if(!MeshGameObject::Load(filename))
	{
		return FALSE;
	}

	if (!LoadSkeleton(filename))
	{
		return FALSE;
	}

	return TRUE;
}

