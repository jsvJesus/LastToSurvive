#include "r3dPCH.h"

#include "RENDERING/DX11/RenderDX11Mesh.h"

#include "RENDERING/DX11/RenderDX11Draw.h"
#include "RENDERING/DX11/RenderDX11DepthOnlyPass.h"
#include "RENDERING/DX11/RenderDX11GBufferPass.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
	float ClampFloat(float value, float minValue, float maxValue)
	{
		return std::max(minValue, std::min(maxValue, value));
	}

	short PackSnorm16(float value)
	{
		value = ClampFloat(value, -1.0f, 1.0f);
		return static_cast<short>(value >= 0.0f ? value * 32767.0f + 0.5f : value * 32767.0f - 0.5f);
	}

	unsigned char PackUnorm8(float value)
	{
		value = ClampFloat(value, 0.0f, 1.0f);
		return static_cast<unsigned char>(value * 255.0f + 0.5f);
	}

	float SafeScale(float value)
	{
		return std::fabs(value) > 0.000001f ? value : 1.0f;
	}
}

r3dDX11MeshResource::r3dDX11MeshResource()
{
}

r3dDX11MeshResource::~r3dDX11MeshResource()
{
	Shutdown();
}

bool r3dDX11MeshResource::Create(ID3D11Device* device, const r3dDX11MeshBuildDesc& desc, const char* debugName)
{
	Shutdown();

	if (!device || !desc.PositionsXYZ || !desc.NormalsXYZ || !desc.TexcoordsUV || !desc.TangentsXYZ || !desc.Indices || desc.NumVertices == 0 || desc.NumIndices == 0)
		return false;

	PositionScale[0] = SafeScale(desc.PositionScale[0]);
	PositionScale[1] = SafeScale(desc.PositionScale[1]);
	PositionScale[2] = SafeScale(desc.PositionScale[2]);
	TexcoordScale[0] = SafeScale(desc.TexcoordScale[0]);
	TexcoordScale[1] = SafeScale(desc.TexcoordScale[1]);
	bSkinned = desc.BlendWeights && desc.BlendIndices;

	if (bSkinned)
	{
		std::vector<r3dDX11PackedSkinnedMeshVertex> vertices;
		vertices.resize(desc.NumVertices);
		for (unsigned int i = 0; i < desc.NumVertices; ++i)
			vertices[i] = PackSkinnedVertex(desc, i);

		if (!VertexBuffer.Create(device, vertices.size() * sizeof(vertices[0]), sizeof(vertices[0]), &vertices[0], R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.SkinnedMesh.VB"))
		{
			Shutdown();
			return false;
		}
	}
	else
	{
		std::vector<r3dDX11PackedMeshVertex> vertices;
		vertices.resize(desc.NumVertices);
		for (unsigned int i = 0; i < desc.NumVertices; ++i)
			vertices[i] = PackVertex(desc, i);

		if (!VertexBuffer.Create(device, vertices.size() * sizeof(vertices[0]), sizeof(vertices[0]), &vertices[0], R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.Mesh.VB"))
		{
			Shutdown();
			return false;
		}
	}

	if (!IndexBuffer.Create(device, desc.NumIndices * sizeof(desc.Indices[0]), DXGI_FORMAT_R32_UINT, desc.Indices, R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.Mesh.IB"))
	{
		Shutdown();
		return false;
	}

	if (desc.NumBatches && desc.Batches)
	{
		Batches.assign(desc.Batches, desc.Batches + desc.NumBatches);
	}
	else
	{
		r3dDX11MeshBatch batch;
		batch.StartIndex = 0;
		batch.IndexCount = desc.NumIndices;
		batch.MaterialIndex = 0;
		Batches.push_back(batch);
	}

	VertexCount = desc.NumVertices;
	IndexCount = desc.NumIndices;
	return true;
}

bool r3dDX11MeshResource::CreateFromPacked(
	ID3D11Device* device,
	const void* vertices,
	unsigned int vertexStride,
	unsigned int numVertices,
	const unsigned int* indices,
	unsigned int numIndices,
	const r3dDX11MeshBatch* batches,
	unsigned int numBatches,
	bool skinned,
	const float* positionScale,
	const float* texcoordScale,
	const char* debugName
)
{
	Shutdown();

	if (!device || !vertices || !indices || numVertices == 0 || numIndices == 0 || vertexStride == 0)
		return false;

	PositionScale[0] = SafeScale(positionScale ? positionScale[0] : 1.0f);
	PositionScale[1] = SafeScale(positionScale ? positionScale[1] : 1.0f);
	PositionScale[2] = SafeScale(positionScale ? positionScale[2] : 1.0f);
	TexcoordScale[0] = SafeScale(texcoordScale ? texcoordScale[0] : 1.0f);
	TexcoordScale[1] = SafeScale(texcoordScale ? texcoordScale[1] : 1.0f);
	bSkinned = skinned;

	const unsigned char* sourceVertices = static_cast<const unsigned char*>(vertices);
	if (bSkinned)
	{
		if (vertexStride < sizeof(r3dDX11PackedSkinnedMeshVertex))
			return false;

		std::vector<r3dDX11PackedSkinnedMeshVertex> packedVertices;
		packedVertices.resize(numVertices);
		for (unsigned int i = 0; i < numVertices; ++i)
			memcpy(&packedVertices[i], sourceVertices + i * vertexStride, sizeof(packedVertices[i]));

		if (!VertexBuffer.Create(device, packedVertices.size() * sizeof(packedVertices[0]), sizeof(packedVertices[0]), &packedVertices[0], R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.SkinnedMesh.VB"))
		{
			Shutdown();
			return false;
		}
	}
	else
	{
		if (vertexStride < sizeof(r3dDX11PackedMeshVertex))
			return false;

		std::vector<r3dDX11PackedMeshVertex> packedVertices;
		packedVertices.resize(numVertices);
		for (unsigned int i = 0; i < numVertices; ++i)
			memcpy(&packedVertices[i], sourceVertices + i * vertexStride, sizeof(packedVertices[i]));

		if (!VertexBuffer.Create(device, packedVertices.size() * sizeof(packedVertices[0]), sizeof(packedVertices[0]), &packedVertices[0], R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.Mesh.VB"))
		{
			Shutdown();
			return false;
		}
	}

	if (!IndexBuffer.Create(device, numIndices * sizeof(indices[0]), DXGI_FORMAT_R32_UINT, indices, R3D_DX11_BUFFER_IMMUTABLE, debugName ? debugName : "DX11.Mesh.IB"))
	{
		Shutdown();
		return false;
	}

	if (numBatches && batches)
		Batches.assign(batches, batches + numBatches);
	else
	{
		r3dDX11MeshBatch batch;
		batch.StartIndex = 0;
		batch.IndexCount = numIndices;
		batch.MaterialIndex = 0;
		Batches.push_back(batch);
	}

	VertexCount = numVertices;
	IndexCount = numIndices;
	return true;
}

void r3dDX11MeshResource::Shutdown()
{
	IndexBuffer.Shutdown();
	VertexBuffer.Shutdown();
	Batches.clear();
	VertexCount = 0;
	IndexCount = 0;
	PositionScale[0] = PositionScale[1] = PositionScale[2] = 1.0f;
	TexcoordScale[0] = TexcoordScale[1] = 1.0f;
	bSkinned = false;
}

void r3dDX11MeshResource::Bind(r3dDX11DrawContext& drawContext)
{
	drawContext.SetVertexBuffer(&VertexBuffer);
	drawContext.SetIndexBuffer(&IndexBuffer);
}

void r3dDX11MeshResource::Draw(r3dDX11DrawContext& drawContext)
{
	Bind(drawContext);
	drawContext.DrawIndexed(IndexCount);
}

void r3dDX11MeshResource::DrawBatch(r3dDX11DrawContext& drawContext, unsigned int batchIndex)
{
	const r3dDX11MeshBatch* batch = GetBatch(batchIndex);
	if (!batch)
		return;

	Bind(drawContext);
	drawContext.DrawIndexed(batch->IndexCount, batch->StartIndex);
}

void r3dDX11MeshResource::DrawBatch(r3dDX11DepthOnlyPass& pass, unsigned int batchIndex)
{
	const r3dDX11MeshBatch* batch = GetBatch(batchIndex);
	if (batch)
		pass.DrawMesh(VertexBuffer, IndexBuffer, batch->IndexCount, batch->StartIndex);
}

void r3dDX11MeshResource::DrawBatch(r3dDX11GBufferPass& pass, unsigned int batchIndex)
{
	const r3dDX11MeshBatch* batch = GetBatch(batchIndex);
	if (batch)
		pass.DrawMesh(VertexBuffer, IndexBuffer, batch->IndexCount, batch->StartIndex);
}

const r3dDX11MeshBatch* r3dDX11MeshResource::GetBatch(unsigned int batchIndex) const
{
	if (batchIndex >= Batches.size())
		return nullptr;

	return &Batches[batchIndex];
}

unsigned int r3dDX11MeshResource::GetBatchCount() const
{
	return static_cast<unsigned int>(Batches.size());
}

unsigned int r3dDX11MeshResource::GetVertexCount() const
{
	return VertexCount;
}

unsigned int r3dDX11MeshResource::GetIndexCount() const
{
	return IndexCount;
}

const float* r3dDX11MeshResource::GetPositionScale() const
{
	return PositionScale;
}

const float* r3dDX11MeshResource::GetTexcoordScale() const
{
	return TexcoordScale;
}

bool r3dDX11MeshResource::IsSkinned() const
{
	return bSkinned;
}

bool r3dDX11MeshResource::IsValid() const
{
	return VertexBuffer.IsValid() && IndexBuffer.IsValid() && VertexCount > 0 && IndexCount > 0;
}

r3dDX11PackedMeshVertex r3dDX11MeshResource::PackVertex(const r3dDX11MeshBuildDesc& desc, unsigned int index)
{
	r3dDX11PackedMeshVertex vertex = {};

	const float* position = desc.PositionsXYZ + index * 3;
	const float* normal = desc.NormalsXYZ + index * 3;
	const float* texcoord = desc.TexcoordsUV + index * 2;
	const float* tangent = desc.TangentsXYZ + index * 3;
	const unsigned char tangentW = desc.TangentSigns ? static_cast<unsigned char>(255 - static_cast<unsigned char>(desc.TangentSigns[index])) : 255;

	vertex.Position[0] = PackSnorm16(position[0] / SafeScale(desc.PositionScale[0]));
	vertex.Position[1] = PackSnorm16(position[1] / SafeScale(desc.PositionScale[1]));
	vertex.Position[2] = PackSnorm16(position[2] / SafeScale(desc.PositionScale[2]));
	vertex.Position[3] = PackSnorm16(1.0f);

	vertex.TexCoord[0] = PackSnorm16(texcoord[0] / SafeScale(desc.TexcoordScale[0]));
	vertex.TexCoord[1] = PackSnorm16(texcoord[1] / SafeScale(desc.TexcoordScale[1]));

	vertex.Normal[0] = PackUnorm8(normal[0] * 0.5f + 0.5f);
	vertex.Normal[1] = PackUnorm8(normal[1] * 0.5f + 0.5f);
	vertex.Normal[2] = PackUnorm8(normal[2] * 0.5f + 0.5f);
	vertex.Normal[3] = 255;

	vertex.Tangent[0] = PackUnorm8(tangent[0] * 0.5f + 0.5f);
	vertex.Tangent[1] = PackUnorm8(tangent[1] * 0.5f + 0.5f);
	vertex.Tangent[2] = PackUnorm8(tangent[2] * 0.5f + 0.5f);
	vertex.Tangent[3] = tangentW;

	return vertex;
}

r3dDX11PackedSkinnedMeshVertex r3dDX11MeshResource::PackSkinnedVertex(const r3dDX11MeshBuildDesc& desc, unsigned int index)
{
	r3dDX11PackedMeshVertex base = PackVertex(desc, index);
	r3dDX11PackedSkinnedMeshVertex vertex = {};

	memcpy(vertex.Position, base.Position, sizeof(vertex.Position));
	memcpy(vertex.TexCoord, base.TexCoord, sizeof(vertex.TexCoord));
	memcpy(vertex.Normal, base.Normal, sizeof(vertex.Normal));
	memcpy(vertex.Tangent, base.Tangent, sizeof(vertex.Tangent));

	const unsigned int weightStride = desc.BlendWeightStride ? desc.BlendWeightStride : sizeof(float) * 4;
	const unsigned int indexStride = desc.BlendIndexStride ? desc.BlendIndexStride : sizeof(unsigned char) * 4;
	const float* weights = reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(desc.BlendWeights) + index * weightStride);
	const unsigned char* indices = desc.BlendIndices + index * indexStride;
	for (unsigned int i = 0; i < 4; ++i)
	{
		vertex.BlendWeights[i] = PackSnorm16(weights[i]);
		vertex.BlendIndices[i] = indices[i];
	}

	return vertex;
}
