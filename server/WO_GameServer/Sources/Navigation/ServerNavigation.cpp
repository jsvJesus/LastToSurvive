#include "Navigation/ServerNavigation.h"

namespace server::navigation
{
	bool ServerNavigation::LoadFromFile(const char* const navigationFilePath)
	{
		Clear();
		if (!navigationFilePath || !navigationFilePath[0])
			return false;

		return mesh_.Load(navigationFilePath);
	}

	void ServerNavigation::Clear() noexcept
	{
		mesh_.Clear();
	}

	bool ServerNavigation::IsReady() const noexcept
	{
		return mesh_.IsReady();
	}

	engine::navigation::NavigationMesh& ServerNavigation::GetMesh() noexcept
	{
		return mesh_;
	}

	const engine::navigation::NavigationMesh& ServerNavigation::GetMesh() const noexcept
	{
		return mesh_;
	}

	ServerNavigation& GetServerNavigation() noexcept
	{
		static ServerNavigation navigation;
		return navigation;
	}
}
