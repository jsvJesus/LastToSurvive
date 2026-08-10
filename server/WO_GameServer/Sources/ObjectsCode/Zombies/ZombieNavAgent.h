#pragma once

#if ENABLE_RECAST_NAVIGATION

#include "Navigation/NavigationAgent.h"

class ZombieNavAgent
{
public:
	enum Status
	{
		Idle,
		ComputingPath,
		Moving,
		Arrived,
		PathNotFound,
		Failed
	};

	bool StartMove(const r3dPoint3D& position, float maximumPathRange);
	void StopMove();
	void SetTargetSpeed(float speed);
	void Update(float elapsedSeconds);
	r3dPoint3D GetPosition() const;

	Status m_status;
	r3dPoint3D m_velocity;

public:
	friend ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D &pos);
	friend void DeleteZombieNavAgent(ZombieNavAgent* a);
	
protected:
	ZombieNavAgent(engine::navigation::NavigationMesh& navigationMesh, const r3dPoint3D& position);
	~ZombieNavAgent();

private:
	engine::navigation::NavigationAgent agent_;
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
