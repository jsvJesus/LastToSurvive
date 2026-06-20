#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"

#include "RENDERING/DX11/RenderDX11World.h"

#include "GameObjects/GameObj.h"
#include "RENDERING/Deffered/RenderDeffered.h"
#include "RENDERING/DX11/RenderDX11.h"
#include "RENDERING/DX11/RenderDX11MeshRenderer.h"
#include "r3dMat.h"

#include <algorithm>

extern RenderArray g_render_arrays[rsCount];

namespace
{
	r3dDX11Texture2D g_DX11ShadowDepthTargets[NumShadowSlices];
	int g_DX11ShadowDepthSizes[NumShadowSlices] = {};

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

	const D3DXMATRIX& GetRenderableWorldMatrix(const MeshShadowRenderable& renderable)
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

	D3DXMATRIX BuildDX11ViewProjectionMatrix(const r3dDX11Renderer& renderer, const r3dCamera& camera, bool allowRendererMatrix)
	{
		if (allowRendererMatrix && r3dRenderer)
			return r3dRenderer->ViewProjMatrix;

		D3DXMATRIX view;
		D3DXMATRIX projection;

		BuildDX11ViewMatrix(camera, view);
		BuildDX11ProjectionMatrix(camera, renderer, projection);

		return view * projection;
	}

	r3dCamera BuildDX11FirstPersonCamera(const r3dCamera& camera)
	{
		r3dCamera firstPersonCamera = camera;
		firstPersonCamera.NearClip = r_first_person_render_z_start->GetFloat();
		firstPersonCamera.FarClip = r_first_person_render_z_end->GetFloat();
		firstPersonCamera.FOV = r_first_person_fov->GetFloat();
		return firstPersonCamera;
	}

	bool EnsureDX11ShadowDepthTarget(r3dDX11Renderer& renderer, int sliceIndex)
	{
		if (sliceIndex < 0 || sliceIndex >= NumShadowSlices)
			return false;

		int size = static_cast<int>(ShadowSlices[sliceIndex].shadowMapSize);
		if (size <= 0 && r_dir_sm_size)
			size = r_dir_sm_size->GetInt();
		size = std::max(64, std::min(size, 8192));

		if (g_DX11ShadowDepthTargets[sliceIndex].IsValid() && g_DX11ShadowDepthSizes[sliceIndex] == size)
			return true;

		char debugName[64];
		sprintf(debugName, "DX11.ShadowDepth.%d", sliceIndex);
		if (!g_DX11ShadowDepthTargets[sliceIndex].Create(
				renderer.GetDevice().GetDevice(),
				size,
				size,
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				R3D_DX11_BIND_DEPTH_STENCIL,
				debugName))
		{
			g_DX11ShadowDepthSizes[sliceIndex] = 0;
			return false;
		}

		g_DX11ShadowDepthSizes[sliceIndex] = size;
		return true;
	}

	bool IsAlphaTestedShadowRenderable(const MeshShadowRenderable& renderable)
	{
		if (!renderable.Mesh)
			return false;

		if (renderable.BatchIdx >= 0 && renderable.BatchIdx < renderable.Mesh->NumMatChunks)
		{
			const r3dMaterial* material = renderable.Mesh->MatChunks[renderable.BatchIdx].Mat;
			return material && (material->Flags & R3D_MAT_HASALPHA) != 0;
		}

		return renderable.Mesh->HasAlphaTextures != 0;
	}

	bool IsDX11MeshPointerUsable(const r3dMesh* mesh)
	{
		if (!mesh)
			return false;

		__try
		{
			if (!mesh->IsLoaded())
				return false;

			if (mesh->NumMatChunks < 0 || mesh->NumMatChunks > r3dMesh::ConstNumMatChunks)
				return false;

		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}

		return true;
	}

	void DrawDX11GBufferQueue(
		r3dDX11Renderer& renderer,
		r3dDX11GBufferPass& pass,
		eRenderStageID queueId,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats& stats
	)
	{
		RenderArray& queue = g_render_arrays[queueId];

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];
			++stats.TotalRenderables;

			MeshDeferredRenderable* meshRenderable = r3dGetMeshDeferredRenderable(&renderable);
			if (!meshRenderable)
			{
				++stats.SkippedUnsupported;
				continue;
			}

