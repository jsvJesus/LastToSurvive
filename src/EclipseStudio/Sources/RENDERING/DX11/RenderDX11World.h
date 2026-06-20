#pragma once

class r3dCamera;
class r3dDX11Renderer;

struct r3dDX11WorldRenderStats
{
	unsigned int TotalRenderables;
	unsigned int MeshRenderables;
	unsigned int DepthDrawnMeshes;
	unsigned int DrawnMeshes;
	unsigned int SkippedUnsupported;
	unsigned int SkippedFailed;
};

void r3dDX11ResetWorldRenderStats(r3dDX11WorldRenderStats& stats);
bool r3dDX11RenderWorldGBuffer(r3dDX11Renderer& renderer, const r3dCamera& camera, r3dDX11WorldRenderStats* stats = 0);
