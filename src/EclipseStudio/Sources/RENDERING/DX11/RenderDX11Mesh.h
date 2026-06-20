#pragma once

#include "RENDERING/DX11/RenderDX11Resources.h"

#include <vector>

class r3dDX11DrawContext;
class r3dDX11DepthOnlyPass;
class r3dDX11GBufferPass;

struct r3dDX11MeshBatch
{
	unsigned int StartIndex = 0;
	unsigned int IndexCount = 0;
	unsigned int MaterialIndex = 0;
};

struct r3dDX11MeshBuildDesc
{
	const float* PositionsXYZ = nullptr;
	const float* NormalsXYZ = nullptr;
	const float* TexcoordsUV = nullptr;
	const float* TangentsXYZ = nullptr;
	const char* TangentSigns = nullptr;
	const unsigned int* Indices = nullptr;
	const unsigned char* BlendIndices = nullptr;
	const float* BlendWeights = nullptr;
	unsigned int BlendIndexStride = 0;
	unsigned int BlendWeightStride = 0;
	unsigned int NumVertices = 0;
	unsigned int NumIndices = 0;
	const r3dDX11MeshBatch* Batches = nullptr;
	unsigned int NumBatches = 0;
	float PositionScale[3] = { 1.0f, 1.0f, 1.0f };
	float TexcoordScale[2] = { 1.0f, 1.0f };
};

#pragma pack(push, 1)
struct r3dDX11PackedMeshVertex
{
	short Position[4];
	short TexCoord[2];
	unsigned char Normal[4];
	unsigned char Tangent[4];
};

struct r3dDX11PackedSkinnedMeshVertex
{
	short Position[4];
	short TexCoord[2];
	unsigned char Normal[4];
	unsigned char Tangent[4];
	short BlendWeights[4];
	unsigned char BlendIndices[4];
};
#pragma pack(pop)

class r3dDX11MeshResource final
{
public:
	r3dDX11MeshResource();
	~r3dDX11MeshResource();

	bool Create(ID3D11Device* device, const r3dDX11MeshBuildDesc& desc, const char* debugName = nullptr);
	void Shutdown();

	void Bind(r3dDX11DrawContext& drawContext);
	void Draw(r3dDX11DrawContext& drawContext);
	void DrawBatch(r3dDX11DrawContext& drawContext, unsigned int batchIndex);
	void DrawBatch(r3dDX11DepthOnlyPass& pass, unsigned int batchIndex);
	void DrawBatch(r3dDX11GBufferPass& pass, unsigned int batchIndex);

	const r3dDX11MeshBatch* GetBatch(unsigned int batchIndex) const;
	unsigned int GetBatchCount() const;
	unsigned int GetVertexCount() const;
	unsigned int GetIndexCount() const;
	const float* GetPositionScale() const;
	const float* GetTexcoordScale() const;
	bool IsSkinned() const;
	bool IsValid() const;

private:
	static r3dDX11PackedMeshVertex PackVertex(const r3dDX11MeshBuildDesc& desc, unsigned int index);
	static r3dDX11PackedSkinnedMeshVertex PackSkinnedVertex(const r3dDX11MeshBuildDesc& desc, unsigned int index);

private:
	r3dDX11VertexBuffer VertexBuffer;
	r3dDX11IndexBuffer IndexBuffer;
	std::vector<r3dDX11MeshBatch> Batches;
	unsigned int VertexCount = 0;
	unsigned int IndexCount = 0;
	float PositionScale[3] = { 1.0f, 1.0f, 1.0f };
	float TexcoordScale[2] = { 1.0f, 1.0f };
	bool bSkinned = false;
};
