#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"

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

	float ClampCameraFov(float fov)
	{
		if (fov < 1.0f || fov > 179.0f)
			return 60.0f;

		return fov;
	}

	float BuildSafeAspect(const r3dCamera& camera, const r3dDX11Renderer& renderer)
	{
		if (camera.Aspect > 0.01f)
			return camera.Aspect;

		const int width = renderer.GetWidth();
		const int height = renderer.GetHeight();

		if (width > 0 && height > 0)
			return static_cast<float>(width) / static_cast<float>(height);

		return 16.0f / 9.0f;
	}

	void BuildDX11ViewMatrix(const r3dCamera& camera, D3DXMATRIX& outView)
	{
		D3DXVECTOR3 eye;
		eye.x = camera.X;
		eye.y = camera.Y;
		eye.z = camera.Z;

		D3DXVECTOR3 at;
		at.x = camera.X + camera.vPointTo.X * 10.0f;
		at.y = camera.Y + camera.vPointTo.Y * 10.0f;
		at.z = camera.Z + camera.vPointTo.Z * 10.0f;

		D3DXVECTOR3 up;
		up.x = camera.vUP.x;
		up.y = camera.vUP.y;
		up.z = camera.vUP.z;

		if (fabsf(up.x) < 0.0001f && fabsf(up.y) < 0.0001f && fabsf(up.z) < 0.0001f)
		{
			up.x = 0.0f;
			up.y = 1.0f;
			up.z = 0.0f;
		}

		const float dirX = at.x - eye.x;
		const float dirY = at.y - eye.y;
		const float dirZ = at.z - eye.z;
		const float dirLenSq = dirX * dirX + dirY * dirY + dirZ * dirZ;

		if (dirLenSq < 0.0001f)
		{
			at.x = eye.x;
			at.y = eye.y;
			at.z = eye.z + 10.0f;
		}

		D3DXMatrixLookAtLH(&outView, &eye, &at, &up);
	}

	void BuildDX11ProjectionMatrix(const r3dCamera& camera, const r3dDX11Renderer& renderer, D3DXMATRIX& outProjection)
	{
		float nearClip = camera.NearClip;
		float farClip = camera.FarClip;

		if (nearClip <= 0.0f)
			nearClip = 0.1f;

		if (farClip <= nearClip + 1.0f)
			farClip = 10000.0f;

		switch (camera.ProjectionType)
		{
		case r3dCamera::PROJTYPE_ORTHO:
			{
				float width = camera.Width;
				float height = camera.Height;

				if (width <= 0.0f)
					width = static_cast<float>(renderer.GetWidth());

				if (height <= 0.0f)
					height = static_cast<float>(renderer.GetHeight());

				if (width <= 0.0f)
					width = 1920.0f;

				if (height <= 0.0f)
					height = 1080.0f;

				D3DXMatrixOrthoLH(
					&outProjection,
					width,
					height,
					nearClip,
					farClip
				);
			}
			break;

		case r3dCamera::PROJTYPE_CUSTOM:
			D3DXMatrixIdentity(&outProjection);
			break;

		case r3dCamera::PROJTYPE_PRESPECTIVE:
		default:
			D3DXMatrixPerspectiveFovLH(
				&outProjection,
				ClampCameraFov(camera.FOV) * D3DX_PI / 180.0f,
				BuildSafeAspect(camera, renderer),
				nearClip,
				farClip
			);
			break;
		}
	}

	D3DXMATRIX BuildDX11ViewProjectionMatrix(const r3dDX11Renderer& renderer, const r3dCamera& camera)
	{
		if (r3dRenderer)
			return r3dRenderer->ViewProjMatrix;

		D3DXMATRIX view;
		D3DXMATRIX projection;

		BuildDX11ViewMatrix(camera, view);
		BuildDX11ProjectionMatrix(camera, renderer, projection);

		return view * projection;
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
	const D3DXMATRIX viewProj = BuildDX11ViewProjectionMatrix(renderer, camera);

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

	return stats->DrawnMeshes > 0 || stats->TotalRenderables == 0;
}