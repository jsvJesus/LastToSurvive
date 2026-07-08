#include "r3dPCH.h"
#include "r3d.h"
#include "GameCommon.h"

#include "BulletShellManager.h"

BulletShell::BulletShell()
{
	m_Type = BST_Pistol;
	m_physObj = 0;
	m_Pos.Assign(0,0,0);
	D3DXMatrixIdentity(&m_RenderMatrix);
}

BulletShell::~BulletShell()
{
	r3d_assert(m_physObj==0);
}

const float BULLET_LIFETIME = 0.5f;

void BulletShell::Init(const r3dPoint3D& pos, const r3dPoint3D& vel,  const PhysicsObjectConfig& config, r3dMesh* mesh, BulletShellType type, const D3DXMATRIX& rotation)
{
	Destroy();

	m_physObj = BasePhysicsObject::CreateDynamicObject(config, this, &pos, &mesh->localBBox.Size, mesh, &rotation);
	r3d_assert(m_physObj!=0);

	m_physObj->addImpulse(vel);
	
	D3DXMatrixTranslation(&m_RenderMatrix, pos.x, pos.y, pos.z);

	m_Type = type;
	m_StartTime = r3dGetTime();
	m_PlayedBrassSound = false;
}

void BulletShell::Destroy()
{
	SAFE_DELETE(m_physObj);
}

void BulletShell::Update()
{
	if(m_physObj)
	{
		if(!m_physObj->IsSleeping())
		{
			r3dVector physPos = m_physObj->GetPosition();
			// sometimes physics returns QNAN position, not sure why
			if(r3d_float_isFinite(physPos.x) && r3d_float_isFinite(physPos.y) && r3d_float_isFinite(physPos.z))
			{
				m_Pos = physPos;
				D3DXMatrixTranslation(&m_RenderMatrix, m_Pos.x, m_Pos.y, m_Pos.z);

				D3DXMATRIX mat = m_physObj->GetRotation();
				mat.m[3][0] = 0.0f; mat.m[3][1] = 0.0f; mat.m[3][2] = 0.0f; mat.m[3][3] = 1.0f;
				m_RenderMatrix = mat * m_RenderMatrix;
 			}
		}
		if((r3dGetTime() - m_StartTime) > BULLET_LIFETIME)
			Destroy();
	}
}

void BulletShell::OnCollide(PhysicsCallbackObject *obj, CollisionInfo &trace)
{
	static int ejectBrassSoundID = -1;
	if(ejectBrassSoundID==-1)
		ejectBrassSoundID = SoundSys.GetEventIDByPath("Sounds/Misc/EjectBrass");
	if(!m_PlayedBrassSound)
	{
		SoundSys.PlayAndForget(ejectBrassSoundID, m_Pos);
		m_PlayedBrassSound = true;
	}
}

//////////////////////////////////////////////////////////////////////////
static r3dMesh* LoadShellMesh(const char* const* paths, int numPaths)
{
	for(int i = 0; i < numPaths; ++i)
	{
		bool fileExists = r3dFileExists(paths[i]);
		if(!fileExists)
		{
			char scbPath[MAX_PATH];
			r3dscpy(scbPath, paths[i]);
			char* ext = strrchr(scbPath, '.');
			if(ext)
			{
				r3dscpy(ext, ".scb");
				fileExists = r3dFileExists(scbPath);
			}
		}

		if(!fileExists)
			continue;

		return r3dGOBAddMesh(paths[i], true, false, false, true);
	}

	return NULL;
}

