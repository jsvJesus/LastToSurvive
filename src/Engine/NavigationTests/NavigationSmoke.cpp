#include "Navigation/NavigationMesh.h"

#include <array>
#include <cstdint>
#include <vector>

int main()
{
    using engine::math::Vector3;
    using engine::navigation::BuildConfig;
    using engine::navigation::NavigationMesh;
    using engine::navigation::TriangleMeshView;

    const std::array<Vector3, 4> vertices
    {{
        {-10.0f, 0.0f, -10.0f},
        {-10.0f, 0.0f, 10.0f},
        {10.0f, 0.0f, 10.0f},
        {10.0f, 0.0f, -10.0f}
    }};
    const std::array<std::uint32_t, 6> indices{{0U, 1U, 2U, 0U, 2U, 3U}};

    BuildConfig config{};
    config.agentRadius = 0.0f;
    config.regionMinimumSize = 1.0f;
    config.regionMergeSize = 1.0f;
    config.tileSizeCells = 16;

    NavigationMesh navigationMesh;
    if (!navigationMesh.Build(
            TriangleMeshView{vertices.data(), vertices.size(), indices.data(), indices.size()},
            config))
    {
        return 1;
    }

    std::vector<Vector3> path;
    if (!navigationMesh.FindPath({-5.0f, 0.0f, -5.0f}, {5.0f, 0.0f, 5.0f}, path) ||
        path.size() < 2U)
    {
        return 2;
    }

    constexpr const char* SavedMeshPath = "build/navigation-smoke.nav";
    if (!navigationMesh.Save(SavedMeshPath))
    {
        return 3;
    }

    NavigationMesh loadedMesh;
    if (!loadedMesh.Load(SavedMeshPath) ||
        !loadedMesh.FindPath({-4.0f, 0.0f, 4.0f}, {4.0f, 0.0f, -4.0f}, path))
    {
        return 4;
    }

    return 0;
}
