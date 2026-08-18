#include "r3dPCH.h"
#include "r3d.h"

#include "ZombieNavAgent.h"
#include "Navigation/ServerNavigation.h"

#if ENABLE_RECAST_NAVIGATION

namespace
{
	engine::math::Vector3 ToLts(const r3dPoint3D& value)
	{
		return engine::math::Vector3(value.x, value.y, value.z);
	}

	r3dPoint3D ToR3d(const engine::math::Vector3& value)
	{
		return r3dPoint3D(value.x, value.y, value.z);
	}
}


ZombieNavAgent::ZombieNavAgent(
	engine::navigation::NavigationMesh& navigationMesh,
	const r3dPoint3D& position)
	: m_status(Idle)
	, m_velocity(0, 0, 0)
	, agent_(navigationMesh)
{
	agent_.SetPosition(ToLts(position));
}

ZombieNavAgent::~ZombieNavAgent()
{
}

bool ZombieNavAgent::StartMove(const r3dPoint3D& position, const float maximumPathRange)
{
	const r3dPoint3D delta = position - GetPosition();
	if (maximumPathRange > 0.0f && delta.Length() > maximumPathRange)
	{
		m_status = PathNotFound;
		return false;
	}

	const bool started = agent_.MoveTo(ToLts(position));
	m_status = started ? Moving : PathNotFound;
	return started;
}

void ZombieNavAgent::StopMove()
{
	agent_.Stop();
	m_velocity = r3dPoint3D(0, 0, 0);
	m_status = Idle;
}

void ZombieNavAgent::SetTargetSpeed(const float speed)
{
	agent_.SetMaximumSpeed(speed);
}

void ZombieNavAgent::Update(const float elapsedSeconds)
{
	const r3dPoint3D previousPosition = GetPosition();
	agent_.Update(elapsedSeconds);
	const r3dPoint3D position = GetPosition();
	m_velocity = elapsedSeconds > 0.0f
		? (position - previousPosition) / elapsedSeconds
		: r3dPoint3D(0, 0, 0);

	switch (agent_.GetStatus())
	{
		case engine::navigation::NavigationAgent::Status::Idle: m_status = Idle; break;
		case engine::navigation::NavigationAgent::Status::Moving: m_status = Moving; break;
		case engine::navigation::NavigationAgent::Status::Arrived: m_status = Arrived; break;
		case engine::navigation::NavigationAgent::Status::PathNotFound: m_status = PathNotFound; break;
	}
}

r3dPoint3D ZombieNavAgent::GetPosition() const
{
	return ToR3d(agent_.GetPosition());
}

ZombieNavAgent* CreateZombieNavAgent(const r3dPoint3D& pos)
{
	server::navigation::ServerNavigation& navigation = server::navigation::GetServerNavigation();
	if (!navigation.IsReady())
		return NULL;

	return new ZombieNavAgent(navigation.GetMesh(), pos);
}

void DeleteZombieNavAgent(ZombieNavAgent* a)
{
	delete a;
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
