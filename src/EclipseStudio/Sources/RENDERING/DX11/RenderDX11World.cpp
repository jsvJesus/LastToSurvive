#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"

#include "RENDERING/DX11/RenderDX11World.h"

#include "GameObjects/GameObj.h"
#include "Editors/CollectionsManager.h"
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

	r3dDX11Texture2D g_DX11TransparentShadowDepthTarget;
	int g_DX11TransparentShadowDepthSize = 0;

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

		float nearClip = r_first_person_render_z_start
			? r_first_person_render_z_start->GetFloat()
			: 0.025f;

		float farClip = r_first_person_render_z_end
			? r_first_person_render_z_end->GetFloat()
			: 4.0f;

		float fov = r_first_person_fov
			? r_first_person_fov->GetFloat()
			: camera.FOV;

		if (nearClip <= 0.0f)
			nearClip = 0.025f;

		if (farClip <= nearClip + 0.001f)
			farClip = nearClip + 4.0f;

		if (fov < 1.0f || fov > 179.0f)
			fov = camera.FOV;

		if (fov < 1.0f || fov > 179.0f)
			fov = 60.0f;

		firstPersonCamera.NearClip = nearClip;
		firstPersonCamera.FarClip = farClip;
		firstPersonCamera.FOV = fov;

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

	bool EnsureDX11TransparentShadowDepthTarget(r3dDX11Renderer& renderer)
	{
		int size = 1024;

		if (r_dir_sm_size)
			size = r_dir_sm_size->GetInt();

		size = std::max(64, std::min(size, 8192));

		if (
			g_DX11TransparentShadowDepthTarget.IsValid() &&
			g_DX11TransparentShadowDepthSize == size
		)
		{
			return true;
		}

		if (!g_DX11TransparentShadowDepthTarget.Create(
				renderer.GetDevice().GetDevice(),
				size,
				size,
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				R3D_DX11_BIND_DEPTH_STENCIL,
				"DX11.TransparentShadowDepth"))
		{
			g_DX11TransparentShadowDepthSize = 0;
			return false;
		}

		g_DX11TransparentShadowDepthSize = size;
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

	bool IsAlphaTestedDeferredRenderable(const MeshDeferredRenderable& renderable)
	{
		if (!renderable.Mesh)
			return false;

		if (
			renderable.BatchIdx >= 0 &&
			renderable.BatchIdx < renderable.Mesh->NumMatChunks
		)
		{
			const r3dMaterial* material =
				renderable.Mesh->MatChunks[renderable.BatchIdx].Mat;

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
		bool firstPersonDepth,
		r3dDX11WorldRenderStats& stats
	)
	{
		RenderArray& queue = g_render_arrays[queueId];

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			++stats.DepthTotalRenderables;

			MeshDeferredRenderable* meshRenderable =
				r3dGetMeshDeferredRenderable(&renderable);

			if (!meshRenderable)
			{
				++stats.DepthSkippedUnsupported;
				continue;
			}

			++stats.DepthMeshRenderables;

			if (firstPersonDepth)
				++stats.DepthFirstPersonMeshes;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh) || meshRenderable->BatchIdx < 0)
			{
				++stats.DepthSkippedFailed;
				continue;
			}

			if (meshRenderable->DX11Skeleton)
				++stats.DepthSkinnedMeshes;
			else
				++stats.DepthStaticMeshes;

			if (IsAlphaTestedDeferredRenderable(*meshRenderable))
				++stats.DepthAlphaTestedMeshes;

			const D3DXMATRIX& world =
				GetRenderableWorldMatrix(*meshRenderable);

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
			else
			{
				++stats.DepthSkippedFailed;
			}
		}
	}

	void DrawDX11ShadowQueue(
		r3dDX11Renderer& renderer,
		r3dDX11DepthOnlyPass& pass,
		eRenderStageID queueId,
		const D3DXMATRIX& viewProj,
		bool transparentShadowCase,
		r3dDX11WorldRenderStats& stats
	)
	{
		RenderArray& queue = g_render_arrays[queueId];

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			if (transparentShadowCase)
				++stats.TransparentShadowRenderables;
			else
				++stats.ShadowRenderables;

			MeshShadowRenderable* meshRenderable =
				r3dGetMeshShadowRenderable(&renderable);

			if (!meshRenderable)
			{
				if (transparentShadowCase)
					++stats.TransparentShadowSkippedUnsupported;
				else
					++stats.ShadowSkippedUnsupported;

				continue;
			}

			if (transparentShadowCase)
				++stats.TransparentShadowMeshRenderables;
			else
				++stats.ShadowMeshRenderables;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh))
			{
				if (transparentShadowCase)
					++stats.TransparentShadowSkippedFailed;
				else
					++stats.ShadowSkippedFailed;

				continue;
			}

			if (meshRenderable->DX11Skeleton)
			{
				if (transparentShadowCase)
					++stats.TransparentShadowSkinnedMeshes;
				else
					++stats.ShadowSkinnedMeshes;
			}
			else
			{
				if (transparentShadowCase)
					++stats.TransparentShadowStaticMeshes;
				else
					++stats.ShadowStaticMeshes;
			}

			if (IsAlphaTestedShadowRenderable(*meshRenderable))
			{
				if (transparentShadowCase)
					++stats.TransparentShadowAlphaTested;
				else
					++stats.ShadowAlphaTested;
			}

			const D3DXMATRIX& world =
				GetRenderableWorldMatrix(*meshRenderable);

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
					meshRenderable->DX11Skeleton
				);
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
					meshRenderable->DX11Skeleton
				);
			}

			if (drawn)
			{
				if (transparentShadowCase)
					++stats.TransparentShadowDrawnMeshes;
				else
					++stats.ShadowDrawnMeshes;
			}
			else
			{
				if (transparentShadowCase)
					++stats.TransparentShadowSkippedFailed;
				else
					++stats.ShadowSkippedFailed;
			}
		}
	}

	void RenderDX11ShadowQueues(
		r3dDX11Renderer& renderer,
		r3dDX11DepthOnlyPass& pass,
		r3dDX11WorldRenderStats& stats
	)
	{
		if (!r_shadows || !r_shadows->GetBool())
			return;

		int activeSlices =
			r_active_shadow_slices
				? r_active_shadow_slices->GetInt()
				: NumShadowSlices;

		activeSlices =
			std::max(
				0,
				std::min(
					activeSlices,
					static_cast<int>(NumShadowSlices)
				)
			);

		for (int i = 0; i < activeSlices; ++i)
		{
			if (!EnsureDX11ShadowDepthTarget(renderer, i))
			{
				++stats.ShadowSkippedFailed;
				continue;
			}

			r3dDX11Texture2D& shadowDepth =
				g_DX11ShadowDepthTargets[i];

			if (!pass.BeginDepthTarget(
					shadowDepth.GetDSV(),
					shadowDepth.GetWidth(),
					shadowDepth.GetHeight(),
					true))
			{
				++stats.ShadowSkippedFailed;
				continue;
			}

			const D3DXMATRIX viewProj =
				ShadowSlices[i].lightView *
				ShadowSlices[i].lightProj;

			renderer.GetTerrainPass().RenderShadow(
				viewProj,
				shadowDepth.GetDSV(),
				shadowDepth.GetWidth(),
				shadowDepth.GetHeight(),
				&stats
			);

			gCollectionsManager.RenderDX11(
				R3D_IDME_SHADOW,
				renderer,
				viewProj,
				&stats
			);

			DrawDX11ShadowQueue(
				renderer,
				pass,
				static_cast<eRenderStageID>(rsCreateSM + i),
				viewProj,
				false,
				stats
			);

			pass.EndDepthTarget();

			++stats.ShadowSlicesRendered;
		}

		// Transparent shadow case:
		// Старый DX9 path рисует это отдельной очередью rsCreateTransparentSM.
		// В DX11 пока валидируем отдельный transparent shadow depth target.
		if (r_particle_shadows && r_particle_shadows->GetBool())
		{
			if (!EnsureDX11TransparentShadowDepthTarget(renderer))
			{
				++stats.TransparentShadowSkippedFailed;
				return;
			}

			r3dDX11Texture2D& transparentDepth =
				g_DX11TransparentShadowDepthTarget;

			if (!pass.BeginDepthTarget(
					transparentDepth.GetDSV(),
					transparentDepth.GetWidth(),
					transparentDepth.GetHeight(),
					true))
			{
				++stats.TransparentShadowSkippedFailed;
				return;
			}

			D3DXMATRIX transparentViewProj;

			if (activeSlices > 0)
			{
				transparentViewProj =
					ShadowSlices[0].lightView *
					ShadowSlices[0].lightProj;
			}
			else
			{
				D3DXMatrixIdentity(&transparentViewProj);
			}

			DrawDX11ShadowQueue(
				renderer,
				pass,
				rsCreateTransparentSM,
				transparentViewProj,
				true,
				stats
			);

			pass.EndDepthTarget();

			++stats.TransparentShadowCasesRendered;
		}
	}
}

