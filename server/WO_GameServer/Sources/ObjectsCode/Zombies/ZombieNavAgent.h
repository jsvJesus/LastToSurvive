#pragma once

#include "../../GameEngine/ai/AutodeskNav/AutodeskNavAgent.h"

#if ENABLE_AUTODESK_NAVIGATION

class ZombieNavAgent : public AutodeskNavAgent
{
public:

public:
	friend ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D &pos);
	friend void DeleteZombieNavAgent(ZombieNavAgent* a);
	
protected:
	ZombieNavAgent();
	~ZombieNavAgent();
}; 

//////////////////////////////////////////////////////////////////////////

ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D &pos);
void DeleteZombieNavAgent(ZombieNavAgent* a);

#else

class ZombieNavAgent
{
};

ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D &pos);
void DeleteZombieNavAgent(ZombieNavAgent* a);

#endif
