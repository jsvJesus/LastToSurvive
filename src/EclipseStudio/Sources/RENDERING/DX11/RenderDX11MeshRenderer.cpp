#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MeshRenderer.h"

#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"
#include "RENDERING/DX11/RenderDX11MeshRenderData.h"
#include "RENDERING/DX11/RenderDX11Texture.h"

#include <cstring>

namespace
{
	bool IsMeshLoadedSafe(r3dMesh& mesh)
	{
		__try
		{
			if (!mesh.IsLoaded())
				return false;

			return mesh.NumMatChunks >= 0 && mesh.NumMatChunks <= r3dMesh::ConstNumMatChunks;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
	}

	void LogDX11MeshDrawFailure(
		const char* passName,
		const char* reason,
		const r3dMesh& mesh,
		int batchIndex,
		const r3dSkeleton* skeleton
	)
	{
		static int LogCount = 0;

		if (LogCount >= 128)
			return;

		++LogCount;

		r3dOutToLog(
			"[DX11][MeshFail][%s] reason=%s mesh='%s' file='%s' batch=%d skeletal=%d skeleton=%p numChunks=%d numVerts=%d numIndices=%d\n",
			passName ? passName : "unknown",
			reason ? reason : "unknown",
			mesh.Name,
			mesh.FileName.c_str(),
			batchIndex,
			mesh.IsSkeletal() ? 1 : 0,
			skeleton,
			mesh.NumMatChunks,
			mesh.NumVertices,
			mesh.NumIndices
		);
	}
}

bool r3dDX11PrepareMeshConstants(r3dDX11MeshConstants& outConstants, const D3DXMATRIX& world, const D3DXMATRIX& viewProj)
{
	D3DXMATRIX worldViewProj = world * viewProj;

	memcpy(outConstants.World, &world, sizeof(outConstants.World));
	memcpy(outConstants.WorldViewProj, &worldViewProj, sizeof(outConstants.WorldViewProj));

	outConstants.PositionScale[0] = 1.0f;
	outConstants.PositionScale[1] = 1.0f;
	outConstants.PositionScale[2] = 1.0f;
	outConstants.PositionScale[3] = 0.0f;

	outConstants.TexcoordScale[0] = 1.0f;
	outConstants.TexcoordScale[1] = 1.0f;
	outConstants.TexcoordScale[2] = 0.0f;
	outConstants.TexcoordScale[3] = 0.0f;

	return true;
}

bool r3dDX11DrawMeshDepthOnly(
	ID3D11Device* device,
	r3dDX11TextureLibrary& textureLibrary,
	r3dDX11DepthOnlyPass& pass,
	r3dMesh& mesh,
	const D3DXMATRIX& world,
	const D3DXMATRIX& viewProj,
	const r3dSkeleton* skeleton,
	bool allowTransparentDepthPrepass
)
{
	if (!device)
	{
		LogDX11MeshDrawFailure("Depth", "device_null", mesh, -1, skeleton);
		return false;
	}

	if (!pass.IsInitialized())
	{
		LogDX11MeshDrawFailure("Depth", "pass_not_initialized", mesh, -1, skeleton);
		return false;
	}

	if (!IsMeshLoadedSafe(mesh))
	{
		LogDX11MeshDrawFailure("Depth", "mesh_not_loaded_or_invalid", mesh, -1, skeleton);
		return false;
	}

	r3dDX11MeshRenderData* renderData =
		r3dDX11GetOrCreateMeshRenderData(
			device,
			textureLibrary,
			mesh,
			mesh.Name
		);

	if (!renderData)
	{
		LogDX11MeshDrawFailure("Depth", "render_data_null", mesh, -1, skeleton);
		return false;
	}

	if (!renderData->IsValid())
	{
		LogDX11MeshDrawFailure("Depth", "render_data_invalid", mesh, -1, skeleton);
		return false;
	}

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);

	const bool drawn =
		renderData->DrawDepthOnly(
			pass,
			constants,
			skeleton,
			allowTransparentDepthPrepass
		);

	if (!drawn)
	{
		LogDX11MeshDrawFailure("Depth", "draw_depth_failed", mesh, -1, skeleton);
	}

	return drawn;
}

