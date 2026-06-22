#include "r3dPCH.h"
#include "r3d.h"
#include "r3dCam.h"

#include "RENDERING/DX11/RenderDX11World.h"

#include "GameObjects/GameObj.h"
#include "GameObjects/ObjManag.h"
#include "Editors/CollectionsManager.h"
#include "RENDERING/Deffered/RenderDeffered.h"
#include "RENDERING/DX11/RenderDX11.h"
#include "RENDERING/DX11/RenderDX11MeshRenderer.h"
#include "r3dMat.h"
#include "r3dAtmosphere.h"

#include <algorithm>

extern RenderArray g_render_arrays[rsCount];
extern ShadowSlice TransparentShadowSlice;
extern r3dVector SunVector;

void FillSliceForSplitDistances(
	ShadowSlice* ioSlice,
	float* shadowSplitDistances,
	ShadowMapOptimizationData* optimizationData,
	r3dPoint3D* oLightSource,
	r3dPoint3D* oLightTarget,
	float fNear,
	float fFar,
	bool allow_optimize,
	float border
);

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
				DXGI_FORMAT_R24G8_TYPELESS,
				R3D_DX11_BIND_DEPTH_STENCIL | R3D_DX11_BIND_SHADER_RESOURCE,
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
		int size = 512;

		if (r_transp_shadowmap_size)
			size = r_transp_shadowmap_size->GetInt();

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
				DXGI_FORMAT_R24G8_TYPELESS,
				R3D_DX11_BIND_DEPTH_STENCIL | R3D_DX11_BIND_SHADER_RESOURCE,
				"DX11.TransparentShadowDepth"))
		{
			g_DX11TransparentShadowDepthSize = 0;
			return false;
		}

		g_DX11TransparentShadowDepthSize = size;
		TransparentShadowSlice.shadowMapSize = static_cast<float>(size);
		return true;
	}

	bool PrepareDX11TransparentShadowQueue(D3DXMATRIX& outViewProj)
	{
		if (!r3dRenderer || !ShadowSplitDistancesTransparent)
			return false;

		const D3DXMATRIX oldView = r3dRenderer->ViewMatrix;
		const D3DXMATRIX oldProj = r3dRenderer->ProjMatrix;
		const float oldNear = r3dRenderer->NearClip;
		const float oldFar = r3dRenderer->FarClip;

		SunVector = GetEnvLightDir();

		ShadowSlice& slice = TransparentShadowSlice;
		const float fNear = ShadowSplitDistancesTransparent[slice.index];
		const float fFar = ShadowSplitDistancesTransparent[slice.index + 1];
		const float fade = r_transp_shadowmap_fade ? r_transp_shadowmap_fade->GetFloat() : 32.0f;
		const float border = fade > 0.0001f ? 1.0f / fade : 0.0f;

		r3dPoint3D lightSource;
		r3dPoint3D lightTarget;
		FillSliceForSplitDistances(
			&slice,
			ShadowSplitDistancesTransparent,
			nullptr,
			&lightSource,
			&lightTarget,
			fNear,
			fFar,
			true,
			border
		);

		slice.camPos = lightSource;

		const float shadowNear = 0.1f;
		const float shadowFar = 10000.0f;
		r3dRenderer->SetCameraEx(slice.lightView, slice.lightProj, shadowNear, shadowFar, false);

		r3dCamera shadowCam;
		shadowCam.FOV = 90.0f;
		shadowCam.SetPosition(lightSource);
		shadowCam.PointTo(lightTarget);
		shadowCam.NearClip = shadowNear;
		shadowCam.FarClip = shadowFar;
		shadowCam.SetOrtho(slice.sphereRadius * 2.0f, slice.sphereRadius * 2.0f);

		g_render_arrays[rsCreateTransparentSM].Clear();
		GameWorld().PrepareTransparentShadowsInterm(shadowCam);

		r3dRenderer->SetCameraEx(oldView, oldProj, oldNear, oldFar, false);

		outViewProj = slice.lightView * slice.lightProj;
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

	bool IsTransparentDepthPrepassMaterial(const r3dMaterial* material)
	{
		if (!material)
			return false;

		if (material->Flags & R3D_MAT_SKIP_DRAW)
			return false;

		const bool isTransparent =
			(material->Flags & (
				R3D_MAT_TRANSPARENT |
				R3D_MAT_TRANSPARENT_CAMOUFLAGE |
				R3D_MAT_TRANSPARENT_CAMO_FP
			)) != 0;

		const bool isCamouflage =
			(material->Flags & (
				R3D_MAT_CAMOUFLAGE |
				R3D_MAT_TRANSPARENT_CAMOUFLAGE |
				R3D_MAT_TRANSPARENT_CAMO_FP
			)) != 0;

		const bool hasAlphaCut =
			(material->Flags & (
				R3D_MAT_HASALPHA |
				R3D_MAT_FORCEHASALPHA
			)) != 0;

		// Не пишем depth для обычного alpha-blended glass/water.
		// Пишем depth для masked transparent и camo silhouettes.
		return
			isCamouflage ||
			(isTransparent && hasAlphaCut);
	}

	bool IsTransparentDepthPrepassRenderable(const MeshDeferredRenderable& renderable)
	{
		if (!renderable.Mesh)
			return false;

		if (
			renderable.BatchIdx < 0 ||
			renderable.BatchIdx >= renderable.Mesh->NumMatChunks
		)
		{
			return false;
		}

		const r3dMaterial* material =
			renderable.Mesh->MatChunks[renderable.BatchIdx].Mat;

		return IsTransparentDepthPrepassMaterial(material);
	}

	bool IsCamouflageDepthPrepassRenderable(const MeshDeferredRenderable& renderable)
	{
		if (
			!renderable.Mesh ||
			renderable.BatchIdx < 0 ||
			renderable.BatchIdx >= renderable.Mesh->NumMatChunks
		)
		{
			return false;
		}

		const r3dMaterial* material =
			renderable.Mesh->MatChunks[renderable.BatchIdx].Mat;

		if (!material)
			return false;

		return
			(material->Flags & (
				R3D_MAT_CAMOUFLAGE |
				R3D_MAT_TRANSPARENT_CAMOUFLAGE |
				R3D_MAT_TRANSPARENT_CAMO_FP
			)) != 0;
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

	void PushDX11RenderableRaw(RenderArray& outQueue, const Renderable& renderable)
	{
		// RenderArray is TTabArray<Renderable, MAX_RENDERABLE_SIZE>.
		// We must copy the whole 64-byte slot, not only sizeof(Renderable),
		// otherwise MeshDeferredRenderable / MeshShadowRenderable data is truncated.
		outQueue.PushBack(renderable, RenderArray::TAB_SIZE);
	}

	bool IsDX11DeferredRenderableUsable(
		Renderable& renderable,
		bool transparentDepthOnly
	)
	{
		MeshDeferredRenderable* meshRenderable =
			r3dGetMeshDeferredRenderable(&renderable);

		if (!meshRenderable)
			return false;

		if (!IsDX11MeshPointerUsable(meshRenderable->Mesh))
			return false;

		if (meshRenderable->BatchIdx < 0)
			return false;

		if (
			meshRenderable->BatchIdx >= meshRenderable->Mesh->NumMatChunks
		)
		{
			return false;
		}

		if (transparentDepthOnly)
		{
			if (!IsTransparentDepthPrepassRenderable(*meshRenderable))
				return false;
		}

		return true;
	}

	bool IsDX11ShadowRenderableUsable(Renderable& renderable)
	{
		MeshShadowRenderable* meshRenderable =
			r3dGetMeshShadowRenderable(&renderable);

		if (!meshRenderable)
			return false;

		if (!IsDX11MeshPointerUsable(meshRenderable->Mesh))
			return false;

		if (
			meshRenderable->BatchIdx >= 0 &&
			meshRenderable->BatchIdx >= meshRenderable->Mesh->NumMatChunks
		)
		{
			return false;
		}

		return true;
	}

	void KeepOnlyDX11DeferredRenderables(
		RenderArray& queue,
		bool transparentDepthOnly
	)
	{
		if (!queue.Count())
			return;

		RenderArray filtered;
		filtered.Reserve(queue.Count());

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			if (!IsDX11DeferredRenderableUsable(renderable, transparentDepthOnly))
				continue;

			PushDX11RenderableRaw(filtered, renderable);
		}

		queue.Swap(filtered);
	}

	void KeepOnlyDX11ShadowRenderables(RenderArray& queue)
	{
		if (!queue.Count())
			return;

		RenderArray filtered;
		filtered.Reserve(queue.Count());

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			if (!IsDX11ShadowRenderableUsable(renderable))
				continue;

			PushDX11RenderableRaw(filtered, renderable);
		}

		queue.Swap(filtered);
	}

	void SanitizeDX11TransparentShadowQueue()
	{
		KeepOnlyDX11ShadowRenderables(g_render_arrays[rsCreateTransparentSM]);
	}

	void CopyDX11RenderableToQueue(RenderArray& outQueue, const Renderable& renderable)
	{
		// RenderArray хранит MAX_RENDERABLE_SIZE байт на элемент.
		// Нельзя делать обычный PushBack(renderable), иначе скопируются только поля base Renderable.
		outQueue.PushBack(renderable, RenderArray::TAB_SIZE);
	}

	void FilterDX11QueueForDeferred(RenderArray& queue, bool transparentDepthOnly)
	{
		if (!queue.Count())
			return;

		RenderArray filtered;
		filtered.Reserve(queue.Count());

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			MeshDeferredRenderable* meshRenderable =
				r3dGetMeshDeferredRenderable(&renderable);

			if (!meshRenderable)
				continue;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh))
				continue;

			if (meshRenderable->BatchIdx < 0)
				continue;

			if (transparentDepthOnly && !IsTransparentDepthPrepassRenderable(*meshRenderable))
				continue;

			CopyDX11RenderableToQueue(filtered, renderable);
		}

		queue.Swap(filtered);
	}

	void FilterDX11QueueForShadow(RenderArray& queue)
	{
		if (!queue.Count())
			return;

		RenderArray filtered;
		filtered.Reserve(queue.Count());

		for (uint32_t i = 0, e = queue.Count(); i < e; ++i)
		{
			Renderable& renderable = queue[i];

			MeshShadowRenderable* meshRenderable =
				r3dGetMeshShadowRenderable(&renderable);

			if (!meshRenderable)
				continue;

			if (!IsDX11MeshPointerUsable(meshRenderable->Mesh))
				continue;

			CopyDX11RenderableToQueue(filtered, renderable);
		}

		queue.Swap(filtered);
	}

	void ClearDX11RuntimeUnsupportedQueue(eRenderStageID queueId)
	{
		g_render_arrays[queueId].Clear();
	}

	void SanitizeDX11WorldRenderQueuesForFrame()
	{
		// Opaque / GBuffer queues.
		// Тут оставляем только MeshDeferredRenderable.
		FilterDX11QueueForDeferred(g_render_arrays[rsFillGBuffer], false);
		FilterDX11QueueForDeferred(g_render_arrays[rsFillGBufferEffects], false);
		FilterDX11QueueForDeferred(g_render_arrays[rsFillGBufferAfterEffects], false);
		FilterDX11QueueForDeferred(g_render_arrays[rsFillGBufferFirstPerson], false);

		// Shadow queues.
		// Тут оставляем только MeshShadowRenderable.
		for (int i = 0; i < NumShadowSlices; ++i)
		{
			FilterDX11QueueForShadow(
				g_render_arrays[static_cast<eRenderStageID>(rsCreateSM + i)]
			);
		}

		FilterDX11QueueForShadow(g_render_arrays[rsCreateTransparentSM]);

		// Transparent queues пока не имеют полноценного DX11 forward pass.
		// Для depth prepass оставляем только masked/camo MeshDeferredRenderable.
		FilterDX11QueueForDeferred(g_render_arrays[rsDrawTransparents], true);
		FilterDX11QueueForDeferred(g_render_arrays[rsDrawDistortion], true);

		// Эти очереди сейчас содержат DX9-style special renderables.
		// В DX11 world runtime их нельзя blindly прогонять через GBuffer/Depth.
		ClearDX11RuntimeUnsupportedQueue(rsDrawDepthEffect);
		ClearDX11RuntimeUnsupportedQueue(rsDrawComposite1);
		ClearDX11RuntimeUnsupportedQueue(rsDrawComposite2);
		ClearDX11RuntimeUnsupportedQueue(rsDrawBloomGlow);
		ClearDX11RuntimeUnsupportedQueue(rsDrawDebugData);
		ClearDX11RuntimeUnsupportedQueue(rsDrawBoundBox);
		ClearDX11RuntimeUnsupportedQueue(rsDrawFlashUI);
		ClearDX11RuntimeUnsupportedQueue(rsDrawDepth);
		ClearDX11RuntimeUnsupportedQueue(rsDepthPrepass);
		ClearDX11RuntimeUnsupportedQueue(rsDrawPhysicsMeshes);
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

	void DrawDX11TransparentDepthPrepassQueue(
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

			++stats.TransparentDepthRenderables;

			MeshDeferredRenderable* meshRenderable =
				r3dGetMeshDeferredRenderable(&renderable);

			if (!meshRenderable)
			{
				++stats.TransparentDepthSkippedUnsupported;
				continue;
			}

			++stats.TransparentDepthMeshRenderables;

			if (
				!IsDX11MeshPointerUsable(meshRenderable->Mesh) ||
				meshRenderable->BatchIdx < 0
			)
			{
				++stats.TransparentDepthSkippedFailed;
				continue;
			}

			if (!IsTransparentDepthPrepassRenderable(*meshRenderable))
				continue;

			if (IsAlphaTestedDeferredRenderable(*meshRenderable))
				++stats.TransparentDepthAlphaTestedMeshes;

			if (IsCamouflageDepthPrepassRenderable(*meshRenderable))
				++stats.TransparentDepthCamouflageMeshes;

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
					meshRenderable->DX11Skeleton,
					true))
			{
				++stats.TransparentDepthDrawnMeshes;
				++stats.DepthDrawnMeshes;
			}
			else
			{
				++stats.TransparentDepthSkippedFailed;
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
					meshRenderable->DX11Skeleton,
					transparentShadowCase
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
					meshRenderable->DX11Skeleton,
					transparentShadowCase
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
		const r3dCamera& camera,
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

			if (!pass.SetDepthBias(ShadowSlices[i].depthBias_HW))
			{
				pass.EndDepthTarget();
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
				&stats,
				ShadowSlices[i].depthBias_HW
			);

			gCollectionsManager.RenderDX11(
				R3D_IDME_SHADOW,
				renderer,
				viewProj,
				&stats,
				ShadowSlices[i].depthBias_HW
			);

			if (!pass.SetDepthBias(ShadowSlices[i].depthBias_HW))
			{
				pass.EndDepthTarget();
				++stats.ShadowSkippedFailed;
				continue;
			}

			const eRenderStageID shadowQueueId =
				static_cast<eRenderStageID>(rsCreateSM + i);

			FilterDX11QueueForShadow(
				g_render_arrays[shadowQueueId]
			);

			DrawDX11ShadowQueue(
				renderer,
				pass,
				shadowQueueId,
				viewProj,
				false,
				stats
			);

			pass.EndDepthTarget();

			++stats.ShadowSlicesRendered;
		}

		// Transparent shadow case mirrors the DX9 rsCreateTransparentSM path:
		// rebuild TransparentShadowSlice, prepare the transparent-shadow queue,
		// then render it into its own depth target.
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
			if (!PrepareDX11TransparentShadowQueue(transparentViewProj))
			{
				pass.EndDepthTarget();
				if (r3dRenderer)
					r3dRenderer->SetCamera(camera, false);
				++stats.TransparentShadowSkippedFailed;
				return;
			}

			FilterDX11QueueForShadow(
				g_render_arrays[rsCreateTransparentSM]
			);
			SanitizeDX11TransparentShadowQueue();

			if (!pass.SetDepthBias(TransparentShadowSlice.depthBias_HW))
			{
				pass.EndDepthTarget();
				if (r3dRenderer)
					r3dRenderer->SetCamera(camera, false);
				++stats.TransparentShadowSkippedFailed;
				return;
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
			if (r3dRenderer)
				r3dRenderer->SetCamera(camera, false);

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

	stats.TransparentDepthRenderables = 0;
	stats.TransparentDepthMeshRenderables = 0;
	stats.TransparentDepthDrawnMeshes = 0;
	stats.TransparentDepthAlphaTestedMeshes = 0;
	stats.TransparentDepthCamouflageMeshes = 0;
	stats.TransparentDepthSkippedUnsupported = 0;
	stats.TransparentDepthSkippedFailed = 0;
}