void r3dDX11ResetWorldRenderStats(r3dDX11WorldRenderStats& stats)
{
	r3dResetMeshDeferredDX11WorldMatrices();

	stats.TotalRenderables = 0;
	stats.MeshRenderables = 0;

	stats.DepthTotalRenderables = 0;
	stats.DepthMeshRenderables = 0;
	stats.DepthStaticMeshes = 0;
	stats.DepthSkinnedMeshes = 0;
	stats.DepthAlphaTestedMeshes = 0;
	stats.DepthFirstPersonMeshes = 0;
	stats.DepthDrawnMeshes = 0;
	stats.DepthSkippedUnsupported = 0;
	stats.DepthSkippedFailed = 0;

	stats.DrawnMeshes = 0;
	stats.SkippedUnsupported = 0;
	stats.SkippedFailed = 0;

	stats.ShadowRenderables = 0;
	stats.ShadowMeshRenderables = 0;
	stats.ShadowStaticMeshes = 0;
	stats.ShadowSkinnedMeshes = 0;
	stats.ShadowDrawnMeshes = 0;
	stats.ShadowAlphaTested = 0;
	stats.ShadowSkippedUnsupported = 0;
	stats.ShadowSkippedFailed = 0;
	stats.ShadowSlicesRendered = 0;

	stats.TransparentShadowRenderables = 0;
	stats.TransparentShadowMeshRenderables = 0;
	stats.TransparentShadowStaticMeshes = 0;
	stats.TransparentShadowSkinnedMeshes = 0;
	stats.TransparentShadowDrawnMeshes = 0;
	stats.TransparentShadowAlphaTested = 0;
	stats.TransparentShadowSkippedUnsupported = 0;
	stats.TransparentShadowSkippedFailed = 0;
	stats.TransparentShadowCasesRendered = 0;

	stats.LightingPasses = 0;
	stats.LightingDirectionalLights = 0;
	stats.LightingPointLights = 0;
	stats.LightingSpotLights = 0;
	stats.LightingShadowed = 0;
	stats.LightingGBufferDecoded = 0;
	stats.LightingSpecGlossDecoded = 0;
	stats.LightingFogApplied = 0;
	stats.LightingAmbientApplied = 0;
	stats.LightingProbeApplied = 0;
	stats.LightingSkippedFailed = 0;

	stats.TerrainGBufferDraws = 0;
	stats.TerrainGBufferTriangles = 0;
	stats.TerrainDepthDraws = 0;
	stats.TerrainDepthTriangles = 0;
	stats.TerrainShadowDraws = 0;
	stats.TerrainShadowTriangles = 0;
	stats.TerrainSplatLayers = 0;
	stats.TerrainDetailLayers = 0;
	stats.TerrainSkippedFailed = 0;

	stats.VegetationGBufferInstances = 0;
	stats.VegetationGBufferDraws = 0;
	stats.VegetationDepthInstances = 0;
	stats.VegetationDepthDraws = 0;
	stats.VegetationShadowInstances = 0;
	stats.VegetationShadowDraws = 0;
	stats.VegetationBendingDraws = 0;
	stats.VegetationSkippedFailed = 0;
}

