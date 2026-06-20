#pragma once

class r3dCamera;
class r3dDX11Renderer;

struct r3dDX11WorldRenderStats
{
	unsigned int TotalRenderables;
	unsigned int MeshRenderables;

	unsigned int DepthTotalRenderables;
	unsigned int DepthMeshRenderables;
	unsigned int DepthStaticMeshes;
	unsigned int DepthSkinnedMeshes;
	unsigned int DepthAlphaTestedMeshes;
	unsigned int DepthFirstPersonMeshes;
	unsigned int DepthDrawnMeshes;
	unsigned int DepthSkippedUnsupported;
	unsigned int DepthSkippedFailed;

	unsigned int DrawnMeshes;
	unsigned int SkippedUnsupported;
	unsigned int SkippedFailed;

	unsigned int ShadowRenderables;
	unsigned int ShadowMeshRenderables;
	unsigned int ShadowDrawnMeshes;
	unsigned int ShadowAlphaTested;
	unsigned int ShadowSkippedUnsupported;
	unsigned int ShadowSkippedFailed;
	unsigned int ShadowSlicesRendered;
};

void r3dDX11ResetWorldRenderStats(r3dDX11WorldRenderStats& stats);

bool r3dDX11RenderWorldDepthOnly(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats* stats = 0,
	bool clearDepth = true
);

bool r3dDX11RenderWorldGBuffer(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats* stats = 0
);