int r3dDX11GetShadowSliceCount()
{
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

	activeSlices =
		std::min(
			activeSlices,
			static_cast<int>(R3D_DX11_MAX_SHADOW_SLICES)
		);

	return activeSlices;
}

bool r3dDX11GetShadowSliceInfo(int sliceIndex, r3dDX11ShadowSliceInfo& outInfo)
{
	memset(&outInfo, 0, sizeof(outInfo));

	if (
		sliceIndex < 0 ||
		sliceIndex >= r3dDX11GetShadowSliceCount() ||
		sliceIndex >= NumShadowSlices
	)
	{
		return false;
	}

	r3dDX11Texture2D& shadowDepth =
		g_DX11ShadowDepthTargets[sliceIndex];

	if (
		!shadowDepth.IsValid() ||
		!shadowDepth.GetSRV() ||
		g_DX11ShadowDepthSizes[sliceIndex] <= 0
	)
	{
		return false;
	}

	const D3DXMATRIX viewProj =
		ShadowSlices[sliceIndex].lightView *
		ShadowSlices[sliceIndex].lightProj;

	memcpy(
		outInfo.ViewProj,
		&viewProj,
		sizeof(outInfo.ViewProj)
	);

	if (ShadowSplitDistancesOpaque)
	{
		outInfo.SplitNear =
			ShadowSplitDistancesOpaque[sliceIndex];

		outInfo.SplitFar =
			ShadowSplitDistancesOpaque[sliceIndex + 1];
	}
	else
	{
		outInfo.SplitNear = 0.0f;
		outInfo.SplitFar = MAX_DIR_SHADOW_LENGTH;
	}

	if (outInfo.SplitFar <= outInfo.SplitNear + 0.001f)
	{
		outInfo.SplitNear = 0.0f;
		outInfo.SplitFar = MAX_DIR_SHADOW_LENGTH;
	}

	outInfo.DepthBias =
		std::max(
			ShadowSlices[sliceIndex].depthBias,
			0.00005f
		);

	outInfo.TexelSize =
		1.0f /
		static_cast<float>(
			std::max(
				1,
				g_DX11ShadowDepthSizes[sliceIndex]
			)
		);

	outInfo.Strength = 0.82f;
	outInfo.SRV = shadowDepth.GetSRV();

	return true;
}

