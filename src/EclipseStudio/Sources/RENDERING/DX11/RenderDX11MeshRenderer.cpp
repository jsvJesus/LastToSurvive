#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MeshRenderer.h"

#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"
#include "RENDERING/DX11/RenderDX11MeshRenderData.h"
#include "RENDERING/DX11/RenderDX11Texture.h"

#include <cstring>

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

bool r3dDX11DrawMeshDepthOnly(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dDX11DepthOnlyPass& pass, r3dMesh& mesh, const D3DXMATRIX& world, const D3DXMATRIX& viewProj, const r3dSkeleton* skeleton)
{
	if (!device || !pass.IsInitialized() || !mesh.IsLoaded())
		return false;

	r3dDX11MeshRenderData* renderData = r3dDX11GetOrCreateMeshRenderData(device, textureLibrary, mesh, mesh.Name);
	if (!renderData || !renderData->IsValid())
		return false;

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);
	return renderData->DrawDepthOnly(pass, constants, skeleton);
}

bool r3dDX11DrawMeshDepthOnlyBatch(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dDX11DepthOnlyPass& pass, r3dMesh& mesh, unsigned int batchIndex, const D3DXMATRIX& world, const D3DXMATRIX& viewProj, const r3dSkeleton* skeleton)
{
	if (!device || !pass.IsInitialized() || !mesh.IsLoaded())
		return false;

	r3dDX11MeshRenderData* renderData = r3dDX11GetOrCreateMeshRenderData(device, textureLibrary, mesh, mesh.Name);
	if (!renderData || !renderData->IsValid() || batchIndex >= renderData->GetBatchCount())
		return false;

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);
	return renderData->DrawDepthOnlyBatch(pass, batchIndex, constants, skeleton);
}

bool r3dDX11DrawMeshGBuffer(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dDX11GBufferPass& pass, r3dMesh& mesh, const D3DXMATRIX& world, const D3DXMATRIX& viewProj, unsigned int objectColorPacked, const r3dSkeleton* skeleton)
{
	if (!device || !pass.IsInitialized() || !mesh.IsLoaded())
		return false;

	r3dDX11MeshRenderData* renderData = r3dDX11GetOrCreateMeshRenderData(device, textureLibrary, mesh, mesh.Name);
	if (!renderData || !renderData->IsValid())
		return false;

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);
	renderData->DrawGBuffer(pass, constants, objectColorPacked, skeleton);
	return true;
}

bool r3dDX11DrawMeshGBufferBatch(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dDX11GBufferPass& pass, r3dMesh& mesh, unsigned int batchIndex, const D3DXMATRIX& world, const D3DXMATRIX& viewProj, unsigned int objectColorPacked, const r3dSkeleton* skeleton)
{
	if (!device || !pass.IsInitialized() || !mesh.IsLoaded())
		return false;

	r3dDX11MeshRenderData* renderData = r3dDX11GetOrCreateMeshRenderData(device, textureLibrary, mesh, mesh.Name);
	if (!renderData || !renderData->IsValid() || batchIndex >= renderData->GetBatchCount())
		return false;

	r3dDX11MeshConstants constants;
	r3dDX11PrepareMeshConstants(constants, world, viewProj);
	renderData->DrawGBufferBatch(pass, batchIndex, constants, objectColorPacked, skeleton);
	return true;
}