bool r3dDX11DrawMeshDepthOnlyBatch(
	ID3D11Device* device,
	r3dDX11TextureLibrary& textureLibrary,
	r3dDX11DepthOnlyPass& pass,
	r3dMesh& mesh,
	unsigned int batchIndex,
	const D3DXMATRIX& world,
	const D3DXMATRIX& viewProj,
	const r3dSkeleton* skeleton,
	bool allowTransparentDepthPrepass
)
{
	if (!device)
	{
		LogDX11MeshDrawFailure("DepthBatch", "device_null", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (!pass.IsInitialized())
	{
		LogDX11MeshDrawFailure("DepthBatch", "pass_not_initialized", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (!IsMeshLoadedSafe(mesh))
	{
		LogDX11MeshDrawFailure("DepthBatch", "mesh_not_loaded_or_invalid", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	r3dDX11MeshRenderData* renderData =
		r3dDX11GetOrCreateMeshRenderData(
			device,
			textureLibrary,
			mesh,
			mesh.Name
		);

	if (!renderData)
	{
		LogDX11MeshDrawFailure("DepthBatch", "render_data_null", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (!renderData->IsValid())
	{
		LogDX11MeshDrawFailure("DepthBatch", "render_data_invalid", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (batchIndex >= renderData->GetBatchCount())
	{
		LogDX11MeshDrawFailure("DepthBatch", "batch_out_of_range", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);

	const bool drawn =
		renderData->DrawDepthOnlyBatch(
			pass,
			batchIndex,
			constants,
			skeleton,
			allowTransparentDepthPrepass
		);

	if (!drawn)
	{
		LogDX11MeshDrawFailure("DepthBatch", "draw_depth_batch_failed", mesh, static_cast<int>(batchIndex), skeleton);
	}

	return drawn;
}

bool r3dDX11DrawMeshGBuffer(
	ID3D11Device* device,
	r3dDX11TextureLibrary& textureLibrary,
	r3dDX11GBufferPass& pass,
	r3dMesh& mesh,
	const D3DXMATRIX& world,
	const D3DXMATRIX& viewProj,
	unsigned int objectColorPacked,
	const r3dSkeleton* skeleton
)
{
	if (!device)
	{
		LogDX11MeshDrawFailure("GBuffer", "device_null", mesh, -1, skeleton);
		return false;
	}

	if (!pass.IsInitialized())
	{
		LogDX11MeshDrawFailure("GBuffer", "pass_not_initialized", mesh, -1, skeleton);
		return false;
	}

	if (!IsMeshLoadedSafe(mesh))
	{
		LogDX11MeshDrawFailure("GBuffer", "mesh_not_loaded_or_invalid", mesh, -1, skeleton);
		return false;
	}

	r3dDX11MeshRenderData* renderData =
		r3dDX11GetOrCreateMeshRenderData(
			device,
			textureLibrary,
			mesh,
			mesh.Name
		);

	if (!renderData)
	{
		LogDX11MeshDrawFailure("GBuffer", "render_data_null", mesh, -1, skeleton);
		return false;
	}

	if (!renderData->IsValid())
	{
		LogDX11MeshDrawFailure("GBuffer", "render_data_invalid", mesh, -1, skeleton);
		return false;
	}

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);

	const bool drawn =
		renderData->DrawGBuffer(
			pass,
			constants,
			objectColorPacked,
			skeleton
		);

	if (!drawn)
	{
		LogDX11MeshDrawFailure("GBuffer", "draw_gbuffer_failed", mesh, -1, skeleton);
	}

	return drawn;
}

bool r3dDX11DrawMeshGBufferBatch(
	ID3D11Device* device,
	r3dDX11TextureLibrary& textureLibrary,
	r3dDX11GBufferPass& pass,
	r3dMesh& mesh,
	unsigned int batchIndex,
	const D3DXMATRIX& world,
	const D3DXMATRIX& viewProj,
	unsigned int objectColorPacked,
	const r3dSkeleton* skeleton
)
{
	if (!device)
	{
		LogDX11MeshDrawFailure("GBufferBatch", "device_null", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (!pass.IsInitialized())
	{
		LogDX11MeshDrawFailure("GBufferBatch", "pass_not_initialized", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (!IsMeshLoadedSafe(mesh))
	{
		LogDX11MeshDrawFailure("GBufferBatch", "mesh_not_loaded_or_invalid", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	r3dDX11MeshRenderData* renderData =
		r3dDX11GetOrCreateMeshRenderData(
			device,
			textureLibrary,
			mesh,
			mesh.Name
		);

	if (!renderData)
	{
		LogDX11MeshDrawFailure("GBufferBatch", "render_data_null", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (!renderData->IsValid())
	{
		LogDX11MeshDrawFailure("GBufferBatch", "render_data_invalid", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	if (batchIndex >= renderData->GetBatchCount())
	{
		LogDX11MeshDrawFailure("GBufferBatch", "batch_out_of_range", mesh, static_cast<int>(batchIndex), skeleton);
		return false;
	}

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);

	const bool drawn =
		renderData->DrawGBufferBatch(
			pass,
			batchIndex,
			constants,
			objectColorPacked,
			skeleton
		);

	if (!drawn)
	{
		LogDX11MeshDrawFailure("GBufferBatch", "draw_gbuffer_batch_failed", mesh, static_cast<int>(batchIndex), skeleton);
	}

	return drawn;
}
