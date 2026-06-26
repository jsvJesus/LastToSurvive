#include "r3dPCH.h"
#include "r3d.h"

#include "ZombieNavAgent.h"
#include "../../GameEngine/ai/AutodeskNav/AutodeskNavMesh.h"
#include "../../GameEngine/gameobjects/PhysXWorld.h"
#include "../../GameEngine/gameobjects/PhysObj.h"

#if ENABLE_AUTODESK_NAVIGATION

ZombieNavAgent::ZombieNavAgent()
{
}

ZombieNavAgent::~ZombieNavAgent()
{
}

ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D& pos)
{
	r3d_assert(gAutodeskNavMesh.GetWorld());

	ZombieNavAgent* a = new ZombieNavAgent();
	a->Init(gAutodeskNavMesh.GetWorld(), pos);
	gAutodeskNavMesh.AddNavAgent(a);

	return a;
}

void DeleteZombieNavAgent(ZombieNavAgent* a)
{
	gAutodeskNavMesh.DeleteNavAgent(a);
}

#else

ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D& pos)
{
	(void)pos;
	return NULL;
}

void DeleteZombieNavAgent(ZombieNavAgent* a)
{
	(void)a;
}

#endif
