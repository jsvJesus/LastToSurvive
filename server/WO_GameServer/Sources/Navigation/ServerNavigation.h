#pragma once

#include "Navigation/NavigationMesh.h"

namespace server::navigation
{
	class ServerNavigation final
	{
	public:
		bool LoadFromFile(const char* navigationFilePath);
		void Clear() noexcept;

		bool IsReady() const noexcept;
		engine::navigation::NavigationMesh& GetMesh() noexcept;
		const engine::navigation::NavigationMesh& GetMesh() const noexcept;

	private:
		engine::navigation::NavigationMesh mesh_;
	};

	ServerNavigation& GetServerNavigation() noexcept;
}