static bool r3dDX11RenderWorldDepthOnlyInternal(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats& stats,
	bool clearDepth
)
{
	if (!renderer.IsInitialized())
		return false;

	r3dDX11DepthOnlyPass& depthPass =
		renderer.GetDepthOnlyPass();

	r3dDX11GBufferResources& gbuffer =
		renderer.GetGBufferResources();

	if (!depthPass.IsInitialized())
	{
		++stats.DepthSkippedFailed;
		return false;
	}

	const D3DXMATRIX viewProj =
		BuildDX11ViewProjectionMatrix(
			renderer,
			camera,
			true
		);

	const r3dCamera firstPersonCamera =
		BuildDX11FirstPersonCamera(
			camera
		);

	const D3DXMATRIX firstPersonViewProj =
		BuildDX11ViewProjectionMatrix(
			renderer,
			firstPersonCamera,
			false
		);

	if (!depthPass.Begin(gbuffer, clearDepth))
	{
		++stats.DepthSkippedFailed;
		return false;
	}

	renderer.GetTerrainPass().RenderDepth(
		viewProj,
		gbuffer.GetDepthStencilView(),
		gbuffer.GetWidth(),
		gbuffer.GetHeight(),
		&stats
	);

	gCollectionsManager.RenderDX11(
		R3D_IDME_DEPTH,
		renderer,
		viewProj,
		&stats
	);

	DrawDX11DepthOnlyQueue(
		renderer,
		depthPass,
		rsFillGBuffer,
		viewProj,
		false,
		stats
	);

	DrawDX11DepthOnlyQueue(
		renderer,
		depthPass,
		rsFillGBufferFirstPerson,
		firstPersonViewProj,
		true,
		stats
	);

	depthPass.End(gbuffer);

	return
		stats.DepthDrawnMeshes > 0 ||
		stats.DepthTotalRenderables == 0;
}

