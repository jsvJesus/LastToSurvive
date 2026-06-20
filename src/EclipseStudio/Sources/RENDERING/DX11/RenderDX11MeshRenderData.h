#pragma once

#include "RENDERING/DX11/RenderDX11GBufferPass.h"
#include "RENDERING/DX11/RenderDX11Material.h"
#include "RENDERING/DX11/RenderDX11Mesh.h"

#include <vector>

class r3dMesh;
class r3dDX11TextureLibrary;

class r3dDX11MeshRenderData final
{
public:
	r3dDX11MeshRenderData();
	~r3dDX11MeshRenderData();

	bool CreateFromR3DMesh(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, const r3dMesh& mesh, const char* debugName = nullptr);
	void Shutdown();

	void DrawGBuffer(r3dDX11GBufferPass& pass, const r3dDX11MeshConstants& constants, unsigned int objectColorPacked = 0xffffffff);
	void DrawGBufferBatch(r3dDX11GBufferPass& pass, unsigned int batchIndex, const r3dDX11MeshConstants& constants, unsigned int objectColorPacked = 0xffffffff);

	r3dDX11MeshResource& GetMeshResource();
	const r3dDX11MeshResource& GetMeshResource() const;
	unsigned int GetBatchCount() const;
	bool IsValid() const;

private:
	r3dDX11MeshConstants ApplyMeshScales(const r3dDX11MeshConstants& constants) const;

private:
	r3dDX11MeshResource MeshResource;
	std::vector<r3dDX11MaterialTextures> Materials;
};

r3dDX11MeshRenderData* r3dDX11GetOrCreateMeshRenderData(ID3D11Device* device, r3dDX11TextureLibrary& textureLibrary, r3dMesh& mesh, const char* debugName = nullptr);
void r3dDX11ReleaseMeshRenderData(r3dMesh& mesh);