BulletShellMngr::BulletShellMngr()
{
	m_numActiveShells = 0;

	const char* pistolShells[] = {
		"Data/ObjectsDepot/ROTB_Weapons_HG/Shell_Pistol.sco",
		"Data/ObjectsDepot/SS_Ammo/Shell_Pistol.sco",
	};
	const char* rifleShells[] = {
		"Data/ObjectsDepot/ROTB_Weapons_ASR/Shell_Rifle.sco",
		"Data/ObjectsDepot/SS_Ammo/Shell_Rifle.sco",
	};
	const char* shotgunShells[] = {
		"Data/ObjectsDepot/ROTB_Weapons_SHG/Shell_Shotgun.sco",
		"Data/ObjectsDepot/SS_Ammo/Shell_Shotgun.sco",
	};

	m_shellMeshes[0] = LoadShellMesh(pistolShells, _countof(pistolShells));
	m_shellMeshes[1] = LoadShellMesh(rifleShells, _countof(rifleShells));
	m_shellMeshes[2] = LoadShellMesh(shotgunShells, _countof(shotgunShells));

	for(int i = 0; i < BST_NumElements; ++i)
	{
		if(!m_shellMeshes[i])
			r3dOutToLog("BulletShellMngr: shell mesh %d is missing, shell ejection disabled for this type\n", i);
	}

	for(int i = 0; i < BST_NumElements; ++i)
	{
		if(!m_shellMeshes[i])
			continue;

		GameObject::LoadPhysicsConfig(m_shellMeshes[i]->FileName.c_str(), m_shellPhysConfigs[i]);
		if(!m_shellPhysConfigs[i].ready)
		{
			r3dOutToLog("BulletShellMngr: shell physics config is missing for '%s'\n", m_shellMeshes[i]->FileName.c_str());
			m_shellMeshes[i] = NULL;
			continue;
		}

		m_shellPhysConfigs[i].group = PHYSCOLL_TINY_GEOMETRY;
		m_shellPhysConfigs[i].isFastMoving = true;
	}

};

BulletShellMngr::~BulletShellMngr()
{
	for(int i=0; i<MAX_SHELLS; ++i)
		m_Shells[i].Destroy();
}

void BulletShellMngr::AddShell(const r3dPoint3D& pos, const r3dPoint3D& vel, const D3DXMATRIX& rotation, BulletShellType shellType)
{
	if(shellType < 0 || shellType >= BST_NumElements)
		return;

	if(!m_shellMeshes[(int)shellType] || !m_shellPhysConfigs[(int)shellType].ready)
		return;

	// PT: sometimes animation has QNAN in it, not sure where is it coming from. Seems like in some cases quaternion becomes fucked up and not able to transform it into a matrix. As all other data in skeleton is fine
	if(!(r3d_float_isFinite(vel.x) && r3d_float_isFinite(vel.y) && r3d_float_isFinite(vel.z)))
		return;

	m_Shells[m_numActiveShells].Init(pos, vel, m_shellPhysConfigs[(int)shellType], m_shellMeshes[(int)shellType], shellType, rotation);
	++m_numActiveShells;
	if(m_numActiveShells == MAX_SHELLS)
		m_numActiveShells = 0;
}

void BulletShellMngr::Update()
{
	for(int i=0; i<MAX_SHELLS; ++i)
		if(m_Shells[i].Active())
			m_Shells[i].Update();
}

struct BulletShellDeferredRenderable : MeshDeferredRenderable
{
	void Init()
	{
		DrawFunc = Draw;
	}

	static void Draw( Renderable* RThis, const r3dCamera& Cam )
	{
		R3DPROFILE_FUNCTION("BulletShellDeferredRenderable");
		BulletShellDeferredRenderable* This = static_cast< BulletShellDeferredRenderable* >( RThis );

		This->Mesh->SetVSConsts( This->Parent->getDrawMatrix() );
		MeshDeferredRenderable::Draw( RThis, Cam );
	}

	BulletShell* Parent;
};

void BulletShellMngr::AppendRenderables(RenderArray(&render_arrays)[rsCount], const r3dCamera& Cam)
{
	R3DPROFILE_FUNCTION("BulletShellMngr::AppendRenderables");
	
	COMPILE_ASSERT( sizeof(BulletShellDeferredRenderable) <= MAX_RENDERABLE_SIZE );
	for(int i=0; i<MAX_SHELLS; ++i)
	{
		if(!m_Shells[i].Active())
			continue;

		r3dMesh* shellMesh = m_shellMeshes[(int)m_Shells[i].m_Type];
		if(!shellMesh)
			continue;

		float distSq = (Cam - m_Shells[i].getPosition()).LengthSq();
		float dist = sqrtf( distSq );

		int idist = R3D_MIN( (int)dist, 0xffff );

		uint32_t prevCount = render_arrays[ rsFillGBuffer ].Count();
		shellMesh->AppendRenderablesDeferred( render_arrays[ rsFillGBuffer ], r3dColor::white);
		for( uint32_t j = prevCount, e = render_arrays[ rsFillGBuffer ].Count(); j < e; j++ )
		{
			BulletShellDeferredRenderable& rend = static_cast<BulletShellDeferredRenderable&>( render_arrays[ rsFillGBuffer ][j] ) ;
			rend.SortValue |= idist ;
			rend.Init() ;
			rend.Parent = &m_Shells[i];

		}
	}
}

