#include "r3dPCH.h"
#include "r3d.h"

#include "RENDERING/DX11/RenderDX11MeshRenderData.h"

#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"
#include "RENDERING/DX11/RenderDX11MaterialAdapter.h"
#include "RENDERING/DX11/RenderDX11MeshAdapter.h"
#include "RENDERING/DX11/RenderDX11Texture.h"

static void r3dDX11DestroyMeshRenderData(r3dDX11MeshRenderData* renderData)
{
	delete renderData;
}

r3dDX11MeshRenderData::r3dDX11MeshRenderData()
{
}

r3dDX11MeshRenderData::~r3dDX11MeshRenderData()
{
	Shutdown();
}

bool r3dDX11MeshRenderData::CreateFromR3DMesh(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, const r3dMesh& mesh, const char* debugName)
{
	Shutdown();

	if (!r3dDX11CreateMeshResourceFromR3DMesh(device, mesh, MeshResource, debugName))
	{
		Shutdown();
		return false;
	}

	const unsigned int batchCount = MeshResource.GetBatchCount();
	Materials.resize(batchCount);

	for (unsigned int i = 0; i < batchCount; ++i)
	{
		const r3dDX11MeshBatch* batch = MeshResource.GetBatch(i);
		if (!batch || batch->MaterialIndex >= static_cast<unsigned int>(mesh.NumMatChunks) || !mesh.MatChunks[batch->MaterialIndex].Mat)
		{
			Materials[i].SetFallbacks(textureLibrary);
			continue;
		}

		if (!r3dDX11CreateMaterialTexturesFromR3DMaterial(textureLibrary, *mesh.MatChunks[batch->MaterialIndex].Mat, Materials[i]))
			Materials[i].SetFallbacks(textureLibrary);
	}

	return true;
}

void r3dDX11MeshRenderData::Shutdown()
{
	Materials.clear();
	MeshResource.Shutdown();
}

bool r3dDX11MeshRenderData::DrawGBuffer(
	r3dDX11GBufferPass& pass,
	const r3dDX11MeshConstants& constants,
	unsigned int objectColorPacked,
	const r3dSkeleton* skeleton
)
{
	bool drewAny = false;

	for (unsigned int i = 0; i < GetBatchCount(); ++i)
	{
		drewAny =
			DrawGBufferBatch(
				pass,
				i,
				constants,
				objectColorPacked,
				skeleton
			) || drewAny;
	}

	return drewAny;
}

bool r3dDX11MeshRenderData::DrawGBufferBatch(
	r3dDX11GBufferPass& pass,
	unsigned int batchIndex,
	const r3dDX11MeshConstants& constants,
	unsigned int objectColorPacked,
	const r3dSkeleton* skeleton
)
{
	if (!IsValid() || batchIndex >= Materials.size())
		return false;

	const r3dDX11MeshConstants scaledConstants =
		ApplyMeshScales(constants);

	pass.SetMeshConstants(scaledConstants);

	pass.SetSkinnedMeshMode(MeshResource.IsSkinned());

	if (MeshResource.IsSkinned() && !pass.SetSkinningBones(skeleton))
		return false;

	if (!pass.SetMaterial(Materials[batchIndex], objectColorPacked))
		return false;

	MeshResource.DrawBatch(pass, batchIndex);

	return true;
}

bool r3dDX11MeshRenderData::DrawDepthOnly(
	r3dDX11DepthOnlyPass& pass,
	const r3dDX11MeshConstants& constants,
	const r3dSkeleton* skeleton,
	bool allowTransparentDepthPrepass
)
{
	bool drewAny = false;

	for (unsigned int i = 0; i < GetBatchCount(); ++i)
	{
		drewAny =
			DrawDepthOnlyBatch(
				pass,
				i,
				constants,
				skeleton,
				allowTransparentDepthPrepass
			) || drewAny;
	}

	return drewAny;
}

bool r3dDX11MeshRenderData::DrawDepthOnlyBatch(
	r3dDX11DepthOnlyPass& pass,
	unsigned int batchIndex,
	const r3dDX11MeshConstants& constants,
	const r3dSkeleton* skeleton,
	bool allowTransparentDepthPrepass
)
{
	if (!IsValid() || batchIndex >= GetBatchCount())
		return false;

	const r3dDX11MeshConstants scaledConstants =
		ApplyMeshScales(constants);

	pass.SetMeshConstants(scaledConstants);

	pass.SetSkinnedMeshMode(MeshResource.IsSkinned());

	if (MeshResource.IsSkinned() && !pass.SetSkinningBones(skeleton))
		return false;

	if (batchIndex >= Materials.size() || !pass.SetMaterial(Materials[batchIndex], allowTransparentDepthPrepass))
		return false;

	MeshResource.DrawBatch(pass, batchIndex);

	return true;
}

r3dDX11MeshResource& r3dDX11MeshRenderData::GetMeshResource()
{
	return MeshResource;
}

const r3dDX11MeshResource& r3dDX11MeshRenderData::GetMeshResource() const
{
	return MeshResource;
}

const r3dDX11MaterialTextures* r3dDX11MeshRenderData::GetMaterialTextures(unsigned int batchIndex) const
{
	if (batchIndex >= Materials.size())
		return nullptr;

	return &Materials[batchIndex];
}

unsigned int r3dDX11MeshRenderData::GetBatchCount() const
{
	return MeshResource.GetBatchCount();
}

bool r3dDX11MeshRenderData::IsValid() const
{
	return MeshResource.IsValid();
}

r3dDX11MeshConstants r3dDX11MeshRenderData::ApplyMeshScales(const r3dDX11MeshConstants& constants) const
{
	r3dDX11MeshConstants result = constants;
	const float* positionScale = MeshResource.GetPositionScale();
	const float* texcoordScale = MeshResource.GetTexcoordScale();

	result.PositionScale[0] = positionScale[0];
	result.PositionScale[1] = positionScale[1];
	result.PositionScale[2] = positionScale[2];
	result.PositionScale[3] = 0.0f;
	result.TexcoordScale[0] = texcoordScale[0];
	result.TexcoordScale[1] = texcoordScale[1];
	result.TexcoordScale[2] = 0.0f;
	result.TexcoordScale[3] = 0.0f;
	return result;
}

r3dDX11MeshRenderData* r3dDX11GetOrCreateMeshRenderData(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dMesh& mesh, const char* debugName)
{
	r3dDX11MeshRenderData* existing = mesh.GetDX11RenderData();
	if (existing && existing->IsValid())
		return existing;

	mesh.ReleaseDX11RenderData();

	r3dDX11MeshRenderData* renderData = new r3dDX11MeshRenderData();
	const char* resourceName = debugName && debugName[0] ? debugName : mesh.Name;
	if (!renderData->CreateFromR3DMesh(device, textureLibrary, mesh, resourceName))
	{
		delete renderData;
		return nullptr;
	}

	mesh.SetDX11RenderData(renderData, r3dDX11DestroyMeshRenderData);
	return renderData;
}

void r3dDX11ReleaseMeshRenderData(r3dMesh& mesh)
{
	mesh.ReleaseDX11RenderData();
}