bool r3dDX11RenderWorldDepthOnly(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats* stats,
	bool clearDepth
)
{
	r3dDX11WorldRenderStats localStats;

	if (!stats)
		stats = &localStats;

	r3dDX11ResetWorldRenderStats(*stats);

	return r3dDX11RenderWorldDepthOnlyInternal(
		renderer,
		camera,
		*stats,
		clearDepth
	);
}

bool r3dDX11RenderWorldGBuffer(
	r3dDX11Renderer& renderer,
	const r3dCamera& camera,
	r3dDX11WorldRenderStats* stats
)
{
	r3dDX11WorldRenderStats localStats;

	if (!stats)
		stats = &localStats;

	r3dDX11ResetWorldRenderStats(*stats);

	if (!renderer.IsInitialized())
		return false;

	r3dDX11DepthOnlyPass& depthPass =
		renderer.GetDepthOnlyPass();

	r3dDX11GBufferPass& pass =
		renderer.GetGBufferPass();

	r3dDX11GBufferResources& gbuffer =
		renderer.GetGBufferResources();

	const D3DXMATRIX viewProj =
		BuildDX11ViewProjectionMatrix(
			renderer,
			camera,
			true
		);

	const r3dCamera firstPersonCamera =
		BuildDX11FirstPersonCamera(
			camera
		);

	const D3DXMATRIX firstPersonViewProj =
		BuildDX11ViewProjectionMatrix(
			renderer,
			firstPersonCamera,
			false
		);

	RenderDX11ShadowQueues(
		renderer,
		depthPass,
		*stats
	);

	const bool depthReady =
		r3dDX11RenderWorldDepthOnlyInternal(
			renderer,
			camera,
			*stats,
			true
		);

	if (!depthReady)
		return false;

	if (!pass.Begin(gbuffer, false))
		return false;

	renderer.GetTerrainPass().RenderGBuffer(
		camera,
		gbuffer,
		viewProj,
		stats
	);

	gCollectionsManager.RenderDX11(
		R3D_IDME_NORMAL,
		renderer,
		viewProj,
		stats
	);

	DrawDX11GBufferQueue(
		renderer,
		pass,
		rsFillGBuffer,
		viewProj,
		*stats
	);

	DrawDX11GBufferQueue(
		renderer,
		pass,
		rsFillGBufferFirstPerson,
		firstPersonViewProj,
		*stats
	);

	pass.End(gbuffer);

	return
		stats->DrawnMeshes > 0 ||
		stats->DepthDrawnMeshes > 0 ||
		stats->TotalRenderables == 0;
}
