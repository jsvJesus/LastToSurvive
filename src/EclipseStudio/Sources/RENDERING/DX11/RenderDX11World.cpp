#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11World.h"

#include "GameObjects/GameObj.h"
#include "RENDERING/DX11/RenderDX11.h"
#include "RENDERING/DX11/RenderDX11MeshRenderer.h"

extern RenderArray g_render_arrays[rsCount];

namespace
{
	const D3DXMATRIX& GetRenderableWorldMatrix(const MeshDeferredRenderable& renderable)
	{
		static D3DXMATRIX identity;
		static bool identityInitialized = false;

		if (!identityInitialized)
		{
			D3DXMatrixIdentity(&identity);
			identityInitialized = true;
		}

		return renderable.DX11WorldTransform ? *renderable.DX11WorldTransform : identity;
	}
}

void r3dDX11ResetWorldRenderStats(r3dDX11WorldRenderStats& stats)
{
	stats.TotalRenderables = 0;
	stats.MeshRenderables = 0;
	stats.DrawnMeshes = 0;
	stats.SkippedUnsupported = 0;
	stats.SkippedFailed = 0;
}

bool r3dDX11RenderWorldGBuffer(r3dDX11Renderer& renderer, const r3dCamera& camera, r3dDX11WorldRenderStats* stats)
{
	r3dDX11WorldRenderStats localStats;
	if (!stats)
		stats = &localStats;

	r3dDX11ResetWorldRenderStats(*stats);

	if (!renderer.IsInitialized())
		return false;

	r3dDX11GBufferPass& pass = renderer.GetGBufferPass();
	r3dDX11GBufferResources& gbuffer = renderer.GetGBufferResources();
	if (!pass.Begin(gbuffer))
		return false;

	RenderArray& queue = g_render_arrays[rsFillGBuffer];
	const D3DXMATRIX viewProj = r3dRenderer->ViewProjMatrix;

	for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
	{
		Renderable& renderable = queue[i];
		++stats->TotalRenderables;

		MeshDeferredRenderable* meshRenderable = r3dGetMeshDeferredRenderable(&renderable);
		if (!meshRenderable)
		{
			++stats->SkippedUnsupported;
			continue;
		}

		++stats->MeshRenderables;

		if (!meshRenderable->Mesh || meshRenderable->BatchIdx < 0)
		{
			++stats->SkippedFailed;
			continue;
		}

		const D3DXMATRIX& world = GetRenderableWorldMatrix(*meshRenderable);
		if (r3dDX11DrawMeshGBufferBatch(
				renderer.GetDevice().GetDevice(),
				renderer.GetTextureLibrary(),
				pass,
				*meshRenderable->Mesh,
				static_cast<unsigned int>(meshRenderable->BatchIdx),
				world,
				viewProj,
				meshRenderable->Color,
				meshRenderable->DX11Skeleton))
		{
			++stats->DrawnMeshes;
		}
		else
		{
			++stats->SkippedFailed;
		}
	}

	pass.End(gbuffer);
	(void)camera;
	return stats->DrawnMeshes > 0 || stats->TotalRenderables == 0;
}