			++stats.MeshRenderables;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh) || meshRenderable->BatchIdx < 0)
			{
				++stats.SkippedFailed;
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
				++stats.DrawnMeshes;
			}
			else
			{
				++stats.SkippedFailed;
			}
		}
	}

	void DrawDX11DepthOnlyQueue(
		r3dDX11Renderer& renderer,
		r3dDX11DepthOnlyPass& pass,
		eRenderStageID queueId,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats& stats
	)
	{
		RenderArray& queue = g_render_arrays[queueId];

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			MeshDeferredRenderable* meshRenderable = r3dGetMeshDeferredRenderable(&renderable);
			if (!meshRenderable)
				continue;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh) || meshRenderable->BatchIdx < 0)
				continue;

			const D3DXMATRIX& world = GetRenderableWorldMatrix(*meshRenderable);

			if (r3dDX11DrawMeshDepthOnlyBatch(
					renderer.GetDevice().GetDevice(),
					renderer.GetTextureLibrary(),
					pass,
					*meshRenderable->Mesh,
					static_cast<unsigned int>(meshRenderable->BatchIdx),
					world,
					viewProj,
					meshRenderable->DX11Skeleton))
			{
				++stats.DepthDrawnMeshes;
			}
		}
	}

	void DrawDX11ShadowQueue(
		r3dDX11Renderer& renderer,
		r3dDX11DepthOnlyPass& pass,
		eRenderStageID queueId,
		const D3DXMATRIX& viewProj,
		r3dDX11WorldRenderStats& stats
	)
	{
		RenderArray& queue = g_render_arrays[queueId];

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];
			++stats.ShadowRenderables;

			MeshShadowRenderable* meshRenderable = r3dGetMeshShadowRenderable(&renderable);
			if (!meshRenderable)
			{
				++stats.ShadowSkippedUnsupported;
				continue;
			}

			++stats.ShadowMeshRenderables;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh))
			{
				++stats.ShadowSkippedFailed;
				continue;
			}

			if (IsAlphaTestedShadowRenderable(*meshRenderable))
				++stats.ShadowAlphaTested;

			const D3DXMATRIX& world = GetRenderableWorldMatrix(*meshRenderable);
			bool drawn = false;

			if (meshRenderable->BatchIdx >= 0)
			{
				drawn = r3dDX11DrawMeshDepthOnlyBatch(
					renderer.GetDevice().GetDevice(),
					renderer.GetTextureLibrary(),
					pass,
					*meshRenderable->Mesh,
					static_cast<unsigned int>(meshRenderable->BatchIdx),
					world,
					viewProj,
					meshRenderable->DX11Skeleton);
			}
			else
			{
				drawn = r3dDX11DrawMeshDepthOnly(
					renderer.GetDevice().GetDevice(),
					renderer.GetTextureLibrary(),
					pass,
					*meshRenderable->Mesh,
					world,
					viewProj,
					meshRenderable->DX11Skeleton);
			}

			if (drawn)
				++stats.ShadowDrawnMeshes;
			else
				++stats.ShadowSkippedFailed;
		}
	}

	void RenderDX11ShadowQueues(r3dDX11Renderer& renderer, r3dDX11DepthOnlyPass& pass, r3dDX11WorldRenderStats& stats)
	{
		int activeSlices = r_active_shadow_slices ? r_active_shadow_slices->GetInt() : NumShadowSlices;
		activeSlices = std::max(0, std::min(activeSlices, static_cast<int>(NumShadowSlices)));

		for (int i = 0; i < activeSlices; ++i)
		{
			if (!EnsureDX11ShadowDepthTarget(renderer, i))
			{
				++stats.ShadowSkippedFailed;
				continue;
			}

			r3dDX11Texture2D& shadowDepth = g_DX11ShadowDepthTargets[i];
			if (!pass.BeginDepthTarget(shadowDepth.GetDSV(), shadowDepth.GetWidth(), shadowDepth.GetHeight(), true))
			{
				++stats.ShadowSkippedFailed;
				continue;
			}

			const D3DXMATRIX viewProj = ShadowSlices[i].lightView * ShadowSlices[i].lightProj;
			DrawDX11ShadowQueue(renderer, pass, static_cast<eRenderStageID>(rsCreateSM + i), viewProj, stats);
			pass.EndDepthTarget();
			++stats.ShadowSlicesRendered;
		}
	}
}

void r3dDX11ResetWorldRenderStats(r3dDX11WorldRenderStats& stats)
{
	r3dResetMeshDeferredDX11WorldMatrices();

	stats.TotalRenderables = 0;
	stats.MeshRenderables = 0;
	stats.DepthDrawnMeshes = 0;
	stats.DrawnMeshes = 0;
	stats.SkippedUnsupported = 0;
	stats.SkippedFailed = 0;
	stats.ShadowRenderables = 0;
	stats.ShadowMeshRenderables = 0;
	stats.ShadowDrawnMeshes = 0;
	stats.ShadowAlphaTested = 0;
	stats.ShadowSkippedUnsupported = 0;
	stats.ShadowSkippedFailed = 0;
	stats.ShadowSlicesRendered = 0;
}

bool r3dDX11RenderWorldGBuffer(r3dDX11Renderer& renderer, const r3dCamera& camera, r3dDX11WorldRenderStats* stats)
{
	r3dDX11WorldRenderStats localStats;
	if (!stats)
		stats = &localStats;

	r3dDX11ResetWorldRenderStats(*stats);

	if (!renderer.IsInitialized())
		return false;

	r3dDX11DepthOnlyPass& depthPass = renderer.GetDepthOnlyPass();
	r3dDX11GBufferPass& pass = renderer.GetGBufferPass();
	r3dDX11GBufferResources& gbuffer = renderer.GetGBufferResources();

	const D3DXMATRIX viewProj = BuildDX11ViewProjectionMatrix(renderer, camera, true);
	const r3dCamera firstPersonCamera = BuildDX11FirstPersonCamera(camera);
	const D3DXMATRIX firstPersonViewProj = BuildDX11ViewProjectionMatrix(renderer, firstPersonCamera, false);

	RenderDX11ShadowQueues(renderer, depthPass, *stats);

	if (!depthPass.Begin(gbuffer))
		return false;

	DrawDX11DepthOnlyQueue(renderer, depthPass, rsFillGBuffer, viewProj, *stats);
	DrawDX11DepthOnlyQueue(renderer, depthPass, rsFillGBufferFirstPerson, firstPersonViewProj, *stats);

	depthPass.End(gbuffer);

	if (!pass.Begin(gbuffer, false))
		return false;

	DrawDX11GBufferQueue(renderer, pass, rsFillGBuffer, viewProj, *stats);
	DrawDX11GBufferQueue(renderer, pass, rsFillGBufferFirstPerson, firstPersonViewProj, *stats);

	pass.End(gbuffer);

	return stats->DrawnMeshes > 0 || stats->TotalRenderables == 0;
}