bool r3dDX11GetTransparentShadowInfo(r3dDX11TransparentShadowInfo& outInfo)
{
	memset(&outInfo, 0, sizeof(outInfo));

	if (
		!g_DX11TransparentShadowDepthTarget.IsValid() ||
		!g_DX11TransparentShadowDepthTarget.GetSRV() ||
		g_DX11TransparentShadowDepthSize <= 0 ||
		!r_particle_shadows ||
		!r_particle_shadows->GetBool()
	)
	{
		return false;
	}

	const D3DXMATRIX viewProj =
		TransparentShadowSlice.lightView *
		TransparentShadowSlice.lightProj;

	memcpy(
		outInfo.ViewProj,
		&viewProj,
		sizeof(outInfo.ViewProj)
	);

	outInfo.DepthBias =
		std::max(
			TransparentShadowSlice.depthBias,
			0.00005f
		);

	outInfo.TexelSize =
		1.0f /
		static_cast<float>(
			std::max(
				1,
				g_DX11TransparentShadowDepthSize
			)
		);

	// Transparent/camo shadows должны быть мягче opaque shadows.
	outInfo.Strength = 0.42f;
	outInfo.Enabled = 1.0f;
	outInfo.SRV = g_DX11TransparentShadowDepthTarget.GetSRV();

	return true;
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
		rsFillGBufferEffects,
		viewProj,
		false,
		stats
	);

	DrawDX11DepthOnlyQueue(
		renderer,
		depthPass,
		rsFillGBufferAfterEffects,
		viewProj,
		false,
		stats
	);

	if (r_dx11_transparent_depth_prepass && r_dx11_transparent_depth_prepass->GetBool())
	{
		DrawDX11TransparentDepthPrepassQueue(
			renderer,
			depthPass,
			rsDrawTransparents,
			viewProj,
			stats
		);

		DrawDX11TransparentDepthPrepassQueue(
			renderer,
			depthPass,
			rsDrawDistortion,
			viewProj,
			stats
		);
	}

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
	SanitizeDX11WorldRenderQueuesForFrame();

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
		camera,
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
		rsFillGBufferEffects,
		viewProj,
		*stats
	);

	DrawDX11GBufferQueue(
		renderer,
		pass,
		rsFillGBufferAfterEffects,
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
